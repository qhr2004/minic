#include "maple/IRGenerator.h"

#include "semantic/Semantic.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace maple {

namespace {

std::string trim(const std::string& text) {
    const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });

    if (first == text.end()) {
        return "";
    }

    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();

    return std::string(first, last);
}

bool startsWith(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::string join(const std::vector<std::string>& parts, const std::string& separator) {
    std::ostringstream oss;

    for (std::size_t index = 0; index < parts.size(); ++index) {
        if (index != 0) {
            oss << separator;
        }

        oss << parts[index];
    }

    return oss.str();
}

bool isIntegerToken(const std::string& token) {
    if (token.empty()) {
        return false;
    }

    std::size_t index = (token[0] == '-' || token[0] == '+') ? 1 : 0;
    if (index == token.size()) {
        return false;
    }

    for (; index < token.size(); ++index) {
        if (!std::isdigit(static_cast<unsigned char>(token[index]))) {
            return false;
        }
    }

    return true;
}

bool isFloatToken(const std::string& token) {
    return token.find('.') != std::string::npos || token.find('e') != std::string::npos || token.find('E') != std::string::npos;
}

std::string stripPercentPrefix(const std::string& name) {
    if (!name.empty() && name.front() == '%') {
        return name.substr(1);
    }

    return name;
}

std::string replaceExtension(const std::string& path, const std::string& extension) {
    const std::size_t slash = path.find_last_of("/\\");
    const std::size_t dot = path.find_last_of('.');

    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
        return path + extension;
    }

    return path.substr(0, dot) + extension;
}

std::string shellQuote(const std::string& text) {
    std::string quoted = "'";

    for (char ch : text) {
        if (ch == '\'') {
            quoted += "'\\''";
            continue;
        }

        quoted.push_back(ch);
    }

    quoted.push_back('\'');
    return quoted;
}

std::string replacePlaceholder(std::string text, const std::string& placeholder, const std::string& value) {
    std::size_t position = 0;

    while ((position = text.find(placeholder, position)) != std::string::npos) {
        text.replace(position, placeholder.size(), value);
        position += value.size();
    }

    return text;
}

std::string backendTemplateFromConfig(const BackendConfig& config) {
    if (!config.commandTemplate.empty()) {
        return config.commandTemplate;
    }

    const char* env = std::getenv("MINIC_ARK_BACKEND_CMD");
    if (env == nullptr) {
        return "";
    }

    return env;
}

std::string invokeBackend(const Module& module, const std::string& outputPath, const BackendConfig& config) {
    const std::string mplPath = replaceExtension(outputPath, ".mpl");
    std::ofstream file(mplPath);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open Maple IR output file: " + mplPath);
    }

    module.print(file);
    file.close();

    std::string commandTemplate = backendTemplateFromConfig(config);
    if (commandTemplate.empty()) {
        throw std::runtime_error(
            "Maple IR generated at '" + mplPath
            + "', but Ark backend is not configured. Set MINIC_ARK_BACKEND_CMD to a shell template using {input} and {output}.");
    }

    if (commandTemplate.find("{input}") == std::string::npos || commandTemplate.find("{output}") == std::string::npos) {
        throw std::runtime_error("MINIC_ARK_BACKEND_CMD must contain both {input} and {output} placeholders");
    }

    std::string command = replacePlaceholder(commandTemplate, "{input}", shellQuote(mplPath));
    command = replacePlaceholder(command, "{output}", shellQuote(outputPath));

    const int exitCode = std::system(command.c_str());
    if (exitCode != 0) {
        throw std::runtime_error("Ark backend command failed with exit code " + std::to_string(exitCode) + ": " + commandTemplate);
    }

    return outputPath;
}

} // namespace

void Module::clear() {
    lines_.clear();
}

void Module::addLine(std::string line) {
    lines_.push_back(std::move(line));
}

const std::vector<std::string>& Module::lines() const {
    return lines_;
}

std::string Module::toString() const {
    std::ostringstream oss;
    print(oss);
    return oss.str();
}

void Module::print(std::ostream& os) const {
    for (const std::string& line : lines_) {
        os << line << '\n';
    }
}

