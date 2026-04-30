#include "ast/AST.h"

#include <iostream>
#include <utility>

namespace {
void printIndent(std::ostream& os, int indent) {
    for (int i = 0; i < indent; ++i) {
        os << ' ';
    }
}

void printChildLabel(std::ostream& os, int indent, const std::string& label) {
    printIndent(os, indent);
    os << label << '\n';
}
}

void printAST(const ASTNode& node, std::ostream& os) {
    node.print(os);
}

ParameterDecl::ParameterDecl(std::string type, std::string name)
    : type_(std::move(type)),
      name_(std::move(name)) {}

const std::string& ParameterDecl::type() const {
    return type_;
}

const std::string& ParameterDecl::name() const {
    return name_;
}

void ParameterDecl::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "Parameter " << type_ << ' ' << name_ << "\n";
}

void Program::addNode(std::unique_ptr<ASTNode> node) {
    nodes_.push_back(std::move(node));
}

const std::vector<std::unique_ptr<ASTNode>>& Program::nodes() const {
    return nodes_;
}

void Program::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "Program\n";

    for (const auto& node : nodes_) {
        node->print(os, indent + 2);
    }
}

void BlockStmt::addStatement(StmtPtr statement) {
    statements_.push_back(std::move(statement));
}

const std::vector<StmtPtr>& BlockStmt::statements() const {
    return statements_;
}

void BlockStmt::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "BlockStmt\n";

    for (const auto& statement : statements_) {
        statement->print(os, indent + 2);
    }
}

FunctionDecl::FunctionDecl(std::string returnType, std::string name, std::vector<ParameterPtr> parameters, std::unique_ptr<BlockStmt> body)
    : returnType_(std::move(returnType)),
      name_(std::move(name)),
      parameters_(std::move(parameters)),
      body_(std::move(body)) {}

const std::string& FunctionDecl::returnType() const {
    return returnType_;
}

const std::string& FunctionDecl::name() const {
    return name_;
}

const std::vector<ParameterPtr>& FunctionDecl::parameters() const {
    return parameters_;
}

const BlockStmt& FunctionDecl::body() const {
    return *body_;
}

void FunctionDecl::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "FunctionDecl " << returnType_ << ' ' << name_ << "\n";

    if (!parameters_.empty()) {
        printChildLabel(os, indent + 2, "Parameters");

        for (const auto& parameter : parameters_) {
            parameter->print(os, indent + 4);
        }
    }

    body_->print(os, indent + 2);
}

VarDecl::VarDecl(std::string type, std::string name, ExprPtr initializer)
    : type_(std::move(type)),
      name_(std::move(name)),
      initializer_(std::move(initializer)) {}

const std::string& VarDecl::type() const {
    return type_;
}

const std::string& VarDecl::name() const {
    return name_;
}

const Expr* VarDecl::initializer() const {
    return initializer_.get();
}

void VarDecl::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "VarDecl " << type_ << ' ' << name_ << "\n";

    if (initializer_) {
        printChildLabel(os, indent + 2, "Initializer");
        initializer_->print(os, indent + 4);
    }
}

ReturnStmt::ReturnStmt(ExprPtr value)
    : value_(std::move(value)) {}

const Expr* ReturnStmt::value() const {
    return value_.get();
}

void ReturnStmt::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "ReturnStmt\n";

    if (value_) {
        value_->print(os, indent + 2);
    }
}

ExprStmt::ExprStmt(ExprPtr expression)
    : expression_(std::move(expression)) {}

const Expr& ExprStmt::expression() const {
    return *expression_;
}

void ExprStmt::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "ExprStmt\n";
    expression_->print(os, indent + 2);
}

IfStmt::IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch)
    : condition_(std::move(condition)),
      thenBranch_(std::move(thenBranch)),
      elseBranch_(std::move(elseBranch)) {}

const Expr& IfStmt::condition() const {
    return *condition_;
}

const Stmt& IfStmt::thenBranch() const {
    return *thenBranch_;
}

const Stmt* IfStmt::elseBranch() const {
    return elseBranch_.get();
}

void IfStmt::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "IfStmt\n";

    printChildLabel(os, indent + 2, "Condition");
    condition_->print(os, indent + 4);

    printChildLabel(os, indent + 2, "Then");
    thenBranch_->print(os, indent + 4);

    if (elseBranch_) {
        printChildLabel(os, indent + 2, "Else");
        elseBranch_->print(os, indent + 4);
    }
}

