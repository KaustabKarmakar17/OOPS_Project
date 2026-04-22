#include "App.h"
#include "ApiError.h"
#include "ApiResponse.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include "LinearRegression.h"
#include "Dataset.h"
#include "KNN.h"
#include "CSVReader.h"
#include <algorithm>

namespace {
// ---------- percentile ----------
double getPercentile(double val, const std::vector<double>& dist) {
    if (dist.empty())
        return 0.0;

    int count = 0;
    for (double x : dist)
        if (x <= val)
            count++;

    return static_cast<double>(count) / static_cast<double>(dist.size());
}

// ---------- category ----------
int getCategory(double val) {
    if (val >= 9.5)
        return 5;
    if (val >= 8.5)
        return 4;
    if (val >= 7.0)
        return 3;
    if (val >= 6.0)
        return 2;
    if (val >= 5.0)
        return 1;
    return 0;
}

std::string getLabel(int c) {
    if (c == 5)
        return "Topper";
    if (c == 4)
        return "Excellent";
    if (c == 3)
        return "Above Average";
    if (c == 2)
        return "Average";
    if (c == 1)
        return "Below Average";
    return "Critical";
}

// ---------- feature ----------
std::vector<double> computeFeatures(const std::vector<double>& sems,
                                    const std::vector<double>& credits) {
    const int n = static_cast<int>(sems.size());

    double avg = 0;
    double total_credits = 0;

    for (int i = 0; i < n; i++) {
        avg += sems[i] * credits[i];
        total_credits += credits[i];
    }

    if (total_credits > 0)
        avg /= total_credits;
    else
        avg = 0;

    const double last = sems[n - 1];
    const double trend = (sems[n - 1] - sems[0]) / static_cast<double>(n);

    double var = 0;
    for (double x : sems)
        var += std::pow(x - avg, 2);
    var /= static_cast<double>(n);

    return {avg, last, trend, var}; // ALWAYS 4 FEATURES
}

// ---------- distribution ----------
std::vector<double> loadDistributionFrom(const std::string& path) {
    std::vector<double> dist;
    std::ifstream file(path);
    double x;
    while (file >> x)
        dist.push_back(x);
    return dist;
}

std::vector<double> loadDistribution() {
    // Match existing CSVReader path convention ("../data.csv")
    auto dist = loadDistributionFrom("../distribution.txt");
    if (!dist.empty())
        return dist;
    dist = loadDistributionFrom("distribution.txt");
    if (!dist.empty())
        return dist;
    return loadDistributionFrom("OOPS_Project/distribution.txt");
}

bool loadModels(LinearRegression& lr, KNN& knn) {
    // Match existing CSVReader path convention ("../data.csv")
    std::ifstream lrCheck("../model.txt");
    std::ifstream knnCheck("../knn.txt");

    if (lrCheck.good() && knnCheck.good()) {
        lr.load("../model.txt");
        knn.load("../knn.txt");
        return true;
    }

    std::ifstream lrCheckLocal("model.txt");
    std::ifstream knnCheckLocal("knn.txt");
    if (lrCheckLocal.good() && knnCheckLocal.good()) {
        lr.load("model.txt");
        knn.load("knn.txt");
        return true;
    }

    std::ifstream lrCheckProj("OOPS_Project/model.txt");
    std::ifstream knnCheckProj("OOPS_Project/knn.txt");
    if (lrCheckProj.good() && knnCheckProj.good()) {
        lr.load("OOPS_Project/model.txt");
        knn.load("OOPS_Project/knn.txt");
        return true;
    }

    return false;
}
} // namespace