const Module& IRGenerator::generate(const Program& program) {
    reset();
    registerFunctions(program);

    std::vector<const Stmt*> topLevelStatements;
    for (const auto& node : program.nodes()) {
        if (const auto* statement = dynamic_cast<const Stmt*>(node.get())) {
            topLevelStatements.push_back(statement);
        }
    }

    if (!topLevelStatements.empty()) {
        functionOrder_.push_back("__minic_entry");
    }

    std::string entryFunction;
    if (functions_.find("main") != functions_.end()) {
        entryFunction = "main";
    } else if (!topLevelStatements.empty()) {
        entryFunction = "__minic_entry";
    } else if (!functionOrder_.empty()) {
        entryFunction = functionOrder_.front();
    } else {
        throw std::runtime_error("Maple IR generation error: program has no functions or statements");
    }

    // Emit the module header first, then lower each MiniC function to a Maple
    // function with explicit labels for the control-flow graph.
    emitProgramHeader(entryFunction);

    for (const auto& node : program.nodes()) {
        const auto* functionDecl = dynamic_cast<const FunctionDecl*>(node.get());
        if (functionDecl == nullptr) {
            continue;
        }

        emitASTFunction(*functionDecl);
    }

    if (!topLevelStatements.empty()) {
        emitSyntheticTopLevelFunction(topLevelStatements);
    }

    return module_;
}

const Module& IRGenerator::generate(const IRModule& irModule, const std::string& functionName) {
    reset();
    functionOrder_.push_back(functionName);
    emitProgramHeader(functionName);
    emitIRFunction(irModule, functionName);
    return module_;
}

const Module& IRGenerator::module() const {
    return module_;
}

void IRGenerator::writeToFile(const std::string& path) const {
    std::ofstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open Maple IR output file: " + path);
    }

    module_.print(file);
}

void IRGenerator::reset() {
    module_.clear();
    functionContext_ = FunctionContext{};
    scopes_.clear();
    functions_.clear();
    functionOrder_.clear();
    localNameCounters_.clear();
    labelCounter_ = 0;
    internalTempCounter_ = 0;
    currentReturnType_ = ValueType::Void;
}

std::string IRGenerator::mapleType(ValueType type) {
    switch (type) {
    case ValueType::Int:
        return "i32";
    case ValueType::Float:
        return "f32";
    case ValueType::Char:
        return "i8";
    case ValueType::Void:
        return "void";
    case ValueType::Unknown:
        return "i32";
    }

    return "i32";
}

std::string IRGenerator::functionSymbol(const std::string& name) {
    return "&" + name;
}

std::string IRGenerator::localSymbol(const std::string& name) {
    return "%" + name;
}

std::string IRGenerator::normalizeFloatLiteral(const std::string& token) {
    if (token.empty()) {
        return token;
    }

    if (token.back() == 'f' || token.back() == 'F') {
        return token;
    }

    return token + "f";
}

int IRGenerator::decodeCharLiteral(const std::string& token) {
    if (token.size() < 3 || token.front() != '\'' || token.back() != '\'') {
        throw std::runtime_error("Maple IR generation error: invalid char literal '" + token + "'");
    }

    if (token[1] != '\\') {
        return static_cast<unsigned char>(token[1]);
    }

    if (token.size() < 4) {
        throw std::runtime_error("Maple IR generation error: invalid escaped char literal '" + token + "'");
    }

    switch (token[2]) {
    case 'n':
        return '\n';
    case 't':
        return '\t';
    case 'r':
        return '\r';
    case '\\':
        return '\\';
    case '\'':
        return '\'';
    case '0':
        return '\0';
    default:
        return static_cast<unsigned char>(token[2]);
    }
}

std::string IRGenerator::newLabel(const std::string& prefix) {
    return "@" + prefix + "_" + std::to_string(labelCounter_++);
}

std::string IRGenerator::oneLiteral(ValueType type) {
    switch (type) {
    case ValueType::Float:
        return "constval f32 1.0f";
    case ValueType::Char:
        return "constval i8 1";
    case ValueType::Int:
    case ValueType::Void:
    case ValueType::Unknown:
        return "constval i32 1";
    }

    return "constval i32 1";
}

void IRGenerator::emitDeclaration(std::string line) {
    functionContext_.declarations.push_back(std::move(line));
}

void IRGenerator::emitBody(std::string line) {
    functionContext_.body.push_back(std::move(line));
}

void IRGenerator::beginScope() {
    scopes_.push_back(ScopeFrame{});
}

void IRGenerator::endScope() {
    if (scopes_.empty()) {
        throw std::runtime_error("Maple IR generation error: scope underflow");
    }

    scopes_.pop_back();
}

IRGenerator::LocalBinding IRGenerator::declareBinding(const std::string& sourceName, ValueType type, bool emitDeclarationFlag) {
    if (scopes_.empty()) {
        throw std::runtime_error("Maple IR generation error: missing active scope for local declaration");
    }

    int& counter = localNameCounters_[sourceName];
    const std::string uniqueName = counter == 0 ? sourceName : sourceName + "_" + std::to_string(counter);
    ++counter;

    LocalBinding binding{localSymbol(uniqueName), type};
    scopes_.back().bindings.emplace(sourceName, binding);

    if (emitDeclarationFlag) {
        emitDeclaration("var " + binding.mapleName + " " + mapleType(type));
    }

    return binding;
}

