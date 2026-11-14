#ifndef OPCODES_H
#define OPCODES_H

#include <stddef.h>
#define REG_COUNT 256

#define REG_SIZE    1
#define OPCODE_SIZE 1
#define INT_SIZE    8
#define FLOAT_SIZE  8
typedef enum {
    OP_NOP        = 0,
    OP_HALT       = 1,
    OP_LOAD_I     = 2,
    OP_ADD_I      = 3,
    OP_SUB_I      = 4,
    OP_MUL_I      = 5,
    OP_DIV_I      = 6,
    OP_CMP_LT_I   = 7,
    OP_PRINT_I    = 8,
    OP_ADD_F      = 9,
    OP_SUB_F      = 10,
    OP_MUL_F      = 11,
    OP_DIV_F      = 12,
    OP_CMP_LT_F   = 13,
    OP_PRINT_F    = 14,
    OP_LOAD_F     = 15,
    OP_MOV        = 16,
    OP_JMP        = 17,
    OP_JMP_IF     = 18,
    OP_JMP_IF_NOT = 19,
} Opcode;

typedef struct {
    const char* name;
    const char* sig;
    Opcode op;
} InstrInfo;

static InstrInfo instructions[] = {
    {"nop",      "",    OP_NOP},
    {"halt",     "",    OP_HALT},

    {"load_i",   "ri",  OP_LOAD_I},
    {"add_i",    "rrr", OP_ADD_I},
    {"sub_i",    "rrr", OP_SUB_I},
    {"mul_i",    "rrr", OP_MUL_I},
    {"div_i",    "rrr", OP_DIV_I},
    {"cmp_lt_i", "rrr", OP_CMP_LT_I},
    {"print_i",  "r",   OP_PRINT_I},
    {"load_f",   "rf",  OP_LOAD_F},
    {"add_f",    "rrr", OP_ADD_F},
    {"sub_f",    "rrr", OP_SUB_F},
    {"mul_f",    "rrr", OP_MUL_F},
    {"div_f",    "rrr", OP_DIV_F},
    {"cmp_lt_f", "rrr", OP_CMP_LT_F},
    {"print_f",  "r",   OP_PRINT_F},

    {"mov",      "rr",  OP_MOV},
    {"jmp",      "l",   OP_JMP},
    {"jnz",      "rl",  OP_JMP_IF},
    {"jz",       "rl",  OP_JMP_IF_NOT},
};

static const size_t instructions_count
    = sizeof(instructions) / sizeof(instructions[0]);

#endif // OPCODES_H

