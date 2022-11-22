#ifndef TKNTPE
#define TKNTPE

#include <string>
#include <any>


namespace TKNTYPE{
    enum TokenType{
        //literals
        IDENTIFIER,
        STRING, CHAR, FLOAT, INT, FALSE, TRUE, NIL,

        //operations
        PLUS, MINUS, STAR, SLASH, MODULO, BANG,
        
        //bitwise
        BW_NOT, BW_AND, BW_OR, BW_XOR, BW_LSHIFT, BW_RSHIFT,

        //comparison
        BANG_EQUAL, EQUAL_EQUAL,
        GREATER, GREATER_EQUAL,
        LESS, LESS_EQUAL,

        //brackets
        LEFT_ROUND, RIGHT_ROUND,
        LEFT_BRACE, RIGHT_BRACE,

        //keywords
        AND, OR, IF, ELSE, WHILE, FOR, CLASS, SUPER, THIS, FN, PRINT, RETURN, VAR,

        //assignment
        EQUAL, 

        //ends
        SEMICOLON, END_OF_FILE
    };
};

using TokenType = TKNTYPE::TokenType;

class Token{
    TokenType type;
    std::string lexeme;
    std::any literal;
    int line;
public:
    Token(TokenType type, std::string lexeme, int line) :
        Token(type, lexeme, std::any{}, line)
    {}

    Token(TokenType type, std::string lexeme, std::any literal, int line):
        type(type),
        lexeme(lexeme),
        literal(literal),
        line(line)
    {}

    std::string toString(){
        return std::to_string(type) + " " + lexeme + "\n";
    }

    TokenType getType(){
        return type;
    }

    std::string getLexeme(){
        return lexeme;
    }

    std::any getLiteral(){
        return literal;
    }

    int getLine(){
        return line;
    }
};

#endif