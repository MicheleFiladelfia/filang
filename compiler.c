#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "compiler.h"
#include "memory.h"
#include "scanner.h"
#include "strings.h"

#define MAX_SCOPE_DEPTH 512

typedef struct {
    Token previous;
    Token current;
    bool has_error;
    bool panic_mode;
} Parser;

struct {
    Token *variables;
    size_t count;
    size_t capacity;
    size_t current_depth;
    size_t locals_in_scope[MAX_SCOPE_DEPTH];
} locals;

Parser parser;
Chunk *compile_chunk;

typedef enum {
    PREC_NONE,          // None
    PREC_ASSIGNMENT,    // =
    PREC_TERNARY,       // ? :
    PREC_OR,            // or
    PREC_AND,           // and
    PREC_BW_OR,         // |
    PREC_BW_XOR,        // ^
    PREC_BW_AND,        // &
    PREC_EQUALS,        // == !=
    PREC_COMPARE,       // < > <= >=
    PREC_BW_SHIFT,      // << >>
    PREC_TERM,          // + -
    PREC_FACTOR,        // * / %
    PREC_UNARY,         // - ! ~
    PREC_POW,           // **
    PREC_CALL           // . ()
} ParsePrec;

typedef void (*parse_fn)(bool assignable);

typedef struct {
    parse_fn prefix;
    parse_fn infix;
    ParsePrec prec;
} ParseRule;

void init_compiler(Chunk *chunk) {
    parser.has_error = false;
    parser.panic_mode = false;
    compile_chunk = chunk;
    locals.count = 0;
    locals.capacity = 0;
    locals.variables = NULL;
    locals.current_depth = 0;
    memset(locals.locals_in_scope, 0, sizeof(int) * 255);
}

static size_t define_local(Token name, size_t depth) {
    if (locals.count + 1 >= locals.capacity) {
        size_t old_capacity = locals.capacity;
        locals.capacity = GROW_ARRAY_CAPACITY(old_capacity);
        locals.variables = GROW_ARRAY(locals.variables, Token, old_capacity, locals.capacity);

        if (locals.variables == NULL) {
            fprintf(stderr, "Failed to allocate memory for local variables\n");
            exit(1);
        }
    }
    locals.variables[locals.count] = name;
    locals.locals_in_scope[depth]++;
    return locals.count++;
}

static size_t get_local_index(Token name) {
    for (int i = (int) locals.count - 1; i >= 0; i--) {
        if (strncmp(name.start, locals.variables[i].start, name.length) == 0) {
            return i;
        }
    }

    return -1;
}

static bool local_exists_in_cur_scope(Token local) {
    for (size_t i = locals.count - locals.locals_in_scope[locals.current_depth]; i < locals.count; i++) {
        if (strncmp(locals.variables[i].start, local.start, locals.variables[i].length) == 0) {
            return true;
        }
    }

    return false;
}

static void compile_error(Token *token, const char *message) {
    if (parser.panic_mode) return;
    parser.panic_mode = true;
    parser.has_error = true;

    fprintf(stderr, "[line %d] CompileError", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
}

static void error_at_current(const char *message) {
    compile_error(&parser.current, message);
}

static void error_at_previous(const char *message) {
    compile_error(&parser.previous, message);
}

static void emit_byte(uint8_t byte) {
    write_chunk(compile_chunk, byte, parser.previous.line);
}

static void emit_bytes(int count, ...) {
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        emit_byte(va_arg(args, int));
    }
    va_end(args);
}

static void emit_constant(Value value) {
    int index = write_constant(compile_chunk, value);

    if (index < 255) {
        emit_bytes(2, OP_CONSTANT, index);
    } else if (index < 65535) {
        emit_bytes(3, OP_CONSTANT_LONG, index & 0xFF, (index >> 8) & 0xFF);
    } else if (index < 16777215) {
        emit_bytes(4, OP_CONSTANT_LONG_LONG, index & 0xFF, (index >> 8) & 0xFF, (index >> 16) & 0xFF);
    } else {
        error_at_previous("Too many constants in one chunk.");
        exit(1);
    }
}

