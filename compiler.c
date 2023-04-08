#include <stdlib.h>
#include <stdio.h>
#include "compiler.h"
#include "scanner.h"

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
    PrecEquals,     // == !=
    PrecCompare,    // < > <= >=
    PrecTerm,       // + -
    PrecFactor,     // * / %
    PrecPow,        // ^
    PrecUnary,      // - !
    PrecCall        // . ()
} ParsePrec;

typedef void (*ParseFn)();

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
        fprintf(stderr, ", unexpected token");
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

static void consume(TokenType expectedType, const char *errorMessage) {
    if (parser.current.type == expectedType) {
        advance();
    } else {
        errorAtCurrent(errorMessage);
    }
}

static ParseRule *getRule(TokenType type);


static void parsePrecedence(ParsePrec precedence) {
    advance();
    ParseFn prefix = getRule(parser.previous.type)->prefix;

    if (prefix == NULL) {
        errorAtPrevious("expected expression.");
        return;
    }

    prefix();

    ParseRule *rule;
    while (rule = getRule(parser.current.type), precedence <= rule->prec) {
        ParseFn infix = rule->infix;
        advance();
        infix();
    }
}

static void expression() {
    parsePrecedence(PrecAssignment);
}

static void print() {
    parsePrecedence(PrecNone + 1);
    emitByte(OP_PRINT);
}

static void unary() {
    TokenType operatorType = parser.previous.type;
    parsePrecedence(PrecUnary);

    switch (operatorType) {
        case TOKEN_NOT:
            emitByte(OP_NOT);
            break;
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        case TOKEN_PLUS:
            break;
        default:
            return;
    }
}

static void binary() {
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
        case TOKEN_POW:
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
        default:
            return;
    }
}

static void ternary() {
    parsePrecedence(PrecTernary);
    consume(TOKEN_COLONS, "expected ':' after '?' operator.");
    parsePrecedence(PrecTernary);
    emitByte(OP_TERNARY);
}

static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "expected ')' after expression.");
}

static void number() {
    if (parser.previous.type == TOKEN_INTEGER) {
        int64_t value = strtol(parser.previous.start, NULL, 10);
        emitConstant(INTEGER_CAST(value));
        return;
    } else {
        double value = strtod(parser.previous.start, NULL);
        emitConstant(FLOAT_CAST(value));
    }
}

static void boolean() {
    if (parser.previous.type == TOKEN_TRUE) {
        emitByte(OP_TRUE);
    } else {
        emitByte(OP_FALSE);
    }
}

static void nil() {
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
        [TOKEN_POW]         =   {NULL, binary, PrecPow},
        [TOKEN_AND]         =   {NULL, binary, PrecAnd},
        [TOKEN_OR]          =   {NULL, binary, PrecOr},
        [TOKEN_NOT]        =   {unary, NULL, PrecUnary},
        [TOKEN_PRINT]       =   {print, NULL, PrecNone},
        [TOKEN_COLONS]      =   {NULL, NULL, PrecNone},
        [TOKEN_INTERROGATION] = {NULL, ternary, PrecTernary},
        [TOKEN_BANG_EQUAL]  =   {NULL, binary, PrecEquals},
        [TOKEN_EQUAL]       =   {NULL, NULL, PrecAssignment},
        [TOKEN_EQUAL_EQUAL] =   {NULL, binary, PrecEquals},
        [TOKEN_GREATER]     =   {NULL, binary, PrecCompare},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PrecCompare},
        [TOKEN_LESS]        =   {NULL, binary, PrecCompare},
        [TOKEN_LESS_EQUAL]  =   {NULL, binary, PrecCompare},
        [TOKEN_IDENTIFIER]  =   {NULL, NULL, PrecNone},
        [TOKEN_STRING]      =   {NULL, NULL, PrecNone},
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
    expression();
    consume(TOKEN_SEMICOLON, "expected ';' after expression.");

    return !parser.hasError;
}