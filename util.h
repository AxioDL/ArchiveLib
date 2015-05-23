#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <zlib.h>

#define BUF_SIZ (512 * 1024)

#ifdef __cplusplus
extern "C" {
#endif

size_t util_compress(const void* src, size_t src_len, void* dst, int32_t level);

size_t util_decompress(const void* src, size_t src_len, void* dst, size_t dst_len);

void gen_random(char *s, const int len);

void dump_pak(char* input, char* output, bool verbose);
void make_pak(char* input, char* output, bool compress, bool verbose);
void print_pak_info(char* input);

#ifdef __cplusplus
}
#endif

#endif // UTIL_H