IRGenerator::LocalBinding IRGenerator::declareInternalTemp(ValueType type, const std::string& hint) {
    return declareBinding("__" + hint + "_" + std::to_string(internalTempCounter_++), type);
}

const IRGenerator::LocalBinding& IRGenerator::lookupBinding(const std::string& sourceName) const {
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
        const auto found = scope->bindings.find(sourceName);
        if (found != scope->bindings.end()) {
            return found->second;
        }
    }

    throw std::runtime_error("Maple IR generation error: unknown local '" + sourceName + "'");
}

IRGenerator::FunctionInfo IRGenerator::buildFunctionInfo(const FunctionDecl& functionDecl) const {
    FunctionInfo info;
    info.name = functionDecl.name();
    info.returnType = valueTypeFromString(functionDecl.returnType());

    for (const auto& parameter : functionDecl.parameters()) {
        info.parameters.emplace_back(parameter->name(), valueTypeFromString(parameter->type()));
    }

    return info;
}

void IRGenerator::registerFunctions(const Program& program) {
    for (const auto& node : program.nodes()) {
        const auto* functionDecl = dynamic_cast<const FunctionDecl*>(node.get());
        if (functionDecl == nullptr) {
            continue;
        }

        FunctionInfo info = buildFunctionInfo(*functionDecl);
        functions_.emplace(info.name, info);
        functionOrder_.push_back(info.name);
    }
}

void IRGenerator::emitProgramHeader(const std::string& entryFunction) {
    module_.addLine("flavor 1");
    module_.addLine("srclang \"MiniC\"");
    module_.addLine("numfuncs " + std::to_string(functionOrder_.size()));
    module_.addLine("entryfunc " + functionSymbol(entryFunction));
    module_.addLine("");
}

void IRGenerator::emitASTFunction(const FunctionDecl& functionDecl) {
    functionContext_ = FunctionContext{};
    scopes_.clear();
    localNameCounters_.clear();
    currentReturnType_ = valueTypeFromString(functionDecl.returnType());

    beginScope();

    std::vector<std::string> parameterDecls;
    for (const auto& parameter : functionDecl.parameters()) {
        const ValueType parameterType = valueTypeFromString(parameter->type());
        const LocalBinding binding = declareBinding(parameter->name(), parameterType, false);
        parameterDecls.push_back("var " + binding.mapleName + " " + mapleType(parameterType));
    }

    // Parameters stay in the function signature, while local declarations are
    // hoisted to the start of the Maple function body for simpler lowering.
    emitBody("# BB entry");
    emitBody("@entry");
    emitBlock(functionDecl.body());

    if (currentReturnType_ == ValueType::Void
        && (functionContext_.body.empty() || functionContext_.body.back() != "return ()")) {
        emitBody("return ()");
    }

    endScope();

    module_.addLine("func " + functionSymbol(functionDecl.name()) + " (" + join(parameterDecls, ", ") + ") " + mapleType(currentReturnType_) + " {");

    for (const std::string& declaration : functionContext_.declarations) {
        module_.addLine("  " + declaration);
    }

    if (!functionContext_.declarations.empty() && !functionContext_.body.empty()) {
        module_.addLine("");
    }

    for (const std::string& line : functionContext_.body) {
        module_.addLine("  " + line);
    }

    module_.addLine("}");
}

void IRGenerator::emitSyntheticTopLevelFunction(const std::vector<const Stmt*>& statements) {
    functionContext_ = FunctionContext{};
    scopes_.clear();
    localNameCounters_.clear();
    currentReturnType_ = ValueType::Void;

    beginScope();
    emitBody("# BB entry");
    emitBody("@entry");

    for (const Stmt* statement : statements) {
        emitStmt(*statement);
    }

    if (functionContext_.body.empty() || functionContext_.body.back() != "return ()") {
        emitBody("return ()");
    }

    endScope();

    module_.addLine("func " + functionSymbol("__minic_entry") + " () void {");

    for (const std::string& declaration : functionContext_.declarations) {
        module_.addLine("  " + declaration);
    }

    if (!functionContext_.declarations.empty() && !functionContext_.body.empty()) {
        module_.addLine("");
    }

    for (const std::string& line : functionContext_.body) {
        module_.addLine("  " + line);
    }

    module_.addLine("}");
}

void IRGenerator::emitBlock(const BlockStmt& blockStmt) {
    beginScope();

    for (const auto& statement : blockStmt.statements()) {
        emitStmt(*statement);
    }

    endScope();
}

