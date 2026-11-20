#ifndef HEAP_H
#define HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "gm.h"

#define HEADER_SIZE 8
#define GM_NULL     0x0000FFFFFFFFFFFFULL

typedef enum {
    OBJ_BYTES = 0,
    OBJ_ARRAY = 1,
} ObjType;

typedef struct {
    uint32_t size;
    uint8_t  type;
    uint8_t  is_forwarded;
} ObjHeader;

uint64_t heap_alloc(Vm *vm, uint8_t type, size_t size);
void gc_collect(Vm *vm);

void heap_debug_info(const Vm *vm);

#endif // HEAP_H
