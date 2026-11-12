#include "gm.h"
#include "sv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "logger.h"

static void usage(FILE *stream, const char *program) {
    fprintf(stream, "Usage: %s [OPTIONS] <input.gasm>\n", program);
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -h              Print this help.\n");
}

int main(int argc, char **argv) {
    const char *program = shift(&argc, &argv);
    const char *input_file_path = NULL;
    if (argc == 0) {
        usage(stdout, program);
        return 0;
    }
    while (argc > 0) {
        const char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-h") == 0) {
            usage(stdout, program);
            return 0;
        }
        else {
            if (input_file_path != NULL) {
                usage(stderr, program);
                LOG_ERROR("Input file is provided as `%s`", input_file_path);
                return 1;
            }
            input_file_path = flag;
        }
    }

    Vm vm = {0};
    uint8_t *code = load_program_from_file(&vm, input_file_path);
    if (!code) return 1;
    int result = execute_program(&vm);
    free(code);
    return result;
}
