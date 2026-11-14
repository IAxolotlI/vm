#include <stdio.h>
#include "logger.h"
#include "abi.h"
#include "memory.h"

void mem_write_u64(Vm *vm, uint64_t addr, uint64_t value) {
    for (size_t i = 0; i < 8; i++) {
        vm->mem[addr + i] = (uint8_t)(value >> (8 * i));
    }
}

uint64_t mem_read_u64(Vm *vm, uint64_t addr) {
    uint64_t value = 0;
    for (size_t i = 0; i < 8; i++) {
        value |= (uint64_t)vm->mem[addr + i] << (8 * i);
    }
    return value;
}

bool stack_push(Vm *vm, Word v) {
    uint64_t sp = vm->regs[RSP].u;
    if (sp > vm->stack_top) {
        LOG_ERROR("Stack pointer is greater than stack top");
        vm->status = VM_FAULT_BAD_FRAME;
        return false;
    }
    if (sp % GM_STACK_ALIGN != 0) {
        LOG_ERROR("Stack pointer isn't aligned by %d bytes", GM_STACK_ALIGN);
        vm->status = VM_FAULT_BAD_ADDR;
        return false;
    }
    if (sp < vm->stack_base + GM_STACK_ALIGN) {
        vm->status = VM_FAULT_STACK_OVERFLOW;
        return false;
    }
    sp -= GM_STACK_ALIGN;
    mem_write_u64(vm, sp, v.u);
    vm->regs[RSP].u = sp;
    return true;
}

bool stack_pop(Vm *vm, Word *out) {
    uint64_t sp = vm->regs[RSP].u;
    if (sp > vm->stack_top) {
        LOG_ERROR("Stack pointer is greater than stack top");
        vm->status = VM_FAULT_BAD_FRAME;
        return false;
    }
    if (sp % GM_STACK_ALIGN != 0) {
        LOG_ERROR("Stack pointer isn't aligned by %d bytes", GM_STACK_ALIGN);
        vm->status = VM_FAULT_BAD_ADDR;
        return false;
    }
    if (sp < vm->stack_base || sp > vm->stack_top - GM_STACK_ALIGN) {
        vm->status = VM_FAULT_STACK_UNDERFLOW;
        return false;
    }
    out->u = mem_read_u64(vm, sp);
    vm->regs[RSP].u = sp + GM_STACK_ALIGN;
    return true;
}

bool stack_addr(const Vm *vm, uint64_t base, int64_t off, uint64_t *out) {
    uint64_t delta;
    uint64_t addr;
    if (off >= 0) {
        delta = (uint64_t)off;
        if (base > UINT64_MAX - delta) {
            LOG_ERROR("Offset too big");
            return false;
        }
        addr = base + delta;
    }
    else {
        delta = (uint64_t)(-(off + 1)) + 1;
        if (base < delta) {
            LOG_ERROR("Offset too big");
            return false;
        }
        addr = base - delta;
    }
    if (addr < vm->stack_base || addr > vm->stack_top) {
        LOG_ERROR("Address is not in range");
        return false;
    }
    if (vm->stack_top - addr < GM_STACK_ALIGN) {
        LOG_ERROR("Address is not in range");
        return false;
    }
    if (addr % GM_STACK_ALIGN != 0) {
        LOG_ERROR("Address isn't aligned by %d bytes", GM_STACK_ALIGN);
        return false;
    }
    *out = addr;
    return true;
}
