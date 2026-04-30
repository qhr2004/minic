#include "ir/IRGenerator.h"

#include <stdexcept>

const IRModule& IRGenerator::generate(const Program& program) {
    module_.clear();
    tempCounter_ = 0;
    labelCounter_ = 0;

    for (const auto& node : program.nodes()) {
        generateNode(*node);
    }

    return module_;
}

const IRModule& IRGenerator::module() const {
    return module_;
}

void IRGenerator::emit(const std::string& instruction) {
    module_.addInstruction(instruction);
}

std::string IRGenerator::newTemp() {
    return "%t" + std::to_string(tempCounter_++);
}

std::string IRGenerator::newLabel() {
    return "L" + std::to_string(labelCounter_++);
}

void IRGenerator::generateNode(const ASTNode& node) {
    if (const auto* functionDecl = dynamic_cast<const FunctionDecl*>(&node)) {
        generateFunctionDecl(*functionDecl);
        return;
    }

    if (const auto* stmt = dynamic_cast<const Stmt*>(&node)) {
        generateStmt(*stmt);
        return;
    }

    throw std::runtime_error("IR generation error: unsupported top-level AST node");
}

void IRGenerator::generateFunctionDecl(const FunctionDecl& functionDecl) {
    generateBlockStmt(functionDecl.body());
}

void IRGenerator::generateBlockStmt(const BlockStmt& blockStmt) {
    for (const auto& statement : blockStmt.statements()) {
        generateStmt(*statement);
    }
}

void IRGenerator::generateStmt(const Stmt& stmt) {
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

    if (const auto* forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        generateForStmt(*forStmt);
        return;
    }

    throw std::runtime_error("IR generation error: unsupported statement node");
}

void IRGenerator::generateVarDeclStmt(const VarDeclStmt& varDeclStmt) {
    emit("var " + varDeclStmt.name());

    if (const Expr* initializer = varDeclStmt.initializer()) {
        const std::string value = generateExpr(*initializer);
        emit(varDeclStmt.name() + " = " + value);
    }
}

void IRGenerator::generateReturnStmt(const ReturnStmt& returnStmt) {
    const Expr* value = returnStmt.value();

    if (value == nullptr) {
        emit("return");
        return;
    }

    emit("return " + generateExpr(*value));
}

void IRGenerator::generateIfStmt(const IfStmt& ifStmt) {
    const std::string elseLabel = newLabel();
    const std::string endLabel = newLabel();
    const std::string condition = generateExpr(ifStmt.condition());

    emit("ifFalse " + condition + " goto " + elseLabel);
    generateStmt(ifStmt.thenBranch());
    emit("goto " + endLabel);
    emit(elseLabel + ":");

    if (const Stmt* elseBranch = ifStmt.elseBranch()) {
        generateStmt(*elseBranch);
    }

    emit(endLabel + ":");
}

void IRGenerator::generateWhileStmt(const WhileStmt& whileStmt) {
    const std::string startLabel = newLabel();
    const std::string endLabel = newLabel();

    emit(startLabel + ":");
    const std::string condition = generateExpr(whileStmt.condition());
    emit("ifFalse " + condition + " goto " + endLabel);
    generateStmt(whileStmt.body());
    emit("goto " + startLabel);
    emit(endLabel + ":");
}

void IRGenerator::generateForStmt(const ForStmt& forStmt) {
    const std::string conditionLabel = newLabel();
    const std::string updateLabel = newLabel();
    const std::string endLabel = newLabel();

    if (const Stmt* initializer = forStmt.initializer()) {
        generateStmt(*initializer);
    }

    emit(conditionLabel + ":");

    if (const Expr* condition = forStmt.condition()) {
        const std::string conditionValue = generateExpr(*condition);
        emit("ifFalse " + conditionValue + " goto " + endLabel);
    }

    generateStmt(forStmt.body());
    emit(updateLabel + ":");

    if (const Expr* update = forStmt.update()) {
        generateExpr(*update);
    }

    emit("goto " + conditionLabel);
    emit(endLabel + ":");
}

std::string IRGenerator::generateExpr(const Expr& expr) {
    if (const auto* integerExpr = dynamic_cast<const IntegerExpr*>(&expr)) {
        return integerExpr->value();
    }

    if (const auto* identifierExpr = dynamic_cast<const IdentifierExpr*>(&expr)) {
        return identifierExpr->name();
    }

    if (const auto* assignmentExpr = dynamic_cast<const AssignmentExpr*>(&expr)) {
        return generateAssignmentExpr(*assignmentExpr);
    }

    if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(&expr)) {
        return generateUnaryExpr(*unaryExpr);
    }

    if (const auto* incDecExpr = dynamic_cast<const IncDecExpr*>(&expr)) {
        return generateIncDecExpr(*incDecExpr);
    }

    if (const auto* binaryExpr = dynamic_cast<const BinaryExpr*>(&expr)) {
        return generateBinaryExpr(*binaryExpr);
    }

    if (const auto* functionCall = dynamic_cast<const FunctionCall*>(&expr)) {
        return generateFunctionCall(*functionCall);
    }

    throw std::runtime_error("IR generation error: unsupported expression node");
}

std::string IRGenerator::generateAssignmentExpr(const AssignmentExpr& assignmentExpr) {
    const std::string value = generateExpr(assignmentExpr.value());
    emit(assignmentExpr.name() + " = " + value);
    return assignmentExpr.name();
}

std::string IRGenerator::generateUnaryExpr(const UnaryExpr& unaryExpr) {
    const std::string operand = generateExpr(unaryExpr.operand());
    const std::string temp = newTemp();
    emit(temp + " = " + unaryExpr.op() + " " + operand);
    return temp;
}

std::string IRGenerator::generateIncDecExpr(const IncDecExpr& incDecExpr) {
    const std::string oldValue = newTemp();
    emit(oldValue + " = " + incDecExpr.name());

    const std::string updatedValue = newTemp();
    const std::string op = incDecExpr.op() == "++" ? "+" : "-";
    emit(updatedValue + " = " + incDecExpr.name() + " " + op + " 1");
    emit(incDecExpr.name() + " = " + updatedValue);

    if (incDecExpr.isPostfix()) {
        return oldValue;
    }

    return updatedValue;
}

std::string IRGenerator::generateBinaryExpr(const BinaryExpr& binaryExpr) {
    const std::string left = generateExpr(binaryExpr.left());
    const std::string right = generateExpr(binaryExpr.right());
    const std::string temp = newTemp();
    emit(temp + " = " + left + " " + binaryExpr.op() + " " + right);
    return temp;
}

std::string IRGenerator::generateFunctionCall(const FunctionCall& functionCall) {
    std::vector<std::string> argumentValues;
    argumentValues.reserve(functionCall.arguments().size());

    for (const auto& argument : functionCall.arguments()) {
        argumentValues.push_back(generateExpr(*argument));
    }

    for (const std::string& argumentValue : argumentValues) {
        emit("param " + argumentValue);
    }

    const std::string temp = newTemp();
    emit(temp + " = call " + functionCall.callee() + ", " + std::to_string(functionCall.arguments().size()));
    return temp;
}
