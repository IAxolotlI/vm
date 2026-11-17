#ifndef NANBOX_H
#define NANBOX_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define NB_QNAN_BASE   0xFFF8000000000000ULL
#define NB_TAG_MASK    0xFFFF000000000000ULL
#define NB_VALUE_MASK  0x0000FFFFFFFFFFFFULL

#define NB_TAG_INT     (NB_QNAN_BASE | (1ULL << 48))
#define NB_TAG_PTR     (NB_QNAN_BASE | (2ULL << 48))
#define NB_TAG_NULL    (NB_QNAN_BASE | (3ULL << 48))

typedef uint64_t Word;

static inline bool nb_is_double(Word w) {
    return w < NB_TAG_INT;
}

static inline bool nb_is_int(Word w) {
    return (w & NB_TAG_MASK) == NB_TAG_INT;
}

static inline bool nb_is_ptr(Word w) {
    return (w & NB_TAG_MASK) == NB_TAG_PTR;
}

static inline bool nb_is_null(Word w) {
    return (w & NB_TAG_MASK) == NB_TAG_NULL;
}

static inline Word nb_box_double(double d) {
    Word w;
    memcpy(&w, &d, sizeof(w));
    if (!nb_is_double(w)) return 0x7FF8000000000000ULL;
    return w;
}

static inline double nb_as_double(Word w) {
    double d;
    memcpy(&d, &w, sizeof(d));
    return d;
}

static inline Word nb_box_int(int64_t i) {
    return NB_TAG_INT | ((uint64_t)i & NB_VALUE_MASK);
}

static inline int64_t nb_as_int(Word w) {
    uint64_t val = w & NB_VALUE_MASK;
    if (val & (1ULL << 47)) {
        val |= ~NB_VALUE_MASK;
    }
    return (int64_t)val;
}

static inline Word nb_box_ptr(void *ptr) {
    return NB_TAG_PTR | ((uint64_t)(uintptr_t)ptr & NB_VALUE_MASK);
}

static inline void *nb_as_ptr(Word w) {
    return (void *)(uintptr_t)(w & NB_VALUE_MASK);
}

#endif // NANBOX_H
