#include "codegen/CodeGenerator.h"

#include <sstream>

namespace {
bool isLabel(const std::string& instruction) {
    return !instruction.empty() && instruction.back() == ':';
}
}

std::string CodeGenerator::generate(const IRModule& module) const {
    std::ostringstream oss;
    oss << "; pseudo target generated from MiniC IR\n";

    for (const std::string& instruction : module.instructions()) {
        oss << lowerInstruction(instruction) << '\n';
    }

    return oss.str();
}

std::string CodeGenerator::lowerInstruction(const std::string& instruction) const {
    if (isLabel(instruction)) {
        return instruction;
    }

    if (instruction.rfind("var ", 0) == 0) {
        return "alloc " + instruction.substr(4);
    }

    if (instruction.rfind("return", 0) == 0) {
        return "ret" + instruction.substr(6);
    }

    return "emit " + instruction;
}
