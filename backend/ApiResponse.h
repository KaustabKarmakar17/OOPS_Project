#ifndef API_RESPONSE_H
#define API_RESPONSE_H

#include "crow.h"
#include <string>

using namespace std;

class ApiResponse {
public:
    bool success;
    string message;
    crow::json::wvalue data;

    // Constructor
    ApiResponse(bool success, const string& message, crow::json::wvalue data = {})
        : success(success), message(message), data(std::move(data)) {}

    // Convert to JSON
    crow::json::wvalue toJson() {
        crow::json::wvalue res;
        res["success"] = success;
        res["message"] = message;

        // ✅ FIX: move instead of copy
        res["data"] = std::move(data);

        return res;
    }
};

#endif