#include "SemanticAnalyzer.h"

#include <sstream>

SemanticError::SemanticError(const std::string& message)
    : std::runtime_error("Semantic error: " + message) {}

void SemanticAnalyzer::analyze(const Program& program) {
    symbols_.enterScope();

    for (const auto& node : program.nodes()) {
        analyzeNode(*node);
    }

    symbols_.exitScope();
}

void SemanticAnalyzer::analyzeNode(const ASTNode& node) {
    if (const auto* functionDecl = dynamic_cast<const FunctionDecl*>(&node)) {
        analyzeFunctionDecl(*functionDecl);
        return;
    }

    if (const auto* stmt = dynamic_cast<const Stmt*>(&node)) {
        analyzeStmt(*stmt);
        return;
    }

    throw SemanticError("unsupported top-level AST node");
}

void SemanticAnalyzer::analyzeFunctionDecl(const FunctionDecl& functionDecl) {
    const ValueType previousReturnType = currentReturnType_;
    currentReturnType_ = valueTypeFromString(functionDecl.returnType());

    if (currentReturnType_ == ValueType::Unknown) {
        throw SemanticError("unknown function return type '" + functionDecl.returnType() + "'");
    }

    analyzeBlockStmt(functionDecl.body());
    currentReturnType_ = previousReturnType;
}

void SemanticAnalyzer::analyzeBlockStmt(const BlockStmt& blockStmt) {
    symbols_.enterScope();

    for (const auto& statement : blockStmt.statements()) {
        analyzeStmt(*statement);
    }

    symbols_.exitScope();
}

void SemanticAnalyzer::analyzeStmt(const Stmt& stmt) {
    if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(&stmt)) {
        analyzeBlockStmt(*blockStmt);
        return;
    }

    if (const auto* varDeclStmt = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        analyzeVarDeclStmt(*varDeclStmt);
        return;
    }

    if (const auto* returnStmt = dynamic_cast<const ReturnStmt*>(&stmt)) {
        analyzeReturnStmt(*returnStmt);
        return;
    }

    if (const auto* exprStmt = dynamic_cast<const ExprStmt*>(&stmt)) {
        analyzeExpr(exprStmt->expression());
        return;
    }

    if (const auto* ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        analyzeIfStmt(*ifStmt);
        return;
    }

    if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        analyzeWhileStmt(*whileStmt);
        return;
    }

    throw SemanticError("unsupported statement node");
}

void SemanticAnalyzer::analyzeVarDeclStmt(const VarDeclStmt& varDeclStmt) {
    const ValueType declaredType = valueTypeFromString(varDeclStmt.type());

    if (declaredType == ValueType::Unknown || declaredType == ValueType::Void) {
        throw SemanticError("unsupported variable type '" + varDeclStmt.type() + "'");
    }

    if (!symbols_.declare(varDeclStmt.name(), declaredType)) {
        throw SemanticError("variable '" + varDeclStmt.name() + "' is already declared in this scope");
    }

    if (const Expr* initializer = varDeclStmt.initializer()) {
        const ValueType initializerType = analyzeExpr(*initializer);
        requireSameType(declaredType, initializerType, "initializer for variable '" + varDeclStmt.name() + "'");
    }
}

void SemanticAnalyzer::analyzeReturnStmt(const ReturnStmt& returnStmt) {
    if (currentReturnType_ == ValueType::Unknown) {
        throw SemanticError("return statement outside function");
    }

    const Expr* value = returnStmt.value();
    const ValueType actualType = value ? analyzeExpr(*value) : ValueType::Void;
    requireSameType(currentReturnType_, actualType, "return statement");
}

void SemanticAnalyzer::analyzeIfStmt(const IfStmt& ifStmt) {
    const ValueType conditionType = analyzeExpr(ifStmt.condition());
    requireSameType(ValueType::Int, conditionType, "if condition");

    analyzeStmt(ifStmt.thenBranch());

    if (const Stmt* elseBranch = ifStmt.elseBranch()) {
        analyzeStmt(*elseBranch);
    }
}

void SemanticAnalyzer::analyzeWhileStmt(const WhileStmt& whileStmt) {
    const ValueType conditionType = analyzeExpr(whileStmt.condition());
    requireSameType(ValueType::Int, conditionType, "while condition");
    analyzeStmt(whileStmt.body());
}

ValueType SemanticAnalyzer::analyzeExpr(const Expr& expr) {
    if (dynamic_cast<const IntegerExpr*>(&expr) != nullptr) {
        return ValueType::Int;
    }

    if (const auto* identifierExpr = dynamic_cast<const IdentifierExpr*>(&expr)) {
        return analyzeIdentifierExpr(*identifierExpr);
    }

    if (const auto* assignmentExpr = dynamic_cast<const AssignmentExpr*>(&expr)) {
        return analyzeAssignmentExpr(*assignmentExpr);
    }

    if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(&expr)) {
        return analyzeUnaryExpr(*unaryExpr);
    }

    if (const auto* binaryExpr = dynamic_cast<const BinaryExpr*>(&expr)) {
        return analyzeBinaryExpr(*binaryExpr);
    }

    throw SemanticError("unsupported expression node");
}

ValueType SemanticAnalyzer::analyzeIdentifierExpr(const IdentifierExpr& identifierExpr) {
    const Symbol* symbol = symbols_.lookup(identifierExpr.name());

    if (symbol == nullptr) {
        throw SemanticError("variable '" + identifierExpr.name() + "' is not declared");
    }

    return symbol->type;
}

ValueType SemanticAnalyzer::analyzeAssignmentExpr(const AssignmentExpr& assignmentExpr) {
    const Symbol* symbol = symbols_.lookup(assignmentExpr.name());

    if (symbol == nullptr) {
        throw SemanticError("variable '" + assignmentExpr.name() + "' is not declared");
    }

    const ValueType valueType = analyzeExpr(assignmentExpr.value());
    requireSameType(symbol->type, valueType, "assignment to variable '" + assignmentExpr.name() + "'");
    return symbol->type;
}

ValueType SemanticAnalyzer::analyzeUnaryExpr(const UnaryExpr& unaryExpr) {
    const ValueType operandType = analyzeExpr(unaryExpr.operand());

    if (!isNumericType(operandType)) {
        throw SemanticError("operator '" + unaryExpr.op() + "' requires a numeric operand");
    }

    return operandType;
}

ValueType SemanticAnalyzer::analyzeBinaryExpr(const BinaryExpr& binaryExpr) {
    const ValueType leftType = analyzeExpr(binaryExpr.left());
    const ValueType rightType = analyzeExpr(binaryExpr.right());

    requireSameType(leftType, rightType, "binary operator '" + binaryExpr.op() + "'");

    if (!isNumericType(leftType)) {
        throw SemanticError("operator '" + binaryExpr.op() + "' requires numeric operands");
    }

    return ValueType::Int;
}

void SemanticAnalyzer::requireSameType(ValueType expected, ValueType actual, const std::string& context) const {
    if (expected == actual) {
        return;
    }

    std::ostringstream oss;
    oss << "type mismatch in " << context << ": expected "
        << valueTypeToString(expected) << ", got " << valueTypeToString(actual);
    throw SemanticError(oss.str());
}

bool SemanticAnalyzer::isNumericType(ValueType type) const {
    return type == ValueType::Int || type == ValueType::Float || type == ValueType::Char;
}
