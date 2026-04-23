#ifndef DATASET_H
#define DATASET_H

#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <string>
#include <cmath>


using namespace std;

class Dataset {
public:
    vector<vector<double>> X;
    vector<double> y;

    void loadCSV(const string& path) {

        ifstream file(path);

        if (!file.is_open()) {
            cout << "ERROR: Cannot open file: " << path << endl;
            exit(0);
        }

        string line;
        getline(file, line); // skip header

        while (getline(file, line)) {

            if (line.empty()) continue;

            stringstream ss(line);
            string val;
            vector<string> cols;

            while (getline(ss, val, ',')) {
                // clean spaces + quotes
                val.erase(remove(val.begin(), val.end(), ' '), val.end());
                val.erase(remove(val.begin(), val.end(), '\"'), val.end());
                cols.push_back(val);
            }

            // New schema (7 cols):
            //   [0] registration_no
            //   [1] actual_cg          <- TARGET
            //   [2] last_gp            <- total grade points (e.g. 244.0), NOT per-semester GPA
            //   [3] avg_gp             <- weighted average GPA (e.g. 8.71)
            //   [4] fail_count         <- number of failed subjects
            //   [5] performance_variance
            //   [6] gp_trend           <- "Stable" | "Improving" | "Declining"
            if (cols.size() < 7) continue;

            try {
                double target   = stod(cols[1]); // actual_cg
                double last_gp  = stod(cols[2]); // total credit-grade points
                double avg_gp   = stod(cols[3]); // average GPA
                double fail_cnt = stod(cols[4]); // fail count
                double variance = stod(cols[5]); // performance variance
                string trend_str = cols[6];      // categorical trend

                // Encode gp_trend: Improving=1, Stable=0, Declining=-1
                double trend = 0.0;
                if (trend_str == "Improving")       trend =  1.0;
                else if (trend_str == "Declining")  trend = -1.0;
                // "Stable" stays 0.0

                // Normalize last_gp: raw total points -> GPA-scale equivalent
                // ~28 credits/semester is standard; last_gp / 28 ≈ per-credit GPA
                double last_gp_norm = last_gp / 28.0;

                // --- Data discrepancy filters ---
                if (target <= 0.0)        continue; // unlabeled / missing result
                if (avg_gp > 10.5)        continue; // impossible GPA (scale corruption)
                if (last_gp_norm > 12.0)  continue; // raw points sanity check
                if (fail_cnt < 0.0)       continue; // corrupt negative value

                // Features: avg_gp, last_gp_norm, trend, variance, fail_count
                X.push_back({avg_gp, last_gp_norm, trend, variance, fail_cnt});
                y.push_back(target);

            } catch (...) {
                continue; // skip malformed rows silently
            }
        }

        cout << "Loaded rows: " << X.size() << endl;

        if (X.size() == 0) {
            cout << "ERROR: No valid data loaded\n";
            exit(0);
        }
    }
};

#endif
