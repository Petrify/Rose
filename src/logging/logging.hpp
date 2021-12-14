#pragma once

#ifdef NDEBUG
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO // All DEBUG/TRACE statements will be removed by the pre-processor
#else
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG 
#endif 

#include <spdlog/spdlog.h>

typedef std::shared_ptr<spdlog::logger> Logger;

void initLogging();

void initSinks();
Logger getLogger(std::string name);