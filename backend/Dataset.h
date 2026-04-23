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