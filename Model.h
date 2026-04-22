#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>

using namespace std;

class Model {
public:
    virtual void fit(const vector<vector<double>>& X,
                     const vector<double>& y) = 0;

    virtual double predict(const vector<double>& x) = 0;

    virtual void save(const string& path) = 0;
    virtual void load(const string& path) = 0;

    virtual ~Model() {}
};

#endif