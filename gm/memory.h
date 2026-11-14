#ifndef MEMORY_H
#define MEMORY_H

#include "gm.h"

void mem_write_u64(Vm *vm, uint64_t addr, uint64_t value);
uint64_t mem_read_u64(Vm *vm, uint64_t addr);
bool stack_push(Vm *vm, Word v);
bool stack_pop(Vm *vm, Word *out);
bool stack_addr(const Vm *vm, uint64_t base, int64_t off, uint64_t *out);

#endif // MEMORY_H
