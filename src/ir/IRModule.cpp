#include "ir/IRModule.h"

#include <ostream>
#include <utility>

void IRModule::addInstruction(std::string instruction) {
    instructions_.push_back(std::move(instruction));
}

void IRModule::clear() {
    instructions_.clear();
}

const std::vector<std::string>& IRModule::instructions() const {
    return instructions_;
}

void IRModule::print(std::ostream& os) const {
    for (const std::string& instruction : instructions_) {
        os << instruction << '\n';
    }
}
