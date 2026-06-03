cmake_minimum_required(VERSION 3.20)
project(Chess-Bot LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(chess_core STATIC
    src/engine/position.cpp
    src/engine/movegen.cpp
    src/engine/attacks.cpp
    src/search/search.cpp
    src/search/eval.cpp
)
target_include_directories(chess_core PUBLIC src/engine src)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(BUILD_SHARED_LIBS OFF)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
FetchContent_Declare(
    SFML
    URL https://github.com/SFML/SFML/archive/refs/tags/2.6.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(SFML)

enable_testing()

add_executable(position_test tests/main_test.cpp)
target_include_directories(position_test PRIVATE src)
target_link_libraries(position_test PRIVATE chess_core GTest::gtest_main)

add_executable(search_test tests/search_test.cpp)
target_include_directories(search_test PRIVATE src)
target_link_libraries(search_test PRIVATE chess_core GTest::gtest_main)
add_test(NAME search_test COMMAND search_test)



add_executable(chess_gui src/main_gui.cpp src/gui/gui.cpp)
target_include_directories(chess_gui PRIVATE src)
target_link_libraries(chess_gui PRIVATE chess_core sfml-graphics sfml-window sfml-system)

add_custom_command(TARGET chess_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sfml-graphics>
        $<TARGET_FILE:sfml-window>
        $<TARGET_FILE:sfml-system>
        $<TARGET_FILE_DIR:chess_gui>
)
add_custom_command(TARGET chess_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:chess_gui>/assets
)

add_executable(chess_bot src/main.cpp src/uci.cpp)
target_include_directories(chess_bot PRIVATE src)
target_link_libraries(chess_bot PRIVATE chess_core)