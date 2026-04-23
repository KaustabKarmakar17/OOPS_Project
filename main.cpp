#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include "LinearRegression.h"
#include "Dataset.h"
#include "KNN.h"

using namespace std;

// ---------- percentile ----------
double getPercentile(double val, vector<double>& dist) {
    int count = 0;
    for (double x : dist)
        if (x <= val) count++;
    return (double)count / dist.size();
}

// ---------- category ----------
int getCategory(double val) {
    if (val >= 9.5) return 5;
    else if (val >= 8.5) return 4;
    else if (val >= 7.0) return 3;
    else if (val >= 6.0) return 2;
    else if (val >= 5.0) return 1;
    else return 0;
}

string getLabel(int c) {
    if (c == 5) return "Topper";
    if (c == 4) return "Excellent";
    if (c == 3) return "Above Average";
    if (c == 2) return "Average";
    if (c == 1) return "Below Average";
    return "Critical";
}

// ---------- feature computation ----------
// Produces the SAME 4-feature vector as dataset.cpp:
//   [0] avg_gp        — credit-weighted average GPA
//   [1] last_gp_norm  — total grade points / (n * 28.0)  (matches CSV last_gp/28)
//   [2] trend         — Improving=1, Stable=0, Declining=-1
//   [3] variance      — variance of semester GPAs
vector<double> computeFeatures(const vector<double>& sems,
                                const vector<double>& credits) {
    int n = sems.size();

    // Weighted average GPA
    double total_grade_points = 0.0;
    double total_credits      = 0.0;
    for (int i = 0; i < n; i++) {
        total_grade_points += sems[i] * credits[i];
        total_credits      += credits[i];
    }
    double avg_gp = (total_credits > 0) ? total_grade_points / total_credits : 0.0;

    // last_gp_norm: matches CSV col[5] (last_gp) / 28.0
    double last_gp_norm = (n > 0) ? (total_grade_points / n) / 28.0 : 0.0;

    // gp_trend: compare last vs first semester GPA
    double trend = 0.0;
    if (n > 1) {
        double slope = (sems[n - 1] - sems[0]);
        if      (slope >  0.3) trend =  1.0; // Improving
        else if (slope < -0.3) trend = -1.0; // Declining
        // else Stable → 0.0
    }

    // Variance of semester GPAs
    double var = 0.0;
    for (double s : sems)
        var += pow(s - avg_gp, 2);
    var /= n;

    return {avg_gp, last_gp_norm, trend, var};
}

// ---------- distribution ----------
vector<double> loadDistribution() {
    vector<double> dist;
    ifstream file("distribution.txt");
    double x;
    while (file >> x) dist.push_back(x);
    return dist;
}

