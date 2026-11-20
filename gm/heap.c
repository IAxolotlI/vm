#include <stdio.h>
#include "heap.h"
#include "gm.h"
#include "abi.h"

static inline size_t align_to_8(size_t size) {
    return (size + 7) & ~(size_t)7;
}

static uint64_t gc_copy_object(Vm *vm, uint64_t old_offset, size_t *free_ptr) {
    if (!(old_offset >= vm->heap_from_start && old_offset < vm->heap_from_end)) {
        return old_offset;
    }

    ObjHeader *old_hdr = (ObjHeader*)(vm->mem + old_offset);
    if (old_hdr->is_forwarded) {
        uint64_t *forwarding_addr = (uint64_t*)(vm->mem + old_offset + sizeof(ObjHeader));
        return *forwarding_addr;
    }

    uint64_t new_offset = (uint64_t)(*free_ptr);
    size_t align_data   = old_hdr->size < sizeof(uint64_t) ? sizeof(uint64_t) : old_hdr->size;
    size_t total_bytes  = align_to_8(sizeof(ObjHeader) + align_data);

    memcpy(vm->mem + new_offset, vm->mem + old_offset, total_bytes);
    *free_ptr += total_bytes;

    old_hdr->is_forwarded = 1;
    uint64_t *forwarding_addr = (uint64_t*)(vm->mem + old_offset + sizeof(ObjHeader));
    *forwarding_addr = new_offset;

    return new_offset;
}

static void gc_update_word(Vm *vm, Word *word_ptr, size_t *free_ptr) {
    Word w = *word_ptr;
    if (!nb_is_obj(w)) {
        return;
    }
    uint64_t old_offset = nb_as_obj(w);
    uint64_t new_offset = gc_copy_object(vm, old_offset, free_ptr);
    *word_ptr = nb_box_obj(new_offset);
}

static void gc_scan_roots(Vm *vm, size_t *free_ptr) {
    for (size_t i = 0; i < REG_COUNT; i++) {
        gc_update_word(vm, &vm->regs[i], free_ptr);
    }
    for (size_t addr = vm->regs[RSP]; addr < vm->mem_size; addr += sizeof(Word)) {
        Word *stack_slot = (Word*)(vm->mem + addr);
        gc_update_word(vm, stack_slot, free_ptr);
    }
}

static void gc_scan_object_fields(Vm *vm, ObjHeader *hdr, size_t *free_ptr) {
    uint8_t *data_ptr = (uint8_t*)hdr + sizeof(ObjHeader);
    switch (hdr->type) {
        case OBJ_BYTES:
            break;
        case OBJ_ARRAY: {
            size_t count = hdr->size / sizeof(Word);
            Word *elements = (Word*)data_ptr;
            for (size_t i = 0; i < count; i++) {
                gc_update_word(vm, &elements[i], free_ptr);
            }
            break;
        }
    }
}

static void gc_cheney_loop(Vm *vm, size_t *free_ptr) {
    size_t scan_ptr = vm->heap_to_start;
    while (scan_ptr < *free_ptr) {
        ObjHeader *hdr = (ObjHeader *)(vm->mem + scan_ptr);
        gc_scan_object_fields(vm, hdr, free_ptr);
        scan_ptr += align_to_8(sizeof(ObjHeader) + hdr->size);
    }
}

static void gc_flip_spaces(Vm *vm, size_t new_alloc_offset) {
    size_t tmp_start = vm->heap_from_start;
    size_t tmp_end   = vm->heap_from_end;
    vm->heap_from_start = vm->heap_to_start;
    vm->heap_from_end   = vm->heap_to_end;
    vm->heap_to_start   = tmp_start;
    vm->heap_to_end     = tmp_end;
    vm->heap_alloc_offset = new_alloc_offset;
}

void gc_collect(Vm *vm) {
    size_t free_ptr = vm->heap_to_start;
    gc_scan_roots(vm, &free_ptr);
    gc_cheney_loop(vm, &free_ptr);
    gc_flip_spaces(vm, free_ptr);
}

uint64_t heap_alloc(Vm *vm, uint8_t type, size_t data_size) {
    size_t align_data_size = data_size < sizeof(uint64_t) ? sizeof(uint64_t) : data_size;
    size_t total_bytes = align_to_8(sizeof(ObjHeader) + align_data_size);

    if (vm->heap_alloc_offset + total_bytes > vm->heap_from_end) {
        gc_collect(vm);
        if (vm->heap_alloc_offset + total_bytes > vm->heap_from_end) {
            vm->status = VM_FAULT_OOM;
            return GM_NULL;
        }
    }

    uint64_t offset = (uint64_t)vm->heap_alloc_offset;
    vm->heap_alloc_offset += total_bytes;

    ObjHeader *hdr = (ObjHeader*)(vm->mem + offset);
    hdr->type         = type;
    hdr->is_forwarded = 0;
    hdr->size         = (uint32_t)data_size;

    return offset;
}