void IRGenerator::emitStmt(const Stmt& stmt) {
    if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(&stmt)) {
        emitBlock(*blockStmt);
        return;
    }

    if (const auto* varDecl = dynamic_cast<const VarDecl*>(&stmt)) {
        emitVarDecl(*varDecl);
        return;
    }

    if (const auto* returnStmt = dynamic_cast<const ReturnStmt*>(&stmt)) {
        emitReturn(*returnStmt);
        return;
    }

    if (const auto* exprStmt = dynamic_cast<const ExprStmt*>(&stmt)) {
        if (dynamic_cast<const AssignmentExpr*>(&exprStmt->expression()) != nullptr
            || dynamic_cast<const IncDecExpr*>(&exprStmt->expression()) != nullptr) {
            emitExpr(exprStmt->expression());
            return;
        }

        if (const auto* functionCall = dynamic_cast<const FunctionCall*>(&exprStmt->expression())) {
            emitFunctionCallStmt(*functionCall);
            return;
        }

        const ExprResult result = emitExpr(exprStmt->expression());
        emitBody("eval (" + result.text + ")");
        return;
    }

    if (const auto* ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        emitIf(*ifStmt);
        return;
    }

    if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        emitWhile(*whileStmt);
        return;
    }

    if (const auto* forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        emitFor(*forStmt);
        return;
    }

    throw std::runtime_error("Maple IR generation error: unsupported statement node");
}

void IRGenerator::emitVarDecl(const VarDecl& varDecl) {
    const ValueType declaredType = valueTypeFromString(varDecl.type());
    const LocalBinding binding = declareBinding(varDecl.name(), declaredType);

    if (const Expr* initializer = varDecl.initializer()) {
        if (const auto* functionCall = dynamic_cast<const FunctionCall*>(initializer)) {
            emitFunctionCallInto(*functionCall, binding);
            return;
        }

        // MiniC initializers become a declaration plus a Maple dassign.
        const ExprResult result = emitExpr(*initializer);
        emitBody("dassign " + binding.mapleName + " (" + result.text + ")");
    }
}

void IRGenerator::emitReturn(const ReturnStmt& returnStmt) {
    if (const Expr* value = returnStmt.value()) {
        const ExprResult result = emitExpr(*value);
        emitBody("return (" + result.text + ")");
        return;
    }

    emitBody("return ()");
}

void IRGenerator::emitIf(const IfStmt& ifStmt) {
    const ExprResult condition = emitExpr(ifStmt.condition());
    const std::string elseLabel = newLabel("if_else");
    const std::string endLabel = newLabel("if_end");

    // Structured MiniC control flow is flattened into labeled Maple basic
    // blocks so the backend sees explicit branches and fallthrough edges.
    emitBody("# BB if_cond");
    emitBody("brfalse " + elseLabel + " (" + condition.text + ")");
    emitBody("# BB if_then");
    emitStmt(ifStmt.thenBranch());
    emitBody("goto " + endLabel);
    emitBody(elseLabel);

    if (const Stmt* elseBranch = ifStmt.elseBranch()) {
        emitBody("# BB if_else");
        emitStmt(*elseBranch);
    }

    emitBody(endLabel);
}

void IRGenerator::emitWhile(const WhileStmt& whileStmt) {
    const std::string condLabel = newLabel("while_cond");
    const std::string bodyLabel = newLabel("while_body");
    const std::string endLabel = newLabel("while_end");

    // A while loop is lowered to cond/body/end blocks with explicit back-edge.
    emitBody("goto " + condLabel);
    emitBody(condLabel);
    emitBody("# BB while_cond");

    const ExprResult condition = emitExpr(whileStmt.condition());
    emitBody("brfalse " + endLabel + " (" + condition.text + ")");
    emitBody("goto " + bodyLabel);
    emitBody(bodyLabel);
    emitBody("# BB while_body");
    emitStmt(whileStmt.body());
    emitBody("goto " + condLabel);
    emitBody(endLabel);
}

void IRGenerator::emitFor(const ForStmt& forStmt) {
    const std::string condLabel = newLabel("for_cond");
    const std::string bodyLabel = newLabel("for_body");
    const std::string updateLabel = newLabel("for_update");
    const std::string endLabel = newLabel("for_end");

    // A for-loop gets its own scope so `for (int i = ...)` variables remain
    // visible in the condition, body, and update expressions.
    beginScope();

    if (const Stmt* initializer = forStmt.initializer()) {
        emitBody("# BB for_init");
        emitStmt(*initializer);
    }

    emitBody("goto " + condLabel);
    emitBody(condLabel);
    emitBody("# BB for_cond");

    if (const Expr* condition = forStmt.condition()) {
        const ExprResult conditionResult = emitExpr(*condition);
        emitBody("brfalse " + endLabel + " (" + conditionResult.text + ")");
    }

    emitBody("goto " + bodyLabel);
    emitBody(bodyLabel);
    emitBody("# BB for_body");
    emitStmt(forStmt.body());
    emitBody("goto " + updateLabel);
    emitBody(updateLabel);
    emitBody("# BB for_update");

    if (const Expr* update = forStmt.update()) {
        emitExpr(*update);
    }

    emitBody("goto " + condLabel);
    emitBody(endLabel);

    endScope();
}