// ---------- main ----------
int main() {

    int choice;
    cout << "1. Train\n2. Predict\nChoice: ";
    cin >> choice;

    if (choice == 1) {

        Dataset data;
        data.loadCSV("data.csv");

        // ----- LR -----
        LinearRegression lr;
        lr.fit(data.X, data.y);
        lr.save("model.txt");

        // ----- KNN -----
        KNN knn(5);
        knn.fit(data.X, data.y);
        knn.save("knn.txt");

        // ----- Distribution (for percentile lookup) -----
        ofstream out("distribution.txt");
        for (double y : data.y)
            out << y << endl;

        cout << "Training Done\n";
    }

    else if (choice == 2) {

        LinearRegression lr;
        lr.load("model.txt");

        KNN knn;
        knn.load("knn.txt");

        vector<double> dist = loadDistribution();

        // ----- Semester count -----
        int n;
        cout << "Sem count: ";
        cin >> n;

        // ----- Department selection -----
        int dept_choice;
        cout << "\nSelect Department:\n";
        cout << "1. CSE\n2. ECE\n3. EE\n4. EI\n5. ME\n6. CE\n7. Other (Manual entry)\nChoice: ";
        cin >> dept_choice;

        // Department-specific credit tables (per semester)
        vector<double> cse_credits = {27, 28, 26, 29, 28, 28, 18, 16};
        vector<double> ece_credits = {27, 28, 28, 28, 27, 28, 19, 15};
        vector<double> ee_credits  = {27, 28, 29, 28, 27, 27, 19, 15};
        vector<double> ei_credits  = {28, 27, 28, 27, 28, 27, 19, 16};
        vector<double> me_credits  = {28, 27, 28, 27, 28, 28, 19, 15};
        vector<double> ce_credits  = {28, 27, 27, 28, 28, 28, 19, 15};

        vector<double> sems(n);
        vector<double> credits(n);

        for (int i = 0; i < n; i++) {
            cout << "\n--- Semester " << i + 1 << " ---\n";
            cout << "Enter GPA: ";
            double val;
            cin >> val;

            // Clamp GPA to valid range
            if (val > 10.0) {
                cout << "Warning: GPA " << val << " exceeds maximum. Clamping to 10.0\n";
                val = 10.0;
            } else if (val < 0.0) {
                cout << "Warning: GPA " << val << " is negative. Clamping to 0.0\n";
                val = 0.0;
            }
            sems[i] = val;

            // Assign department credits
            if      (dept_choice == 1 && i < (int)cse_credits.size()) { credits[i] = cse_credits[i]; cout << "Assigned CSE Credits: " << credits[i] << "\n"; }
            else if (dept_choice == 2 && i < (int)ece_credits.size()) { credits[i] = ece_credits[i]; cout << "Assigned ECE Credits: " << credits[i] << "\n"; }
            else if (dept_choice == 3 && i < (int)ee_credits.size())  { credits[i] = ee_credits[i];  cout << "Assigned EE Credits: "  << credits[i] << "\n"; }
            else if (dept_choice == 4 && i < (int)ei_credits.size())  { credits[i] = ei_credits[i];  cout << "Assigned EI Credits: "  << credits[i] << "\n"; }
            else if (dept_choice == 5 && i < (int)me_credits.size())  { credits[i] = me_credits[i];  cout << "Assigned ME Credits: "  << credits[i] << "\n"; }
            else if (dept_choice == 6 && i < (int)ce_credits.size())  { credits[i] = ce_credits[i];  cout << "Assigned CE Credits: "  << credits[i] << "\n"; }
            else {
                cout << "Enter Credits for this semester: ";
                double cred;
                cin >> cred;
                if (cred <= 0.0) {
                    cout << "Warning: Credits must be > 0. Defaulting to 20.0\n";
                    cred = 20.0;
                }
                credits[i] = cred;
            }
        }

        // ----- Fail count -----
        int fail_cnt;
        cout << "\nTotal failed subjects (across all semesters): ";
        cin >> fail_cnt;
        if (fail_cnt < 0) fail_cnt = 0;
        (void)fail_cnt; // not used as a feature (4-feature model)

        // ----- Compute features -----
        vector<double> features = computeFeatures(sems, credits);

        // ----- LR prediction -----
        double pred_cg = lr.predict(features);

        // [Partial Semester Heuristic]
        // The ML model was trained on 8-semester (complete) data. For early-semester
        // students it regresses toward the population mean — OOD penalty.
        // Fix: weight the student's current avg_gp heavily for small n, and add a
        // consistency bonus (near-zero variance = student is steady, trust their avg more).
        if (n < 8) {
            // Trend reward: Improving=+1, Stable=0, Declining=-1
            double trend_bonus = features[2] * (8 - n) * 0.1;
            pred_cg += trend_bonus;

            double variance = features[3];
            // Base avg_gp weight: 0.80 for n=1, 0.45 for n=7
            double base_weight = 0.85 - n * 0.05;
            // Consistency bonus: up to +0.10 for perfectly stable students (var=0)
            double consistency_bonus = max(0.0, 0.10 - variance * 0.5);
            double avg_weight = min(0.90, base_weight + consistency_bonus);

            pred_cg = pred_cg * (1.0 - avg_weight) + features[0] * avg_weight;
        }

        // Clamp to valid GPA range
        pred_cg = max(0.0, min(10.0, pred_cg));

        double p       = getPercentile(pred_cg, dist);
        int cat_lr     = getCategory(pred_cg);

        // ----- KNN prediction -----
        int cat_knn = (int)knn.predict(features);

        // ----- Confidence -----
        string confidence;
        if      (cat_lr == cat_knn)          confidence = "High";
        else if (abs(cat_lr - cat_knn) == 1) confidence = "Medium";
        else                                  confidence = "Low";

        double sem_conf = min(1.0, (double)n / 6.0);

        // ----- OUTPUT -----
        cout << "\nPredicted CG (LR): " << pred_cg << endl;

        cout << "\n[LR Category]\n";
        cout << "Percentile: " << p * 100 << "%\n";
        cout << "Category: " << getLabel(cat_lr) << endl;

        cout << "\n[KNN Category]\n";
        cout << "Category: " << getLabel(cat_knn) << endl;

        cout << "\nAgreement Confidence: " << confidence << endl;
        cout << "Data Confidence: " << sem_conf * 100 << "%\n";
    }

    return 0;
}
