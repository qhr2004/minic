#pragma once

#include <string>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    INTEGER,
    FLOAT,
    CHAR_LITERAL,
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
