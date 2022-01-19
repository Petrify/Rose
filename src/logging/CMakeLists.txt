add_library(RoseLogging
    logging.cpp
)

target_link_libraries(RoseLogging
    PUBLIC spdlog::spdlog spdlog::spdlog_header_only
)

target_include_directories(RoseLogging 
    PUBLIC inc
)

