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

namespace
{
    // ---------- percentile ----------
    double getPercentile(double val, const std::vector<double> &dist)
    {
        if (dist.empty())
            return 0.0;

        int count = 0;
        for (double x : dist)
            if (x <= val)
                count++;

        return static_cast<double>(count) / static_cast<double>(dist.size());
    }

    // ---------- category ----------
    int getCategory(double val)
    {
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

    std::string getLabel(int c)
    {
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
    std::vector<double> computeFeatures(const std::vector<double> &sems,
                                        const std::vector<double> &credits)
    {
        const int n = static_cast<int>(sems.size());

        double avg = 0;
        double total_credits = 0;

        for (int i = 0; i < n; i++)
        {
            avg += sems[i] * credits[i];
            total_credits += credits[i];
        }

        if (total_credits > 0)
            avg /= total_credits;
        else
            avg = 0;

        // The ML model was trained on 'last_gp' from the CSV, which is exactly 'avg_gp * 28.0'
        // Passing sems[n - 1] (e.g., 8.9) breaks the prediction since the model expects ~250.0
        const double last = avg * 28.0;
        const double trend = (sems[n - 1] - sems[0]) / static_cast<double>(n);

        double var = 0;
        for (double x : sems)
            var += std::pow(x - avg, 2);
        var /= static_cast<double>(n);

        return {avg, last, trend, var}; // ALWAYS 4 FEATURES
    }

    // ---------- distribution ----------
    std::vector<double> loadDistributionFrom(const std::string &path)
    {
        std::vector<double> dist;
        std::ifstream file(path);
        double x;
        while (file >> x)
            dist.push_back(x);
        return dist;
    }

    std::vector<double> loadDistribution()
    {
        // Match existing CSVReader path convention ("../data.csv")
        auto dist = loadDistributionFrom("../distribution.txt");
        if (!dist.empty())
            return dist;
        dist = loadDistributionFrom("distribution.txt");
        if (!dist.empty())
            return dist;
        return loadDistributionFrom("OOPS_Project/distribution.txt");
    }

    bool loadModels(LinearRegression &lr, KNN &knn)
    {
        // Match existing CSVReader path convention ("../data.csv")
        std::ifstream lrCheck("../model.txt");
        std::ifstream knnCheck("../knn.txt");

        if (lrCheck.good() && knnCheck.good())
        {
            lr.load("../model.txt");
            knn.load("../knn.txt");
            return true;
        }

        std::ifstream lrCheckLocal("model.txt");
        std::ifstream knnCheckLocal("knn.txt");
        if (lrCheckLocal.good() && knnCheckLocal.good())
        {
            lr.load("model.txt");
            knn.load("knn.txt");
            return true;
        }

        std::ifstream lrCheckProj("OOPS_Project/model.txt");
        std::ifstream knnCheckProj("OOPS_Project/knn.txt");
        if (lrCheckProj.good() && knnCheckProj.good())
        {
            lr.load("OOPS_Project/model.txt");
            knn.load("OOPS_Project/knn.txt");
            return true;
        }

        return false;
    }
} // namespace

void App::registerRoutes()
{
    // Enable CORS (fixes browser "CORS error" / preflight failures)
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin("*")
        .headers("Content-Type")
        .methods("GET"_method, "POST"_method, "PUT"_method, "OPTIONS"_method);

    // GET /users
    CROW_ROUTE(app, "/users")
        .methods("GET"_method)([&](const crow::request &req, crow::response &res)
                                                 {
        auto users = CSVReader::readUsers("../data.csv");

        if (users.empty()) {
            ApiError err("No users found or file error", "EMPTY_DATA");
            res.code = 404;
            res.body = err.toJson().dump();
            res.end();
            return;
        }

        crow::json::wvalue data;
        crow::json::wvalue::list userList;

        for (auto &u : users) {
            userList.push_back(std::move(u));
        }

        data["users"] = std::move(userList);
        data["count"] = (int)users.size();

        ApiResponse api_res(true, "Users fetched successfully", data);
        res.code = 200;
        res.body = api_res.toJson().dump();
        res.end();
    });

    // ---------------------------------------------------------
    // 2. /users/<string> (GET, PUT)
    // ---------------------------------------------------------
    CROW_ROUTE(app, "/users/<string>")
        .methods("GET"_method, "PUT"_method)([&](const crow::request &req, crow::response &res, const std::string &reg_no)
                                                               {
            if (req.method == crow::HTTPMethod::GET)
            {
                auto users = CSVReader::readUsers("../data.csv");

                for (auto &user : users)
                {
                    if (user.has("registration_no") && user["registration_no"].dump() == "\"" + reg_no + "\"")
                    {
                        crow::json::wvalue data;
                        data["user"] = std::move(user);
                        ApiResponse api_res(true, "User found", data);
                        res.code = 200;
                        res.body = api_res.toJson().dump();
                        res.end();
                        return;
                    }
                }
                ApiError err("User not found", "USER_404");
                res.code = 404;
                res.body = err.toJson().dump();
                res.end();
                return;
            }
            else if (req.method == crow::HTTPMethod::PUT)
            {
                auto body = crow::json::load(req.body);

                if (!body)
                {
                    ApiError err("Invalid JSON", "BAD_REQUEST");
                    res.code = 400;
                    res.body = err.toJson().dump();
                    res.end();
                    return;
                }

                bool updated = CSVReader::updateUser("../data.csv", reg_no, body);

                if (!updated)
                {
                    ApiError err("User not found", "USER_404");
                    res.code = 404;
                    res.body = err.toJson().dump();
                    res.end();
                    return;
                }

                crow::json::wvalue data;
                data["registration_no"] = reg_no;

                ApiResponse api_res(true, "User updated successfully", data);
                res.code = 200;
                res.body = api_res.toJson().dump();
                res.end();
                return;
            }

            res.code = 405; 
            res.end(); });

    // ---------------------------------------------------------
    // 3. /predict (POST)
    // ---------------------------------------------------------
    CROW_ROUTE(app, "/predict")
        .methods("POST"_method)([&](const crow::request &req, crow::response &res)
                                                  {
        auto body = crow::json::load(req.body);
        if (!body) {
            ApiError err("Invalid JSON", "BAD_REQUEST");
            res.code = 400;
            res.body = err.toJson().dump();
            res.end();
            return;
        }

        if (!body.has("sems") || body["sems"].t() != crow::json::type::List) {
            ApiError err("Missing/invalid field: sems (array)", "BAD_REQUEST");
            res.code = 400;
            res.body = err.toJson().dump();
            res.end();
            return;
        }

        const auto& semList = body["sems"];
        const int n = static_cast<int>(semList.size());
        if (n <= 0) {
            ApiError err("sems array cannot be empty", "BAD_REQUEST");
            res.code = 400;
            res.body = err.toJson().dump();
            res.end();
            return;
        }

        std::vector<double> sems;
        sems.reserve(n);

        for (int i = 0; i < n; i++) {
            double val = semList[i].d();
            if (val > 10.0) val = 10.0;
            else if (val < 0.0) val = 0.0;
            sems.push_back(val);
        }

        std::vector<double> predefined_credits = {24, 28, 25, 29, 24, 26, 18, 19};
        std::vector<double> credits;
        credits.reserve(n);

        if (body.has("credits") && body["credits"].t() == crow::json::type::List &&
            static_cast<int>(body["credits"].size()) == n) {
            const auto& creditList = body["credits"];
            for (int i = 0; i < n; i++) {
                double c = creditList[i].d();
                if (c <= 0.0) c = 20.0;
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
            res.code = 500;
            res.body = err.toJson().dump();
            res.end();
            return;
        }

        std::vector<double> dist = loadDistribution();
        if (dist.empty()) {
            ApiError err("distribution.txt missing or empty", "DIST_MISSING");
            res.code = 500;
            res.body = err.toJson().dump();
            res.end();
            return;
        }

        const std::vector<double> features = computeFeatures(sems, credits);

        const double pred_cg = lr.predict(features);
        const double p = getPercentile(pred_cg, dist);
        const int cat_lr = getCategory(pred_cg);

        const int cat_knn = static_cast<int>(knn.predict(features));

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

        data["features"]["avg"] = features[0];
        data["features"]["last"] = features[1];
        data["features"]["trend"] = features[2];
        data["features"]["variance"] = features[3];

        ApiResponse api_res(true, "Prediction successful", std::move(data));
        res.code = 200;
        res.body = api_res.toJson().dump();
        res.end();
    });
}

void App::run(int port)
{
    // Configure CORS global middleware
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("*")
        .methods("POST"_method, "GET"_method, "PUT"_method, "OPTIONS"_method)
        .origin("*");

    registerRoutes();
    app.port(port).multithreaded().run();
}