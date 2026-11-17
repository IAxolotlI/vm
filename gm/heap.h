#ifndef HEAP_H
#define HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "gm.h"

typedef enum {
    OBJ_ARRAY,
    OBJ_BYTES,
} ObjKind;

typedef struct {
    ObjKind          kind;
    bool             marked;
    size_t           count;
    struct ObjHeader *next;
} ObjHeader;

ObjHeader *heap_alloc_array(Vm *vm, size_t count);
ObjHeader *heap_alloc_bytes(Vm *vm, size_t size);

static inline Word *obj_array_data(ObjHeader *obj) {
    return (Word *)(obj + 1);
}

static inline uint8_t *obj_bytes_data(ObjHeader *obj) {
    return (uint8_t *)(obj + 1);
}

void gc_collect(Vm *vm);
void heap_free_all(Vm *vm);

#endif // HEAP_H
