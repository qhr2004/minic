#include "ast/DotExporter.h"

#include <ostream>
#include <string>

namespace {

std::string escapeDotLabel(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());

    for (char ch : text) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }

    return escaped;
}

std::string literalKindName(LiteralKind kind) {
    switch (kind) {
    case LiteralKind::Integer:
        return "Integer";
    case LiteralKind::Float:
        return "Float";
    case LiteralKind::Char:
        return "Char";
    }

    return "Unknown";
}

class DotEmitter {
public:
    explicit DotEmitter(std::ostream& os)
        : os_(os) {}

    void emit(const ASTNode& node) {
        os_ << "digraph AST {\n";
        os_ << "  rankdir=TB;\n";
        os_ << "  node [shape=box, style=\"rounded\", fontname=\"Courier\"];\n";
        os_ << "  edge [fontname=\"Courier\"];\n";
        emitNode(node);
        os_ << "}\n";
    }

private:
    int emitNode(const ASTNode& node) {
        if (const auto* program = dynamic_cast<const Program*>(&node)) {
            return emitProgram(*program);
        }
        if (const auto* functionDecl = dynamic_cast<const FunctionDecl*>(&node)) {
            return emitFunctionDecl(*functionDecl);
        }
        if (const auto* parameterDecl = dynamic_cast<const ParameterDecl*>(&node)) {
            return emitParameterDecl(*parameterDecl);
        }
        if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(&node)) {
            return emitBlockStmt(*blockStmt);
        }
        if (const auto* varDecl = dynamic_cast<const VarDecl*>(&node)) {
            return emitVarDecl(*varDecl);
        }
        if (const auto* returnStmt = dynamic_cast<const ReturnStmt*>(&node)) {
            return emitReturnStmt(*returnStmt);
        }
        if (const auto* exprStmt = dynamic_cast<const ExprStmt*>(&node)) {
            return emitExprStmt(*exprStmt);
        }
        if (const auto* ifStmt = dynamic_cast<const IfStmt*>(&node)) {
            return emitIfStmt(*ifStmt);
        }
        if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(&node)) {
            return emitWhileStmt(*whileStmt);
        }
        if (const auto* forStmt = dynamic_cast<const ForStmt*>(&node)) {
            return emitForStmt(*forStmt);
        }
        if (const auto* literal = dynamic_cast<const Literal*>(&node)) {
            return emitLiteral(*literal);
        }
        if (const auto* identifier = dynamic_cast<const Identifier*>(&node)) {
            return emitIdentifier(*identifier);
        }
        if (const auto* assignmentExpr = dynamic_cast<const AssignmentExpr*>(&node)) {
            return emitAssignmentExpr(*assignmentExpr);
        }
        if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(&node)) {
            return emitUnaryExpr(*unaryExpr);
        }
        if (const auto* incDecExpr = dynamic_cast<const IncDecExpr*>(&node)) {
            return emitIncDecExpr(*incDecExpr);
        }
        if (const auto* binaryExpr = dynamic_cast<const BinaryExpr*>(&node)) {
            return emitBinaryExpr(*binaryExpr);
        }
        if (const auto* functionCall = dynamic_cast<const FunctionCall*>(&node)) {
            return emitFunctionCall(*functionCall);
        }

        return emitLabel("ASTNode");
    }

    int emitProgram(const Program& program) {
        const int id = emitLabel("Program");
        int index = 0;

        for (const auto& child : program.nodes()) {
            connect(id, "node[" + std::to_string(index++) + "]", emitNode(*child));
        }

        return id;
    }

    int emitFunctionDecl(const FunctionDecl& functionDecl) {
        const int id = emitLabel(
            "FunctionDecl\nname=" + functionDecl.name() + "\nreturn=" + functionDecl.returnType());

        int parameterIndex = 0;
        for (const auto& parameter : functionDecl.parameters()) {
            connect(id, "param[" + std::to_string(parameterIndex++) + "]", emitNode(*parameter));
        }

        connect(id, "body", emitNode(functionDecl.body()));
        return id;
    }

    int emitParameterDecl(const ParameterDecl& parameterDecl) {
        return emitLabel(
            "ParameterDecl\nname=" + parameterDecl.name() + "\ntype=" + parameterDecl.type());
    }

    int emitBlockStmt(const BlockStmt& blockStmt) {
        const int id = emitLabel("BlockStmt");
        int index = 0;

        for (const auto& statement : blockStmt.statements()) {
            connect(id, "stmt[" + std::to_string(index++) + "]", emitNode(*statement));
        }

        return id;
    }

    int emitVarDecl(const VarDecl& varDecl) {
        const int id = emitLabel("VarDecl\nname=" + varDecl.name() + "\ntype=" + varDecl.type());

        if (const Expr* initializer = varDecl.initializer()) {
            connect(id, "init", emitNode(*initializer));
        }

        return id;
    }

    int emitReturnStmt(const ReturnStmt& returnStmt) {
        const int id = emitLabel("ReturnStmt");

        if (const Expr* value = returnStmt.value()) {
            connect(id, "value", emitNode(*value));
        }

        return id;
    }

    int emitExprStmt(const ExprStmt& exprStmt) {
        const int id = emitLabel("ExprStmt");
        connect(id, "expr", emitNode(exprStmt.expression()));
        return id;
    }

    int emitIfStmt(const IfStmt& ifStmt) {
        const int id = emitLabel("IfStmt");
        connect(id, "condition", emitNode(ifStmt.condition()));
        connect(id, "then", emitNode(ifStmt.thenBranch()));

        if (const Stmt* elseBranch = ifStmt.elseBranch()) {
            connect(id, "else", emitNode(*elseBranch));
        }

        return id;
    }

    int emitWhileStmt(const WhileStmt& whileStmt) {
        const int id = emitLabel("WhileStmt");
        connect(id, "condition", emitNode(whileStmt.condition()));
        connect(id, "body", emitNode(whileStmt.body()));
        return id;
    }

    int emitForStmt(const ForStmt& forStmt) {
        const int id = emitLabel("ForStmt");

        if (const Stmt* initializer = forStmt.initializer()) {
            connect(id, "init", emitNode(*initializer));
        }

        if (const Expr* condition = forStmt.condition()) {
            connect(id, "condition", emitNode(*condition));
        }

        if (const Expr* update = forStmt.update()) {
            connect(id, "update", emitNode(*update));
        }

        connect(id, "body", emitNode(forStmt.body()));
        return id;
    }

    int emitLiteral(const Literal& literal) {
        return emitLabel(
            "Literal\nkind=" + literalKindName(literal.kind()) + "\nvalue=" + literal.value());
    }

    int emitIdentifier(const Identifier& identifier) {
        return emitLabel("Identifier\nname=" + identifier.name());
    }

    int emitAssignmentExpr(const AssignmentExpr& assignmentExpr) {
        const int id = emitLabel("AssignmentExpr\nname=" + assignmentExpr.name());
        connect(id, "value", emitNode(assignmentExpr.value()));
        return id;
    }

    int emitUnaryExpr(const UnaryExpr& unaryExpr) {
        const int id = emitLabel("UnaryExpr\nop=" + unaryExpr.op());
        connect(id, "operand", emitNode(unaryExpr.operand()));
        return id;
    }

    int emitIncDecExpr(const IncDecExpr& incDecExpr) {
        return emitLabel(
            "IncDecExpr\nform=" + std::string(incDecExpr.isPostfix() ? "postfix" : "prefix")
            + "\nop=" + incDecExpr.op() + "\nname=" + incDecExpr.name());
    }

    int emitBinaryExpr(const BinaryExpr& binaryExpr) {
        const int id = emitLabel("BinaryExpr\nop=" + binaryExpr.op());
        connect(id, "left", emitNode(binaryExpr.left()));
        connect(id, "right", emitNode(binaryExpr.right()));
        return id;
    }

    int emitFunctionCall(const FunctionCall& functionCall) {
        const int id = emitLabel("FunctionCall\ncallee=" + functionCall.callee());
        int index = 0;

        for (const auto& argument : functionCall.arguments()) {
            connect(id, "arg[" + std::to_string(index++) + "]", emitNode(*argument));
        }

        return id;
    }

    int emitLabel(const std::string& label) {
        const int id = nextNodeId_++;
        os_ << "  node" << id << " [label=\"" << escapeDotLabel(label) << "\"];\n";
        return id;
    }

    void connect(int from, const std::string& label, int to) {
        os_ << "  node" << from << " -> node" << to
            << " [label=\"" << escapeDotLabel(label) << "\"];\n";
    }

    std::ostream& os_;
    int nextNodeId_ = 0;
};

} // namespace

void writeASTDot(const ASTNode& node, std::ostream& os) {
    DotEmitter emitter(os);
    emitter.emit(node);
}
