#pragma once

#include "ir/IRModule.h"

#include <string>

class CodeGenerator {
public:
    std::string generate(const IRModule& module) const;

private:
    std::string lowerInstruction(const std::string& instruction) const;
};
