#include <string.h>
#include <stdbool.h>
#include <malloc.h>
#include <ctype.h>
#include "scanner.h"
#include "token.h"

typedef struct {
    const char *start;
    const char *current;
    int line;
} Scanner;

Scanner scanner;

void init_scanner(const char *source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool is_at_end() {
    return *scanner.current == '\0';
}

static char peek() {
    return *scanner.current;
}

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static char peek_next() {
    if (is_at_end()) return '\0';
    return scanner.current[1];
}

static void skip_whitespace() {
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
                while (peek() != '\n' && !is_at_end()) advance();
                break;
            default:
                return;
        }
    }
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

static Token make_token(TokenType type) {
    Token token;

    token.type = type;
    token.start = scanner.start;
    token.line = scanner.line;
    token.length = (int) (scanner.current - scanner.start);

    return token;
}

static Token number() {
    bool is_float = false;

    while (isdigit(peek())) advance();

    if (peek() == '.' && isdigit(peek_next())) {
        is_float = true;
        advance();
        while (isdigit(peek())) advance();
    }

    return is_float ? make_token(TOKEN_FLOAT) : make_token(TOKEN_INTEGER);
}

bool check_keyword(char *expected) {
    int i = 0;
    while (is_alpha(peek()) || isdigit(peek()) || i < (int) strlen(expected)) {
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
            if (check_keyword("nd")) return make_token(TOKEN_AND);
            break;
        case 'o':
            if (check_keyword("r")) return make_token(TOKEN_OR);
            break;
        case 'i':
            if (check_keyword("f")) return make_token(TOKEN_IF);
            break;
        case 'e':
            if (check_keyword("lse")) return make_token(TOKEN_ELSE);
            break;
        case 'p':
            if (check_keyword("rint")) return make_token(TOKEN_PRINT);
            break;
        case 'r':
            if (check_keyword("eturn")) return make_token(TOKEN_RETURN);
            break;
        case 'f':
            if (check_keyword("alse")) return make_token(TOKEN_FALSE);
            break;
        case 't':
            if (check_keyword("rue")) return make_token(TOKEN_TRUE);
            if (check_keyword("ypeof")) return make_token(TOKEN_TYPEOF);
            break;
        case 'n':
            if (check_keyword("il")) return make_token(TOKEN_NIL);
            if (check_keyword("ot")) return make_token(TOKEN_NOT);
            break;
        case 'c':
            if (check_keyword("lock")) return make_token(TOKEN_CLOCK);
            break;

    }


    while (is_alpha(peek()) || isdigit(peek())) advance();

    return make_token(TOKEN_IDENTIFIER);
}

static bool match(char expected) {
    if (is_at_end()) return false;
    if (*scanner.current != expected) return false;

    scanner.current++;
    return true;
}

static Token error_token(char *error_message, const char *error_char) {
    Token token;
    char *error = malloc(strlen(error_message) + strlen(error_char) + 1);
    strcpy(error, error_message);
    strcat(error, error_char);
    strcat(error, "\0");
    token.start = error;
    token.length = (int) strlen(error);
    token.line = scanner.line;
    token.type = TOKEN_ERROR;

    return token;
}

static Token string(char terminator) {
    while (peek() != terminator) {
        if (is_at_end()) return error_token("Unterminated string.", "");
        if (peek() == '\n') scanner.line++;
        if (peek() == '\\') advance();
        advance();
    }

    advance();
    return make_token(TOKEN_STRING);
}

Token scan_token() {
    skip_whitespace();
    scanner.start = scanner.current;

    if (is_at_end()) return make_token(TOKEN_EOF);

    char c = advance();

    if (isdigit(c)) return number();
    if (is_alpha(c)) return identifier();

    switch (c) {
        case '(':
            return make_token(TOKEN_LEFT_PAREN);
        case ')':
            return make_token(TOKEN_RIGHT_PAREN);
        case '{':
            return make_token(TOKEN_LEFT_BRACE);
        case '}':
            return make_token(TOKEN_RIGHT_BRACE);
        case ',':
            return make_token(TOKEN_COMMA);
        case '.':
            return make_token(TOKEN_DOT);
        case '-':
            return make_token(TOKEN_MINUS);
        case '+':
            return make_token(TOKEN_PLUS);
        case '*':
            if (match('*')) {
                return make_token(TOKEN_STAR_STAR);
            } else {
                return make_token(TOKEN_STAR);
            }
        case '/':
            return make_token(TOKEN_SLASH);
        case '%':
            return make_token(TOKEN_PERCENT);
        case ';':
            return make_token(TOKEN_SEMICOLON);
        case ':':
            return make_token(TOKEN_COLONS);
        case '?':
            return make_token(TOKEN_INTERROGATION);
        case '=':
            if (match('=')) {
                return make_token(TOKEN_EQUAL_EQUAL);
            } else {
                return make_token(TOKEN_EQUAL);
            }
        case '!':
            if (match('=')) {
                return make_token(TOKEN_BANG_EQUAL);
            } else {
                return error_token("Unexpected character", "!");
            }
        case '>':
            if (match('=')) {
                return make_token(TOKEN_GREATER_EQUAL);
            } else if (match('>')) {
                return make_token(TOKEN_GREATER_GREATER);
            } else {
                return make_token(TOKEN_GREATER);
            }
        case '<':
            if (match('=')) {
                return make_token(TOKEN_LESS_EQUAL);
            } else if (match('<')) {
                return make_token(TOKEN_LESS_LESS);
            } else {
                return make_token(TOKEN_LESS);
            }
        case '^':
            return make_token(TOKEN_CARET);
        case '|':
            return make_token(TOKEN_PIPE);
        case '&':
            return make_token(TOKEN_AMPERSAND);
        case '~':
            return make_token(TOKEN_TILDE);
        case '"' :
        case '\'':
            return string(c);
        default:
            if (isprint(c)) {
                return error_token("Unexpected character: ", (char[4]) {'\'', c, '\'', '.'});
            }

            return error_token("Unexpected character.", "");
    }
}

