#include "lexer/Token.h"

std::string tokenTypeToString(TokenType type) {
    switch (type) {
    case TokenType::KEYWORD:
        return "KEYWORD";
    case TokenType::IDENTIFIER:
        return "IDENTIFIER";
    case TokenType::INTEGER:
        return "INTEGER";
    case TokenType::FLOAT:
        return "FLOAT";
    case TokenType::CHAR_LITERAL:
        return "CHAR_LITERAL";
    case TokenType::OPERATOR:
        return "OPERATOR";
    case TokenType::DELIMITER:
        return "DELIMITER";
    case TokenType::END:
        return "END";
    case TokenType::UNKNOWN:
        return "UNKNOWN";
    }

    return "UNKNOWN";
}
