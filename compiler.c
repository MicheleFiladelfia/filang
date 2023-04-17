#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "memory.h"
#include "scanner.h"
#include "strings.h"

typedef struct {
    Token previous;
    Token current;
    bool hasError;
    bool panicMode;
} Parser;

Parser parser;
Chunk *compileChunk;

typedef enum {
    PrecNone,       // None
    PrecAssignment, // =
    PrecTernary,    // ? :
    PrecOr,         // or
    PrecAnd,        // and
    PrecBWOr,       // |
    PrecXor,        // ^
    PrecBWAnd,      // &
    PrecEquals,     // == !=
    PrecCompare,    // < > <= >=
    PrecShift,      // << >>
    PrecTerm,       // + -
    PrecFactor,     // * / %
    PrecUnary,      // - ! ~
    PrecPow,        // **
    PrecCall        // . ()
} ParsePrec;

typedef void (*ParseFn)(bool allowAssignment);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    ParsePrec prec;
} ParseRule;


static void compileError(Token *token, const char *message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    parser.hasError = true;


    fprintf(stderr, "[line %d] CompileError", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
}

static void errorAtCurrent(const char *message) {
    compileError(&parser.current, message);
}

static void errorAtPrevious(const char *message) {
    compileError(&parser.previous, message);
}

static void emitByte(uint8_t byte) {
    writeChunk(compileChunk, byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}


static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, writeConstant(compileChunk, value));
}

static void advance() {
    parser.previous = parser.current;

    while (true) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;
        errorAtCurrent(parser.current.start);
    }
}

static void skipToNextStatement() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;

        switch (parser.current.type) {
            case TOKEN_VAR:
            case TOKEN_PRINT:
                return;
            default:
                break;
        }

        advance();
    }
}

static void consume(TokenType expectedType, const char *errorMessage) {
    if (parser.current.type == expectedType) {
        advance();
    } else {
        errorAtCurrent(errorMessage);
    }
}

static bool match(TokenType expectedType) {
    if (parser.current.type == expectedType) {
        advance();
        return true;
    }
    return false;
}


static ParseRule *getRule(TokenType type);


static void parsePrecedence(ParsePrec precedence) {
    advance();
    ParseFn prefix = getRule(parser.previous.type)->prefix;

    if (prefix == NULL) {
        errorAtPrevious("expected expression.");
        return;
    }

    bool allowAssignment = precedence <= PrecAssignment;
    prefix(allowAssignment);

    ParseRule *rule;
    while (rule = getRule(parser.current.type), precedence <= rule->prec) {
        ParseFn infix = rule->infix;
        advance();
        infix(allowAssignment);
    }

    if (allowAssignment && match(TOKEN_EQUAL)) {
        errorAtPrevious("Invalid assignment target.");
    }
}


static void expression() {
    parsePrecedence(PrecAssignment);
}

static void print() {
    parsePrecedence(PrecNone + 1);
    emitByte(OP_PRINT);
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        print();
        consume(TOKEN_SEMICOLON, "expected ';' after print statement.");
    } else {
        expression();
        emitByte(OP_POP);
        consume(TOKEN_SEMICOLON, "expected ';' after expression.");
    }
}

