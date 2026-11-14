#ifndef EMIT_H
#define EMIT_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "opcodes.h"

static inline bool write_u8(FILE *out, uint8_t value) {
    return fputc(value, out) != EOF;
}

static inline bool write_u64(FILE *out, uint64_t value) {
    uint8_t bytes[8];
    for (size_t i = 0; i < sizeof(bytes); i++) {
        bytes[i] = (uint8_t)(value >> (8 * i));
    }
    return fwrite(bytes, 1, sizeof(bytes), out) == sizeof(bytes);
}

static inline bool emit_operand(FILE *out, char sig, const Operand *op) {
    switch (sig) {
        case 'r':
            if (op->kind != OPERAND_REG) return false;
            return write_u8(out, op->as.reg);

        case 'i':
            if (op->kind != OPERAND_INT) return false;
            return write_u64(out, (uint64_t)op->as.i);

        case 'f': {
            if (op->kind != OPERAND_FLOAT) return false;
            uint64_t bits;
            memcpy(&bits, &op->as.f, sizeof(bits));
            return write_u64(out, bits);
        }

        case 'l':
            if (op->kind != OPERAND_INT) return false;
            return write_u64(out, (uint64_t)op->as.i);

        default:
            return false;
    }
}

static inline bool emit_instr(FILE *out, const Instr *ins) {
    if (!ins->info) return false;

    const char *sig = ins->info->sig;
    if (ins->count != strlen(sig)) return false;

    if (!write_u8(out, (uint8_t)ins->info->op)) return false;

    for (size_t i = 0; i < ins->count; i++) {
        if (!emit_operand(out, sig[i], &ins->items[i])) return false;
    }

    return true;
}

#endif // EMIT_H
