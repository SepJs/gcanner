cmake_minimum_required(VERSION 3.14)
project(nlohmann_json LANGUAGES CXX)

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(nlohmann_json INTERFACE cxx_std_17)

# Create alias for compatibility
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
