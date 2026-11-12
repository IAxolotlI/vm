#ifndef COMPILER_H
#define COMPILER_H

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include "opcodes.h"
#include "sv.h"

typedef enum {
    PARSE_OK = 0,
    PARSE_EMPTY_ARG,
    PARSE_TRAILING_COMMA,
    PARSE_MISSING_COMMA,
    PARSE_UNKNOWN_OP,
    PARSE_MANY_ARGUMENTS,
    PARSE_FEW_ARGUMENTS,
    PARSE_BAD_SIG,
    PARSE_NOT_REG,
    PARSE_BIG_REG,
    PARSE_NAN,
    PARSE_ERROR_EMIT,
    PARSE_LABEL_REDEF,
    PARSE_MISSING_LABEL,
    PARSE_BAD_LABEL,
} ParseStatus;

typedef struct {
    ParseStatus status;
    size_t err_col;
} ParseError;

typedef enum {
    OPERAND_REG,
    OPERAND_INT,
    OPERAND_FLOAT,
    OPERAND_LABEL,
} OperandKind;

typedef struct {
    OperandKind kind;
    size_t col;
    union {
        uint8_t reg;
        int64_t i;
        double f;
        String_View label;
    } as;
} Operand;

typedef struct {
    const InstrInfo *info;
    Operand *items;
    size_t count;
    size_t capacity;
    size_t col;
} Instr;

int emit_bytecode_from_program(const char *filename, FILE *input_file, FILE *output_file);

#endif // COMPILER_H
