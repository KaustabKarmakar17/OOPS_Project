#include "KNN.h"
#include <cmath>
#include <algorithm>
#include <fstream>

using namespace std;

void KNN::fit(const vector<vector<double>>& X_train, const vector<double>& y_train) {

    X = X_train;
    y.clear();

    if (X.empty()) return;
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

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            X[i][j] = (X[i][j] - means[j]) / stds[j];
        }
    }

    for (double val : y_train) {

        int cat;

        if (val >= 9.5) cat = 5;
        else if (val >= 8.5) cat = 4;
        else if (val >= 7.0) cat = 3;
        else if (val >= 6.0) cat = 2;
        else if (val >= 5.0) cat = 1;
        else cat = 0;

        y.push_back(cat);
    }
}

double KNN::predict(const vector<double>& x) {

    vector<double> x_norm = x;
    for (int j = 0; j < x_norm.size() && j < means.size(); j++) {
        x_norm[j] = (x_norm[j] - means[j]) / stds[j];
    }

    vector<pair<double, int>> dist;

    for (int i = 0; i < X.size(); i++) {

        double d = 0;

        for (int j = 0; j < x_norm.size() && j < X[i].size(); j++) {
            d += pow(X[i][j] - x_norm[j], 2);
        }

        d = sqrt(d);

        dist.push_back({d, y[i]});
    }

    sort(dist.begin(), dist.end());

    vector<int> count(6, 0);

    int limit = min(k, (int)dist.size());

    for (int i = 0; i < limit; i++) {
        count[dist[i].second]++;
    }

    int best = 0;

    for (int i = 1; i < 6; i++) {
        if (count[i] > count[best])
            best = i;
    }

    return best;
}

void KNN::save(const string& path) {

    ofstream out(path);

    out << X.size() << " " << X[0].size() << endl;

    for (double mean : means) out << mean << " ";
    out << endl;
    for (double std : stds) out << std << " ";
    out << endl;

    for (int i = 0; i < X.size(); i++) {
        for (double val : X[i])
            out << val << " ";
        out << y[i] << endl;
    }
}

void KNN::load(const string& path) {

    ifstream in(path);

    int n, m;
    in >> n >> m;

    means.assign(m, 0.0);
    stds.assign(m, 0.0);

    for (int j = 0; j < m; j++) in >> means[j];
    for (int j = 0; j < m; j++) in >> stds[j];

    X.assign(n, vector<double>(m));
    y.assign(n, 0);

    for (int i = 0; i < n; i++) {

        for (int j = 0; j < m; j++)
            in >> X[i][j];

        in >> y[i];
    }
}