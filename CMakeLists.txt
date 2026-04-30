cmake_minimum_required(VERSION 3.10)

project(minic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

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
    src/maple/IRGenerator.cpp
    src/codegen/CodeGenerator.cpp
)

target_include_directories(minic PRIVATE src)

add_test(
    NAME maple_increment_expression
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_maple_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/increment_expression.mc
            ${CMAKE_CURRENT_BINARY_DIR}/increment_expression.mpl
            "dassign %__inc_old_"
            "dassign %__inc_new_"
            "callassigned &add (dread i32 %y, dread i32 %z) { dassign %__callret_"
            "callassigned &add (dread i32 %x, dread i32 %__callret_"
            "dassign %r 0 }"
)

add_test(
    NAME maple_nested_control_flow
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_maple_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/nested_control_flow_call.mc
            ${CMAKE_CURRENT_BINARY_DIR}/nested_control_flow_call.mpl
            "# BB for_body"
            "# BB if_then"
            "# BB while_body"
            "call &add (dread i32 %sum, dread i32 %i)"
            "callassigned &add (dread i32 %sum, constval i32 42) { dassign %result 0 }"
)

add_test(
    NAME maple_nested_increment_call
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_maple_contains.sh --abc
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/nested_increment_call.mc
            ${CMAKE_CURRENT_BINARY_DIR}/nested_increment_call.abc
            "# BB for_body"
            "call &add (dread i32 %sum, dread i32 %i)"
            "call &add (dread i32 %sum, dread i32 %j)"
            "callassigned &add (dread i32 %sum, constval i32 42) { dassign %x 0 }"
            "callassigned &add (dread i32 %x, constval i32 2) { dassign %__callret_"
            "dassign %y (add i32 (dread i32 %__inc_old_"
            "callassigned &add (dread i32 %y, dread i32 %__inc_old_"
)
