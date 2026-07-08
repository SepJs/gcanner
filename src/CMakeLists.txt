# Source CMakeLists.txt
set(SOURCES
    core/main.cpp
    core/application.cpp
    core/config.cpp
    core/logger.cpp
    
    scanner/file_scanner.cpp
    scanner/file_type_detector.cpp
    scanner/archive_handler.cpp
    
    analyzers/texture_analyzer.cpp
    analyzers/model_analyzer.cpp
    analyzers/audio_analyzer.cpp
    analyzers/script_analyzer.cpp
    analyzers/executable_analyzer.cpp
    analyzers/asset_aggregator.cpp
    analyzers/requirement_calculator.cpp
    
    hardware/hardware_database.cpp
    hardware/cpu_database.cpp
    hardware/gpu_database.cpp
    hardware/ram_database.cpp
    hardware/storage_database.cpp
    hardware/hardware_updater.cpp
    hardware/benchmark_scraper.cpp
    
    ui/terminal_ui.cpp
    ui/progress_display.cpp
    ui/results_formatter.cpp
    ui/interactive_mode.cpp
    
    utils/platform_utils.cpp
    utils/file_utils.cpp
    utils/hash_utils.cpp
    utils/math_utils.cpp
    utils/json_utils.cpp
    utils/string_utils.cpp
    utils/time_utils.cpp
)

add_executable(game-req-analyzer ${SOURCES})

target_link_libraries(game-req-analyzer PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
    fmt::fmt
    spdlog
)

if(OPENSSL_FOUND)
    target_link_libraries(game-req-analyzer PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

if(CURL_FOUND)
    target_link_libraries(game-req-analyzer PRIVATE CURL::libcurl)
endif()

if(LibMagic_FOUND)
    target_link_libraries(game-req-analyzer PRIVATE LibMagic::LibMagic)
endif()

target_compile_options(game-req-analyzer PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-pthread>
)

# Precompiled header for faster builds
# set_source_files_properties(pch.hpp PROPERTIES HEADER_FILE_ONLY TRUE)
# target_precompile_headers(game-req-analyzer PRIVATE pch.hpp)
