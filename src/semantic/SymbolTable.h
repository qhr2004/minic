#pragma once

#include <cstddef>
#include <memory>
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

enum class SymbolKind {
    Variable,
    Parameter,
    Function
};

struct Symbol {
    std::string name;
    ValueType type;
    SymbolKind kind = SymbolKind::Variable;
    std::vector<ValueType> parameterTypes;
};

std::string valueTypeToString(ValueType type);
ValueType valueTypeFromString(const std::string& type);

class SymbolTable {
public:
    void clear();
    void enterScope();
    void exitScope();

    bool declare(const Symbol& symbol);
    bool declare(const std::string& name, ValueType type, SymbolKind kind = SymbolKind::Variable, std::vector<ValueType> parameterTypes = {});
    const Symbol* lookup(const std::string& name) const;
    const Symbol* lookupCurrentScope(const std::string& name) const;

    bool empty() const;
    std::size_t scopeDepth() const;

private:
    struct Scope {
        explicit Scope(Scope* parentScope) : parent(parentScope) {}

        Scope* parent;
        std::unordered_map<std::string, Symbol> symbols;
    };

    Scope* currentScope_ = nullptr;
    std::vector<std::unique_ptr<Scope>> ownedScopes_;
};
