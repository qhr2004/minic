#pragma once

#include <iosfwd>
#include <string>
#include <vector>

class IRModule {
public:
    void addInstruction(std::string instruction);
    void clear();

    const std::vector<std::string>& instructions() const;
    void print(std::ostream& os) const;

private:
    std::vector<std::string> instructions_;
};
