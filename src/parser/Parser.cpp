#include "parser/Parser.h"

#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
bool isTypeKeyword(const Token& token) {
    if (token.type != TokenType::KEYWORD) {
        return false;
    }

    return token.text == "int" || token.text == "float" || token.text == "char";
}
}

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)),
      position_(0) {
    if (tokens_.empty() || tokens_.back().type != TokenType::END) {
        tokens_.push_back(Token{TokenType::END, "", 0, 0});
    }
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();

    while (!isAtEnd()) {
        program->addNode(parseTopLevelNode());
    }

    expect(TokenType::END, "end of file");
    return program;
}

ExprPtr Parser::parseStandaloneExpression() {
    ExprPtr expression = parseExpression();
    expect(TokenType::END, "expected end of expression");
    return expression;
}

std::unique_ptr<ASTNode> Parser::parseTopLevelNode() {
    if (isFunctionDeclAhead()) {
        return parseFunctionDecl();
    }

    return parseStatement();
}

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
    Token returnType = expectTypeToken("expected function return type");
    Token name = expect(TokenType::IDENTIFIER, "expected function name");

    expect(TokenType::DELIMITER, "(", "expected '(' after function name");
    std::vector<ParameterPtr> parameters = parseParameterList();
    expect(TokenType::DELIMITER, ")", "expected ')' after function parameter list");

    auto body = parseBlockStmt();
    return std::make_unique<FunctionDecl>(returnType.text, name.text, std::move(parameters), std::move(body));
}

std::vector<ParameterPtr> Parser::parseParameterList() {
    std::vector<ParameterPtr> parameters;

    if (check(TokenType::DELIMITER, ")")) {
        return parameters;
    }

    parameters.push_back(parseParameter());

    while (match(TokenType::DELIMITER, ",")) {
        parameters.push_back(parseParameter());
    }

    return parameters;
}

ParameterPtr Parser::parseParameter() {
    Token type = expectTypeToken("expected parameter type");
    Token name = expect(TokenType::IDENTIFIER, "expected parameter name");
    return std::make_unique<ParameterDecl>(type.text, name.text);
}

StmtPtr Parser::parseStatement() {
    if (isTypeKeyword(peek())) {
        return parseVarDeclStmt();
    }

    if (match(TokenType::KEYWORD, "return")) {
        return parseReturnStmt();
    }

    if (match(TokenType::KEYWORD, "if")) {
        return parseIfStmt();
    }

    if (match(TokenType::KEYWORD, "while")) {
        return parseWhileStmt();
    }

    if (check(TokenType::DELIMITER, "{")) {
        return parseBlockStmt();
    }

    return parseExprStmt();
}

StmtPtr Parser::parseVarDeclStmt() {
    Token type = expectTypeToken("expected variable type");
    Token name = expect(TokenType::IDENTIFIER, "expected variable name");

    ExprPtr initializer;
    if (match(TokenType::OPERATOR, "=")) {
        initializer = parseExpression();
    }

    expect(TokenType::DELIMITER, ";", "expected ';' after variable declaration");
    return std::make_unique<VarDeclStmt>(type.text, name.text, std::move(initializer));
}

StmtPtr Parser::parseReturnStmt() {
    ExprPtr value;

    if (!check(TokenType::DELIMITER, ";")) {
        value = parseExpression();
    }

    expect(TokenType::DELIMITER, ";", "expected ';' after return statement");
    return std::make_unique<ReturnStmt>(std::move(value));
}