static void advance() {
    parser.previous = parser.current;

    while (true) {
        parser.current = scan_token();
        if (parser.current.type != TOKEN_ERROR) break;
        error_at_current(parser.current.start);
    }
}

static void skip_to_next_statement() {
    parser.panic_mode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;

        switch (parser.current.type) {
            case TOKEN_COLONS:
            case TOKEN_PRINT:
            case TOKEN_LEFT_BRACE:
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
        error_at_current(errorMessage);
    }
}

static bool match(TokenType expectedType) {
    if (parser.current.type == expectedType) {
        advance();
        return true;
    }
    return false;
}


static ParseRule *get_rule(TokenType type);


static void parse_expression(ParsePrec precedence) {
    advance();
    parse_fn prefix = get_rule(parser.previous.type)->prefix;

    if (prefix == NULL) {
        error_at_previous("expected expression.");
        return;
    }

    bool assignable = precedence <= PREC_ASSIGNMENT;
    prefix(assignable);

    ParseRule *rule;
    while (rule = get_rule(parser.current.type), precedence <= rule->prec) {
        parse_fn infix = rule->infix;
        advance();
        infix(assignable);
    }

    if (assignable && match(TOKEN_EQUAL)) {
        error_at_previous("Invalid assignment target.");
    }
}


static void expression() {
    parse_expression(PREC_ASSIGNMENT);
}

static void print() {
    parse_expression(PREC_NONE + 1);
    emit_byte(OP_PRINT);
}

static void end_scope() {
    locals.count -= locals.locals_in_scope[locals.current_depth];
    locals.locals_in_scope[locals.current_depth] = 0;
    locals.current_depth--;
}

static void start_scope() {
    locals.current_depth++;
}

static void definition();

static void block() {
    start_scope();

    if (locals.current_depth >= MAX_SCOPE_DEPTH) {
        error_at_current("too many nested blocks.");
        exit(1);
    }
    while (true) {
        if (match(TOKEN_RIGHT_BRACE)) {
            break;
        }

        if (match(TOKEN_EOF)) {
            error_at_current("Expected '}' at the end of block.");
            break;
        }

        definition();
    }

    end_scope();
}

static int emit_jump(uint8_t jump) {
    emit_byte(jump);
    emit_bytes(2, OP_ERROR, OP_ERROR); // placeholder for jump offset
    return compile_chunk->count - 2;
}

static void fix_jump_index(int jump_index) {
    int offset = compile_chunk->count - jump_index - 2;

    if (offset > 65535) {
        error_at_current("jump then_offset too large.");
        exit(1);
    }

    compile_chunk->code[jump_index] = offset & 0xFF;
    compile_chunk->code[jump_index + 1] = (offset >> 8) & 0xFF;
}

static void statement();

static void if_statement() {
    consume(TOKEN_LEFT_PAREN, "expected '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "expected ')' after condition.");

    int jump_then_index = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);

    if (!match(TOKEN_LEFT_BRACE)) {
        error_at_current("expected '{' after condition.");
        exit(1);
    }

    block();

    int jump_end_else_index = emit_jump(OP_JUMP);

    fix_jump_index(jump_then_index);

    emit_byte(OP_POP);

    if (match(TOKEN_COLONS)) {
        if (!match(TOKEN_LEFT_BRACE)) {
            error_at_current("expected '{' after ':'.");
            exit(1);
        }
        block();
    }

    fix_jump_index(jump_end_else_index);
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        print();
        consume(TOKEN_SEMICOLON, "expected ';' after print statement.");
    } else if (match(TOKEN_LEFT_BRACE)) {
        block();
    } else if (match(TOKEN_INTERROGATION)) {
        if_statement();
    } else {
        expression();
        emit_byte(OP_POP);
        consume(TOKEN_SEMICOLON, "expected ';' after expression.");
    }
}

