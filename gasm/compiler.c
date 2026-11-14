#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "container.h"
#include "compiler.h"
#include "emit.h"
#include "logger.h"
#include "opcodes.h"
#include "sv.h"

#define LABEL_MARK ':'

typedef struct {
    char *name;
    size_t addr;
} Label;

typedef struct {
    Label *items;
    size_t count;
    size_t capacity;
} LabelTable;

typedef struct {
    const char *filename;
    LabelTable labels;
} Compiler;

typedef struct {
    bool has_label;
    String_View label;
    size_t label_col;
    bool has_instr;
    const InstrInfo *info;
    size_t col;
    String_View args;
} LineHeader;

typedef struct {
    LineHeader header;
    Instr instr;
} ParsedLine;

static ParseError parse_error(ParseStatus status, size_t col) {
    return (ParseError){.status = status, .err_col = col};
}

static const InstrInfo *find_instruction(String_View name) {
    for (size_t i = 0; i < instructions_count; i++) {
        if (sv_eq_cstr(name, instructions[i].name)) {
            return &instructions[i];
        }
    }
    return NULL;
}

static bool label_lookup(const LabelTable *table, String_View name, size_t *addr) {
    for (size_t i = 0; i < table->count; i++) {
        if (!sv_eq_cstr(name, table->items[i].name)) continue;
        if (addr) *addr = table->items[i].addr;
        return true;
    }
    return false;
}

static bool label_define(LabelTable *table, String_View name, size_t addr) {
    if (label_lookup(table, name, NULL)) return false;

    char *copy = strndup(name.data, name.count);
    if (!copy) {
        LOG_ERROR("Malloc failed");
        exit(EXIT_FAILURE);
    }

    Label label = {.name = copy, .addr = addr};
    da_append(table, label);
    return true;
}

static void label_table_free(LabelTable *table) {
    if (table) {
        for (size_t i = 0; i < table->count; i++) {
            free(table->items[i].name);
        }
        free(table->items);
        *table = (LabelTable){0};
    }
}

static ParseStatus sv_is_register(String_View sv, uint8_t *num) {
    if (sv.count < 2 || (sv.data[0] != 'r' && sv.data[0] != 'R')) {
        return PARSE_NOT_REG;
    }

    long value = 0;
    for (size_t i = 1; i < sv.count; i++) {
        unsigned char c = (unsigned char)sv.data[i];
        if (!isdigit(c)) return PARSE_NOT_REG;
        value = value * 10 + (c - '0');
        if (value >= REG_COUNT) return PARSE_BIG_REG;
    }

    *num = (uint8_t)value;
    return PARSE_OK;
}

static ParseStatus sv_to_double(String_View sv, double *out) {
    char *endptr;
    char *str = strndup(sv.data, sv.count);
    assert(str != NULL && "Out of RAM");

    errno = 0;
    double value = strtod(str, &endptr);
    bool ok = errno != ERANGE && endptr != str && *endptr == '\0';
    if (ok) *out = value;

    free(str);
    return ok ? PARSE_OK : PARSE_NAN;
}

static ParseStatus sv_to_int(String_View sv, int64_t *out) {
    char *endptr;
    char *str = strndup(sv.data, sv.count);
    assert(str != NULL && "Out of RAM");

    errno = 0;
    long long value = strtoll(str, &endptr, 10);
    bool ok = errno != ERANGE && endptr != str && *endptr == '\0';
    if (ok) *out = value;

    free(str);
    return ok ? PARSE_OK : PARSE_NAN;
}

static ParseStatus parse_operand(String_View text, char sig, size_t col, Instr *ins) {
    Operand operand = {.col = col};
    switch (sig) {
        case 'r': {
            uint8_t reg;
            ParseStatus status = sv_is_register(text, &reg);
            if (status != PARSE_OK) return status;
            operand.kind = OPERAND_REG;
            operand.as.reg = reg;
            break;
        }

        case 'i': {
            int64_t value;
            ParseStatus status = sv_to_int(text, &value);
            if (status != PARSE_OK) return status;
            operand.kind = OPERAND_INT;
            operand.as.i = value;
            break;
        }

        case 'l': {
            int64_t value;
            if (sv_to_int(text, &value) == PARSE_OK) {
                operand.kind = OPERAND_INT;
                operand.as.i = value;
            } else {
                operand.kind = OPERAND_LABEL;
                operand.as.label = text;
            }
            break;
        }

        case 'f': {
            double value;
            ParseStatus status = sv_to_double(text, &value);
            if (status != PARSE_OK) return status;
            operand.kind = OPERAND_FLOAT;
            operand.as.f = value;
            break;
        }

        default:
            return PARSE_BAD_SIG;
    }

    da_append(ins, operand);
    return PARSE_OK;
}

