cmake_minimum_required(VERSION 3.10)

project(minic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(minic
    src/main.cpp
    src/lexer/Token.cpp
    src/lexer/Lexer.cpp
    src/ast/AST.cpp
    src/parser/Parser.cpp
    src/semantic/SymbolTable.cpp
    src/semantic/Semantic.cpp
    src/ir/IRModule.cpp
    src/ir/IRGenerator.cpp
    src/codegen/CodeGenerator.cpp
)

target_include_directories(minic PRIVATE src)