static void emit_local(size_t idx) {
    if (idx < 255) {
        emit_bytes(2, OP_CONSTANT, idx);
    } else if (idx < 65535) {
        emit_bytes(3, OP_CONSTANT_LONG, idx & 0xFF, (idx >> 8) & 0xFF);
    } else if (idx < 16777215) {
        emit_bytes(4, OP_CONSTANT_LONG_LONG, idx & 0xFF, (idx >> 8) & 0xFF, (idx >> 16) & 0xFF);
    } else {
        error_at_previous("too many constants in one chunk.");
        exit(1);
    }
}

static void var_definition() {
    consume(TOKEN_IDENTIFIER, "expected identifier after variable definition.");

    Token prev = parser.previous;

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emit_byte(OP_NIL);
    }

    if (locals.current_depth == 0) {
        emit_byte(OP_DEFINE_GLOBAL);
        emit_constant(NEW_OBJECT(make_objstring(prev.start, prev.length)));
    } else {
        if (local_exists_in_cur_scope(prev)) {
            compile_error(&prev, "variable with this name already defined in this scope.");
            exit(1);
        }
        emit_byte(OP_SET_LOCAL);
        size_t local_idx = define_local(prev, locals.current_depth);
        emit_local(local_idx);
    }

}

static void identifier(bool assignable) {
    Value name = NEW_OBJECT(make_objstring(parser.previous.start, parser.previous.length));
    Token prev = parser.previous;

    if (match(TOKEN_EQUAL) && assignable) {
        expression();
        size_t local_idx = get_local_index(prev);
        if (local_idx != -1) {
            emit_byte(OP_SET_LOCAL);
            emit_local(local_idx);
        } else {
            emit_byte(OP_SET_GLOBAL);
            emit_constant(name);
        }
    } else {
        size_t local_idx = get_local_index(prev);
        if (local_idx != -1) {
            emit_byte(OP_GET_LOCAL);
            emit_local(local_idx);
        } else {
            emit_byte(OP_GET_GLOBAL);
            emit_constant(name);
        }

    }
}

static void definition() {
    if (match(TOKEN_COLONS)) {
        var_definition();
        consume(TOKEN_SEMICOLON, "expected ';' after variable declaration.");
    } else {
        statement();
    }

    if (parser.panic_mode) skip_to_next_statement();
}

static void unary(bool assignable) {
    TokenType operator_type = parser.previous.type;
    parse_expression(PREC_UNARY);

    switch (operator_type) {
        case TOKEN_NOT:
            emit_byte(OP_NOT);
            break;
        case TOKEN_MINUS:
            emit_byte(OP_NEGATE);
            break;
        case TOKEN_TILDE:
            emit_byte(OP_BW_NOT);
        case TOKEN_PLUS:
            break;
        default:
            return;
    }
}

