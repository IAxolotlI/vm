#ifndef FORMAT_H
#define FORMAT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define GM_HEADER_SIZE    24
#define GM_FORMAT_VERSION 1

static const uint8_t GM_MAGIC[4] = {'G', 'M', 0, 0};

static inline uint32_t gm_get_u32(const uint8_t *p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static inline uint64_t gm_get_u64(const uint8_t *p) {
    return (uint64_t)p[0]
         | ((uint64_t)p[1] << 8)
         | ((uint64_t)p[2] << 16)
         | ((uint64_t)p[3] << 24)
         | ((uint64_t)p[4] << 32)
         | ((uint64_t)p[5] << 40)
         | ((uint64_t)p[6] << 48)
         | ((uint64_t)p[7] << 56);
}

static inline void gm_put_u32(uint8_t *p, uint32_t value) {
    for (size_t i = 0; i < 4; i++) {
        p[i] = (uint8_t)(value >> (8 * i));
    }
}

static inline void gm_put_u64(uint8_t *p, uint64_t value) {
    for (size_t i = 0; i < 8; i++) {
        p[i] = (uint8_t)(value >> (8 * i));
    }
}

static inline bool gm_write_header(FILE *out, uint64_t entry, uint64_t code_len) {
    uint8_t header[GM_HEADER_SIZE] = {0};
    memcpy(header, GM_MAGIC, sizeof(GM_MAGIC));
    gm_put_u32(header + 4, GM_FORMAT_VERSION);
    gm_put_u64(header + 8, entry);
    gm_put_u64(header + 16, code_len);
    return fwrite(header, 1, sizeof(header), out) == sizeof(header);
}

#endif // FORMAT_H
