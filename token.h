#ifndef FILANG_TOKEN_H
#define FILANG_TOKEN_H

typedef enum {

    //parenthesis
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,

    //operators
    TOKEN_COMMA, TOKEN_DOT,
    TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH,
    TOKEN_STAR, TOKEN_POW,
    TOKEN_AND, TOKEN_OR, TOKEN_NOT,
    TOKEN_PRINT, TOKEN_COLONS,

    //comparison
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_INTERROGATION,

    //literals
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_INTEGER, TOKEN_FLOAT,

    //keywords
    TOKEN_RETURN,

    //conditionals
    TOKEN_IF, TOKEN_ELSE,

    //declarations
    TOKEN_FN, TOKEN_VAR,

    //constants
    TOKEN_EOF, TOKEN_TRUE, TOKEN_FALSE, TOKEN_NIL,

    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int length;
    int line;
} Token;

#endif //FILANG_TOKEN_H
