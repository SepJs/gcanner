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
    analyzers/shader_analyzer.cpp
    analyzers/particle_analyzer.cpp
    analyzers/physics_analyzer.cpp
    analyzers/ai_analyzer.cpp
    analyzers/network_analyzer.cpp
    analyzers/asset_aggregator.cpp
    analyzers/requirement_calculator.cpp
    
    hardware/hardware_database.cpp
    hardware/cpu_database.cpp
    hardware/gpu_database.cpp
    # hardware/ram_database.cpp
    # hardware/storage_database.cpp
    # hardware/hardware_updater.cpp
    hardware/benchmark_scraper.cpp
    
    # ui/terminal_ui.cpp
    # ui/progress_display.cpp
    # ui/results_formatter.cpp
    # ui/interactive_mode.cpp
    
    utils/stubs.cpp
)

add_executable(gcanner ${SOURCES})

target_link_libraries(gcanner PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
    fmt::fmt
)

if(OPENSSL_FOUND)
    target_link_libraries(gcanner PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

if(CURL_FOUND)
    target_link_libraries(gcanner PRIVATE CURL::libcurl)
endif()

if(LibMagic_FOUND)
    target_link_libraries(gcanner PRIVATE LibMagic::LibMagic)
endif()

if(PNG_FOUND)
    target_link_libraries(gcanner PRIVATE PNG::PNG)
endif()

target_compile_options(gcanner PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-pthread>
)

# Precompiled header for faster builds
# set_source_files_properties(pch.hpp PROPERTIES HEADER_FILE_ONLY TRUE)
# target_precompile_headers(gcanner PRIVATE pch.hpp)