StmtPtr Parser::parseIfStmt() {
    expect(TokenType::DELIMITER, "(", "expected '(' after 'if'");
    ExprPtr condition = parseExpression();
    expect(TokenType::DELIMITER, ")", "expected ')' after if condition");

    StmtPtr thenBranch = parseStatement();
    StmtPtr elseBranch;

    if (match(TokenType::KEYWORD, "else")) {
        elseBranch = parseStatement();
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

StmtPtr Parser::parseWhileStmt() {
    expect(TokenType::DELIMITER, "(", "expected '(' after 'while'");
    ExprPtr condition = parseExpression();
    expect(TokenType::DELIMITER, ")", "expected ')' after while condition");

    StmtPtr body = parseStatement();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<BlockStmt> Parser::parseBlockStmt() {
    expect(TokenType::DELIMITER, "{", "expected '{' to start block");
    auto block = std::make_unique<BlockStmt>();

    while (!check(TokenType::DELIMITER, "}") && !isAtEnd()) {
        block->addStatement(parseStatement());
    }

    expect(TokenType::DELIMITER, "}", "expected '}' after block");
    return block;
}

StmtPtr Parser::parseExprStmt() {
    ExprPtr expression = parseExpression();
    expect(TokenType::DELIMITER, ";", "expected ';' after expression");
    return std::make_unique<ExprStmt>(std::move(expression));
}

ExprPtr Parser::parseExpression() {
    return parseAssignment();
}

ExprPtr Parser::parseAssignment() {
    ExprPtr expression = parseEquality();

    if (match(TokenType::OPERATOR, "=")) {
        const auto* identifier = dynamic_cast<const IdentifierExpr*>(expression.get());
        if (identifier == nullptr) {
            throw errorAt(previous(), "invalid assignment target");
        }

        std::string name = identifier->name();
        ExprPtr value = parseAssignment();
        return std::make_unique<AssignmentExpr>(std::move(name), std::move(value));
    }

    return expression;
}

ExprPtr Parser::parseEquality() {
    ExprPtr expression = parseComparison();

    while (match(TokenType::OPERATOR, "==") || match(TokenType::OPERATOR, "!=")) {
        std::string op = previous().text;
        ExprPtr right = parseComparison();
        expression = std::make_unique<BinaryExpr>(std::move(op), std::move(expression), std::move(right));
    }

    return expression;
}

ExprPtr Parser::parseComparison() {
    ExprPtr expression = parseExpr();

    while (match(TokenType::OPERATOR, ">") || match(TokenType::OPERATOR, ">=")
        || match(TokenType::OPERATOR, "<") || match(TokenType::OPERATOR, "<=")
        || match(TokenType::OPERATOR, "&&") || match(TokenType::OPERATOR, "||")) {
        std::string op = previous().text;
        ExprPtr right = parseExpr();
        expression = std::make_unique<BinaryExpr>(std::move(op), std::move(expression), std::move(right));
    }

    return expression;
}

ExprPtr Parser::parseExpr() {
    ExprPtr expression = parseTerm();

    while (match(TokenType::OPERATOR, "+") || match(TokenType::OPERATOR, "-")) {
        std::string op = previous().text;
        ExprPtr right = parseTerm();
        expression = std::make_unique<BinaryExpr>(std::move(op), std::move(expression), std::move(right));
    }

    return expression;
}

ExprPtr Parser::parseTerm() {
    ExprPtr expression = parseFactor();

    while (match(TokenType::OPERATOR, "*") || match(TokenType::OPERATOR, "/")
        || match(TokenType::OPERATOR, "%")) {
        std::string op = previous().text;
        ExprPtr right = parseFactor();
        expression = std::make_unique<BinaryExpr>(std::move(op), std::move(expression), std::move(right));
    }

    return expression;
}

ExprPtr Parser::parseFactor() {
    if (match(TokenType::INTEGER)) {
        return std::make_unique<Literal>(previous().text, LiteralKind::Integer);
    }

    if (match(TokenType::FLOAT)) {
        return std::make_unique<Literal>(previous().text, LiteralKind::Float);
    }

    if (match(TokenType::CHAR_LITERAL)) {
        return std::make_unique<Literal>(previous().text, LiteralKind::Char);
    }

    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<Identifier>(previous().text);
    }

    if (match(TokenType::DELIMITER, "(")) {
        ExprPtr expression = parseExpression();
        expect(TokenType::DELIMITER, ")", "expected ')' after expression");
        return expression;
    }

    throw errorAt(peek(), "expected expression");
}

const Token& Parser::peek(std::size_t offset) const {
    const std::size_t index = position_ + offset;
    if (index >= tokens_.size()) {
        return tokens_.back();
    }

    return tokens_[index];
}

const Token& Parser::previous() const {
    return tokens_[position_ - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END;
}

bool Parser::check(TokenType type, const std::string& text, std::size_t offset) const {
    const Token& token = peek(offset);
    if (token.type != type) {
        return false;
    }

    return text.empty() || token.text == text;
}

bool Parser::match(TokenType type, const std::string& text) {
    if (!check(type, text)) {
        return false;
    }

    advance();
    return true;
}

Token Parser::advance() {
    if (!isAtEnd()) {
        ++position_;
        return tokens_[position_ - 1];
    }

    return peek();
}

Token Parser::expect(TokenType type, const std::string& text, const std::string& message) {
    if (check(type, text)) {
        return advance();
    }

    throw errorAt(peek(), message);
}

Token Parser::expect(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }

    throw errorAt(peek(), message);
}

Token Parser::expectTypeToken(const std::string& message) {
    if (isTypeKeyword(peek())) {
        return advance();
    }

    throw errorAt(peek(), message);
}

bool Parser::isFunctionDeclAhead() const {
    return isTypeKeyword(peek())
        && check(TokenType::IDENTIFIER, "", 1)
        && check(TokenType::DELIMITER, "(", 2);
}

std::runtime_error Parser::errorAt(const Token& token, const std::string& message) const {
    std::ostringstream oss;
    oss << "Parse error at " << token.line << ':' << token.column << ": " << message;

    if (token.type != TokenType::END && !token.text.empty()) {
        oss << " near '" << token.text << "'";
    }

    return std::runtime_error(oss.str());
}