IRGenerator::ExprResult IRGenerator::emitExpr(const Expr& expr) {
    if (const auto* literal = dynamic_cast<const Literal*>(&expr)) {
        return emitLiteral(*literal);
    }

    if (const auto* identifier = dynamic_cast<const Identifier*>(&expr)) {
        return emitIdentifier(*identifier);
    }

    if (const auto* assignmentExpr = dynamic_cast<const AssignmentExpr*>(&expr)) {
        return emitAssignment(*assignmentExpr);
    }

    if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(&expr)) {
        return emitUnary(*unaryExpr);
    }

    if (const auto* incDecExpr = dynamic_cast<const IncDecExpr*>(&expr)) {
        return emitIncDec(*incDecExpr);
    }

    if (const auto* binaryExpr = dynamic_cast<const BinaryExpr*>(&expr)) {
        return emitBinary(*binaryExpr);
    }

    if (const auto* functionCall = dynamic_cast<const FunctionCall*>(&expr)) {
        return emitFunctionCall(*functionCall);
    }

    throw std::runtime_error("Maple IR generation error: unsupported expression node");
}

IRGenerator::ExprResult IRGenerator::emitLiteral(const Literal& literal) {
    switch (literal.kind()) {
    case LiteralKind::Integer:
        return ExprResult{"constval i32 " + literal.value(), ValueType::Int};
    case LiteralKind::Float:
        return ExprResult{"constval f32 " + normalizeFloatLiteral(literal.value()), ValueType::Float};
    case LiteralKind::Char:
        return ExprResult{"constval i8 " + std::to_string(decodeCharLiteral(literal.value())), ValueType::Char};
    }

    throw std::runtime_error("Maple IR generation error: unsupported literal kind");
}

IRGenerator::ExprResult IRGenerator::emitIdentifier(const Identifier& identifier) {
    const LocalBinding& binding = lookupBinding(identifier.name());
    return ExprResult{"dread " + mapleType(binding.type) + " " + binding.mapleName, binding.type};
}

IRGenerator::ExprResult IRGenerator::emitAssignment(const AssignmentExpr& assignmentExpr) {
    const LocalBinding& binding = lookupBinding(assignmentExpr.name());

    if (const auto* functionCall = dynamic_cast<const FunctionCall*>(&assignmentExpr.value())) {
        return emitFunctionCallInto(*functionCall, binding);
    }

    const ExprResult value = emitExpr(assignmentExpr.value());
    emitBody("dassign " + binding.mapleName + " (" + value.text + ")");
    return ExprResult{"dread " + mapleType(binding.type) + " " + binding.mapleName, binding.type};
}

IRGenerator::ExprResult IRGenerator::emitUnary(const UnaryExpr& unaryExpr) {
    const ExprResult operand = emitExpr(unaryExpr.operand());

    if (unaryExpr.op() == "-") {
        return ExprResult{"neg " + mapleType(operand.type) + " (" + operand.text + ")", operand.type};
    }

    if (unaryExpr.op() == "!") {
        return ExprResult{"lnot i32 (" + operand.text + ")", ValueType::Int};
    }

    throw std::runtime_error("Maple IR generation error: unsupported unary operator '" + unaryExpr.op() + "'");
}

IRGenerator::ExprResult IRGenerator::emitIncDec(const IncDecExpr& incDecExpr) {
    const LocalBinding& binding = lookupBinding(incDecExpr.name());
    const LocalBinding snapshotBinding = declareInternalTemp(binding.type, "inc_old");
    const LocalBinding updatedBinding = declareInternalTemp(binding.type, "inc_new");

    // Preserve the operand's previous value so postfix `i++`/`i--` can return
    // the old result even if later subexpressions keep mutating the same local.
    emitBody("dassign " + snapshotBinding.mapleName + " (dread " + mapleType(binding.type) + " " + binding.mapleName + ")");

    const std::string op = incDecExpr.op() == "++" ? "add " : "sub ";
    emitBody(
        "dassign " + updatedBinding.mapleName + " ("
        + op + mapleType(binding.type) + " (dread " + mapleType(binding.type) + " " + binding.mapleName
        + ", " + oneLiteral(binding.type) + "))");
    emitBody("dassign " + binding.mapleName + " (dread " + mapleType(updatedBinding.type) + " " + updatedBinding.mapleName + ")");

    if (incDecExpr.isPostfix()) {
        return ExprResult{
            "dread " + mapleType(snapshotBinding.type) + " " + snapshotBinding.mapleName,
            snapshotBinding.type};
    }

    return ExprResult{
        "dread " + mapleType(updatedBinding.type) + " " + updatedBinding.mapleName,
        updatedBinding.type};
}

