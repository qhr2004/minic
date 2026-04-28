#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

class ASTNode {
public:
    virtual ~ASTNode() = default;

    virtual void print(std::ostream& os, int indent = 0) const = 0;
};

void printAST(const ASTNode& node, std::ostream& os);

class Expr : public ASTNode {
public:
    ~Expr() override = default;
};

class Stmt : public ASTNode {
public:
    ~Stmt() override = default;
};

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

enum class LiteralKind {
    Integer,
    Float,
    Char
};

class ParameterDecl : public ASTNode {
public:
    ParameterDecl(std::string type, std::string name);

    const std::string& type() const;
    const std::string& name() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    std::string type_;
    std::string name_;
};

using ParameterPtr = std::unique_ptr<ParameterDecl>;

class Program : public ASTNode {
public:
    void addNode(std::unique_ptr<ASTNode> node);
    const std::vector<std::unique_ptr<ASTNode>>& nodes() const;

    void print(std::ostream& os, int indent = 0) const override;

private:
    std::vector<std::unique_ptr<ASTNode>> nodes_;
};

class BlockStmt : public Stmt {
public:
    void addStatement(StmtPtr statement);
    const std::vector<StmtPtr>& statements() const;

    void print(std::ostream& os, int indent = 0) const override;

private:
    std::vector<StmtPtr> statements_;
};

class FunctionDecl : public ASTNode {
public:
    FunctionDecl(std::string returnType, std::string name, std::vector<ParameterPtr> parameters, std::unique_ptr<BlockStmt> body);

    const std::string& returnType() const;
    const std::string& name() const;
    const std::vector<ParameterPtr>& parameters() const;
    const BlockStmt& body() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    std::string returnType_;
    std::string name_;
    std::vector<ParameterPtr> parameters_;
    std::unique_ptr<BlockStmt> body_;
};

class VarDecl : public Stmt {
public:
    VarDecl(std::string type, std::string name, ExprPtr initializer);

    const std::string& type() const;
    const std::string& name() const;
    const Expr* initializer() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    std::string type_;
    std::string name_;
    ExprPtr initializer_;
};

using VarDeclStmt = VarDecl;

class ReturnStmt : public Stmt {
public:
    explicit ReturnStmt(ExprPtr value);

    const Expr* value() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    ExprPtr value_;
};

class ExprStmt : public Stmt {
public:
    explicit ExprStmt(ExprPtr expression);

    const Expr& expression() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    ExprPtr expression_;
};

class IfStmt : public Stmt {
public:
    IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch);

    const Expr& condition() const;
    const Stmt& thenBranch() const;
    const Stmt* elseBranch() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    ExprPtr condition_;
    StmtPtr thenBranch_;
    StmtPtr elseBranch_;
};

class WhileStmt : public Stmt {
public:
    WhileStmt(ExprPtr condition, StmtPtr body);

    const Expr& condition() const;
    const Stmt& body() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    ExprPtr condition_;
    StmtPtr body_;
};

class Literal : public Expr {
public:
    Literal(std::string value, LiteralKind kind);

    const std::string& value() const;
    LiteralKind kind() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    std::string value_;
    LiteralKind kind_;
};

using IntegerExpr = Literal;

class Identifier : public Expr {
public:
    explicit Identifier(std::string name);

    const std::string& name() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    std::string name_;
};

using IdentifierExpr = Identifier;

class AssignmentExpr : public Expr {
public:
    AssignmentExpr(std::string name, ExprPtr value);

    const std::string& name() const;
    const Expr& value() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    std::string name_;
    ExprPtr value_;
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(std::string op, ExprPtr operand);

    const std::string& op() const;
    const Expr& operand() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    std::string op_;
    ExprPtr operand_;
};

class BinaryExpr : public Expr {
public:
    BinaryExpr(std::string op, ExprPtr left, ExprPtr right);

    const std::string& op() const;
    const Expr& left() const;
    const Expr& right() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    std::string op_;
    ExprPtr left_;
    ExprPtr right_;
};
