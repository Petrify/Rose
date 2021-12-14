#include "logging.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#define MAX_LOG_FILE_SIZE 1024 * 1024 * 4
#define MAX_LOG_FILES 8

static std::vector<spdlog::sink_ptr> sinks;

void initSinks() {

    if(!sinks.empty()) return;

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::debug);
    sinks.push_back(consoleSink);

    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/logfile.txt", MAX_LOG_FILE_SIZE, MAX_LOG_FILES, true);
    fileSink->set_level(spdlog::level::debug);
    sinks.push_back(fileSink);
}

Logger newLogger(std::string name) {
    auto logger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
    logger ->set_level(spdlog::level::debug);
    spdlog::register_logger(logger);
    logger->info("Logger Created Successfully");
    return logger;
}

Logger getLogger(std::string name) {
    Logger logger = spdlog::get(name);
    if (logger) return logger;
    return newLogger(name);
}

void initLogging() {
    initSinks();
}