void App::registerRoutes() {

    // GET /users
    CROW_ROUTE(app, "/users")
    ([]() {
        auto users = CSVReader::readUsers("../data.csv");

        if (users.empty()) {
            ApiError err("No users found or file error", "EMPTY_DATA");
            return crow::response(404, err.toJson().dump());
        }

        crow::json::wvalue data;
        crow::json::wvalue::list userList;

        for (auto &u : users) {
            userList.push_back(std::move(u)); // ✅ move instead of copy
        }

        data["users"] = std::move(userList);
        data["count"] = (int)users.size();

        // for (int i = 0; i < users.size(); i++) {
        //     data["users"][i] = users[i];
        // }

        ApiResponse res(true, "Users fetched successfully", data);
        return crow::response(200, res.toJson().dump());
    });
    CROW_ROUTE(app, "/users/<string>")
    ([](const std::string &reg_no) {
        auto users = CSVReader::readUsers("../data.csv");

        for (auto &user : users) {

            // crow::json::wvalue access
            if (user.has("registration_no") &&
                user["registration_no"].dump() == "\"" + reg_no + "\"") {
                crow::json::wvalue data;

                // ✅ move single user
                data["user"] = std::move(user);

                ApiResponse res(true, "User found", data);
                return crow::response(200, res.toJson().dump());
            }
        }

        // ❌ not found
        ApiError err("User not found", "USER_404");
        return crow::response(404, err.toJson().dump());
    });
    CROW_ROUTE(app, "/users/<string>")
        .methods("PUT"_method)([](const crow::request &req,
                                  const std::string &reg_no) {
            auto body = crow::json::load(req.body);

            if (!body) {
                ApiError err("Invalid JSON", "BAD_REQUEST");
                return crow::response(400, err.toJson().dump());
            }

            bool updated = CSVReader::updateUser("../data.csv", reg_no, body);

            if (!updated) {
                ApiError err("User not found", "USER_404");
                return crow::response(404, err.toJson().dump());
            }

            crow::json::wvalue data;
            data["registration_no"] = reg_no;

            ApiResponse res(true, "User updated successfully", data);
            return crow::response(200, res.toJson().dump());
        });

    // POST /predict
    // Body:
    // {
    //   "sems": [7.8, 8.1, 8.4],
    //   "credits": [24, 28, 25]   // optional (predefined defaults used if missing)
    // }
    CROW_ROUTE(app, "/predict")
        .methods("POST"_method)([](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body) {
                ApiError err("Invalid JSON", "BAD_REQUEST");
                return crow::response(400, err.toJson().dump());
            }

            if (!body.has("sems") || body["sems"].t() != crow::json::type::List) {
                ApiError err("Missing/invalid field: sems (array)", "BAD_REQUEST");
                return crow::response(400, err.toJson().dump());
            }

            const auto& semList = body["sems"];
            const int n = static_cast<int>(semList.size());
            if (n <= 0) {
                ApiError err("sems array cannot be empty", "BAD_REQUEST");
                return crow::response(400, err.toJson().dump());
            }

            std::vector<double> sems;
            sems.reserve(n);

            for (int i = 0; i < n; i++) {
                double val = semList[i].d();
                if (val > 10.0)
                    val = 10.0;
                else if (val < 0.0)
                    val = 0.0;
                sems.push_back(val);
            }

            // Credits: optional
            std::vector<double> predefined_credits = {24, 28, 25, 29, 24, 26, 18, 19};
            std::vector<double> credits;
            credits.reserve(n);

            if (body.has("credits") && body["credits"].t() == crow::json::type::List &&
                static_cast<int>(body["credits"].size()) == n) {
                const auto& creditList = body["credits"];
                for (int i = 0; i < n; i++) {
                    double c = creditList[i].d();
                    if (c <= 0.0)
                        c = 20.0;
                    credits.push_back(c);
                }
            } else {
                for (int i = 0; i < n; i++) {
                    if (i < static_cast<int>(predefined_credits.size()))
                        credits.push_back(predefined_credits[i]);
                    else
                        credits.push_back(20.0);
                }
            }

            LinearRegression lr;
            KNN knn;
            if (!loadModels(lr, knn)) {
                ApiError err("Model files not found (model.txt/knn.txt)", "MODEL_MISSING");
                return crow::response(500, err.toJson().dump());
            }

            std::vector<double> dist = loadDistribution();
            if (dist.empty()) {
                ApiError err("distribution.txt missing or empty", "DIST_MISSING");
                return crow::response(500, err.toJson().dump());
            }

            const std::vector<double> features = computeFeatures(sems, credits);

            // ----- LR prediction -----
            const double pred_cg = lr.predict(features);
            const double p = getPercentile(pred_cg, dist);
            const int cat_lr = getCategory(pred_cg);

            // ----- KNN prediction -----
            const int cat_knn = static_cast<int>(knn.predict(features));

            // ----- Confidence -----
            std::string agreement_confidence;
            if (cat_lr == cat_knn)
                agreement_confidence = "High";
            else if (std::abs(cat_lr - cat_knn) == 1)
                agreement_confidence = "Medium";
            else
                agreement_confidence = "Low";

            const double data_conf = std::min(1.0, static_cast<double>(n) / 6.0);

            crow::json::wvalue data;
            data["predicted_cg_lr"] = pred_cg;
            data["percentile"] = p * 100.0;
            data["lr_category"] = getLabel(cat_lr);
            data["knn_category"] = getLabel(cat_knn);
            data["agreement_confidence"] = agreement_confidence;
            data["data_confidence"] = data_conf * 100.0;

            // Helpful debug/inspection values (same 4 features used by models)
            data["features"]["avg"] = features[0];
            data["features"]["last"] = features[1];
            data["features"]["trend"] = features[2];
            data["features"]["variance"] = features[3];

            ApiResponse res(true, "Prediction successful", std::move(data));
            return crow::response(200, res.toJson().dump());
        });
}

void App::run(int port) {
    registerRoutes();
    app.port(port).multithreaded().run();
}