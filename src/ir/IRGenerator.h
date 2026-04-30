#pragma once

#include "ast/AST.h"
#include "ir/IRModule.h"

#include <iosfwd>
#include <string>

class IRGenerator {
public:
    const IRModule& generate(const Program& program);
    const IRModule& module() const;

private:
    IRModule module_;
    int tempCounter_ = 0;
    int labelCounter_ = 0;

    void emit(const std::string& instruction);
    std::string newTemp();
    std::string newLabel();

    void generateNode(const ASTNode& node);
    void generateFunctionDecl(const FunctionDecl& functionDecl);
    void generateBlockStmt(const BlockStmt& blockStmt);
    void generateStmt(const Stmt& stmt);
    void generateVarDeclStmt(const VarDeclStmt& varDeclStmt);
    void generateReturnStmt(const ReturnStmt& returnStmt);
    void generateIfStmt(const IfStmt& ifStmt);
    void generateWhileStmt(const WhileStmt& whileStmt);
    void generateForStmt(const ForStmt& forStmt);

    std::string generateExpr(const Expr& expr);
    std::string generateAssignmentExpr(const AssignmentExpr& assignmentExpr);
    std::string generateUnaryExpr(const UnaryExpr& unaryExpr);
    std::string generateIncDecExpr(const IncDecExpr& incDecExpr);
    std::string generateBinaryExpr(const BinaryExpr& binaryExpr);
    std::string generateFunctionCall(const FunctionCall& functionCall);
};