IRGenerator::ExprResult IRGenerator::emitBinary(const BinaryExpr& binaryExpr) {
    const ExprResult left = emitExpr(binaryExpr.left());
    const ExprResult right = emitExpr(binaryExpr.right());
    const std::string& op = binaryExpr.op();

    if (op == "+") {
        return ExprResult{"add " + mapleType(left.type) + " (" + left.text + ", " + right.text + ")", left.type};
    }

    if (op == "-") {
        return ExprResult{"sub " + mapleType(left.type) + " (" + left.text + ", " + right.text + ")", left.type};
    }

    if (op == "*") {
        return ExprResult{"mul " + mapleType(left.type) + " (" + left.text + ", " + right.text + ")", left.type};
    }

    if (op == "/") {
        return ExprResult{"div " + mapleType(left.type) + " (" + left.text + ", " + right.text + ")", left.type};
    }

    if (op == "%") {
        return ExprResult{"rem " + mapleType(left.type) + " (" + left.text + ", " + right.text + ")", left.type};
    }

    // Arithmetic keeps the operand type, while comparisons/logical operators
    // produce MiniC's int truth values.
    if (op == ">") {
        return ExprResult{"gt i32 " + mapleType(left.type) + " (" + left.text + ", " + right.text + ")", ValueType::Int};
    }

    if (op == ">=") {
        return ExprResult{"ge i32 " + mapleType(left.type) + " (" + left.text + ", " + right.text + ")", ValueType::Int};
    }

    if (op == "<") {
        return ExprResult{"lt i32 " + mapleType(left.type) + " (" + left.text + ", " + right.text + ")", ValueType::Int};
    }

    if (op == "<=") {
        return ExprResult{"le i32 " + mapleType(left.type) + " (" + left.text + ", " + right.text + ")", ValueType::Int};
    }

    if (op == "==") {
        return ExprResult{"eq i32 " + mapleType(left.type) + " (" + left.text + ", " + right.text + ")", ValueType::Int};
    }

    if (op == "!=") {
        return ExprResult{"ne i32 " + mapleType(left.type) + " (" + left.text + ", " + right.text + ")", ValueType::Int};
    }

    if (op == "&&") {
        return ExprResult{"land i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
    }

    if (op == "||") {
        return ExprResult{"lior i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
    }

    throw std::runtime_error("Maple IR generation error: unsupported binary operator '" + op + "'");
}

void IRGenerator::emitFunctionCallStmt(const FunctionCall& functionCall) {
    std::vector<std::string> arguments;
    arguments.reserve(functionCall.arguments().size());

    for (const auto& argument : functionCall.arguments()) {
        arguments.push_back(emitExpr(*argument).text);
    }

    // When the MiniC source discards a function result, a plain Maple call is
    // enough; no temporary local is needed.
    emitBody("call " + functionSymbol(functionCall.callee()) + " (" + join(arguments, ", ") + ")");
}

IRGenerator::ExprResult IRGenerator::emitFunctionCall(const FunctionCall& functionCall) {
    const auto found = functions_.find(functionCall.callee());
    if (found == functions_.end()) {
        throw std::runtime_error("Maple IR generation error: unknown function '" + functionCall.callee() + "'");
    }

    std::vector<std::string> arguments;
    arguments.reserve(functionCall.arguments().size());

    // Arguments are lowered left-to-right so assignment or nested call side
    // effects preserve the MiniC evaluation order.
    for (const auto& argument : functionCall.arguments()) {
        arguments.push_back(emitExpr(*argument).text);
    }

    const ValueType returnType = found->second.returnType;
    const LocalBinding resultBinding = declareInternalTemp(returnType, "callret");
    return emitFunctionCallInto(functionCall, resultBinding);
}

IRGenerator::ExprResult IRGenerator::emitFunctionCallInto(const FunctionCall& functionCall, const LocalBinding& targetBinding) {
    std::vector<std::string> arguments;
    arguments.reserve(functionCall.arguments().size());

    for (const auto& argument : functionCall.arguments()) {
        arguments.push_back(emitExpr(*argument).text);
    }

    emitBody(
        "callassigned " + functionSymbol(functionCall.callee()) + " (" + join(arguments, ", ")
        + ") { dassign " + targetBinding.mapleName + " 0 }");

    return ExprResult{
        "dread " + mapleType(targetBinding.type) + " " + targetBinding.mapleName,
        targetBinding.type};
}

void IRGenerator::emitIRFunction(const IRModule& irModule, const std::string& functionName) {
    functionContext_ = FunctionContext{};
    scopes_.clear();
    localNameCounters_.clear();
    currentReturnType_ = inferIRReturnType(irModule);

    beginScope();
    emitBody("# BB entry");
    emitBody("@entry");

    // The current MiniC TAC is string-based and mostly untyped, so this path
    // preserves the control-flow shape and defaults scalars to i32.
    std::vector<ExprResult> pendingCallArguments;
    for (const std::string& instruction : irModule.instructions()) {
        const std::string text = trim(instruction);
        if (text.empty()) {
            continue;
        }

        if (startsWith(text, "var ")) {
            ensureIRBindingDeclared(text.substr(4));
            continue;
        }

        if (startsWith(text, "ifFalse ")) {
            const std::size_t gotoPos = text.find(" goto ");
            if (gotoPos == std::string::npos) {
                throw std::runtime_error("Maple IR generation error: malformed IR branch '" + text + "'");
            }

            const std::string conditionToken = text.substr(8, gotoPos - 8);
            const std::string target = text.substr(gotoPos + 6);
            const ExprResult condition = parseIRAtom(trim(conditionToken));
            emitBody("brfalse @" + trim(target) + " (" + condition.text + ")");
            continue;
        }

        if (startsWith(text, "goto ")) {
            emitBody("goto @" + trim(text.substr(5)));
            continue;
        }

        if (startsWith(text, "param ")) {
            pendingCallArguments.push_back(parseIRAtom(trim(text.substr(6))));
            continue;
        }

        if (!text.empty() && text.back() == ':') {
            emitBody("@" + text.substr(0, text.size() - 1));
            continue;
        }

        if (startsWith(text, "call ")) {
            const std::size_t commaPos = text.find(',');
            if (commaPos == std::string::npos) {
                throw std::runtime_error("Maple IR generation error: malformed IR call '" + text + "'");
            }

            const std::string callee = trim(text.substr(5, commaPos - 5));
            const std::size_t argumentCount = static_cast<std::size_t>(std::stoul(trim(text.substr(commaPos + 1))));
            if (argumentCount != pendingCallArguments.size()) {
                throw std::runtime_error("Maple IR generation error: IR call argument count mismatch for '" + callee + "'");
            }

            std::vector<std::string> arguments;
            arguments.reserve(pendingCallArguments.size());
            for (const ExprResult& argument : pendingCallArguments) {
                arguments.push_back(argument.text);
            }

            emitBody("call " + functionSymbol(callee) + " (" + join(arguments, ", ") + ")");
            pendingCallArguments.clear();
            continue;
        }

        if (startsWith(text, "return")) {
            if (text == "return") {
                emitBody("return ()");
            } else {
                const ExprResult value = parseIRAtom(trim(text.substr(7)));
                emitBody("return (" + value.text + ")");
            }

            continue;
        }

        const std::size_t assignPos = text.find(" = ");
        if (assignPos == std::string::npos) {
            throw std::runtime_error("Maple IR generation error: unsupported IR instruction '" + text + "'");
        }

        const std::string lhsName = trim(text.substr(0, assignPos));
        const std::string rhs = trim(text.substr(assignPos + 3));

        if (startsWith(rhs, "call ")) {
            const std::size_t commaPos = rhs.find(',');
            if (commaPos == std::string::npos) {
                throw std::runtime_error("Maple IR generation error: malformed IR call expression '" + rhs + "'");
            }

            const std::string callee = trim(rhs.substr(5, commaPos - 5));
            const std::size_t argumentCount = static_cast<std::size_t>(std::stoul(trim(rhs.substr(commaPos + 1))));
            if (argumentCount != pendingCallArguments.size()) {
                throw std::runtime_error("Maple IR generation error: IR call argument count mismatch for '" + callee + "'");
            }

            ensureIRBindingDeclared(lhsName);
            const LocalBinding& binding = lookupBinding(stripPercentPrefix(lhsName));
            std::vector<std::string> arguments;
            arguments.reserve(pendingCallArguments.size());
            for (const ExprResult& argument : pendingCallArguments) {
                arguments.push_back(argument.text);
            }

            emitBody(
                "callassigned " + functionSymbol(callee) + " (" + join(arguments, ", ")
                + ") { dassign " + binding.mapleName + " 0 }");
            pendingCallArguments.clear();
            continue;
        }

        ensureIRBindingDeclared(lhsName);
        const LocalBinding& binding = lookupBinding(stripPercentPrefix(lhsName));
        const ExprResult value = parseIRRhs(rhs);
        emitBody("dassign " + binding.mapleName + " (" + value.text + ")");
    }

    if (currentReturnType_ == ValueType::Void
        && (functionContext_.body.empty() || functionContext_.body.back() != "return ()")) {
        emitBody("return ()");
    }

    endScope();

    module_.addLine("func " + functionSymbol(functionName) + " () " + mapleType(currentReturnType_) + " {");

    for (const std::string& declaration : functionContext_.declarations) {
        module_.addLine("  " + declaration);
    }

    if (!functionContext_.declarations.empty() && !functionContext_.body.empty()) {
        module_.addLine("");
    }

    for (const std::string& line : functionContext_.body) {
        module_.addLine("  " + line);
    }

    module_.addLine("}");
}

void IRGenerator::ensureIRBindingDeclared(const std::string& token) {
    const std::string name = stripPercentPrefix(trim(token));
    if (name.empty() || isIntegerToken(name)) {
        return;
    }

    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
        if (scope->bindings.find(name) != scope->bindings.end()) {
            return;
        }
    }

    declareBinding(name, ValueType::Int);
}

IRGenerator::ExprResult IRGenerator::parseIRAtom(const std::string& token) {
    const std::string value = trim(token);

    if (isIntegerToken(value)) {
        return ExprResult{"constval i32 " + value, ValueType::Int};
    }

    if (isFloatToken(value)) {
        throw std::runtime_error("Maple IR generation error: untyped MiniC IR cannot safely lower float literals; use AST input instead");
    }

    if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
        return ExprResult{"constval i32 " + std::to_string(decodeCharLiteral(value)), ValueType::Int};
    }

    ensureIRBindingDeclared(value);
    const LocalBinding& binding = lookupBinding(stripPercentPrefix(value));
    return ExprResult{"dread i32 " + binding.mapleName, ValueType::Int};
}