static ParseError parse_arguments(String_View text, const char *line,
                                  Instr *ins) {
    size_t sig_len = strlen(ins->info->sig);
    size_t arg_idx = 0;
    bool after_comma = false;
    size_t comma_col = 1;

    while (true) {
        sv_trim_left(&text);
        if (text.count == 0) {
            if (after_comma) {
                return parse_error(PARSE_TRAILING_COMMA, comma_col);
            }
            break;
        }

        size_t comma = sv_find(text, ',');
        String_View arg = sv_from_parts(text.data, comma);
        sv_trim_right(&arg);
        size_t arg_col = (size_t)(arg.data - line) + 1;

        if (arg.count == 0) {
            ParseStatus status = (comma < text.count)
                ? PARSE_EMPTY_ARG
                : PARSE_TRAILING_COMMA;
            return parse_error(status, arg_col);
        }

        for (size_t i = 0; i < arg.count; i++) {
            if (isspace((unsigned char)arg.data[i])) {
                size_t col = (size_t)(arg.data - line) + i + 1;
                return parse_error(PARSE_MISSING_COMMA, col);
            }
        }

        if (arg_idx >= sig_len) {
            return parse_error(PARSE_MANY_ARGUMENTS, arg_col);
        }

        ParseStatus status = parse_operand(arg, ins->info->sig[arg_idx],
                arg_col, ins);
        if (status != PARSE_OK) return parse_error(status, arg_col);

        arg_idx++;
        after_comma = false;

        bool has_comma = comma < text.count;
        if (!has_comma) break;

        comma_col = (size_t)(text.data - line) + comma + 1;
        sv_shift_by_n(&text, comma + 1);
        after_comma = true;
    }

    if (arg_idx < sig_len) {
        return parse_error(PARSE_FEW_ARGUMENTS, ins->col);
    }
    return parse_error(PARSE_OK, 1);
}

static ParseError parse_line_header(const char *text, LineHeader *out) {
    String_View rest = sv_from_cstr(text);
    sv_trim(&rest);
    if (rest.count == 0) return parse_error(PARSE_OK, 1);

    size_t colon = sv_find(rest, LABEL_MARK);
    if (colon < rest.count) {
        String_View label = sv_from_parts(rest.data, colon);
        sv_trim_right(&label);

        bool is_label = true;
        for (size_t i = 0; i < label.count; i++) {
            if (isspace((unsigned char)label.data[i])) {
                is_label = false;
                break;
            }
        }

        if (is_label) {
            if (label.count == 0) {
                return parse_error(PARSE_BAD_LABEL, (size_t)(rest.data - text) + 1);
            }

            out->has_label = true;
            out->label = label;
            out->label_col = (size_t)(label.data - text) + 1;

            sv_shift_by_n(&rest, colon + 1);
            sv_trim(&rest);
            if (rest.count == 0) return parse_error(PARSE_OK, 1);
        }
    }

    size_t name_len = 0;
    while (name_len < rest.count && !isspace((unsigned char)rest.data[name_len])) {
        name_len++;
    }

    String_View name = sv_from_parts(rest.data, name_len);
    out->info = find_instruction(name);
    if (!out->info) {
        return parse_error(PARSE_UNKNOWN_OP, (size_t)(name.data - text) + 1);
    }

    out->has_instr = true;
    out->col = (size_t)(name.data - text) + 1;

    sv_shift_by_n(&rest, name_len);
    out->args = rest;
    return parse_error(PARSE_OK, 1);
}

static ParseError parse_line(const char *text, ParsedLine *out) {
    ParseError err = parse_line_header(text, &out->header);
    if (err.status != PARSE_OK || !out->header.has_instr) return err;

    out->instr.info = out->header.info;
    out->instr.col = out->header.col;
    return parse_arguments(out->header.args, text, &out->instr);
}

static size_t operand_width(char sig) {
    switch (sig) {
        case 'r': return REG_SIZE;
        case 'i': return INT_SIZE;
        case 'f': return FLOAT_SIZE;
        case 'l': return INT_SIZE;
        default:  return 0;
    }
}

static size_t instruction_size(const InstrInfo *info) {
    size_t size = OPCODE_SIZE;
    for (const char *p = info->sig; *p; p++) {
        size += operand_width(*p);
    }
    return size;
}

