#ifndef GM_H
#define GM_H

#include <stddef.h>
#include <stdint.h>
#include "opcodes.h"

typedef enum {
    TAG_INT    = 0,
    TAG_DOUBLE = 1,
    TAG_NIL    = 2,
} ValueTag;

typedef struct {
    ValueTag tag;
    union {
        int64_t i;
        double  f;
    } as;
} Value;

typedef enum {
    VM_RUNNING = 0,
    VM_HALTED,
    VM_FAULT_BAD_OP,
    VM_FAULT_BAD_IP,
    VM_FAULT_OOB,
    VM_FAULT_DIV_ZERO,
    VM_FAULT_OVERFLOW,
    VM_FAULT_TYPE,
} VmStatus;

typedef struct {
    Value          regs[REG_COUNT];
    const uint8_t *code;
    size_t         code_len;
    size_t         ip;
    VmStatus       status;
} Vm;

const char *vm_get_status(VmStatus s);
void vm_execute_step(Vm *vm);
void print_value(Value v);
uint8_t *load_program_from_file(Vm *vm, const char *input_file_path);
int execute_program(Vm *vm);

#endif // GM_H
