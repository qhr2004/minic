#pragma once

#include <string>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    INTEGER,
    OPERATOR,
    DELIMITER,
    END,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string text;
    int line;
    int column;
};

std::string tokenTypeToString(TokenType type);
