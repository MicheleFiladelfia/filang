#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "scanner.h"
#include "token.h"

typedef struct {
    const char *start;
    const char *current;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const char *source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAtEnd() {
    return *scanner.current == '\0';
}

static char peek() {
    return *scanner.current;
}

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '#':
                while (peek() != '\n' && !isAtEnd()) advance();
                break;
            default:
                return;
        }
    }
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}


static Token makeToken(TokenType type) {
    Token token;

    token.type = type;
    token.start = scanner.start;
    token.line = scanner.line;
    token.length = scanner.current - scanner.start;

    return token;
}

static Token number() {
    bool isFloat = false;

    while (isDigit(peek())) advance();

    if (peek() == '.' && isDigit(peekNext())) {
        isFloat = true;
        advance();
        while (isDigit(peek())) advance();
    }

    return isFloat ? makeToken(TOKEN_FLOAT) : makeToken(TOKEN_INTEGER);
}

bool checkKeyword(char *expected) {
    int i = 0;
    while (isAlpha(peek()) || isDigit(peek()) || i < strlen(expected)) {
        if (expected[i] != peek()) {
            return false;
        }
        advance();
        i++;
    }
    return true;
}

static Token identifier() {
    switch (scanner.start[0]) {
        case 'a':
            if (checkKeyword("nd")) return makeToken(TOKEN_AND);
        case 'o':
            if (checkKeyword("r")) return makeToken(TOKEN_OR);
        case 'i':
            if (checkKeyword("f")) return makeToken(TOKEN_IF);
        case 'e':
            if (checkKeyword("lse")) return makeToken(TOKEN_ELSE);
        case 'p':
            if (checkKeyword("rint")) return makeToken(TOKEN_PRINT);
        case 'r':
            if (checkKeyword("eturn")) return makeToken(TOKEN_RETURN);
        case 'f':
            if (checkKeyword("n")) return makeToken(TOKEN_FN);
            if (checkKeyword("alse")) return makeToken(TOKEN_FALSE);
        case 'v':
            if (checkKeyword("ar")) return makeToken(TOKEN_VAR);
        case 't':
            if (checkKeyword("rue")) return makeToken(TOKEN_TRUE);
        case 'n':
            if (checkKeyword("il")) return makeToken(TOKEN_NIL);
            if (checkKeyword("ot")) return makeToken(TOKEN_NOT);

    }


    while (isAlpha(peek()) || isDigit(peek())) advance();

    return makeToken(TOKEN_IDENTIFIER);
}

static bool match(char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;

    scanner.current++;
    return true;
}

static Token errorToken(char *errorMessage) {
    Token token;

    token.start = scanner.start;
    token.length = (int) strlen(errorMessage);
    token.line = scanner.line;
    token.type = TOKEN_ERROR;

    return token;
}

static Token string() {
    while (peek() != '"') {
        if (isAtEnd()) return errorToken("Unterminated string.");
        if (peek() == '\n') scanner.line++;
        advance();
    }

    advance();
    return makeToken(TOKEN_STRING);
}

Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    char c = advance();

    if (isDigit(c)) return number();
    if (isAlpha(c)) return identifier();

    switch (c) {
        case '(':
            return makeToken(TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(TOKEN_RIGHT_BRACE);
        case ',':
            return makeToken(TOKEN_COMMA);
        case '.':
            return makeToken(TOKEN_DOT);
        case '-':
            return makeToken(TOKEN_MINUS);
        case '+':
            return makeToken(TOKEN_PLUS);
        case '*':
            return makeToken(TOKEN_STAR);
        case '/':
            return makeToken(TOKEN_SLASH);
        case '%':
            return makeToken(TOKEN_PERCENT);
        case ';':
            return makeToken(TOKEN_SEMICOLON);
        case ':':
            return makeToken(TOKEN_COLONS);
        case '^':
            return makeToken(TOKEN_POW);
        case '?':
            return makeToken(TOKEN_INTERROGATION);
        case '=':
            if (match('=')) {
                return makeToken(TOKEN_EQUAL_EQUAL);
            } else {
                return makeToken(TOKEN_EQUAL);
            }
        case '!':
            if (match('=')) {
                return makeToken(TOKEN_BANG_EQUAL);
            }
        case '>':
            if (match('=')) {
                return makeToken(TOKEN_GREATER_EQUAL);
            } else {
                return makeToken(TOKEN_GREATER);
            }
        case '<':
            if (match('=')) {
                return makeToken(TOKEN_LESS_EQUAL);
            } else {
                return makeToken(TOKEN_LESS);
            }

        case '"' :
            return string();
    }

    return errorToken("Unexpected character.");
}