static void varDefinition() {
    consume(TOKEN_IDENTIFIER, "expected identifier after variable definition.");
    uint8_t global = writeConstant(compileChunk,
                                   OBJECT_CAST(makeObjString(parser.previous.start, parser.previous.length)));

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void identifier(bool allowAssignment) {
    uint8_t con = writeConstant(compileChunk,
                                OBJECT_CAST(makeObjString(parser.previous.start, parser.previous.length)));

    if (match(TOKEN_EQUAL) && allowAssignment) {
        expression();
        emitBytes(OP_SET_GLOBAL, con);
    } else {
        emitBytes(OP_GET_GLOBAL, con);
    }
}

static void definition() {
    if (match(TOKEN_VAR)) {
        varDefinition();
        consume(TOKEN_SEMICOLON, "expected ';' after variable declaration.");
    } else {
        statement();
    }

    if (parser.panicMode) skipToNextStatement();
}

static void unary(bool allowAssignment) {
    TokenType operatorType = parser.previous.type;
    parsePrecedence(PrecUnary);

    switch (operatorType) {
        case TOKEN_NOT:
            emitByte(OP_NOT);
            break;
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        case TOKEN_TILDE:
            emitByte(OP_BW_NOT);
        case TOKEN_PLUS:
            break;
        default:
            return;
    }
}

static void binary(bool allowAssignment) {
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence(rule->prec + 1);

    switch (operatorType) {
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE);
            break;
        case TOKEN_PERCENT:
            emitByte(OP_MODULO);
            break;
        case TOKEN_STAR_STAR:
            emitByte(OP_POW);
            break;
        case TOKEN_AND:
            emitByte(OP_AND);
            break;
        case TOKEN_OR:
            emitByte(OP_OR);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUALS);
            break;
        case TOKEN_BANG_EQUAL:
            emitBytes(OP_EQUALS, OP_NOT);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OP_GREATER, OP_NOT);
            break;
        case TOKEN_AMPERSAND:
            emitByte(OP_BW_AND);
            break;
        case TOKEN_PIPE:
            emitByte(OP_BW_OR);
            break;
        case TOKEN_CARET:
            emitByte(OP_XOR);
            break;
        case TOKEN_LESS_LESS:
            emitByte(OP_SHIFT_LEFT);
            break;
        case TOKEN_GREATER_GREATER:
            emitByte(OP_SHIFT_RIGHT);
            break;
        default:
            return;
    }
}

static void ternary(bool allowAssignment) {
    parsePrecedence(PrecTernary);
    consume(TOKEN_COLONS, "expected ':' after '?' operator.");
    parsePrecedence(PrecTernary);
    emitByte(OP_TERNARY);
}

static void grouping(bool allowAssignment) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "expected ')' after expression.");
}

static void number(bool allowAssignment) {
    if (parser.previous.type == TOKEN_INTEGER) {
        int64_t value = strtol(parser.previous.start, NULL, 10);
        emitConstant(INTEGER_CAST(value));
        return;
    } else {
        double value = strtod(parser.previous.start, NULL);
        emitConstant(FLOAT_CAST(value));
    }
}

static void escapeString(char *chars, int *length) {
#define escapeChar(r) chars[i] = r; memmove(chars + i + 1, chars + i + 2, *length - i - 1); (*length)--

    for (int i = 0; i < *length - 1; i++) {
        if (chars[i] == '\\') {
            switch (chars[i + 1]) {
                case 'n':
                escapeChar('\n');
                    break;
                case 't':
                escapeChar('\t');
                    break;
                case 'r':
                escapeChar('\r');
                    break;
                case 'a':
                escapeChar('\a');
                    break;
                case 'b':
                escapeChar('\b');
                    break;
                case 'v':
                escapeChar('\v');
                    break;
                case 'f':
                escapeChar('\f');
                    break;
                case '\\':
                escapeChar('\\');
                    break;
                case '\'':
                escapeChar('\'');
                    break;
                case '\"':
                escapeChar('\"');
                    break;
                case 'x':
                    if (*length - i < 4) {
                        errorAtPrevious("invalid escape sequence.");
                    }

                    char hex[3] = {chars[i + 2], chars[i + 3], '\0'};

                    char *end;
                    long hexValue = strtol(hex, &end, 16);

                    if (*end != '\0') {
                        errorAtPrevious("invalid escape sequence.");
                    }

                    chars[i] = (char) hexValue;
                    memmove(chars + i + 1, chars + i + 4, *length - i - 3);
                    (*length) -= 3;
                    break;
            }
        }
    }
#undef replaceChar
}

static void string(bool allowAssignment) {
    ObjString *string = makeObjString(parser.previous.start + 1, parser.previous.length - 2);

    escapeString(string->chars, &string->length);

    emitConstant(OBJECT_CAST(string));
}

static void boolean(bool allowAssignment) {
    if (parser.previous.type == TOKEN_TRUE) {
        emitByte(OP_TRUE);
    } else {
        emitByte(OP_FALSE);
    }
}

static void nil(bool allowAssignment) {
    emitByte(OP_NIL);
}

