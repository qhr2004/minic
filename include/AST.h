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
    FunctionDecl(std::string returnType, std::string name, std::unique_ptr<BlockStmt> body);

    const std::string& returnType() const;
    const std::string& name() const;
    const BlockStmt& body() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    std::string returnType_;
    std::string name_;
    std::unique_ptr<BlockStmt> body_;
};

class VarDeclStmt : public Stmt {
public:
    VarDeclStmt(std::string type, std::string name, ExprPtr initializer);

    const std::string& type() const;
    const std::string& name() const;
    const Expr* initializer() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    std::string type_;
    std::string name_;
    ExprPtr initializer_;
};

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

class IntegerExpr : public Expr {
public:
    explicit IntegerExpr(std::string value);

    const std::string& value() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    std::string value_;
};

class IdentifierExpr : public Expr {
public:
    explicit IdentifierExpr(std::string name);

    const std::string& name() const;
    void print(std::ostream& os, int indent = 0) const override;

private:
    std::string name_;
};

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
