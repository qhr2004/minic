#include "semantic/Semantic.h"

#include <sstream>

SemanticError::SemanticError(const std::string& message)
    : std::runtime_error("Semantic error: " + message) {}

void SemanticAnalyzer::analyze(const Program& program) {
    symbols_.clear();
    functions_.clear();
    currentReturnType_ = ValueType::Unknown;

    symbols_.enterScope();
    registerFunctions(program);

    for (const auto& node : program.nodes()) {
        analyzeNode(*node);
    }

    symbols_.exitScope();
}

void SemanticAnalyzer::registerFunctions(const Program& program) {
    for (const auto& node : program.nodes()) {
        const auto* functionDecl = dynamic_cast<const FunctionDecl*>(node.get());
        if (functionDecl == nullptr) {
            continue;
        }

        FunctionSignature signature = buildFunctionSignature(*functionDecl);
        if (!functions_.emplace(signature.name, signature).second) {
            throw SemanticError("function '" + signature.name + "' is already declared");
        }

        if (!symbols_.declare(signature.name, signature.returnType, SymbolKind::Function, signature.parameterTypes)) {
            throw SemanticError("symbol '" + signature.name + "' is already declared in this scope");
        }
    }
}

FunctionSignature SemanticAnalyzer::buildFunctionSignature(const FunctionDecl& functionDecl) const {
    FunctionSignature signature;
    signature.name = functionDecl.name();
    signature.returnType = resolveDeclaredType(functionDecl.returnType(), "function '" + functionDecl.name() + "'");

    for (const auto& parameter : functionDecl.parameters()) {
        const ValueType parameterType = resolveDeclaredType(
            parameter->type(),
            "parameter '" + parameter->name() + "' in function '" + functionDecl.name() + "'");

        if (parameterType == ValueType::Void || parameterType == ValueType::Unknown) {
            throw SemanticError("unsupported parameter type '" + parameter->type() + "'");
        }

        signature.parameterTypes.push_back(parameterType);
    }

    return signature;
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
    const auto found = functions_.find(functionDecl.name());
    if (found == functions_.end()) {
        throw SemanticError("missing signature for function '" + functionDecl.name() + "'");
    }

    const ValueType previousReturnType = currentReturnType_;
    currentReturnType_ = found->second.returnType;

    symbols_.enterScope();
    declareParameters(functionDecl);
    analyzeFunctionBody(functionDecl.body());
    symbols_.exitScope();

    currentReturnType_ = previousReturnType;
}

void SemanticAnalyzer::analyzeFunctionBody(const BlockStmt& body) {
    for (const auto& statement : body.statements()) {
        analyzeStmt(*statement);
    }
}

void SemanticAnalyzer::analyzeBlockStmt(const BlockStmt& blockStmt) {
    symbols_.enterScope();
    analyzeFunctionBody(blockStmt);
    symbols_.exitScope();
}

void SemanticAnalyzer::analyzeStmt(const Stmt& stmt) {
    if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(&stmt)) {
        analyzeBlockStmt(*blockStmt);
        return;
    }

    if (const auto* varDecl = dynamic_cast<const VarDecl*>(&stmt)) {
        analyzeVarDecl(*varDecl);
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

    if (const auto* forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        analyzeForStmt(*forStmt);
        return;
    }

    throw SemanticError("unsupported statement node");
}

