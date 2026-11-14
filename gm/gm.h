#ifndef GM_H
#define GM_H

#include <stddef.h>
#include <stdint.h>
#include "opcodes.h"

typedef union {
    int64_t  i;
    double   f;
    uint64_t u;
} Word;

typedef enum {
    VM_RUNNING = 0,
    VM_HALTED,
    VM_FAULT_BAD_OP,
    VM_FAULT_BAD_IP,
    VM_FAULT_OOB,
    VM_FAULT_DIV_ZERO,
    VM_FAULT_OVERFLOW,
} VmStatus;

typedef struct {
    Word          regs[REG_COUNT];
    const uint8_t *code;
    size_t         code_len;
    size_t         ip;
    VmStatus       status;
} Vm;

const char *vm_get_status(VmStatus s);
void vm_execute_step(Vm *vm);
uint8_t *load_program_from_file(Vm *vm, const char *input_file_path);
int execute_program(Vm *vm);

#endif // GM_H
