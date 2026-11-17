#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include "gm.h"
#include "logger.h"
#include "format.h"
#include "abi.h"

bool vm_init(Vm *vm, size_t mem_size, size_t stack_size) {
    if (!vm) return false;
    if (stack_size < GM_STACK_MIN) {
        LOG_ERROR("Stack size must be greater than %d bytes", GM_STACK_MIN);
        return false;
    }
    if (stack_size % GM_STACK_ALIGN != 0) {
        LOG_ERROR("Stack size must divide by %d bytes", GM_STACK_ALIGN);
        return false;
    }
    if (mem_size % GM_STACK_ALIGN != 0) {
        LOG_ERROR("Memory size must divide by %d bytes", GM_STACK_ALIGN);
        return false;
    }
    if (mem_size <= stack_size) {
        LOG_ERROR("Stack size must be less than memory size");
        return false;
    }

    *vm = (Vm){0};
    vm->mem = calloc(1, mem_size);
    if (!vm->mem) {
        LOG_ERROR("Malloc failed");
        return false;
    }
    vm->mem_size   = mem_size;
    vm->stack_top  = mem_size;
    vm->stack_base = mem_size - stack_size;
    vm->regs[RSP] = vm->stack_top;
    vm->regs[RBP] = vm->stack_top;
    vm->ip = 0;
    vm->status = VM_RUNNING;
    return true;
}

void vm_destroy(Vm *vm) {
    if (!vm) return;
    free(vm->mem);
    *vm = (Vm){0};
}


uint8_t *load_program_from_file(Vm *vm, const char *input_file_path) {
    FILE *input_file = fopen(input_file_path, "rb");
    if (!input_file) {
        LOG_ERROR("Couldn't open file `%s`: %s", input_file_path, strerror(errno));
        return NULL;
    }
    fseek(input_file, 0, SEEK_END);
    long size = ftell(input_file);
    if (size <= 0) {
        LOG_ERROR("%s is empty or unreadable", input_file_path);
        fclose(input_file);
        return NULL;
    }
    rewind(input_file);
    
    uint8_t *code = malloc((size_t)size);
    if (!code) {
        LOG_ERROR("Malloc failed");
        fclose(input_file);
        return NULL;
    }

    if (fread(code, 1, (size_t)size, input_file) != (size_t)size) {
        LOG_ERROR("Failed to read file `%s`: %s", input_file_path, strerror(errno));
        free(code);
        fclose(input_file);
        return NULL;
    }
    fclose(input_file);

    size_t file_size = (size_t)size;
    if (file_size < GM_HEADER_SIZE) {
        LOG_ERROR("File size is less than magic number");
        free(code);
        return NULL;
    }
    if (memcmp(code, GM_MAGIC, sizeof(GM_MAGIC)) == 0) {
        uint32_t version = gm_get_u32(code + 4);
        if (version != GM_FORMAT_VERSION) {
            LOG_ERROR("Unsupported .gm version %u (expected %u)",
                      version, GM_FORMAT_VERSION);
            free(code);
            return NULL;
        }

        uint64_t entry = gm_get_u64(code + 8);
        uint64_t code_len = gm_get_u64(code + 16);
        size_t payload_size = file_size - GM_HEADER_SIZE;

        if (code_len != (uint64_t)payload_size) {
            LOG_ERROR("Invalid code length in header: header=%" PRIu64
                      ", file=%zu", code_len, payload_size);
            free(code);
            return NULL;
        }
        if (code_len == 0 || entry >= code_len) {
            LOG_ERROR("Invalid entry point: entry=%" PRIu64
                      ", code_len=%" PRIu64, entry, code_len);
            free(code);
            return NULL;
        }
        vm->code = code + GM_HEADER_SIZE;
        vm->code_len = (size_t)code_len;
        vm->ip = (size_t)entry;
    }
    return code;
}

int execute_program(Vm *vm) {
    while (vm->status == VM_RUNNING) {
        vm_execute_step(vm);
    }
    if (vm->status != VM_HALTED) {
        LOG_ERROR("VM stopped: %s (ip=%zu)", vm_get_status(vm->status), vm->ip);
        return 1;
    }
    return 0;
}