static void binary(bool assignable) {
    TokenType operator_type = parser.previous.type;
    ParseRule *rule = get_rule(operator_type);
    parse_expression(rule->prec + 1);

    switch (operator_type) {
        case TOKEN_PLUS:
            emit_byte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emit_byte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emit_byte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emit_byte(OP_DIVIDE);
            break;
        case TOKEN_PERCENT:
            emit_byte(OP_MODULO);
            break;
        case TOKEN_STAR_STAR:
            emit_byte(OP_POW);
            break;
        case TOKEN_AND:
            emit_byte(OP_AND);
            break;
        case TOKEN_OR:
            emit_byte(OP_OR);
            break;
        case TOKEN_EQUAL_EQUAL:
            emit_byte(OP_EQUALS);
            break;
        case TOKEN_BANG_EQUAL:
            emit_bytes(2, OP_EQUALS, OP_NOT);
            break;
        case TOKEN_GREATER:
            emit_byte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emit_bytes(2, OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emit_byte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emit_bytes(2, OP_GREATER, OP_NOT);
            break;
        case TOKEN_AMPERSAND:
            emit_byte(OP_BW_AND);
            break;
        case TOKEN_PIPE:
            emit_byte(OP_BW_OR);
            break;
        case TOKEN_CARET:
            emit_byte(OP_XOR);
            break;
        case TOKEN_LESS_LESS:
            emit_byte(OP_SHIFT_LEFT);
            break;
        case TOKEN_GREATER_GREATER:
            emit_byte(OP_SHIFT_RIGHT);
            break;
        default:
            return;
    }
}

static void ternary(bool assignable) {
    parse_expression(PREC_TERNARY);
    consume(TOKEN_COLONS, "expected ':' after '?' operator.");
    parse_expression(PREC_TERNARY);
    emit_byte(OP_TERNARY);
}

static void grouping(bool assignable) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "expected ')' after expression.");
}

static void number(bool assignable) {
    if (parser.previous.type == TOKEN_INTEGER) {
        int64_t value = strtol(parser.previous.start, NULL, 10);
        emit_constant(NEW_INTEGER(value));
        return;
    } else {
        double value = strtod(parser.previous.start, NULL);
        emit_constant(NEW_DECIMAL(value));
    }
}

static void escape_string(char *chars, int *length) {
#define ESCAPE_CHAR(r) chars[i] = r; memmove(chars + i + 1, chars + i + 2, *length - i - 1); (*length)--

    for (int i = 0; i < *length - 1; i++) {
        if (chars[i] == '\\') {
            switch (chars[i + 1]) {
                case 'n':
                ESCAPE_CHAR('\n');
                    break;
                case 't':
                ESCAPE_CHAR('\t');
                    break;
                case 'r':
                ESCAPE_CHAR('\r');
                    break;
                case 'a':
                ESCAPE_CHAR('\a');
                    break;
                case 'b':
                ESCAPE_CHAR('\b');
                    break;
                case 'v':
                ESCAPE_CHAR('\v');
                    break;
                case 'f':
                ESCAPE_CHAR('\f');
                    break;
                case '\\':
                ESCAPE_CHAR('\\');
                    break;
                case '\'':
                ESCAPE_CHAR('\'');
                    break;
                case '\"':
                ESCAPE_CHAR('\"');
                    break;
                case 'x':
                    if (*length - i < 4) {
                        error_at_previous("invalid escape sequence.");
                    }

                    char hex[3] = {chars[i + 2], chars[i + 3], '\0'};

                    char *end;
                    long hex_value = strtol(hex, &end, 16);

                    if (*end != '\0') {
                        error_at_previous("invalid escape sequence.");
                    }

                    chars[i] = (char) hex_value;
                    memmove(chars + i + 1, chars + i + 4, *length - i - 3);
                    (*length) -= 3;
                    break;
            }
        }
    }
#undef ESCAPE_CHAR
}

static void string(bool assignable) {
    ObjString *string = make_objstring(parser.previous.start + 1, parser.previous.length - 2);

    escape_string(string->chars, &string->length);

    emit_constant(NEW_OBJECT(string));
}

static void clock(bool assignable) {
    emit_byte(OP_CLOCK);
}

static void type_of(bool assignable) {
    parse_expression(PREC_NONE + 1);
    emit_byte(OP_TYPEOF);
}

static void boolean(bool assignable) {
    if (parser.previous.type == TOKEN_TRUE) {
        emit_byte(OP_TRUE);
    } else {
        emit_byte(OP_FALSE);
    }
}

static void nil(bool assignable) {
    emit_byte(OP_NIL);
}

