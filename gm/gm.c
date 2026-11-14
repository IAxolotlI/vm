#include "logger.h"
#include "gm.h"
#include "opcodes.h"
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

static int64_t add_i64(int64_t a, int64_t b) {
    uint64_t r = (uint64_t)a + (uint64_t)b;
    int64_t out;
    memcpy(&out, &r, sizeof out);
    return out;
}

static int64_t sub_i64(int64_t a, int64_t b) {
    uint64_t r = (uint64_t)a - (uint64_t)b;
    int64_t out;
    memcpy(&out, &r, sizeof out);
    return out;
}

static int64_t mul_i64(int64_t a, int64_t b) {
    uint64_t r = (uint64_t)a * (uint64_t)b;
    int64_t out;
    memcpy(&out, &r, sizeof out);
    return out;
}

const char *vm_get_status(VmStatus s) {
    switch (s) {
        case VM_RUNNING:         return "Running";
        case VM_HALTED:          return "Halted";
        case VM_FAULT_BAD_OP:    return "Bad operation";
        case VM_FAULT_BAD_IP:    return "Bad instruction pointer";
        case VM_FAULT_OOB:       return "Out of bounds";
        case VM_FAULT_DIV_ZERO:  return "Division by zero";
        case VM_FAULT_OVERFLOW:  return "Number overflow";
        default:                 return "Unknown";
    }
}

static bool vm_read_byte(Vm *vm, uint8_t *out) {
    if (vm->ip >= vm->code_len) {
        LOG_ERROR("Read8 out of bounadries: ip = %zu, code_len = %zu",
                  vm->ip, vm->code_len);
        vm->status = VM_FAULT_OOB;
        return false;
    }
    *out = vm->code[vm->ip++];
    return true;
}

static bool vm_read_8byte(Vm *vm, uint64_t *out) {
    if (vm->code_len - vm->ip < 8) {
        LOG_ERROR("Read8 out of bounadries: ip = %zu, code_len = %zu",
                  vm->ip, vm->code_len);
        vm->status = VM_FAULT_OOB;
        return false;
    }
    const uint8_t *p = &vm->code[vm->ip];
    *out = (uint64_t)p[0]
         | ((uint64_t)p[1] << 8)
         | ((uint64_t)p[2] << 16)
         | ((uint64_t)p[3] << 24)
         | ((uint64_t)p[4] << 32)
         | ((uint64_t)p[5] << 40)
         | ((uint64_t)p[6] << 48)
         | ((uint64_t)p[7] << 56);
    vm->ip += 8;
    return true;
}

static bool vm_jump(Vm *vm, int64_t off) {
    int64_t target = (int64_t)vm->ip + off;
    if (target < 0 || (uint64_t)target >= vm->code_len) {
        LOG_ERROR("JMP out of boundaries: ip=%zu off=%" PRId64 " target=%" PRId64,
                  vm->ip, off, target);
        vm->status = VM_FAULT_BAD_IP;
        return false;
    }
    vm->ip = (size_t)target;
    return true;
}