static ParseError resolve_labels(Instr *ins, const LabelTable *labels, size_t addr) {
    size_t next_addr = addr + instruction_size(ins->info);
    for (size_t i = 0; i < ins->count; i++) {
        Operand *operand = &ins->items[i];
        if (operand->kind != OPERAND_LABEL) continue;

        size_t label_addr;
        if (!label_lookup(labels, operand->as.label, &label_addr)) {
            return parse_error(PARSE_MISSING_LABEL, operand->col);
        }

        operand->kind = OPERAND_INT;
        operand->as.i = (int64_t)label_addr - (int64_t)next_addr;
    }

    return parse_error(PARSE_OK, 1);
}

static void instr_free(Instr *ins) {
    free(ins->items);
    *ins = (Instr){0};
}

static const char *parse_status_message(ParseStatus status) {
    switch (status) {
        case PARSE_OK:               return NULL;
        case PARSE_EMPTY_ARG:        return "Empty or missed argument";
        case PARSE_TRAILING_COMMA:   return "Trailing comma";
        case PARSE_MISSING_COMMA:    return "Missing comma between arguments";
        case PARSE_UNKNOWN_OP:       return "Unknown command";
        case PARSE_MANY_ARGUMENTS:   return "Too many arguments";
        case PARSE_FEW_ARGUMENTS:    return "Too few arguments";
        case PARSE_BAD_SIG:          return "Bad signature";
        case PARSE_NOT_REG:          return "Expected register in argument";
        case PARSE_BIG_REG:          return "Register doesn't exist";
        case PARSE_NAN:              return "Expected number or label in argument";
        case PARSE_ERROR_EMIT:       return "Error occurred during emission";
        case PARSE_LABEL_REDEF:      return "Label redefined";
        case PARSE_MISSING_LABEL:    return "Label doesn't exist";
        case PARSE_BAD_LABEL:        return "Invalid label";
    }
    return "Unknown parse error";
}

static int report_error(ParseError err, size_t line, const char *filename) {
    if (err.status == PARSE_OK) return 0;

    const char *message = parse_status_message(err.status);
    SrcLocation loc = {
        .file = filename,
        .line = (int)line,
        .col = (int)err.err_col,
    };
    PRINT_ERROR(loc, "%s", message);
    return 1;
}

static int check_read_error(const Compiler *compiler, FILE *input_file) {
    if (!ferror(input_file)) return 0;

    LOG_ERROR("Couldn't read file `%s`: %s", compiler->filename, strerror(errno));
    return 1;
}

static int collect_labels(Compiler *compiler, FILE *input_file) {
    int result = 0;
    char *line = NULL;
    size_t line_capacity = 0;
    size_t lineno = 0;
    size_t pc = 0;

    while (getline(&line, &line_capacity, input_file) != -1) {
        lineno++;
        LineHeader header = {0};
        ParseError err = parse_line_header(line, &header);

        if (err.status == PARSE_OK && header.has_label
                && !label_define(&compiler->labels, header.label, pc)) {
            err = parse_error(PARSE_LABEL_REDEF, header.label_col);
        }
        if (err.status == PARSE_OK && header.has_instr) {
            pc += instruction_size(header.info);
        }
        if (err.status != PARSE_OK) {
            result = report_error(err, lineno, compiler->filename);
            break;
        }
    }

    if (result == 0) result = check_read_error(compiler, input_file);
    free(line);
    return result;
}

static int emit_instructions(Compiler *compiler, FILE *input_file, FILE *output_file) {
    int result = 0;
    char *line = NULL;
    size_t line_capacity = 0;
    size_t lineno = 0;
    size_t pc = 0;

    while (getline(&line, &line_capacity, input_file) != -1) {
        lineno++;
        ParsedLine parsed = {0};
        ParseError err = parse_line(line, &parsed);

        if (err.status == PARSE_OK && parsed.header.has_instr) {
            err = resolve_labels(&parsed.instr, &compiler->labels, pc);
            if (err.status == PARSE_OK && !emit_instr(output_file, &parsed.instr)) {
                err = parse_error(PARSE_ERROR_EMIT, parsed.instr.col);
            }
            if (err.status == PARSE_OK) {
                pc += instruction_size(parsed.instr.info);
            }
        }
        instr_free(&parsed.instr);
        if (err.status != PARSE_OK) {
            result = report_error(err, lineno, compiler->filename);
            break;
        }
    }

    if (result == 0) result = check_read_error(compiler, input_file);
    free(line);
    return result;
}

int emit_bytecode_from_program(const char *filename, FILE *input_file, FILE *output_file) {
    Compiler compiler = {.filename = filename};

    int result = collect_labels(&compiler, input_file);
    if (result == 0) {
        rewind(input_file);
        result = emit_instructions(&compiler, input_file, output_file);
    }

    label_table_free(&compiler.labels);
    return result;
}
