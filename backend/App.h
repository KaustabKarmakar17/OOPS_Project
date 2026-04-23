#ifndef APP_H
#define APP_H

#include "crow.h"
#include "crow/middlewares/cors.h"

class App {
private:
    crow::App<crow::CORSHandler> app;

    void registerRoutes();

public:
    void run(int port = 18080);
};

#endif