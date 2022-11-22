#ifndef scanner
#define scanner

#include <vector>
#include <string>
#include <unordered_map>
#include "Token.hpp"
#include "../../Fi.hpp"


class Lexer{
    std::vector<Token> tokens;
    std::string source;
    int start = 0;
    int current = 0;
    int line = 1;
    std::unordered_map<std::string, TokenType> identifiers= {
        {"false",   TokenType::FALSE},
        {"true",    TokenType::TRUE},
        {"and",     TokenType::AND},
        {"or",      TokenType::OR},
        {"if",      TokenType::IF},
        {"else",    TokenType::ELSE},
        {"while",   TokenType::WHILE},
        {"for",     TokenType::FOR},
        {"fn",      TokenType::FN},
        {"print",   TokenType::PRINT},
        {"return",  TokenType::RETURN},
        {"var",     TokenType::VAR}
    };

public:
    Lexer(const std::string &source): source(source) {}
    std::vector<Token> scan();
    void number();
    void identifier();
    bool isEOF();
    void advance();
    char peek();
    char next();
    void addToken(TokenType type);
    void addToken(TokenType type, std::any literal);
};

#endif