ParseRule parseRules[] = {
        [TOKEN_LEFT_PAREN]  =   {grouping, NULL, PrecNone},
        [TOKEN_RIGHT_PAREN] =   {NULL, NULL, PrecNone},
        [TOKEN_LEFT_BRACE]  =   {NULL, NULL, PrecNone},
        [TOKEN_RIGHT_BRACE] =   {NULL, NULL, PrecNone},
        [TOKEN_COMMA]       =   {NULL, NULL, PrecNone},
        [TOKEN_DOT]         =   {NULL, NULL, PrecCall},
        [TOKEN_MINUS]       =   {unary, binary, PrecTerm},
        [TOKEN_PLUS]        =   {unary, binary, PrecTerm},
        [TOKEN_SEMICOLON]   =   {NULL, NULL, PrecNone},
        [TOKEN_SLASH]       =   {NULL, binary, PrecFactor},
        [TOKEN_STAR]        =   {NULL, binary, PrecFactor},
        [TOKEN_PERCENT]     =   {NULL, binary, PrecFactor},
        [TOKEN_STAR_STAR]         =   {NULL, binary, PrecPow},
        [TOKEN_AND]         =   {NULL, binary, PrecAnd},
        [TOKEN_OR]          =   {NULL, binary, PrecOr},
        [TOKEN_NOT]        =   {unary, NULL, PrecUnary},
        [TOKEN_PRINT]       =   {NULL, NULL, PrecNone},
        [TOKEN_COLONS]      =   {NULL, NULL, PrecNone},
        [TOKEN_AMPERSAND]   =   {NULL, binary, PrecBWAnd},
        [TOKEN_PIPE]        =   {NULL, binary, PrecBWOr},
        [TOKEN_CARET]       =   {NULL, binary, PrecXor},
        [TOKEN_LESS_LESS]   =   {NULL, binary, PrecShift},
        [TOKEN_GREATER_GREATER]   =   {NULL, binary, PrecShift},
        [TOKEN_TILDE]       =   {unary, NULL, PrecUnary},
        [TOKEN_INTERROGATION] = {NULL, ternary, PrecTernary},
        [TOKEN_BANG_EQUAL]  =   {NULL, binary, PrecEquals},
        [TOKEN_EQUAL]       =   {NULL, NULL, PrecAssignment},
        [TOKEN_EQUAL_EQUAL] =   {NULL, binary, PrecEquals},
        [TOKEN_GREATER]     =   {NULL, binary, PrecCompare},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PrecCompare},
        [TOKEN_LESS]        =   {NULL, binary, PrecCompare},
        [TOKEN_LESS_EQUAL]  =   {NULL, binary, PrecCompare},
        [TOKEN_IDENTIFIER]  =   {identifier, NULL, PrecNone},
        [TOKEN_STRING]      =   {string, NULL, PrecNone},
        [TOKEN_INTEGER]      =   {number, NULL, PrecNone},
        [TOKEN_FLOAT]      =   {number, NULL, PrecNone},
        [TOKEN_RETURN]      =   {NULL, NULL, PrecNone},
        [TOKEN_IF]          =   {NULL, NULL, PrecNone},
        [TOKEN_ELSE]        =   {NULL, NULL, PrecNone},
        [TOKEN_FN]          =   {NULL, NULL, PrecNone},
        [TOKEN_VAR]         =   {NULL, NULL, PrecNone},
        [TOKEN_EOF]         =   {NULL, NULL, PrecNone},
        [TOKEN_TRUE]        =   {boolean, NULL, PrecNone},
        [TOKEN_FALSE]       =   {boolean, NULL, PrecNone},
        [TOKEN_NIL]         =   {nil, NULL, PrecNone},
        [TOKEN_ERROR]       =   {NULL, NULL, PrecNone}
};


static ParseRule *getRule(TokenType type) {
    return &parseRules[type];
}


bool compile(Chunk *chunk, const char *source) {
    initScanner(source);
    compileChunk = chunk;

    parser.hasError = false;
    parser.panicMode = false;

    advance();
    while (!match(TOKEN_EOF)) {
        definition();
    }

    emitByte(OP_RETURN);

    return !parser.hasError;
}