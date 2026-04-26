#include "SymbolTable.h"

#include <stdexcept>

std::string valueTypeToString(ValueType type) {
    switch (type) {
    case ValueType::Int:
        return "int";
    case ValueType::Float:
        return "float";
    case ValueType::Char:
        return "char";
    case ValueType::Void:
        return "void";
    case ValueType::Unknown:
        return "unknown";
    }

    return "unknown";
}

ValueType valueTypeFromString(const std::string& type) {
    if (type == "int") {
        return ValueType::Int;
    }

    if (type == "float") {
        return ValueType::Float;
    }

    if (type == "char") {
        return ValueType::Char;
    }

    if (type == "void") {
        return ValueType::Void;
    }

    return ValueType::Unknown;
}

void SymbolTable::enterScope() {
    scopes_.emplace_back();
}

void SymbolTable::exitScope() {
    if (scopes_.empty()) {
        throw std::runtime_error("Cannot exit scope: symbol table has no active scope");
    }

    scopes_.pop_back();
}

bool SymbolTable::declare(const std::string& name, ValueType type) {
    if (scopes_.empty()) {
        enterScope();
    }

    auto& currentScope = scopes_.back();
    if (currentScope.find(name) != currentScope.end()) {
        return false;
    }

    currentScope.emplace(name, Symbol{name, type});
    return true;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
        auto found = scope->find(name);
        if (found != scope->end()) {
            return &found->second;
        }
    }

    return nullptr;
}

const Symbol* SymbolTable::lookupCurrentScope(const std::string& name) const {
    if (scopes_.empty()) {
        return nullptr;
    }

    const auto& currentScope = scopes_.back();
    auto found = currentScope.find(name);
    if (found == currentScope.end()) {
        return nullptr;
    }

    return &found->second;
}

bool SymbolTable::empty() const {
    return scopes_.empty();
}

std::size_t SymbolTable::scopeDepth() const {
    return scopes_.size();
}
