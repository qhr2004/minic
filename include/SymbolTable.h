#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

enum class ValueType {
    Int,
    Float,
    Char,
    Void,
    Unknown
};

struct Symbol {
    std::string name;
    ValueType type;
};

std::string valueTypeToString(ValueType type);
ValueType valueTypeFromString(const std::string& type);

class SymbolTable {
public:
    void enterScope();
    void exitScope();

    bool declare(const std::string& name, ValueType type);
    const Symbol* lookup(const std::string& name) const;
    const Symbol* lookupCurrentScope(const std::string& name) const;

    bool empty() const;
    std::size_t scopeDepth() const;

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
};
