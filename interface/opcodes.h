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
    OP_CALL       = 20,
    OP_RET        = 21,
    OP_ENTER      = 22,
    OP_LEAVE      = 23,
    OP_PUSH       = 24,
    OP_POP        = 25,
    OP_LOAD       = 26,
    OP_STORE      = 27,
    OP_NEW_ARRAY  = 28,
    OP_NEW_BYTES  = 29,
    OP_BYTE_GET   = 30,
    OP_BYTE_SET   = 31,
    OP_ARR_GET    = 32,
    OP_ARR_SET    = 33,
    OP_ARR_LEN    = 34,
    OP_GC_COLLECT = 35,
} Opcode;

typedef struct {
    const char* name;
    const char* sig;
    Opcode op;
} InstrInfo;

static InstrInfo instructions[] = {
    {"nop",        "",    OP_NOP},
    {"halt",       "",    OP_HALT},
    {"load_i",     "ri",  OP_LOAD_I},
    {"add_i",      "rrr", OP_ADD_I},
    {"sub_i",      "rrr", OP_SUB_I},
    {"mul_i",      "rrr", OP_MUL_I},
    {"div_i",      "rrr", OP_DIV_I},
    {"cmp_lt_i",   "rrr", OP_CMP_LT_I},
    {"print_i",    "r",   OP_PRINT_I},
    {"load_f",     "rf",  OP_LOAD_F},
    {"add_f",      "rrr", OP_ADD_F},
    {"sub_f",      "rrr", OP_SUB_F},
    {"mul_f",      "rrr", OP_MUL_F},
    {"div_f",      "rrr", OP_DIV_F},
    {"cmp_lt_f",   "rrr", OP_CMP_LT_F},
    {"print_f",    "r",   OP_PRINT_F},
    {"mov",        "rr",  OP_MOV},
    {"jmp",        "l",   OP_JMP},
    {"jnz",        "rl",  OP_JMP_IF},
    {"jz",         "rl",  OP_JMP_IF_NOT},
    {"call",       "l",   OP_CALL},
    {"ret",        "",    OP_RET},
    {"enter",      "i",   OP_ENTER},
    {"leave",      "",    OP_LEAVE},
    {"push",       "r",   OP_PUSH},
    {"pop",        "r",   OP_POP},
    {"load",       "rri", OP_LOAD},
    {"store",      "rir", OP_STORE},
    {"new_array",  "ri",  OP_NEW_ARRAY},
    {"new_bytes",  "ri",  OP_NEW_BYTES},
    {"byte_get",   "rrr", OP_BYTE_GET},
    {"byte_set",   "rrr", OP_BYTE_SET},
    {"gc_collect", "",    OP_GC_COLLECT},
    {"arr_get",    "rrr", OP_ARR_GET},
    {"arr_set",    "rrr", OP_ARR_SET},
    {"arr_len",    "rr",  OP_ARR_LEN},

};

static const size_t instructions_count
    = sizeof(instructions) / sizeof(instructions[0]);

#endif // OPCODES_H

