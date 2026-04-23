#include "CSVReader.h"
#include <fstream>
#include <sstream>

using namespace std;

vector<crow::json::wvalue> CSVReader::readUsers(const string& filename) {
    ifstream file(filename);
    vector<crow::json::wvalue> users;

    if (!file.is_open()) {
        return users;
    }

    string line;
    getline(file, line); // skip header

    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string val;
        vector<string> cols;

        while (getline(ss, val, ',')) {
            cols.push_back(val);
        }

        if (cols.size() < 7) continue;

        try {
            crow::json::wvalue user;

            user["registration_no"] = cols[0];
            user["actual_cg"] = stod(cols[1]);
            user["last_gp"] = stod(cols[2]);
            user["avg_gp"] = stod(cols[3]);
            user["fail_count"] = stoi(cols[4]);
            user["performance_variance"] = stod(cols[5]);
            user["gp_trend"] = cols[6];       // string: "Stable" | "Improving" | "Declining"

            users.push_back(user);

        } catch (...) {
            continue;
        }
    }

    return users;
}
bool CSVReader::updateUser(const string& filename,
                           const string& reg_no,
                           const crow::json::rvalue& body) {

    ifstream file(filename);
    if (!file.is_open()) return false;

    vector<vector<string>> rows;
    string line;

    // store header
    getline(file, line);
    rows.push_back({line});

    bool updated = false;

    while (getline(file, line)) {
        stringstream ss(line);
        string val;
        vector<string> cols;

        while (getline(ss, val, ',')) {
            cols.push_back(val);
        }

        if (cols.size() < 7) continue;

        if (cols[0] == reg_no) {

            // update fields (only if present)
            if (body.has("actual_cg")) cols[1] = to_string(body["actual_cg"].d());
            if (body.has("last_gp")) cols[2] = to_string(body["last_gp"].d());
            if (body.has("avg_gp")) cols[3] = to_string(body["avg_gp"].d());
            if (body.has("fail_count")) cols[4] = to_string(body["fail_count"].i());
            if (body.has("performance_variance")) cols[5] = to_string(body["performance_variance"].d());
            if (body.has("gp_trend")) cols[6] = body["gp_trend"].s(); // string field

            updated = true;
        }

        rows.push_back(cols);
    }

    file.close();

    // rewrite file
    ofstream out(filename);
    if (!out.is_open()) return false;

    for (int i = 0; i < rows.size(); i++) {

        if (i == 0) {
            out << rows[i][0] << "\n"; // header
            continue;
        }

        for (int j = 0; j < rows[i].size(); j++) {
            out << rows[i][j];
            if (j != rows[i].size() - 1) out << ",";
        }
        out << "\n";
    }

    return updated;
}
crow::json::wvalue CSVReader::getUserById(const string& filename,
                                          const string& reg_no) {

    ifstream file(filename);
    string line;

    crow::json::wvalue user;

    getline(file, line); // skip header

    while (getline(file, line)) {

        stringstream ss(line);
        string val;
        vector<string> cols;

        while (getline(ss, val, ',')) {
            val.erase(remove(val.begin(), val.end(), ' '), val.end());
            val.erase(remove(val.begin(), val.end(), '"'), val.end());
            cols.push_back(val);
        }

        if (cols.size() < 7) continue;

        if (cols[0] == reg_no) {

            user["registration_no"] = cols[0];
            user["actual_cg"] = stod(cols[1]);
            user["last_gp"] = stod(cols[2]);
            user["avg_gp"] = stod(cols[3]);
            user["fail_count"] = stoi(cols[4]);
            user["performance_variance"] = stod(cols[5]);
            user["gp_trend"] = cols[6];       // string: "Stable" | "Improving" | "Declining"

            return user;
        }
    }

    return crow::json::wvalue(); // empty
}