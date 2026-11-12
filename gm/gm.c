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

static inline Value val_int(int64_t x) {
    Value v = { .tag = TAG_INT, .as = { .i = x } };
    return v;
}

static inline Value val_double(double x) {
    Value v = { .tag = TAG_DOUBLE, .as = { .f = x } };
    return v;
}

static inline bool is_num(Value v) {
    return v.tag == TAG_INT || v.tag == TAG_DOUBLE;
}

static inline double as_double(Value v) { return v.tag == TAG_DOUBLE ? v.as.f : (double)v.as.i; }

void print_value(Value v) {
    switch (v.tag) {
        case TAG_INT:    printf("%" PRId64 "\n", v.as.i); break;
        case TAG_DOUBLE: printf("%g\n", v.as.f); break;
        case TAG_NIL:    printf("nil\n"); break;
        default:         printf("bad tag %d\n", (int)v.tag); break;
    }
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
        case VM_FAULT_TYPE:  return "Type error";
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

static bool vm_read_4byte(Vm *vm, uint32_t *out) {
    if (vm->code_len - vm->ip < 4) {
        LOG_ERROR("Read8 out of bounadries: ip = %zu, code_len = %zu",
                  vm->ip, vm->code_len);
        vm->status = VM_FAULT_OOB;
        return false;
    }
    const uint8_t *p = &vm->code[vm->ip];
    *out = (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
    vm->ip += 4;
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

static inline bool check_reg_cond(Value v) {
    switch (v.tag) {
        case TAG_INT:    return v.as.i != 0;
        case TAG_DOUBLE: return v.as.f != 0.0;
        case TAG_NIL:
        default:         return false;
    }
}

static bool vm_jump(Vm *vm, int32_t off) {
    int64_t target = (int64_t)vm->ip + off;
    if (target < 0 || (uint64_t)target >= vm->code_len) {
        LOG_ERROR("JMP out of boundaries: ip=%zu off=%" PRId32 " target=%" PRId64,
                  vm->ip, off, target);
        vm->status = VM_FAULT_BAD_IP;
        return false;
    }
    vm->ip = (size_t)target;
    return true;
}

static bool arithmetic_op(Opcode op, Value x, Value y, Value* out, VmStatus *err) {
    if (x.tag == TAG_NIL || y.tag == TAG_NIL) {
        *err = VM_FAULT_TYPE;
        return false;
    }
    else if (x.tag == TAG_INT && y.tag == TAG_INT) {
        int64_t a = x.as.i, b = y.as.i;
        switch (op) {
            case OP_ADD: {
                if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) {
                    *err = VM_FAULT_OVERFLOW;
                    return false;
                }
                *out = val_int((uint64_t)(a + b));
                break;
            }
            case OP_SUB: {
                if ((b < 0 && a > INT64_MAX + b) || (b > 0 && a < INT64_MIN + b)) {
                    *err = VM_FAULT_OVERFLOW;
                    return false;
                }
                *out = val_int((uint64_t)(a - b));
                break;
            }
            case OP_MUL: {
                uint64_t ua = (a < 0) ? (uint64_t)(-(a + 1)) + 1 : (uint64_t)a;
                uint64_t ub = (b < 0) ? (uint64_t)(-(b + 1)) + 1 : (uint64_t)b;
                bool neg = ((a < 0) != (b < 0));
                uint64_t limit = neg ? ((uint64_t)INT64_MAX + 1) : (uint64_t)INT64_MAX;
                if (ub != 0) {
                    if (ua > limit / ub) {
                        *err = VM_FAULT_OVERFLOW;
                        return false;
                    }
                }
                *out = val_int((uint64_t)(a * b));
                break;
            }
            case OP_DIV: {
                if (b == 0) {
                    *err = VM_FAULT_DIV_ZERO;
                    return false;
                }
                if (a == INT64_MIN && b == -1) {
                    *err = VM_FAULT_OVERFLOW;
                    return false;
                }
                *out = val_int((uint64_t)(a / b));
                break;
            }
            default: { *err = VM_FAULT_BAD_OP; return false; }
        }
    }
    else {
        double a = as_double(x), b = as_double(y);
        switch (op) {
            case OP_ADD: *out = val_double(a + b); return true;
            case OP_SUB: *out = val_double(a - b); return true;
            case OP_MUL: *out = val_double(a * b); return true;
            case OP_DIV: {
                if (b == 0.0) {
                    *err = VM_FAULT_DIV_ZERO;
                    return false;
                }
                *out = val_double(a / b);
                break;
            }
            default: { *err = VM_FAULT_BAD_OP; return false; }
        }
    }
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

        case OP_LOADI: {
            uint8_t  dst;
            uint32_t imm;
            if (!vm_read_byte(vm, &dst) || !vm_read_4byte(vm, &imm)) break;
            vm->regs[dst] = val_int((int32_t)imm);
            break;
        }

        case OP_LOADF: {
            uint8_t  dst;
            uint64_t imm;
            if (!vm_read_byte(vm, &dst) || !vm_read_8byte(vm, &imm)) break;
            double d;
            memcpy(&d, &imm, sizeof d);
            vm->regs[dst] = val_double(d);
            break;
        }

        case OP_MOV: {
            uint8_t dst, src;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &src)) break;
            vm->regs[dst] = vm->regs[src];
            break;
        }

        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV: {
            uint8_t dst, a, b;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &a) || !vm_read_byte(vm, &b)) break;
            if (!arithmetic_op(opcode, vm->regs[a], vm->regs[b], &vm->regs[dst], &vm->status)) break;
            break;
        }

        case OP_CMP_LT: {
            uint8_t dst, a, b;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &a) || !vm_read_byte(vm, &b)) break;

            Value x = vm->regs[a], y = vm->regs[b];
            if (!is_num(x) || !is_num(y)) {
                vm->status = VM_FAULT_TYPE;
                break;
            }

            int less;
            if (x.tag == TAG_INT && y.tag == TAG_INT) {
                less = x.as.i < y.as.i;
            }
            else {
                less = as_double(x) < as_double(y);
            }
            vm->regs[dst] = val_int(less);
            break;
        }
            
        case OP_JMP: {
            uint32_t off;
            if (!vm_read_4byte(vm, &off)) break;
            vm_jump(vm, (int32_t)off);
            break;
        }

        case OP_JMP_IF:
        case OP_JMP_IF_NOT: {
            uint8_t src;
            uint32_t off;
            if (!vm_read_byte(vm, &src) || !vm_read_4byte(vm, &off)) break;

            bool take = check_reg_cond(vm->regs[src]);
            if ((Opcode)opcode == OP_JMP_IF_NOT) take = !take;
            if (take) vm_jump(vm, (int32_t)off);
            break;
        }
            
        case OP_PRINT: {
            uint8_t src;
            if (!vm_read_byte(vm, &src)) break;
            print_value(vm->regs[src]);
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
