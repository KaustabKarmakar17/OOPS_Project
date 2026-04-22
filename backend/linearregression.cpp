#include "LinearRegression.h"
#include <cmath>
#include <fstream>
#include <iostream>

using namespace std;

void LinearRegression::fit(const vector<vector<double>>& X, const vector<double>& y) {

    int n = X.size();
    int m = X[0].size();

    means.assign(m, 0.0);
    stds.assign(m, 0.0);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            means[j] += X[i][j];
        }
    }
    for (int j = 0; j < m; j++) means[j] /= n;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            stds[j] += pow(X[i][j] - means[j], 2);
        }
    }
    for (int j = 0; j < m; j++) {
        stds[j] = sqrt(stds[j] / n);
        if (stds[j] == 0) stds[j] = 1.0;
    }

    vector<vector<double>> X_norm = X;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            X_norm[i][j] = (X[i][j] - means[j]) / stds[j];
        }
    }

    weights.assign(m, 0.0);
    bias = 0.0;

    double lr = 0.02;
    int epochs = 4000;

    for (int e = 0; e < epochs; e++) {

        vector<double> dw(m, 0.0);
        double db = 0;

        for (int i = 0; i < n; i++) {

            double pred = bias;

            for (int j = 0; j < m; j++)  // SAFE
                pred += weights[j] * X_norm[i][j];

            double error = pred - y[i];

            for (int j = 0; j < m; j++)  // SAFE
                dw[j] += error * X_norm[i][j];

            db += error;
        }

        for (int j = 0; j < m; j++)
            weights[j] -= lr * dw[j] / n;

        bias -= lr * db / n;
    }
}

double LinearRegression::predict(const vector<double>& x) {

    double pred = bias;

    int m = weights.size();

    for (int j = 0; j < m; j++) {

        if (j < x.size()) { // SAFETY CHECK
            double x_norm = (x[j] - means[j]) / stds[j];
            pred += weights[j] * x_norm;
        }
    }

    return pred;
}

void LinearRegression::save(const string& path) {
    ofstream out(path);

    out << weights.size() << endl;

    for (double w : weights)
        out << w << " ";
    out << endl;

    for (double m : means)
        out << m << " ";
    out << endl;

    for (double s : stds)
        out << s << " ";

    out << endl << bias;
}

void LinearRegression::load(const string& path) {

    ifstream in(path);

    int size;
    in >> size;

    weights.resize(size);
    for (int i = 0; i < size; i++)
        in >> weights[i];

    means.resize(size);
    for (int i = 0; i < size; i++)
        in >> means[i];

    stds.resize(size);
    for (int i = 0; i < size; i++)
        in >> stds[i];

    in >> bias;
}