WhileStmt::WhileStmt(ExprPtr condition, StmtPtr body)
    : condition_(std::move(condition)),
      body_(std::move(body)) {}

const Expr& WhileStmt::condition() const {
    return *condition_;
}

const Stmt& WhileStmt::body() const {
    return *body_;
}

void WhileStmt::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "WhileStmt\n";

    printChildLabel(os, indent + 2, "Condition");
    condition_->print(os, indent + 4);

    printChildLabel(os, indent + 2, "Body");
    body_->print(os, indent + 4);
}

ForStmt::ForStmt(StmtPtr initializer, ExprPtr condition, ExprPtr update, StmtPtr body)
    : initializer_(std::move(initializer)),
      condition_(std::move(condition)),
      update_(std::move(update)),
      body_(std::move(body)) {}

const Stmt* ForStmt::initializer() const {
    return initializer_.get();
}

const Expr* ForStmt::condition() const {
    return condition_.get();
}

const Expr* ForStmt::update() const {
    return update_.get();
}

const Stmt& ForStmt::body() const {
    return *body_;
}

void ForStmt::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "ForStmt\n";

    if (initializer_) {
        printChildLabel(os, indent + 2, "Initializer");
        initializer_->print(os, indent + 4);
    }

    if (condition_) {
        printChildLabel(os, indent + 2, "Condition");
        condition_->print(os, indent + 4);
    }

    if (update_) {
        printChildLabel(os, indent + 2, "Update");
        update_->print(os, indent + 4);
    }

    printChildLabel(os, indent + 2, "Body");
    body_->print(os, indent + 4);
}

Literal::Literal(std::string value, LiteralKind kind)
    : value_(std::move(value)),
      kind_(kind) {}

const std::string& Literal::value() const {
    return value_;
}

LiteralKind Literal::kind() const {
    return kind_;
}

void Literal::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "Literal " << value_ << "\n";
}

Identifier::Identifier(std::string name)
    : name_(std::move(name)) {}

const std::string& Identifier::name() const {
    return name_;
}

void Identifier::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "Identifier " << name_ << "\n";
}

AssignmentExpr::AssignmentExpr(std::string name, ExprPtr value)
    : name_(std::move(name)),
      value_(std::move(value)) {}

const std::string& AssignmentExpr::name() const {
    return name_;
}

const Expr& AssignmentExpr::value() const {
    return *value_;
}

void AssignmentExpr::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "AssignmentExpr " << name_ << "\n";
    value_->print(os, indent + 2);
}

UnaryExpr::UnaryExpr(std::string op, ExprPtr operand)
    : op_(std::move(op)),
      operand_(std::move(operand)) {}

const std::string& UnaryExpr::op() const {
    return op_;
}

const Expr& UnaryExpr::operand() const {
    return *operand_;
}

void UnaryExpr::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "UnaryExpr " << op_ << "\n";
    operand_->print(os, indent + 2);
}

IncDecExpr::IncDecExpr(std::string op, std::string name, bool isPostfix)
    : op_(std::move(op)),
      name_(std::move(name)),
      isPostfix_(isPostfix) {}

const std::string& IncDecExpr::op() const {
    return op_;
}

const std::string& IncDecExpr::name() const {
    return name_;
}

bool IncDecExpr::isPostfix() const {
    return isPostfix_;
}

void IncDecExpr::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "IncDecExpr " << (isPostfix_ ? "postfix " : "prefix ") << op_ << ' ' << name_ << "\n";
}

BinaryExpr::BinaryExpr(std::string op, ExprPtr left, ExprPtr right)
    : op_(std::move(op)),
      left_(std::move(left)),
      right_(std::move(right)) {}

const std::string& BinaryExpr::op() const {
    return op_;
}

const Expr& BinaryExpr::left() const {
    return *left_;
}

const Expr& BinaryExpr::right() const {
    return *right_;
}

void BinaryExpr::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "BinaryExpr " << op_ << "\n";
    left_->print(os, indent + 2);
    right_->print(os, indent + 2);
}

FunctionCall::FunctionCall(std::string callee, std::vector<ExprPtr> arguments)
    : callee_(std::move(callee)),
      arguments_(std::move(arguments)) {}

const std::string& FunctionCall::callee() const {
    return callee_;
}

const std::vector<ExprPtr>& FunctionCall::arguments() const {
    return arguments_;
}

void FunctionCall::print(std::ostream& os, int indent) const {
    printIndent(os, indent);
    os << "FunctionCall " << callee_ << "\n";

    for (const auto& argument : arguments_) {
        argument->print(os, indent + 2);
    }
}
