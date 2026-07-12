cmake_minimum_required(VERSION 3.20)
project(gcanner
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Premium Game System Requirements Analyzer - Cross-platform terminal UX"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wno-conversion -Wno-sign-conversion -Wno-null-dereference -Wno-double-promotion -Wno-reorder")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /permissive- /std:c++20")
endif()

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

if(WIN32)
    add_definitions(-DGAME_REQ_WINDOWS)
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
elseif(APPLE)
    add_definitions(-DGAME_REQ_MACOS)
    find_library(COREFOUNDATION CoreFoundation)
    find_library(CORESERVICES CoreServices)
elseif(UNIX)
    add_definitions(-DGAME_REQ_LINUX)
endif()

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
set(FMT_BUILD_TESTS OFF CACHE BOOL "Build fmt testing" FORCE)
add_subdirectory("${THIRD_PARTY_DIR}/nlohmann_json" nlohmann_json)
add_subdirectory("${THIRD_PARTY_DIR}/fmt-main" fmt)

find_package(Threads REQUIRED)

# Find optional dependencies
find_package(OpenSSL QUIET)
if(OPENSSL_FOUND)
    add_definitions(-DHAVE_OPENSSL)
endif()

find_package(CURL QUIET)
if(CURL_FOUND)
    add_definitions(-DHAVE_CURL)
endif()

find_package(LibMagic QUIET)
if(LibMagic_FOUND)
    add_definitions(-DHAVE_LIBMAGIC)
endif()

find_package(PNG QUIET)
if(PNG_FOUND)
    add_definitions(-DHAVE_LIBPNG)
endif()

# Qt6 - Optional for GUI
find_package(Qt6 QUIET COMPONENTS Core Widgets Gui Charts Svg)
if(Qt6_FOUND)
    add_definitions(-DHAVE_QT6)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
else()
    message(WARNING "Qt6 not found - GUI application will not be built")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Build analyzer library
set(ANALYZER_SOURCES
    src/analyzers/texture_analyzer.cpp
    src/analyzers/model_analyzer.cpp
    src/analyzers/audio_analyzer.cpp
    src/analyzers/script_analyzer.cpp
    src/analyzers/executable_analyzer.cpp
    src/analyzers/shader_analyzer.cpp
    src/analyzers/particle_analyzer.cpp
    src/analyzers/physics_analyzer.cpp
    src/analyzers/ai_analyzer.cpp
    src/analyzers/network_analyzer.cpp
    src/analyzers/asset_aggregator.cpp
    src/analyzers/requirement_calculator.cpp
)

add_library(gcanner_analyzers STATIC ${ANALYZER_SOURCES})
target_link_libraries(gcanner_analyzers PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
    fmt::fmt
)

if(OPENSSL_FOUND)
    target_link_libraries(gcanner_analyzers PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

if(CURL_FOUND)
    target_link_libraries(gcanner_analyzers PRIVATE CURL::libcurl)
endif()

if(LibMagic_FOUND)
    target_link_libraries(gcanner_analyzers PRIVATE LibMagic::LibMagic)
endif()

if(PNG_FOUND)
    target_link_libraries(gcanner_analyzers PRIVATE PNG::PNG)
endif()

# ===========================================
# GUI Application (optional - requires Qt6)
# ===========================================
if(Qt6_FOUND)
    # GUI Sources
    set(GUI_SOURCES
        src/gui/main.cpp
        src/gui/MainWindow.cpp
        src/gui/ScanWidget.cpp
        src/gui/ResultsWidget.cpp
        src/gui/SettingsDialog.cpp
        src/gui/ThemeManager.cpp
        src/gui/OSDetector.cpp
        src/gui/ProgressDialog.cpp
        src/gui/HardwareBrowserWidget.cpp
        src/gui/ReportViewer.cpp
    )

    set(GUI_HEADERS
        src/gui/MainWindow.hpp
        src/gui/ScanWidget.hpp
        src/gui/ResultsWidget.hpp
        src/gui/SettingsDialog.hpp
        src/gui/ThemeManager.hpp
        src/gui/OSDetector.hpp
        src/gui/ProgressDialog.hpp
        src/gui/HardwareBrowserWidget.hpp
        src/gui/ReportViewer.hpp
    )

    set(GUI_RESOURCES
        src/gui/resources.qrc
    )

    add_executable(gcanner_gui ${GUI_SOURCES} ${GUI_HEADERS} ${GUI_RESOURCES})
    target_link_libraries(gcanner_gui PRIVATE
        gcanner_analyzers
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        Qt6::Charts
        Qt6::Svg
    )

    if(WIN32)
        target_link_libraries(gcanner_gui PRIVATE ${COREFOUNDATION} ${CORESERVICES})
    endif()

    # Install GUI
    install(TARGETS gcanner_gui
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION bin
    )
endif()

# Install analyzer library
install(TARGETS gcanner_analyzers
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

install(DIRECTORY data/
    DESTINATION share/gcanner
    FILES_MATCHING PATTERN "*.json" PATTERN "*.json.zst"
)

set(CPACK_PACKAGE_NAME "gcanner")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Premium Game System Requirements Analyzer")
set(CPACK_GENERATOR "TGZ;ZIP")
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
endif()
include(CPack)