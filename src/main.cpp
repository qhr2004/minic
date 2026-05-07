#include "ast/DotExporter.h"
#include "codegen/CodeGenerator.h"
#include "ir/IRGenerator.h"
#include "lexer/Lexer.h"
#include "lexer/Token.h"
#include "maple/CFGDotExporter.h"
#include "maple/IRGenerator.h"
#include "parser/Parser.h"
#include "semantic/SemanticAnalyzer.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

std::string readFile(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void printTokens(const std::vector<Token>& tokens) {
    for (const Token& token : tokens) {
        std::cout << token.line << ":" << token.column
                  << "  " << tokenTypeToString(token.type)
                  << "  \"" << token.text << "\""
                  << std::endl;
    }
}

int main(int argc, char* argv[]) {
    const bool tokenMode = argc >= 3 && std::string(argv[1]) == "--tokens";
    const bool exprMode = argc >= 3 && std::string(argv[1]) == "--expr";
    const bool emitASTDotMode = argc >= 4 && std::string(argv[1]) == "--emit-ast-dot";
    const bool emitCFGDotMode = argc >= 4 && std::string(argv[1]) == "--emit-cfg-dot";
    const bool emitMapleMode = argc >= 4 && std::string(argv[1]) == "--emit-maple";
    const bool emitABCMode = argc >= 4 && std::string(argv[1]) == "--emit-abc";
    const std::string sourcePath = tokenMode
        ? argv[2]
        : (exprMode ? ""
                    : ((emitASTDotMode || emitCFGDotMode || emitMapleMode || emitABCMode)
                           ? argv[2]
                           : (argc >= 2 ? argv[1] : "tests/test.mc")));
    const std::string outputPath = (emitASTDotMode || emitCFGDotMode || emitMapleMode || emitABCMode) ? argv[3] : "";

    try {
        std::string source = exprMode ? argv[2] : readFile(sourcePath);
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        if (tokenMode) {
            printTokens(tokens);
            return 0;
        }

        Parser parser(std::move(tokens));

        if (exprMode) {
            ExprPtr expression = parser.parseStandaloneExpression();
            expression->print(std::cout);
            return 0;
        }

        std::unique_ptr<Program> program = parser.parseProgram();

        if (emitASTDotMode) {
            std::ofstream file(outputPath);

            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file: " + outputPath);
            }

            writeASTDot(*program, file);
            std::cout << "AST DOT written to " << outputPath << std::endl;
            return 0;
        }

        SemanticAnalyzer semanticAnalyzer;
        semanticAnalyzer.analyze(*program);

        if (emitCFGDotMode) {
            maple::IRGenerator mapleGenerator;
            const maple::Module& module = mapleGenerator.generate(*program);
            std::ofstream file(outputPath);

            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file: " + outputPath);
            }

            maple::writeCFGDot(module, file);
            std::cout << "CFG DOT written to " << outputPath << std::endl;
            return 0;
        }

        if (emitMapleMode) {
            maple::IRGenerator mapleGenerator;
            mapleGenerator.generate(*program);
            mapleGenerator.writeToFile(outputPath);
            std::cout << "Maple IR written to " << outputPath << std::endl;
            return 0;
        }

        if (emitABCMode) {
            const std::string abcPath = maple::generate_abc(*program, outputPath);
            std::cout << "ABC written to " << abcPath << std::endl;
            return 0;
        }

        IRGenerator irGenerator;
        const IRModule& irModule = irGenerator.generate(*program);

        CodeGenerator codeGenerator;
        const std::string targetCode = codeGenerator.generate(irModule);

        std::cout << "=== AST ===" << std::endl;
        program->print(std::cout);

        std::cout << std::endl;
        std::cout << "=== IR ===" << std::endl;
        irModule.print(std::cout);

        std::cout << std::endl;
        std::cout << "=== Codegen ===" << std::endl;
        std::cout << targetCode;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
