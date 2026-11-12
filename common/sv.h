#ifndef SV_H_
#define SV_H_

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

typedef struct {
    const char *data;
    size_t count;
} String_View;

static inline String_View sv_from_parts(const char *data, size_t count) {
    String_View sv;
    sv.count = count;
    sv.data = data;
    return sv;
}

static inline size_t sv_find(String_View sv, char ch) {
    for (size_t i = 0; i < sv.count; i++) {
        if (sv.data[i] == ch) return i;
    }
    return sv.count;
}

static inline String_View sv_from_cstr(const char *s) {
    return sv_from_parts(s, strlen(s));
}

static inline void sv_shift_by_n(String_View *sv, size_t n) {
    if (n > sv->count) {
        sv->data += sv->count;
        sv->count = 0;
        return;
    }
    sv->data += n;
    sv->count -= n;
}

static inline bool char_in_string(char c, const char* delim) {
    const char *p_delim = delim;
    while (*p_delim) {
        if (*p_delim == c) {
            return true;
        }
        p_delim++;
    }
    return false;
}

static inline void sv_trim_left(String_View *sv) {
    while (sv->count > 0 && isspace((unsigned char)sv->data[0])) {
        sv->data++;
        sv->count--;
    }
}

static inline void sv_trim_right(String_View *sv) {
    while (sv->count > 0 && isspace((unsigned char)sv->data[sv->count - 1])) {
        sv->count--;
    }
}

static inline void sv_trim(String_View *sv) {
    sv_trim_left(sv);
    sv_trim_right(sv);
}

static inline bool sv_eq_cstr(String_View sv, const char *cstr) {
    size_t n = strlen(cstr);
    return sv.count == n && memcmp(sv.data, cstr, n) == 0;
}

static inline void sv_print(String_View sv) {
    for (size_t i = 0; i < sv.count; i++) printf("%c", sv.data[i]);
}

static inline char *shift(int *argc, char ***argv) {
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

#endif // SV_H_
