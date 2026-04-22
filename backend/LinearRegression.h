#ifndef LINEAR_REGRESSION_H
#define LINEAR_REGRESSION_H

#include "Model.h"

class LinearRegression : public Model {
private:
    vector<double> weights;
    vector<double> means;
    vector<double> stds;
    double bias;

public:
    void fit(const vector<vector<double>>& X, const vector<double>& y) override;
    double predict(const vector<double>& x) override;
    void save(const string& path) override;
    void load(const string& path) override;
};

#endif