void vm_execute_step(Vm *vm) {
    if (vm->status != VM_RUNNING) return;

    uint8_t opcode;
    if (!vm_read_byte(vm, &opcode)) return;

    switch ((Opcode)opcode) {
        case OP_NOP:
            break;

        case OP_HALT:
            vm->status = VM_HALTED;
            break;

        case OP_LOAD_I: {
            uint8_t  dst;
            uint64_t imm;
            if (!vm_read_byte(vm, &dst) || !vm_read_8byte(vm, &imm)) break;
            vm->regs[dst] = (Word){ .i = ((int64_t)imm) };
            break;
        }

        case OP_LOAD_F: {
            uint8_t  dst;
            uint64_t imm;
            if (!vm_read_byte(vm, &dst) || !vm_read_8byte(vm, &imm)) break;
            double d;
            memcpy(&d, &imm, sizeof d);
            vm->regs[dst] = (Word){.f = d };
            break;
        }

        case OP_MOV: {
            uint8_t dst, src;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &src)) break;
            vm->regs[dst] = vm->regs[src];
            break;
        }

        case OP_ADD_I: {
            uint8_t dst, a, b;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &a) || !vm_read_byte(vm, &b)) break;
            vm->regs[dst] = (Word){ .i = add_i64(vm->regs[a].i, vm->regs[b].i) };
            break;
        }

        case OP_ADD_F: {
            uint8_t dst, a, b;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &a) || !vm_read_byte(vm, &b)) break;
            vm->regs[dst] = (Word){ .f = vm->regs[a].f + vm->regs[b].f };
            break;
        }
        
        case OP_SUB_I: {
            uint8_t dst, a, b;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &a) || !vm_read_byte(vm, &b)) break;
            vm->regs[dst] = (Word){ .i = sub_i64(vm->regs[a].i, vm->regs[b].i) };
            break;
        }
        
        case OP_SUB_F: {
            uint8_t dst, a, b;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &a) || !vm_read_byte(vm, &b)) break;
            vm->regs[dst] = (Word){ .f = vm->regs[a].f - vm->regs[b].f };
            break;
        }
                
        case OP_MUL_I: {
            uint8_t dst, a, b;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &a) || !vm_read_byte(vm, &b)) break;
            vm->regs[dst] = (Word){ .i = mul_i64(vm->regs[a].i, vm->regs[b].i) };
            break;
        }
        
        case OP_MUL_F: {
            uint8_t dst, a, b;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &a) || !vm_read_byte(vm, &b)) break;
            vm->regs[dst] = (Word){ .f = vm->regs[a].f * vm->regs[b].f };
            break;
        }
        
        case OP_DIV_I: {
            uint8_t dst, a, b;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &a) || !vm_read_byte(vm, &b)) break;
            int64_t x = vm->regs[a].i, y = vm->regs[b].i;
            if (y == 0) { vm->status = VM_FAULT_DIV_ZERO; break; }
            if (x == INT64_MIN && y == -1) {
                vm->regs[dst].i = INT64_MIN;
                break;
            }
            vm->regs[dst].i = x / y;
            break;
        }

        case OP_DIV_F: {
            uint8_t dst, a, b;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &a) || !vm_read_byte(vm, &b)) break;
            double x = vm->regs[a].f, y = vm->regs[b].f;
            vm->regs[dst].f = x / y;
            break;
        }
        
        case OP_CMP_LT_I: {
            uint8_t dst, a, b;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &a) || !vm_read_byte(vm, &b)) break;
            vm->regs[dst] = (Word){ .i = vm->regs[a].i < vm->regs[b].i };
            break;
        }

        case OP_CMP_LT_F: {
            uint8_t dst, a, b;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &a) || !vm_read_byte(vm, &b)) break;
            vm->regs[dst] = (Word){ .i = vm->regs[a].f < vm->regs[b].f };
            break;
        }
        
        case OP_JMP: {
            uint64_t raw;
            int64_t off;
            if (!vm_read_8byte(vm, &raw)) break;
            memcpy(&off, &raw, sizeof off);
            vm_jump(vm, off);
            break;
        }

        case OP_JMP_IF:
        case OP_JMP_IF_NOT: {
            uint8_t src;
            uint64_t raw;
            int64_t off;
            if (!vm_read_byte(vm, &src) || !vm_read_8byte(vm, &raw)) break;
            memcpy(&off, &raw, sizeof off);
            bool take = vm->regs[src].i;
            if ((Opcode)opcode == OP_JMP_IF_NOT) take = !take;
            if (take) vm_jump(vm, off);
            break;
        }
            
        case OP_PRINT_I: {
            uint8_t src;
            if (!vm_read_byte(vm, &src)) break;
            printf("%"PRId64"\n", vm->regs[src].i);
            break;
        }
        
        case OP_PRINT_F: {
            uint8_t src;
            if (!vm_read_byte(vm, &src)) break;
            printf("%g\n", vm->regs[src].f);
            break;
        }
        
        default:
            LOG_ERROR("Unknown opcode 0x%02X at IP %zu", opcode, vm->ip - 1);
            vm->status = VM_FAULT_BAD_OP;
            break;
    }
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
    vm->code = code;
    vm->code_len = (size_t)size;
    
    return code;
}

int execute_program(Vm *vm) {
    while (vm->status == VM_RUNNING) {
        vm_execute_step(vm);
    }
    if (vm->status != VM_HALTED) {
        LOG_ERROR("VM stoppped: %s\n%zu", vm_get_status(vm->status), vm->ip);
        return 1;
    }
    return 0;
}
