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
    src/ast/DotExporter.cpp
    src/parser/Parser.cpp
    src/semantic/SymbolTable.cpp
    src/semantic/Semantic.cpp
    src/ir/IRModule.cpp
    src/ir/IRGenerator.cpp
    src/maple/CFGDotExporter.cpp
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

add_test(
    NAME abc_backend_unset_env
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_abc_backend_error.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/basic.mc
            unset-env
)

add_test(
    NAME abc_backend_missing_input
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_abc_backend_error.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/basic.mc
            missing-input
)

add_test(
    NAME abc_backend_missing_output
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_abc_backend_error.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/basic.mc
            missing-output
)

add_test(
    NAME abc_backend_command_fails
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_abc_backend_error.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/basic.mc
            command-fails
)

add_test(
    NAME maple_ast_scalar_types
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_maple_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/maple_ast_types.mc
            ${CMAKE_CURRENT_BINARY_DIR}/maple_ast_types.mpl
            "func &float_id (var %x f32) f32 {"
            "var %y f32"
            "dassign %y (dread f32 %x)"
            "return (dread f32 %y)"
            "func &char_id (var %c i8) i8 {"
            "var %d i8"
            "dassign %d (dread i8 %c)"
            "return (dread i8 %d)"
            "func &int_id (var %i i32) i32 {"
            "var %j i32"
            "dassign %j (dread i32 %i)"
            "return (dread i32 %j)"
)

add_test(
    NAME maple_if_else_lowering
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_maple_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/if_else.mc
            ${CMAKE_CURRENT_BINARY_DIR}/if_else_lowering.mpl
            "# BB if_cond"
            "brfalse @if_else_"
            "# BB if_then"
            "goto @if_end_"
            "# BB if_else"
            "dassign %a (sub i32 (dread i32 %a, constval i32 1))"
)

add_test(
    NAME maple_while_lowering
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_maple_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/while.mc
            ${CMAKE_CURRENT_BINARY_DIR}/while_lowering.mpl
            "goto @while_cond_0"
            "# BB while_cond"
            "brfalse @while_end_2"
            "goto @while_body_1"
            "# BB while_body"
            "dassign %sum (add i32 (dread i32 %sum, dread i32 %i))"
            "dassign %i (sub i32 (dread i32 %i, constval i32 1))"
)

add_test(
    NAME maple_for_lowering
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_maple_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/for_loop.mc
            ${CMAKE_CURRENT_BINARY_DIR}/for_lowering.mpl
            "# BB for_init"
            "# BB for_cond"
            "brfalse @for_end_3"
            "# BB for_body"
            "goto @for_update_2"
            "# BB for_update"
            "dassign %__inc_old_"
            "dassign %__inc_new_"
)

add_test(
    NAME maple_function_call_lowering
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_maple_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/function_call.mc
            ${CMAKE_CURRENT_BINARY_DIR}/function_call_lowering.mpl
            "func &add (var %a i32, var %b i32) i32 {"
            "return (add i32 (dread i32 %a, dread i32 %b))"
            "callassigned &add (dread i32 %a, dread i32 %b) { dassign %x 0 }"
            "return (dread i32 %x)"
)

add_test(
    NAME maple_prefix_increment_lowering
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_maple_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/increment_expression.mc
            ${CMAKE_CURRENT_BINARY_DIR}/prefix_increment_lowering.mpl
            "dassign %__inc_new_"
            "dassign %i (dread i32 %__inc_new_"
            "dassign %y (dread i32 %__inc_new_"
)

add_test(
    NAME maple_postfix_increment_lowering
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_maple_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/increment_expression.mc
            ${CMAKE_CURRENT_BINARY_DIR}/postfix_increment_lowering.mpl
            "dassign %__inc_old_"
            "dassign %i (dread i32 %__inc_new_"
            "dassign %x (dread i32 %__inc_old_"
)

add_test(
    NAME maple_nested_function_call_lowering
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_maple_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/increment_expression.mc
            ${CMAKE_CURRENT_BINARY_DIR}/nested_function_call_lowering.mpl
            "callassigned &add (dread i32 %y, dread i32 %z) { dassign %__callret_"
            "callassigned &add (dread i32 %x, dread i32 %__callret_"
            "dassign %r 0 }"
)

