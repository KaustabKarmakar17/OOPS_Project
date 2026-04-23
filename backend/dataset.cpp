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

        // Actual CSV schema (7 cols):
        //   [0] registration_no
        //   [1] actual_cg  <- TARGET
        //   [2] last_gp  (raw total grade points, e.g. 244.0)
        //   [3] avg_gp
        //   [4] fail_count
        //   [5] performance_variance
        //   [6] gp_trend ("Stable" | "Improving" | "Declining")
        if (cols.size() < 7)
            continue;

        try {
            double target    = stod(cols[1]);
            double last      = stod(cols[2]);
            double avg       = stod(cols[3]);
            double variance  = stod(cols[5]);
            string trend_str = cols[6]; // "Stable" | "Improving" | "Declining"
            double trend = 0.0;
            if (trend_str == "Improving")      trend =  1.0;
            else if (trend_str == "Declining") trend = -1.0;
            // "Stable" stays 0.0

            if (target <= 0.0 || avg > 20.0)
                continue;

            // Features: avg_gp, last_gp (raw), trend, variance
            // Matches computeFeatures in App.cpp: {avg, last, trend, var}
            X.push_back({avg, last, trend, variance});
            y.push_back(target);

        } catch (...) {
            continue;
        }
    }

    cout << "Loaded rows: " << X.size() << endl;
}