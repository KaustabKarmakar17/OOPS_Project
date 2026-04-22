#ifndef API_ERROR_H
#define API_ERROR_H

#include "crow.h"
#include <string>

using namespace std;

class ApiError {
public:
    bool success;
    string message;
    string errorCode;

    ApiError(const string& message, const string& errorCode)
        : success(false), message(message), errorCode(errorCode) {}

    crow::json::wvalue toJson() const {
        crow::json::wvalue res;
        res["success"] = false;
        res["message"] = message;
        res["error"] = errorCode;
        return res;
    }
};

#endif