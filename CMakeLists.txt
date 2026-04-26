cmake_minimum_required(VERSION 3.10)

project(minic)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(minic
    src/main.cpp
    src/Lexer.cpp
)