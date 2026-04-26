#include "Lexer.h"

#include <cctype>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace {
const std::unordered_set<std::string> kKeywords = {
    "int",
    "if",
    "else",
    "while",
    "return",
};

bool isIdentifierStart(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) != 0;
}

bool isIdentifierBody(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

bool isOperator(char ch) {
    return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '=' || ch == '<' || ch == '>';
}

bool isDelimiter(char ch) {
    return ch == ';' || ch == ',' || ch == '(' || ch == ')' || ch == '{' || ch == '}';
}
}

Lexer::Lexer(std::string source)
    : source_(std::move(source)),
      position_(0),
      line_(1),
      column_(1) {}

std::string tokenTypeToString(TokenType type) {
    switch (type) {
    case TokenType::KEYWORD:
        return "KEYWORD";
    case TokenType::IDENTIFIER:
        return "IDENTIFIER";
    case TokenType::INTEGER:
        return "INTEGER";
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

Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    if (isAtEnd()) {
        return makeToken(TokenType::END, "", line_, column_);
    }

    const char ch = peek();

    if (isIdentifierStart(ch)) {
        return identifierOrKeyword();
    }

    if (std::isdigit(static_cast<unsigned char>(ch)) != 0) {
        return integer();
    }

    if ((ch == '=' || ch == '!' || ch == '<' || ch == '>') && peek(1) == '=') {
        const int startLine = line_;
        const int startColumn = column_;
        std::string text;
        text += advance();
        text += advance();
        return makeToken(TokenType::OPERATOR, text, startLine, startColumn);
    }

    if (isOperator(ch)) {
        return singleCharacterToken(TokenType::OPERATOR);
    }

    if (isDelimiter(ch)) {
        return singleCharacterToken(TokenType::DELIMITER);
    }

    return singleCharacterToken(TokenType::UNKNOWN);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        Token token = nextToken();
        tokens.push_back(token);

        if (token.type == TokenType::END) {
            break;
        }
    }

    return tokens;
}

bool Lexer::isAtEnd() const {
    return position_ >= source_.size();
}

char Lexer::peek(std::size_t offset) const {
    const std::size_t index = position_ + offset;
    if (index >= source_.size()) {
        return '\0';
    }

    return source_[index];
}

char Lexer::advance() {
    const char ch = source_[position_++];

    if (ch == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }

    return ch;
}

void Lexer::skipWhitespaceAndComments() {
    bool consumed = true;

    while (consumed && !isAtEnd()) {
        consumed = false;

        while (!isAtEnd() && std::isspace(static_cast<unsigned char>(peek())) != 0) {
            advance();
            consumed = true;
        }

        if (peek() == '/' && peek(1) == '/') {
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
            consumed = true;
            continue;
        }

        if (peek() == '/' && peek(1) == '*') {
            advance();
            advance();
            consumed = true;

            while (!isAtEnd() && !(peek() == '*' && peek(1) == '/')) {
                advance();
            }

            if (isAtEnd()) {
                throw std::runtime_error("Unterminated block comment");
            }

            advance();
            advance();
        }
    }
}

Token Lexer::makeToken(TokenType type, const std::string& text, int line, int column) const {
    return Token{type, text, line, column};
}

Token Lexer::identifierOrKeyword() {
    const int startLine = line_;
    const int startColumn = column_;
    std::string text;

    text += advance();
    while (!isAtEnd() && isIdentifierBody(peek())) {
        text += advance();
    }

    const TokenType type = kKeywords.count(text) > 0 ? TokenType::KEYWORD : TokenType::IDENTIFIER;
    return makeToken(type, text, startLine, startColumn);
}

Token Lexer::integer() {
    const int startLine = line_;
    const int startColumn = column_;
    std::string text;

    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
        text += advance();
    }

    return makeToken(TokenType::INTEGER, text, startLine, startColumn);
}

Token Lexer::singleCharacterToken(TokenType type) {
    const int startLine = line_;
    const int startColumn = column_;
    const std::string text(1, advance());

    return makeToken(type, text, startLine, startColumn);
}
