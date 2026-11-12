#ifndef OPCODES_H
#define OPCODES_H

#include <stddef.h>
#define REG_COUNT 256
typedef enum {
    OP_NOP        = 0,
    OP_HALT       = 1,
    OP_LOADI      = 2,
    OP_LOADF      = 3,
    OP_MOV        = 4,
    OP_ADD        = 5,
    OP_SUB        = 6,
    OP_MUL        = 7,
    OP_DIV        = 8,
    OP_CMP_LT     = 9,
    OP_JMP        = 10,
    OP_JMP_IF     = 11,
    OP_JMP_IF_NOT = 12,
    OP_PRINT      = 13,
} Opcode;

typedef struct {
    const char* name;
    const char* sig;
    Opcode op;
} InstrInfo;

#define REG_SIZE 1
#define OPCODE_SIZE 1
#define INT_SIZE 4
#define FLOAT_SIZE 8
static InstrInfo instructions[] = {
    {"nop",    "",    OP_NOP},
    {"halt",   "",    OP_HALT},
    // `load` emits OP_LOADI or OP_LOADF depending on the literal type.
    {"load",   "rn",  OP_LOADI},
    {"mov",    "rr",  OP_MOV},
    {"add",    "rrr", OP_ADD},
    {"sub",    "rrr", OP_SUB},
    {"mul",    "rrr", OP_MUL},
    {"div",    "rrr", OP_DIV},
    {"cmp_lt", "rrr", OP_CMP_LT},
    {"jmp",    "l",   OP_JMP},
    {"jnz",    "rl",  OP_JMP_IF},
    {"jz",     "rl",  OP_JMP_IF_NOT},
    {"print",  "r",   OP_PRINT},
};

static const size_t instructions_count
    = sizeof(instructions) / sizeof(instructions[0]);

#endif // OPCODES_H
