#ifndef PARSER_H
#define PARSER_H

#include "assembler.h"

ParseError parse_error(ParseStatus status, size_t col);
ParseStatus parse_operand(String_View text, char sig, size_t col, Instr *ins);
ParseError parse_arguments(String_View text, const char *line, Instr *ins);
ParseError parse_line_header(const char *text, LineHeader *out);
ParseError parse_line(const char *text, ParsedLine *out);

#endif // PARSER_H
