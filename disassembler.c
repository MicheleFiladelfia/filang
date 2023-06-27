#include <stdio.h>
#include "disassembler.h"

void disassemble(Chunk *chunk) {
    for (int i = 0; i < chunk->count; i++) {
        switch (chunk->code[i]) {
            case OP_RETURN:
                printf("OP_RETURN\n");
                break;
            case OP_CONSTANT:
                printf("OP_CONSTANT\n");
                i++;
                break;
            case OP_CONSTANT_LONG:
                printf("OP_CONSTANT_LONG\n");
                i += 2;
                break;
            case OP_CONSTANT_LONG_LONG:
                printf("OP_CONSTANT_LONG_LONG\n");
                i += 3;
                break;
            case OP_NEGATE:
                printf("OP_NEGATE\n");
                break;
            case OP_ADD:
                printf("OP_ADD\n");
                break;
            case OP_SUBTRACT:
                printf("OP_SUBTRACT\n");
                break;
            case OP_MULTIPLY:
                printf("OP_MULTIPLY\n");
                break;
            case OP_DIVIDE:
                printf("OP_DIVIDE\n");
                break;
            case OP_NIL:
                printf("OP_NIL\n");
                break;
            case OP_TRUE:
                printf("OP_TRUE\n");
                break;
            case OP_FALSE:
                printf("OP_FALSE\n");
                break;
            case OP_NOT:
                printf("OP_NOT\n");
                break;
            case OP_EQUALS:
                printf("OP_EQUAL\n");
                break;
            case OP_GREATER:
                printf("OP_GREATER\n");
                break;
            case OP_LESS:
                printf("OP_LESS\n");
                break;
            case OP_PRINT:
                printf("OP_PRINT\n");
                break;
            case OP_POP:
                printf("OP_POP\n");
                break;
            case OP_DEFINE_GLOBAL:
                printf("OP_DEFINE_GLOBAL\n");
                break;
            case OP_GET_GLOBAL:
                printf("OP_GET_GLOBAL\n");
                break;
            case OP_SET_GLOBAL:
                printf("OP_SET_GLOBAL\n");
                break;
            case OP_GET_LOCAL:
                printf("OP_GET_LOCAL\n");
                break;
            case OP_SET_LOCAL:
                printf("OP_SET_LOCAL\n");
                break;
            case OP_CLOCK:
                printf("OP_CLOCK\n");
                break;
            case OP_TYPEOF:
                printf("OP_TYPEOF\n");
                break;
            case OP_MODULO:
                printf("OP_MODULO\n");
                break;
            case OP_POW:
                printf("OP_POW\n");
                break;
            case OP_AND:
                printf("OP_AND\n");
                break;
            case OP_OR:
                printf("OP_OR\n");
                break;
            case OP_BW_AND:
                printf("OP_BW_AND\n");
                break;
            case OP_BW_OR:
                printf("OP_BW_OR\n");
                break;
            case OP_XOR:
                printf("OP_XOR\n");
                break;
            case OP_BW_NOT:
                printf("OP_BW_NOT\n");
                break;
            case OP_SHIFT_LEFT:
                printf("OP_SHIFT_LEFT\n");
                break;
            case OP_SHIFT_RIGHT:
                printf("OP_SHIFT_RIGHT\n");
                break;
            case OP_TERNARY:
                printf("OP_TERNARY\n");
                break;
            case OP_ERROR:
                printf("OP_ERROR\n");
                break;
            case OP_JUMP:
                printf("OP_JUMP\n");
                i += 2;
                break;
            case OP_JUMP_IF_FALSE:
                printf("OP_JUMP_IF_FALSE\n");
                i += 2;
                break;
            default:
                printf("Unknown opcode %d\n", chunk->code[i]);
        }
    }
}