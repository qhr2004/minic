#pragma once

#include "AST.h"

#include <iosfwd>
#include <string>
#include <vector>

class IRGenerator {
public:
    std::vector<std::string> generate(const Program& program);
    const std::vector<std::string>& instructions() const;
    void print(std::ostream& os) const;

private:
    std::vector<std::string> instructions_;
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

    std::string generateExpr(const Expr& expr);
    std::string generateAssignmentExpr(const AssignmentExpr& assignmentExpr);
    std::string generateUnaryExpr(const UnaryExpr& unaryExpr);
    std::string generateBinaryExpr(const BinaryExpr& binaryExpr);
};