add_test(
    NAME maple_nested_loop_lowering
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_maple_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/nested_increment_call.mc
            ${CMAKE_CURRENT_BINARY_DIR}/nested_loop_lowering.mpl
            "@for_cond_0"
            "@for_body_1"
            "@for_update_2"
            "@for_cond_4"
            "@for_body_5"
            "@for_update_6"
            "call &add (dread i32 %sum, dread i32 %i)"
            "call &add (dread i32 %sum, dread i32 %j)"
)

add_test(
    NAME semantic_type_checks
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_semantic_checks.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME semantic_float_ok
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_compiler_success.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/float_ok.mc
)

add_test(
    NAME semantic_char_ok
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_compiler_success.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/char_ok.mc
)

add_test(
    NAME semantic_float_type_mismatch
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_compiler_error.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/float_type_mismatch.mc
            "Semantic error:"
            "initializer for variable 'value'"
            "expected float, got int"
)

add_test(
    NAME semantic_char_type_mismatch
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_compiler_error.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/char_type_mismatch.mc
            "Semantic error:"
            "assignment to variable 'value'"
            "expected char, got int"
)

add_test(
    NAME semantic_function_arg_float_type_mismatch
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_compiler_error.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/function_arg_float_type_mismatch.mc
            "Semantic error:"
            "argument 1 of function 'passthrough'"
            "expected float, got int"
)

add_test(
    NAME semantic_function_arg_char_type_mismatch
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_compiler_error.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/function_arg_char_type_mismatch.mc
            "Semantic error:"
            "argument 1 of function 'passthrough'"
            "expected char, got int"
)

add_test(
    NAME ast_dot_control_flow
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_ast_dot_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/nested_control_flow_call.mc
            ${CMAKE_CURRENT_BINARY_DIR}/nested_control_flow_call.dot
            "digraph AST {"
            "FunctionDecl\\nname=main\\nreturn=int"
            "BlockStmt"
            "ForStmt"
            "IfStmt"
            "WhileStmt"
            "BinaryExpr\\nop=<"
            "BinaryExpr\\nop=+"
            "FunctionCall\\ncallee=add"
            "ReturnStmt"
)

add_test(
    NAME ast_dot_unary_expr
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_ast_dot_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/ast_unary.mc
            ${CMAKE_CURRENT_BINARY_DIR}/ast_unary.dot
            "UnaryExpr\\nop=-"
            "ReturnStmt"
            "Identifier\\nname=value"
)

add_test(
    NAME cfg_if_else
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_cfg_dot_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/if_else.mc
            ${CMAKE_CURRENT_BINARY_DIR}/if_else_cfg.dot
            "digraph CFG {"
            "Function &main"
            "entry (@entry)\\ldassign %a (constval i32 1)\\l"
            "if_cond\\lbrfalse @if_else_0 (dread i32 %a)\\l"
            "true branch"
            "false branch"
            "fallthrough"
)

add_test(
    NAME cfg_while
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_cfg_dot_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/while.mc
            ${CMAKE_CURRENT_BINARY_DIR}/while_cfg.dot
            "while_cond (@while_cond_0)\\lbrfalse @while_end_2 (dread i32 %i)\\lgoto @while_body_1\\l"
            "while_body (@while_body_1)\\ldassign %sum (add i32 (dread i32 %sum, dread i32 %i))\\l"
            "loop back edge"
            "false branch"
            "true branch"
)

add_test(
    NAME cfg_for
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_cfg_dot_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/for_loop.mc
            ${CMAKE_CURRENT_BINARY_DIR}/for_cfg.dot
            "for_init\\ldassign %i (constval i32 0)\\lgoto @for_cond_0\\l"
            "for_cond (@for_cond_0)\\lbrfalse @for_end_3"
            "for_update (@for_update_2)\\ldassign %__inc_old_0"
            "loop back edge"
            "true branch"
            "false branch"
)

add_test(
    NAME cfg_nested_branch
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/assert_cfg_dot_contains.sh
            $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/nested_control_flow_call.mc
            ${CMAKE_CURRENT_BINARY_DIR}/nested_control_flow_cfg.dot
            "for_body (@for_body_1)\\l"
            "if_cond\\lbrfalse @if_else_4 (dread i32 %i)\\l"
            "while_cond (@while_cond_6)\\lbrfalse @while_end_8"
            "call &add (dread i32 %sum, dread i32 %i)\\l"
            "loop back edge"
            "false branch"
)
