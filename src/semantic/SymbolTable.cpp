#include "semantic/SymbolTable.h"

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

void SymbolTable::clear() {
    currentScope_ = nullptr;
    ownedScopes_.clear();
}

void SymbolTable::enterScope() {
    ownedScopes_.push_back(std::make_unique<Scope>(currentScope_));
    currentScope_ = ownedScopes_.back().get();
}

void SymbolTable::exitScope() {
    if (currentScope_ == nullptr) {
        throw std::runtime_error("Cannot exit scope: symbol table has no active scope");
    }

    currentScope_ = currentScope_->parent;
}

bool SymbolTable::declare(const Symbol& symbol) {
    if (currentScope_ == nullptr) {
        enterScope();
    }

    auto& symbols = currentScope_->symbols;
    if (symbols.find(symbol.name) != symbols.end()) {
        return false;
    }

    symbols.emplace(symbol.name, symbol);
    return true;
}

bool SymbolTable::declare(const std::string& name, ValueType type, SymbolKind kind, std::vector<ValueType> parameterTypes) {
    return declare(Symbol{name, type, kind, std::move(parameterTypes)});
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    for (Scope* scope = currentScope_; scope != nullptr; scope = scope->parent) {
        auto found = scope->symbols.find(name);
        if (found != scope->symbols.end()) {
            return &found->second;
        }
    }

    return nullptr;
}

const Symbol* SymbolTable::lookupCurrentScope(const std::string& name) const {
    if (currentScope_ == nullptr) {
        return nullptr;
    }

    auto found = currentScope_->symbols.find(name);
    if (found == currentScope_->symbols.end()) {
        return nullptr;
    }

    return &found->second;
}

bool SymbolTable::empty() const {
    return currentScope_ == nullptr;
}

std::size_t SymbolTable::scopeDepth() const {
    std::size_t depth = 0;

    for (Scope* scope = currentScope_; scope != nullptr; scope = scope->parent) {
        ++depth;
    }

    return depth;
}
