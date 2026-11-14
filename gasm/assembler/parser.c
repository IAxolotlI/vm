#include <errno.h>
#include "sv.h"
#include "abi.h"
#include "assembler.h"
#include "container.h"

ParseError parse_error(ParseStatus status, size_t col) {
    return (ParseError){ .status = status, .err_col = col };
}

static const InstrInfo *find_instruction(String_View name) {
    for (size_t i = 0; i < instructions_count; i++) {
        if (sv_eq_cstr(name, instructions[i].name)) {
            return &instructions[i];
        }
    }
    return NULL;
}

static ParseStatus sv_is_register(String_View sv, uint8_t *num) {
    if (sv.count < 2 || (sv.data[0] != 'r' && sv.data[0] != 'R')) {
        return PARSE_NOT_REG;
    }
    
    if (sv.count == 3) {
        unsigned char second = (unsigned char)tolower((unsigned char)sv.data[1]);
        unsigned char third  = (unsigned char)tolower((unsigned char)sv.data[2]);
        if (third == 'p' && second == 's') {
            *num = RSP;
            return PARSE_OK;
        }
        if (third == 'p' && second == 'b') {
            *num = RBP;
            return PARSE_OK;
        }
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

static ParseError parse_arguments(String_View text, const char *line, Instr *ins) {
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

ParseError parse_line_header(const char *text, LineHeader *out) {
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

ParseError parse_line(const char *text, ParsedLine *out) {
    ParseError err = parse_line_header(text, &out->header);
    if (err.status != PARSE_OK || !out->header.has_instr) return err;

    out->instr.info = out->header.info;
    out->instr.col = out->header.col;
    return parse_arguments(out->header.args, text, &out->instr);
}
