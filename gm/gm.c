#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "logger.h"
#include "gm.h"
#include "abi.h"
#include "opcodes.h"
#include "memory.h"

static bool vm_read_byte(Vm *vm, uint8_t *out) {
    if (vm->ip >= vm->code_len) {
        LOG_ERROR("Read byte out of bounds: ip = %zu, code_len = %zu",
                  vm->ip, vm->code_len);
        vm->status = VM_FAULT_OOB;
        return false;
    }
    *out = vm->code[vm->ip++];
    return true;
}

static bool vm_read_8byte(Vm *vm, uint64_t *out) {
    if (vm->ip > vm->code_len || vm->code_len - vm->ip < 8) {
        LOG_ERROR("Read 8 bytes out of bounds: ip = %zu, code_len = %zu",
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

const char *vm_get_status(VmStatus s) {
    switch (s) {
        case VM_RUNNING:               return "Running";
        case VM_HALTED:                return "Halted";
        case VM_FAULT_BAD_OP:          return "Bad operation";
        case VM_FAULT_BAD_IP:          return "Bad instruction pointer";
        case VM_FAULT_OOB:             return "Out of bounds";
        case VM_FAULT_DIV_ZERO:        return "Division by zero";
        case VM_FAULT_STACK_OVERFLOW:  return "Stack overflow";
        case VM_FAULT_STACK_UNDERFLOW: return "Stack underflow";
        case VM_FAULT_BAD_FRAME:       return "Bad stack frame";
        case VM_FAULT_BAD_ADDR:        return "Bad memory address";
        default:                       return "Unknown";
    }
}

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

static bool vm_relative_target(const Vm *vm, int64_t off, size_t *out) {
    uint64_t base = vm->ip;
    uint64_t target;

    if (off >= 0) {
        uint64_t delta = (uint64_t)off;
        if (delta > vm->code_len || base > vm->code_len - delta) {
            return false;
        }
        target = base + delta;
    }
    else {
        uint64_t delta = (uint64_t)(-(off + 1)) + 1;
        if (delta > base) return false;
        target = base - delta;
    }

    if (target >= vm->code_len) return false;
    *out = (size_t)target;
    return true;
}

static bool vm_jump(Vm *vm, int64_t off) {
    size_t target;
    if (!vm_relative_target(vm, off, &target)) {
        LOG_ERROR("Jump out of boundaries: ip=%zu off=%" PRId64,
                  vm->ip, off);
        vm->status = VM_FAULT_BAD_IP;
        return false;
    }
    vm->ip = target;
    return true;
}

static bool vm_call(Vm *vm, int64_t off) {
    size_t target;
    if (!vm_relative_target(vm, off, &target)) {
        LOG_ERROR("Call out of boundaries: ip=%zu off=%" PRId64,
                  vm->ip, off);
        vm->status = VM_FAULT_BAD_IP;
        return false;
    }
    if (!stack_push(vm, (Word) { .u = vm->ip })) return false;
    vm->ip = target;
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
            int64_t val;
            memcpy(&val, &imm, sizeof val);
            vm->regs[dst] = (Word){ .i = val };
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

        case OP_ENTER: {
            uint64_t raw;
            int64_t val;
            if (!vm_read_8byte(vm, &raw)) break;
            memcpy(&val, &raw, sizeof val);
            if (val < 0 || val % GM_STACK_ALIGN != 0) {
                LOG_ERROR("Argument of enter instruction "
                          "must be >= 0 and divisible by 8");
                vm->status = VM_FAULT_BAD_FRAME;
                break;
            }
            if (vm->regs[RSP].u < vm->stack_base + (uint64_t)val + GM_STACK_ALIGN) {
                LOG_ERROR("Argument of enter instruction "
                          "must be less than space left on the stack");
                vm->status = VM_FAULT_STACK_OVERFLOW;
                break;
            }
            if (!stack_push(vm, vm->regs[RBP])) break;

            vm->regs[RBP] = vm->regs[RSP];
            vm->regs[RSP].u -= (uint64_t)val;
            break;
        }
        
        case OP_LEAVE: {
            uint64_t bp = vm->regs[RBP].u;
            if (bp < vm->regs[RSP].u) {
                LOG_ERROR("Base pointer is further than stack pointer:"
                          " bp=%" PRIu64 ", rsp=%" PRIu64,
                          bp, vm->regs[RSP].u);
                vm->status = VM_FAULT_BAD_FRAME;
                break;
            }
            if (bp > vm->stack_top - GM_STACK_ALIGN) {
                LOG_ERROR("Base pointer is bigger than stack top:"
                          " bp=%" PRIu64 ", top=%zu",
                          bp, vm->stack_top);
                vm->status = VM_FAULT_BAD_FRAME;
                break;
            }
            if (bp % GM_STACK_ALIGN != 0) {
                LOG_ERROR("Base pointer isn't aligned by %d bytes",
                          GM_STACK_ALIGN);
                vm->status = VM_FAULT_BAD_FRAME;
                break;
            }
            vm->regs[RSP].u = bp;
            stack_pop(vm, &vm->regs[RBP]);
            break;
        }
        
        case OP_CALL: {
            uint64_t raw;
            int64_t off;
            if (!vm_read_8byte(vm, &raw)) break;
            memcpy(&off, &raw, sizeof off);
            vm_call(vm, off);
            break;
        }

        case OP_RET: {
            Word addr;
            if (!stack_pop(vm, &addr)) break;
            if (addr.u >= vm->code_len) {
                LOG_ERROR("RET out of boundaries: target=%" PRIu64
                        ", code_len=%zu", addr.u, vm->code_len);
                vm->status = VM_FAULT_BAD_IP;
                break;
            }
            vm->ip = (size_t)addr.u;
            break;
        }
        case OP_PUSH: {
            uint8_t src;
            if (!vm_read_byte(vm, &src)) break;
            if (!stack_push(vm, vm->regs[src])) break;
            break;
        }
        case OP_POP: {
            uint8_t dst;
            if (!vm_read_byte(vm, &dst)) break;
            if (!stack_pop(vm, &vm->regs[dst])) break;
            break;
        }
        case OP_LOAD: {
            uint8_t dst, base;
            uint64_t raw;
            int64_t off;
            if (!vm_read_byte(vm, &dst) || !vm_read_byte(vm, &base) || !vm_read_8byte(vm, &raw)) break;
            memcpy(&off, &raw, sizeof off);
            uint64_t addr;
            if (!stack_addr(vm, vm->regs[base].u, off, &addr)) {
                vm->status = VM_FAULT_BAD_ADDR;
                break;
            }
            vm->regs[dst].u = mem_read_u64(vm, addr);
            break;
        }
        case OP_STORE: {
            uint8_t base, src;
            uint64_t raw;
            int64_t off;
            if (!vm_read_byte(vm, &base) || !vm_read_8byte(vm, &raw) || !vm_read_byte(vm, &src)) break;
            memcpy(&off, &raw, sizeof off);
            uint64_t addr;
            if (!stack_addr(vm, vm->regs[base].u, off, &addr)) {
                vm->status = VM_FAULT_BAD_ADDR;
                break;
            }
            mem_write_u64(vm, addr, vm->regs[src].u);
            break;
        }
        default:
            LOG_ERROR("Unknown opcode 0x%02X at IP %zu", opcode, vm->ip - 1);
            vm->status = VM_FAULT_BAD_OP;
            break;
    }
}
