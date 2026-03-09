#define PLOG_OMIT_LOG_DEFINES
#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/ColorConsoleAppender.h>

#include "server_instance.h"

int main() {
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::debug, &consoleAppender);
    PLOG_DEBUG << "Starting server!";

    ServerInstance server;

    server.run();
    
    return 0;
}
