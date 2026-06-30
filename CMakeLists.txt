cmake_minimum_required(VERSION 4.2)
project(leetcmd)

set(CMAKE_CXX_STANDARD 20)

find_package(CURL REQUIRED)
find_package(Curses REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(leetcmd main.cpp
        http_client.cpp
        include/http_client.h
        include/credits.h
        leetcode_client.cpp
        include/leetcode_client.h)

include_directories(${CURSES_INCLUDE_DIR})
target_link_libraries(leetcmd PRIVATE CURL::libcurl ${CURSES_LIBRARIES} nlohmann_json::nlohmann_json)