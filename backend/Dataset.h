#ifndef DATASET_H
#define DATASET_H

#include <vector>
#include <string>
using namespace std;

class Dataset {
public:
    vector<vector<double>> X;
    vector<double> y;

    void loadCSV(const string& path);
};

#endif
// #ifndef DATASET_H
// #define DATASET_H

// #include <vector>
// #include <fstream>
// #include <sstream>
// #include <algorithm>
// #include <iostream>

// using namespace std;

// class Dataset {
// public:
//     vector<vector<double>> X;
//     vector<double> y;

//     void loadCSV(const string& path) {

//     ifstream file(path);

//     if (!file.is_open()) {
//         cout << "ERROR: Cannot open file: " << path << endl;
//         exit(0);
//     }

//     string line;

//     getline(file, line); // skip header

//     while (getline(file, line)) {

//         stringstream ss(line);
//         string val;
//         vector<string> cols;

//         while (getline(ss, val, ',')) {

//             // clean spaces + quotes
//             val.erase(remove(val.begin(), val.end(), ' '), val.end());
//             val.erase(remove(val.begin(), val.end(), '"'), val.end());

//             cols.push_back(val);
//         }

//         // DEBUG
//         // cout << "Cols: " << cols.size() << endl;

//         if (cols.size() < 8) continue;

//         try {
//             double variance = stod(cols[2]);
//             double avg = stod(cols[4]);
//             double last = stod(cols[5]);
//             double trend = stod(cols[6]);
//             double target = stod(cols[7]);

//             // Filter out anomalous data (corrupted rows/dropouts and incorrect grade scales)
//             if (target <= 0.0 || avg > 20.0) {
//                 continue;
//             }

//             X.push_back({avg, last, trend, variance});
//             y.push_back(target);

//         } catch (...) {
//             continue;
//         }
//     }

//     cout << "Loaded rows: " << X.size() << endl;

//     if (X.size() == 0) {
//         cout << "ERROR: No valid data loaded\n";
//         exit(0);
//     }
//     }
// };

// #endif