IRGenerator::ExprResult IRGenerator::parseIRRhs(const std::string& rhs) {
    std::istringstream iss(rhs);
    std::vector<std::string> parts;
    std::string token;

    while (iss >> token) {
        parts.push_back(token);
    }

    if (parts.empty()) {
        throw std::runtime_error("Maple IR generation error: empty IR expression");
    }

    if (parts.size() == 1) {
        return parseIRAtom(parts[0]);
    }

    if (parts.size() == 2) {
        const ExprResult operand = parseIRAtom(parts[1]);

        if (parts[0] == "-") {
            return ExprResult{"neg i32 (" + operand.text + ")", ValueType::Int};
        }

        if (parts[0] == "!") {
            return ExprResult{"lnot i32 (" + operand.text + ")", ValueType::Int};
        }

        throw std::runtime_error("Maple IR generation error: unsupported unary IR operator '" + parts[0] + "'");
    }

    if (parts.size() == 3) {
        const ExprResult left = parseIRAtom(parts[0]);
        const ExprResult right = parseIRAtom(parts[2]);
        const std::string& op = parts[1];

        if (op == "+") {
            return ExprResult{"add i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        if (op == "-") {
            return ExprResult{"sub i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        if (op == "*") {
            return ExprResult{"mul i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        if (op == "/") {
            return ExprResult{"div i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        if (op == "%") {
            return ExprResult{"rem i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        if (op == ">") {
            return ExprResult{"gt i32 i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        if (op == ">=") {
            return ExprResult{"ge i32 i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        if (op == "<") {
            return ExprResult{"lt i32 i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        if (op == "<=") {
            return ExprResult{"le i32 i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        if (op == "==") {
            return ExprResult{"eq i32 i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        if (op == "!=") {
            return ExprResult{"ne i32 i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        if (op == "&&") {
            return ExprResult{"land i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        if (op == "||") {
            return ExprResult{"lior i32 (" + left.text + ", " + right.text + ")", ValueType::Int};
        }

        throw std::runtime_error("Maple IR generation error: unsupported binary IR operator '" + op + "'");
    }

    throw std::runtime_error("Maple IR generation error: unsupported IR expression '" + rhs + "'");
}

ValueType IRGenerator::inferIRReturnType(const IRModule& irModule) const {
    for (const std::string& instruction : irModule.instructions()) {
        const std::string text = trim(instruction);
        if (startsWith(text, "return ") && text != "return") {
            return ValueType::Int;
        }
    }

    return ValueType::Void;
}

std::string generate_abc(const Program& program, const std::string& outputPath, const BackendConfig& config) {
    SemanticAnalyzer semanticAnalyzer;
    semanticAnalyzer.analyze(program);

    IRGenerator generator;
    generator.generate(program);
    return invokeBackend(generator.module(), outputPath, config);
}

std::string generate_abc(const IRModule& irModule, const std::string& outputPath, const BackendConfig& config) {
    IRGenerator generator;
    generator.generate(irModule);
    return invokeBackend(generator.module(), outputPath, config);
}

} // namespace maple
