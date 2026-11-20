#ifndef GM_H
#define GM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "opcodes.h"
#include "nanbox.h"

typedef enum {
    VM_RUNNING = 0,
    VM_HALTED,
    VM_FAULT_BAD_OP,
    VM_FAULT_BAD_IP,
    VM_FAULT_OOB,
    VM_FAULT_DIV_ZERO,
    VM_FAULT_STACK_OVERFLOW,
    VM_FAULT_STACK_UNDERFLOW,
    VM_FAULT_BAD_FRAME,
    VM_FAULT_BAD_ADDR,
    VM_FAULT_NULL_REF,
    VM_FAULT_TYPE,
    VM_FAULT_OOM,
} VmStatus;

typedef struct {
    Word          regs[REG_COUNT];
    const uint8_t *code;
    size_t        code_len;

    uint8_t       *mem;
    size_t        mem_size;

    size_t        stack_base;
    size_t        stack_top;

    size_t        heap_from_start;
    size_t        heap_from_end;
    size_t        heap_to_start;
    size_t        heap_to_end;
    size_t        heap_alloc_offset;
    
    size_t        ip;
    VmStatus      status;
} Vm;

const char *vm_get_status(VmStatus s);
void vm_execute_step(Vm *vm);

#endif // GM_H
