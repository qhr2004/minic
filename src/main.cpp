#include "IRGenerator.h"
#include "Lexer.h"
#include "MapleIRGenerator.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>

std::string readFile(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: minic <source.mc>" << std::endl;
        return 1;
    }

    try {
        std::string source = readFile(argv[1]);
        Lexer lexer(source);
        Parser parser(lexer.tokenize());
        std::unique_ptr<Program> program = parser.parseProgram();
        SemanticAnalyzer semanticAnalyzer;
        semanticAnalyzer.analyze(*program);

        IRGenerator irGenerator;
        irGenerator.generate(*program);
        std::cout << "=== Three Address Code IR ===" << std::endl;
        irGenerator.print(std::cout);

        MapleIRGenerator mapleIRGenerator;
        mapleIRGenerator.generate(*program);
        std::cout << std::endl;
        std::cout << "=== Simplified Maple IR ===" << std::endl;
        mapleIRGenerator.print(std::cout);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
