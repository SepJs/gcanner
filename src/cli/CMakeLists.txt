cmake_minimum_required(VERSION 3.20)
project(gcanner_cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use the analyzer library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. gcanner_build)

add_executable(gcanner src/cli/gcanner_cli.cpp)
target_link_libraries(gcanner PRIVATE gcanner_analyzers)

# Install
install(TARGETS gcanner RUNTIME DESTINATION bin)