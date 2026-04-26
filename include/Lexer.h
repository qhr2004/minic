#pragma once

#include "Token.h"

#include <cstddef>
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(std::string source);

    Token nextToken();
    std::vector<Token> tokenize();

private:
    std::string source_;
    std::size_t position_;
    int line_;
    int column_;

    bool isAtEnd() const;
    char peek(std::size_t offset = 0) const;
    char advance();
    void skipWhitespaceAndComments();

    Token makeToken(TokenType type, const std::string& text, int line, int column) const;
    Token identifierOrKeyword();
    Token integer();
    Token singleCharacterToken(TokenType type);
};
