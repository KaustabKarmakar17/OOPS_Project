#ifndef CSV_READER_H
#define CSV_READER_H

#include "crow.h"
#include <string>
#include <vector>

class CSVReader {
  public:
    static std::vector<crow::json::wvalue>
    readUsers(const std::string &filename);
    static bool updateUser(const std::string &filename,
                           const std::string &reg_no,
                           const crow::json::rvalue &body);
    static crow::json::wvalue getUserById(const std::string &filename,
                                          const std::string &reg_no);
};

#endif