void SemanticAnalyzer::analyzeVarDecl(const VarDecl& varDecl) {
    const ValueType declaredType = resolveDeclaredType(varDecl.type(), "variable '" + varDecl.name() + "'");

    if (declaredType == ValueType::Void || declaredType == ValueType::Unknown) {
        throw SemanticError("unsupported variable type '" + varDecl.type() + "'");
    }

    if (!symbols_.declare(varDecl.name(), declaredType, SymbolKind::Variable)) {
        throw SemanticError("variable '" + varDecl.name() + "' is already declared in this scope");
    }

    if (const Expr* initializer = varDecl.initializer()) {
        const ValueType initializerType = analyzeExpr(*initializer);
        requireSameType(declaredType, initializerType, "initializer for variable '" + varDecl.name() + "'");
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

void SemanticAnalyzer::analyzeForStmt(const ForStmt& forStmt) {
    symbols_.enterScope();

    if (const Stmt* initializer = forStmt.initializer()) {
        analyzeStmt(*initializer);
    }

    if (const Expr* condition = forStmt.condition()) {
        const ValueType conditionType = analyzeExpr(*condition);
        requireSameType(ValueType::Int, conditionType, "for condition");
    }

    analyzeStmt(forStmt.body());

    if (const Expr* update = forStmt.update()) {
        analyzeExpr(*update);
    }

    symbols_.exitScope();
}

void SemanticAnalyzer::declareParameters(const FunctionDecl& functionDecl) {
    for (const auto& parameter : functionDecl.parameters()) {
        const ValueType parameterType = resolveDeclaredType(
            parameter->type(),
            "parameter '" + parameter->name() + "' in function '" + functionDecl.name() + "'");

        if (parameterType == ValueType::Void || parameterType == ValueType::Unknown) {
            throw SemanticError("unsupported parameter type '" + parameter->type() + "'");
        }

        if (!symbols_.declare(parameter->name(), parameterType, SymbolKind::Parameter)) {
            throw SemanticError("parameter '" + parameter->name() + "' is already declared in this scope");
        }
    }
}

ValueType SemanticAnalyzer::analyzeExpr(const Expr& expr) {
    if (const auto* literal = dynamic_cast<const Literal*>(&expr)) {
        return analyzeLiteral(*literal);
    }

    if (const auto* identifier = dynamic_cast<const Identifier*>(&expr)) {
        return analyzeIdentifier(*identifier);
    }

    if (const auto* assignmentExpr = dynamic_cast<const AssignmentExpr*>(&expr)) {
        return analyzeAssignmentExpr(*assignmentExpr);
    }

    if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(&expr)) {
        return analyzeUnaryExpr(*unaryExpr);
    }

    if (const auto* incDecExpr = dynamic_cast<const IncDecExpr*>(&expr)) {
        return analyzeIncDecExpr(*incDecExpr);
    }

    if (const auto* binaryExpr = dynamic_cast<const BinaryExpr*>(&expr)) {
        return analyzeBinaryExpr(*binaryExpr);
    }

    if (const auto* functionCall = dynamic_cast<const FunctionCall*>(&expr)) {
        return analyzeFunctionCall(*functionCall);
    }

    throw SemanticError("unsupported expression node");
}

ValueType SemanticAnalyzer::analyzeLiteral(const Literal& literal) const {
    switch (literal.kind()) {
    case LiteralKind::Integer:
        return ValueType::Int;
    case LiteralKind::Float:
        return ValueType::Float;
    case LiteralKind::Char:
        return ValueType::Char;
    }

    return ValueType::Unknown;
}

ValueType SemanticAnalyzer::analyzeIdentifier(const Identifier& identifier) {
    const Symbol* symbol = symbols_.lookup(identifier.name());

    if (symbol == nullptr) {
        throw SemanticError("undeclared variable '" + identifier.name() + "'");
    }

    if (symbol->kind == SymbolKind::Function) {
        throw SemanticError("function '" + identifier.name() + "' cannot be used as a variable");
    }

    return symbol->type;
}

ValueType SemanticAnalyzer::analyzeAssignmentExpr(const AssignmentExpr& assignmentExpr) {
    const Symbol* symbol = symbols_.lookup(assignmentExpr.name());

    if (symbol == nullptr) {
        throw SemanticError("undeclared variable '" + assignmentExpr.name() + "'");
    }

    if (symbol->kind == SymbolKind::Function) {
        throw SemanticError("cannot assign to function '" + assignmentExpr.name() + "'");
    }

    const ValueType valueType = analyzeExpr(assignmentExpr.value());
    requireSameType(symbol->type, valueType, "assignment to variable '" + assignmentExpr.name() + "'");
    return symbol->type;
}

ValueType SemanticAnalyzer::analyzeUnaryExpr(const UnaryExpr& unaryExpr) {
    const ValueType operandType = analyzeExpr(unaryExpr.operand());

    if (unaryExpr.op() == "-") {
        if (!isNumericType(operandType)) {
            throw SemanticError("operator '" + unaryExpr.op() + "' requires a numeric operand");
        }

        return operandType;
    }

    if (unaryExpr.op() == "!") {
        requireSameType(ValueType::Int, operandType, "logical operator '!'");
        return ValueType::Int;
    }

    throw SemanticError("unsupported unary operator '" + unaryExpr.op() + "'");
}

ValueType SemanticAnalyzer::analyzeIncDecExpr(const IncDecExpr& incDecExpr) {
    const Symbol* symbol = symbols_.lookup(incDecExpr.name());

    if (symbol == nullptr) {
        throw SemanticError("undeclared variable '" + incDecExpr.name() + "'");
    }

    if (symbol->kind == SymbolKind::Function) {
        throw SemanticError("cannot apply '" + incDecExpr.op() + "' to function '" + incDecExpr.name() + "'");
    }

    if (!isNumericType(symbol->type)) {
        throw SemanticError("operator '" + incDecExpr.op() + "' requires a numeric operand");
    }

    return symbol->type;
}

ValueType SemanticAnalyzer::analyzeBinaryExpr(const BinaryExpr& binaryExpr) {
    const ValueType leftType = analyzeExpr(binaryExpr.left());
    const ValueType rightType = analyzeExpr(binaryExpr.right());
    const std::string& op = binaryExpr.op();

    if (op == "+" || op == "-" || op == "*" || op == "/") {
        requireSameType(leftType, rightType, "binary operator '" + op + "'");

        if (!isNumericType(leftType)) {
            throw SemanticError("operator '" + op + "' requires numeric operands");
        }

        return leftType;
    }

    if (op == "%") {
        requireSameType(leftType, rightType, "binary operator '%'");

        if (!isIntegralType(leftType)) {
            throw SemanticError("operator '%' requires integral operands");
        }

        return leftType;
    }

    if (op == ">" || op == ">=" || op == "<" || op == "<=") {
        requireSameType(leftType, rightType, "comparison operator '" + op + "'");

        if (!isNumericType(leftType)) {
            throw SemanticError("operator '" + op + "' requires numeric operands");
        }

        return ValueType::Int;
    }

    if (op == "==" || op == "!=") {
        requireSameType(leftType, rightType, "equality operator '" + op + "'");
        return ValueType::Int;
    }

    if (op == "&&" || op == "||") {
        requireSameType(ValueType::Int, leftType, "logical operator '" + op + "'");
        requireSameType(ValueType::Int, rightType, "logical operator '" + op + "'");
        return ValueType::Int;
    }

    throw SemanticError("unsupported binary operator '" + op + "'");
}

ValueType SemanticAnalyzer::analyzeFunctionCall(const FunctionCall& functionCall) {
    const auto found = functions_.find(functionCall.callee());
    if (found == functions_.end()) {
        throw SemanticError("undeclared function '" + functionCall.callee() + "'");
    }

    const FunctionSignature& signature = found->second;
    if (signature.parameterTypes.size() != functionCall.arguments().size()) {
        throw SemanticError(
            "function '" + functionCall.callee() + "' expects "
            + std::to_string(signature.parameterTypes.size()) + " argument(s), got "
            + std::to_string(functionCall.arguments().size()));
    }

    for (std::size_t index = 0; index < functionCall.arguments().size(); ++index) {
        const ValueType actualType = analyzeExpr(*functionCall.arguments()[index]);
        requireSameType(signature.parameterTypes[index], actualType,
                        "argument " + std::to_string(index + 1) + " of function '" + functionCall.callee() + "'");
    }

    return signature.returnType;
}

ValueType SemanticAnalyzer::resolveDeclaredType(const std::string& typeName, const std::string& context) const {
    const ValueType type = valueTypeFromString(typeName);

    if (type == ValueType::Unknown) {
        throw SemanticError("unknown type '" + typeName + "' in " + context);
    }

    return type;
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

bool SemanticAnalyzer::isIntegralType(ValueType type) const {
    return type == ValueType::Int || type == ValueType::Char;
}
