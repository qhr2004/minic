#pragma once

#include "AST.h"
#include "SymbolTable.h"

#include <stdexcept>
#include <string>

class SemanticError : public std::runtime_error {
public:
    explicit SemanticError(const std::string& message);
};

class SemanticAnalyzer {
public:
    void analyze(const Program& program);

private:
    SymbolTable symbols_;
    ValueType currentReturnType_ = ValueType::Unknown;

    void analyzeNode(const ASTNode& node);
    void analyzeFunctionDecl(const FunctionDecl& functionDecl);
    void analyzeBlockStmt(const BlockStmt& blockStmt);
    void analyzeStmt(const Stmt& stmt);
    void analyzeVarDeclStmt(const VarDeclStmt& varDeclStmt);
    void analyzeReturnStmt(const ReturnStmt& returnStmt);
    void analyzeIfStmt(const IfStmt& ifStmt);
    void analyzeWhileStmt(const WhileStmt& whileStmt);

    ValueType analyzeExpr(const Expr& expr);
    ValueType analyzeIdentifierExpr(const IdentifierExpr& identifierExpr);
    ValueType analyzeAssignmentExpr(const AssignmentExpr& assignmentExpr);
    ValueType analyzeUnaryExpr(const UnaryExpr& unaryExpr);
    ValueType analyzeBinaryExpr(const BinaryExpr& binaryExpr);

    void requireSameType(ValueType expected, ValueType actual, const std::string& context) const;
    bool isNumericType(ValueType type) const;
};
