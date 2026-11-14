#include "assembler.h"
#include "logger.h"
#include "sv.h"
#include <errno.h>
#include <string.h>

static void usage(FILE *stream, const char *program) {
    fprintf(stream, "Usage: %s [OPTIONS] <input.gasm>\n", program);
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -o <output.gm>  Provide output path. Default - \"a.gm\"\n");
    fprintf(stream, "    -h              Print this help.\n");
}

static char* get_flag_value(int *argc, char ***argv,
                            const char *flag,
                            const char *program) {
    if (*argc == 0) {
        usage(stderr, program);
        LOG_ERROR("No value provided for flag `%s`\n", flag);
        exit(1);
    }

    return shift(argc, argv);
}

int main(int argc, char **argv) {
    const char *program = shift(&argc, &argv);
    const char *input_file_path = NULL;
    const char *output_file_path = "a.gm";
    if (argc == 0) {
        usage(stdout, program);
        return 0;
    }
    while (argc > 0) {
        const char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-o") == 0) {
            output_file_path = get_flag_value(&argc, &argv, flag, program);
        }
        else if (strcmp(flag, "-h") == 0) {
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
    
    if (!input_file_path) {
        usage(stderr, program);
        LOG_ERROR("No input file provided");
        return 1;
    }

    FILE *input = fopen(input_file_path, "r");
    if (!input) {
        LOG_ERROR("Couldn't open file `%s`: %s", input_file_path, strerror(errno));
        return 1;
    }
    FILE *output = fopen(output_file_path, "wb");
    if (!output) {
        LOG_ERROR("Couldn't open file `%s`: %s", output_file_path, strerror(errno));
        fclose(input);
        return 1;
    }
    int result = emit_bytecode_from_program(input_file_path, input, output);

    fclose(input);
    fclose(output);
    return result;
}
