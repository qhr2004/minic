#pragma once

#include "AST.h"
#include "Token.h"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::unique_ptr<Program> parseProgram();

private:
    std::vector<Token> tokens_;
    std::size_t position_;

    std::unique_ptr<ASTNode> parseTopLevelNode();
    std::unique_ptr<FunctionDecl> parseFunctionDecl();

    StmtPtr parseStatement();
    StmtPtr parseVarDeclStmt();
    StmtPtr parseReturnStmt();
    StmtPtr parseIfStmt();
    StmtPtr parseWhileStmt();
    std::unique_ptr<BlockStmt> parseBlockStmt();
    StmtPtr parseExprStmt();

    ExprPtr parseExpression();
    ExprPtr parseAssignment();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseAdditive();
    ExprPtr parseMultiplicative();
    ExprPtr parseUnary();
    ExprPtr parsePrimary();

    const Token& peek(std::size_t offset = 0) const;
    const Token& previous() const;
    bool isAtEnd() const;
    bool check(TokenType type, const std::string& text = "", std::size_t offset = 0) const;
    bool match(TokenType type, const std::string& text = "");
    Token advance();
    Token expect(TokenType type, const std::string& text, const std::string& message);
    Token expect(TokenType type, const std::string& message);

    bool isFunctionDeclAhead() const;
    std::runtime_error errorAt(const Token& token, const std::string& message) const;
};
