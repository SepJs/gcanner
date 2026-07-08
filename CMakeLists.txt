cmake_minimum_required(VERSION 3.20)
project(GameReqAnalyzer
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Premium Game System Requirements Analyzer - Cross-platform terminal UX"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /permissive- /std:c++20")
endif()

# Build types
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

# Platform detection
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

# Third party libraries
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# Options to disable tests for dependencies
set(FMT_BUILD_TESTS OFF CACHE BOOL "Build fmt testing" FORCE)

# Add third-party libraries
add_subdirectory("${THIRD_PARTY_DIR}/nlohmann_json" nlohmann_json)
add_subdirectory("${THIRD_PARTY_DIR}/fmt-main" fmt)

# Find dependencies that we still need from system
find_package(Threads REQUIRED)

# Optional: OpenSSL for secure downloads
find_package(OpenSSL QUIET)
if(OPENSSL_FOUND)
    add_definitions(-DHAVE_OPENSSL)
endif()

# Optional: libcurl for hardware database downloads
find_package(CURL QUIET)
if(CURL_FOUND)
    add_definitions(-DHAVE_CURL)
endif()

# Optional: libmagic for file type detection
find_package(LibMagic QUIET)
if(LibMagic_FOUND)
    add_definitions(-DHAVE_LIBMAGIC)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Subdirectories
add_subdirectory(src)

# Testing
enable_testing()
# add_subdirectory(tests)

# Install
install(TARGETS game-req-analyzer
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin
)

install(DIRECTORY data/
    DESTINATION share/game-req-analyzer
    FILES_MATCHING PATTERN "*.json" PATTERN "*.json.zst"
)

# Packaging
set(CPACK_PACKAGE_NAME "GameReqAnalyzer")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Premium Game System Requirements Analyzer")
set(CPACK_GENERATOR "TGZ;ZIP")
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
endif()
include(CPack)
