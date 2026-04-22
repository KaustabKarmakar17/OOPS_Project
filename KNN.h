#ifndef KNN_H
#define KNN_H

#include "Model.h"

class KNN : public Model {
private:
    vector<vector<double>> X;
    vector<int> y;
    int k;

    vector<double> means;
    vector<double> stds;

public:
    KNN(int k_val = 5) { k = k_val; }

    void fit(const vector<vector<double>>& X_train, const vector<double>& y_train) override;
    double predict(const vector<double>& x) override;

    void save(const string& path) override;
    void load(const string& path) override;
};

#endif