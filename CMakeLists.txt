cmake_minimum_required(VERSION 4.3)
project(game_engine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(fmt CONFIG REQUIRED)

add_executable(game_engine src/main.cpp)

target_link_libraries(game_engine PRIVATE fmt::fmt)
