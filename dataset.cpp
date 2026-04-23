#include "Dataset.h"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

void Dataset::loadCSV(const string &filename) {
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        cout << "ERROR: Cannot open file\n";
        return;
    }

    getline(file, line); // skip header

    while (getline(file, line)) {
        if (line.empty())
            continue;

        stringstream ss(line);
        string val;
        vector<string> cols;

        while (getline(ss, val, ',')) {
            cols.push_back(val);
        }

        if (cols.size() < 8)
            continue;

        try {
            double variance  = stod(cols[2]);
            double avg       = stod(cols[4]);
            double last      = stod(cols[5]);
            string trend_str = cols[6]; // "Stable" | "Improving" | "Declining"
            double trend = 0.0;
            if (trend_str == "Improving")      trend =  1.0;
            else if (trend_str == "Declining") trend = -1.0;
            double target = stod(cols[7]);

            if (target <= 0.0 || avg > 20.0)
                continue;

            X.push_back({avg, last, trend, variance});
            y.push_back(target);

        } catch (...) {
            continue;
        }
    }

    cout << "Loaded rows: " << X.size() << endl;
}
// #include "Dataset.h"
// #include <fstream>
// #include <sstream>

// void Dataset::loadCSV(const string& filename) {
//     ifstream file(filename);
//     string line;

//     getline(file, line); // skip header

//     while (getline(file, line)) {
//         if (line.empty()) continue;

//         stringstream ss(line);
//         string value;
//         vector<double> row;

//         while (getline(ss, value, ',')) {
//             row.push_back(stod(value));
//         }

//         data.push_back(row);
//     }
// }

// vector<vector<double>> Dataset::getFeatures(int col) {
//     vector<vector<double>> X;

//     for (auto& row : data) {
//         X.push_back({row[col]});
//     }

//     return X;
// }

// vector<double> Dataset::getColumn(int col) {
//     vector<double> y;

//     for (auto& row : data) {
//         y.push_back(row[col]);
//     }

//     return y;
// }