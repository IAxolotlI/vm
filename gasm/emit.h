#ifndef EMIT_H
#define EMIT_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "opcodes.h"

static inline bool write_u8(FILE *out, uint8_t value) {
    return fputc(value, out) != EOF;
}

static inline bool write_u32(FILE *out, uint32_t value) {
    uint8_t bytes[4] = {
        (uint8_t)(value),
        (uint8_t)(value >> 8),
        (uint8_t)(value >> 16),
        (uint8_t)(value >> 24),
    };
    return fwrite(bytes, 1, sizeof(bytes), out) == sizeof(bytes);
}

static inline bool write_u64(FILE *out, uint64_t value) {
    uint8_t bytes[8];
    for (size_t i = 0; i < sizeof(bytes); i++) {
        bytes[i] = (uint8_t)(value >> (8 * i));
    }
    return fwrite(bytes, 1, sizeof(bytes), out) == sizeof(bytes);
}

static inline bool emit_load(FILE *out, const Instr *ins) {
    if (ins->count != 2) return false;

    const Operand *dst = &ins->items[0];
    const Operand *src = &ins->items[1];
    if (dst->kind != OPERAND_REG) return false;

    if (src->kind == OPERAND_INT) {
        if (src->as.i < INT32_MIN || src->as.i > INT32_MAX) return false;
        return write_u8(out, OP_LOADI)
            && write_u8(out, dst->as.reg)
            && write_u32(out, (uint32_t)(int32_t)src->as.i);
    }

    if (src->kind == OPERAND_FLOAT) {
        uint64_t bits;
        memcpy(&bits, &src->as.f, sizeof(bits));
        return write_u8(out, OP_LOADF)
            && write_u8(out, dst->as.reg)
            && write_u64(out, bits);
    }

    return false;
}

static inline bool emit_rrr(FILE *out, Opcode op, const Instr *ins) {
    if (ins->count != 3) return false;

    const Operand *dst = &ins->items[0];
    const Operand *a = &ins->items[1];
    const Operand *b = &ins->items[2];
    if (dst->kind != OPERAND_REG
        || a->kind != OPERAND_REG
        || b->kind != OPERAND_REG) {
        return false;
    }

    return write_u8(out, (uint8_t)op)
        && write_u8(out, dst->as.reg)
        && write_u8(out, a->as.reg)
        && write_u8(out, b->as.reg);
}

static inline bool emit_jmp(FILE *out, const Instr *ins) {
    if (ins->count != 1) return false;

    const Operand *off = &ins->items[0];
    if (off->kind != OPERAND_INT) return false;
    if (off->as.i < INT32_MIN || off->as.i > INT32_MAX) return false;

    return write_u8(out, OP_JMP)
        && write_u32(out, (uint32_t)(int32_t)off->as.i);
}

static inline bool emit_jmp_if(FILE *out, Opcode op, const Instr *ins) {
    if (ins->count != 2) return false;

    const Operand *reg = &ins->items[0];
    const Operand *off = &ins->items[1];
    if (reg->kind != OPERAND_REG || off->kind != OPERAND_INT) return false;
    if (off->as.i < INT32_MIN || off->as.i > INT32_MAX) return false;

    return write_u8(out, (uint8_t)op)
        && write_u8(out, reg->as.reg)
        && write_u32(out, (uint32_t)(int32_t)off->as.i);
}

static inline bool emit_mov(FILE *out, const Instr *ins) {
    if (ins->count != 2) return false;

    const Operand *dst = &ins->items[0];
    const Operand *src = &ins->items[1];
    if (dst->kind != OPERAND_REG || src->kind != OPERAND_REG) return false;

    return write_u8(out, OP_MOV)
        && write_u8(out, dst->as.reg)
        && write_u8(out, src->as.reg);
}

static inline bool emit_print(FILE *out, const Instr *ins) {
    if (ins->count != 1) return false;

    const Operand *src = &ins->items[0];
    if (src->kind != OPERAND_REG) return false;

    return write_u8(out, OP_PRINT)
        && write_u8(out, src->as.reg);
}

static inline bool emit_instr(FILE *out, const Instr *ins) {
    switch (ins->info->op) {
        case OP_NOP:
        case OP_HALT:
            return write_u8(out, (uint8_t)ins->info->op);

        case OP_LOADI:
        case OP_LOADF:
            return emit_load(out, ins);

        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_CMP_LT:
            return emit_rrr(out, ins->info->op, ins);

        case OP_JMP:
            return emit_jmp(out, ins);

        case OP_JMP_IF:
        case OP_JMP_IF_NOT:
            return emit_jmp_if(out, ins->info->op, ins);

        case OP_MOV:
            return emit_mov(out, ins);

        case OP_PRINT:
            return emit_print(out, ins);
    }

    return false;
}

#endif // EMIT_H