ParseRule parse_rules[] = {
        [TOKEN_LEFT_PAREN]  =   {grouping, NULL, PREC_NONE},
        [TOKEN_RIGHT_PAREN] =   {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE]  =   {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE] =   {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA]       =   {NULL, NULL, PREC_NONE},
        [TOKEN_DOT]         =   {NULL, NULL, PREC_CALL},
        [TOKEN_MINUS]       =   {unary, binary, PREC_TERM},
        [TOKEN_PLUS]        =   {unary, binary, PREC_TERM},
        [TOKEN_SEMICOLON]   =   {NULL, NULL, PREC_NONE},
        [TOKEN_SLASH]       =   {NULL, binary, PREC_FACTOR},
        [TOKEN_STAR]        =   {NULL, binary, PREC_FACTOR},
        [TOKEN_PERCENT]     =   {NULL, binary, PREC_FACTOR},
        [TOKEN_STAR_STAR]         =   {NULL, binary, PREC_POW},
        [TOKEN_AND]         =   {NULL, binary, PREC_AND},
        [TOKEN_OR]          =   {NULL, binary, PREC_OR},
        [TOKEN_NOT]        =   {unary, NULL, PREC_UNARY},
        [TOKEN_PRINT]       =   {NULL, NULL, PREC_NONE},
        [TOKEN_COLONS]      =   {NULL, NULL, PREC_NONE},
        [TOKEN_AMPERSAND]   =   {NULL, binary, PREC_BW_AND},
        [TOKEN_PIPE]        =   {NULL, binary, PREC_BW_OR},
        [TOKEN_CARET]       =   {NULL, binary, PREC_BW_XOR},
        [TOKEN_LESS_LESS]   =   {NULL, binary, PREC_BW_SHIFT},
        [TOKEN_GREATER_GREATER]   =   {NULL, binary, PREC_BW_SHIFT},
        [TOKEN_TILDE]       =   {unary, NULL, PREC_UNARY},
        [TOKEN_INTERROGATION] = {NULL, ternary, PREC_TERNARY},
        [TOKEN_BANG_EQUAL]  =   {NULL, binary, PREC_EQUALS},
        [TOKEN_EQUAL]       =   {NULL, NULL, PREC_ASSIGNMENT},
        [TOKEN_EQUAL_EQUAL] =   {NULL, binary, PREC_EQUALS},
        [TOKEN_GREATER]     =   {NULL, binary, PREC_COMPARE},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARE},
        [TOKEN_LESS]        =   {NULL, binary, PREC_COMPARE},
        [TOKEN_LESS_EQUAL]  =   {NULL, binary, PREC_COMPARE},
        [TOKEN_IDENTIFIER]  =   {identifier, NULL, PREC_NONE},
        [TOKEN_STRING]      =   {string, NULL, PREC_NONE},
        [TOKEN_INTEGER]      =   {number, NULL, PREC_NONE},
        [TOKEN_FLOAT]      =   {number, NULL, PREC_NONE},
        [TOKEN_RETURN]      =   {NULL, NULL, PREC_NONE},
        [TOKEN_IF]          =   {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE]        =   {NULL, NULL, PREC_NONE},
        [TOKEN_EOF]         =   {NULL, NULL, PREC_NONE},
        [TOKEN_TRUE]        =   {boolean, NULL, PREC_NONE},
        [TOKEN_FALSE]       =   {boolean, NULL, PREC_NONE},
        [TOKEN_NIL]         =   {nil, NULL, PREC_NONE},
        [TOKEN_ERROR]       =   {NULL, NULL, PREC_NONE},
        [TOKEN_CLOCK]      =   {clock, NULL, PREC_NONE},
        [TOKEN_TYPEOF]    =   {type_of, NULL, PREC_NONE},
};


static ParseRule *get_rule(TokenType type) {
    return &parse_rules[type];
}


bool compile(Chunk *chunk, const char *source) {
    init_scanner(source);
    init_compiler(chunk);

    advance();
    while (!match(TOKEN_EOF)) {
        definition();
    }

    emit_byte(OP_RETURN);

    return !parser.has_error;
}