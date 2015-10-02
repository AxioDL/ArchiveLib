#include "util.h"
#include <time.h>

#define CHUNK 16384

size_t util_compress(const void* src, size_t src_len, void* dst, int32_t level) {
    int32_t ret;
    size_t dst_len = src_len;

    ret = compress2(dst, &dst_len, src, src_len, level);
    if (ret != Z_OK)
        return -1;

    return dst_len;
}


size_t util_decompress(const void* src, size_t src_len, void* dst, size_t dst_len)
{
    int32_t ret;
    ret = uncompress(dst, &dst_len, src, src_len);
    if (ret != Z_OK)
        return ret;

    return dst_len;
}

void gen_random(char *s, const int len)
{
    srand(time(NULL));
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}
