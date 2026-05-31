cmake_minimum_required(VERSION 3.20)
project(Chess-Bot LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(chess_core
    src/position.cpp
    src/movegen.cpp
    src/attacks.cpp
)
target_include_directories(chess_core PUBLIC src)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(position_test tests/main_test.cpp)
target_link_libraries(position_test PRIVATE chess_core GTest::gtest_main)

add_test(NAME position_test COMMAND position_test)

