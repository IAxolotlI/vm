#ifndef GMIINIT_H
#define GMIINIT_H

#include <inttypes.h>
#include "gm.h"

bool vm_init(Vm *vm, size_t mem_size, size_t stack_size);
void vm_destroy(Vm *vm);
uint8_t *load_program_from_file(Vm *vm, const char *input_file_path);
int execute_program(Vm *vm);


#endif // GMINIT_H
