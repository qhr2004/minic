#pragma once

#include "ast/AST.h"
#include "semantic/SymbolTable.h"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class SemanticError : public std::runtime_error {
public:
    explicit SemanticError(const std::string& message);
};

struct FunctionSignature {
    std::string name;
    ValueType returnType = ValueType::Unknown;
    std::vector<ValueType> parameterTypes;
};

class SemanticAnalyzer {
public:
    void analyze(const Program& program);

private:
    SymbolTable symbols_;
    std::unordered_map<std::string, FunctionSignature> functions_;
    ValueType currentReturnType_ = ValueType::Unknown;

    void registerFunctions(const Program& program);
    FunctionSignature buildFunctionSignature(const FunctionDecl& functionDecl) const;

    void analyzeNode(const ASTNode& node);
    void analyzeFunctionDecl(const FunctionDecl& functionDecl);
    void analyzeFunctionBody(const BlockStmt& body);
    void analyzeBlockStmt(const BlockStmt& blockStmt);
    void analyzeStmt(const Stmt& stmt);
    void analyzeVarDecl(const VarDecl& varDecl);
    void analyzeReturnStmt(const ReturnStmt& returnStmt);
    void analyzeIfStmt(const IfStmt& ifStmt);
    void analyzeWhileStmt(const WhileStmt& whileStmt);
    void analyzeForStmt(const ForStmt& forStmt);
    void declareParameters(const FunctionDecl& functionDecl);

    ValueType analyzeExpr(const Expr& expr);
    ValueType analyzeLiteral(const Literal& literal) const;
    ValueType analyzeIdentifier(const Identifier& identifier);
    ValueType analyzeAssignmentExpr(const AssignmentExpr& assignmentExpr);
    ValueType analyzeUnaryExpr(const UnaryExpr& unaryExpr);
    ValueType analyzeIncDecExpr(const IncDecExpr& incDecExpr);
    ValueType analyzeBinaryExpr(const BinaryExpr& binaryExpr);
    ValueType analyzeFunctionCall(const FunctionCall& functionCall);
    ValueType resolveDeclaredType(const std::string& typeName, const std::string& context) const;

    void requireSameType(ValueType expected, ValueType actual, const std::string& context) const;
    bool isNumericType(ValueType type) const;
    bool isIntegralType(ValueType type) const;
};
