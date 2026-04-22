#ifndef APP_H
#define APP_H

#include "crow.h"

class App {
private:
    crow::SimpleApp app;

    void registerRoutes();

public:
    void run(int port = 18080);
};

#endif