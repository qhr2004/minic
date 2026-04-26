#include "MapleIRGenerator.h"

#include <iostream>
#include <stdexcept>

std::vector<std::string> MapleIRGenerator::generate(const Program& program) {
    instructions_.clear();
    tempCounter_ = 0;
    labelCounter_ = 0;
    indentLevel_ = 0;

    for (const auto& node : program.nodes()) {
        generateNode(*node);
    }

    return instructions_;
}

const std::vector<std::string>& MapleIRGenerator::instructions() const {
    return instructions_;
}

void MapleIRGenerator::print(std::ostream& os) const {
    for (const std::string& instruction : instructions_) {
        os << instruction << '\n';
    }
}

void MapleIRGenerator::emit(const std::string& instruction) {
    std::string line;
    for (int i = 0; i < indentLevel_; ++i) {
        line += "   ";
    }

    line += instruction;
    instructions_.push_back(line);
}

std::string MapleIRGenerator::newTemp() {
    return "%t" + std::to_string(tempCounter_++);
}

std::string MapleIRGenerator::newLabel() {
    return "L" + std::to_string(labelCounter_++);
}

std::string MapleIRGenerator::typeName(const std::string& sourceType) const {
    if (sourceType == "int") {
        return "i32";
    }

    if (sourceType == "float") {
        return "f32";
    }

    if (sourceType == "char") {
        return "i8";
    }

    if (sourceType == "void") {
        return "void";
    }

    return "unknown";
}

std::string MapleIRGenerator::variableName(const std::string& name) const {
    return "%" + name;
}

std::string MapleIRGenerator::mapleOp(const std::string& op) const {
    if (op == "+") {
        return "add";
    }

    if (op == "-") {
        return "sub";
    }

    if (op == "*") {
        return "mul";
    }

    if (op == "/") {
        return "div";
    }

    if (op == ">") {
        return "gt";
    }

    if (op == ">=") {
        return "ge";
    }

    if (op == "<") {
        return "lt";
    }

    if (op == "<=") {
        return "le";
    }

    if (op == "==") {
        return "eq";
    }

    if (op == "!=") {
        return "ne";
    }

    return op;
}

void MapleIRGenerator::generateNode(const ASTNode& node) {
    if (const auto* functionDecl = dynamic_cast<const FunctionDecl*>(&node)) {
        generateFunctionDecl(*functionDecl);
        return;
    }

    if (const auto* stmt = dynamic_cast<const Stmt*>(&node)) {
        generateStmt(*stmt);
        return;
    }

    throw std::runtime_error("Maple IR generation error: unsupported top-level AST node");
}

void MapleIRGenerator::generateFunctionDecl(const FunctionDecl& functionDecl) {
    emit("func &" + functionDecl.name() + "() " + typeName(functionDecl.returnType()) + " {");
    ++indentLevel_;
    generateBlockStmt(functionDecl.body());
    --indentLevel_;
    emit("}");
}

void MapleIRGenerator::generateBlockStmt(const BlockStmt& blockStmt) {
    for (const auto& statement : blockStmt.statements()) {
        generateStmt(*statement);
    }
}

void MapleIRGenerator::generateStmt(const Stmt& stmt) {
    if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(&stmt)) {
        generateBlockStmt(*blockStmt);
        return;
    }

    if (const auto* varDeclStmt = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        generateVarDeclStmt(*varDeclStmt);
        return;
    }

    if (const auto* returnStmt = dynamic_cast<const ReturnStmt*>(&stmt)) {
        generateReturnStmt(*returnStmt);
        return;
    }

    if (const auto* exprStmt = dynamic_cast<const ExprStmt*>(&stmt)) {
        generateExpr(exprStmt->expression());
        return;
    }

    if (const auto* ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        generateIfStmt(*ifStmt);
        return;
    }

    if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        generateWhileStmt(*whileStmt);
        return;
    }

    throw std::runtime_error("Maple IR generation error: unsupported statement node");
}

void MapleIRGenerator::generateVarDeclStmt(const VarDeclStmt& varDeclStmt) {
    emit("var " + variableName(varDeclStmt.name()) + " " + typeName(varDeclStmt.type()));

    if (const Expr* initializer = varDeclStmt.initializer()) {
        const std::string value = generateExpr(*initializer);
        emit("dassign " + variableName(varDeclStmt.name()) + " " + value);
    }
}

void MapleIRGenerator::generateReturnStmt(const ReturnStmt& returnStmt) {
    const Expr* value = returnStmt.value();

    if (value == nullptr) {
        emit("return");
        return;
    }

    emit("return " + generateExpr(*value));
}

void MapleIRGenerator::generateIfStmt(const IfStmt& ifStmt) {
    const std::string elseLabel = newLabel();
    const std::string endLabel = newLabel();
    const std::string condition = generateExpr(ifStmt.condition());

    emit("brfalse " + condition + " @" + elseLabel);
    generateStmt(ifStmt.thenBranch());
    emit("goto @" + endLabel);
    emit("label @" + elseLabel);

    if (const Stmt* elseBranch = ifStmt.elseBranch()) {
        generateStmt(*elseBranch);
    }

    emit("label @" + endLabel);
}

void MapleIRGenerator::generateWhileStmt(const WhileStmt& whileStmt) {
    const std::string startLabel = newLabel();
    const std::string endLabel = newLabel();

    emit("label @" + startLabel);
    const std::string condition = generateExpr(whileStmt.condition());
    emit("brfalse " + condition + " @" + endLabel);
    generateStmt(whileStmt.body());
    emit("goto @" + startLabel);
    emit("label @" + endLabel);
}

std::string MapleIRGenerator::generateExpr(const Expr& expr) {
    if (const auto* integerExpr = dynamic_cast<const IntegerExpr*>(&expr)) {
        return "(constval i32 " + integerExpr->value() + ")";
    }

    if (const auto* identifierExpr = dynamic_cast<const IdentifierExpr*>(&expr)) {
        return variableName(identifierExpr->name());
    }

    if (const auto* assignmentExpr = dynamic_cast<const AssignmentExpr*>(&expr)) {
        return generateAssignmentExpr(*assignmentExpr);
    }

    if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(&expr)) {
        return generateUnaryExpr(*unaryExpr);
    }

    if (const auto* binaryExpr = dynamic_cast<const BinaryExpr*>(&expr)) {
        return generateBinaryExpr(*binaryExpr);
    }

    throw std::runtime_error("Maple IR generation error: unsupported expression node");
}

std::string MapleIRGenerator::generateAssignmentExpr(const AssignmentExpr& assignmentExpr) {
    const std::string value = generateExpr(assignmentExpr.value());
    const std::string target = variableName(assignmentExpr.name());
    emit("dassign " + target + " " + value);
    return target;
}

std::string MapleIRGenerator::generateUnaryExpr(const UnaryExpr& unaryExpr) {
    const std::string operand = generateExpr(unaryExpr.operand());

    if (unaryExpr.op() == "+") {
        return operand;
    }

    const std::string temp = newTemp();
    emit("regassign " + temp + " (neg i32 " + operand + ")");
    return temp;
}

std::string MapleIRGenerator::generateBinaryExpr(const BinaryExpr& binaryExpr) {
    const std::string left = generateExpr(binaryExpr.left());
    const std::string right = generateExpr(binaryExpr.right());
    const std::string temp = newTemp();
    emit("regassign " + temp + " (" + mapleOp(binaryExpr.op()) + " i32 " + left + " " + right + ")");
    return temp;
}
