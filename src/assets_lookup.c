/* assets_lookup.c — поиск ресурса по имени. */
#include "assets_gen.h"
#include <string.h>

const unsigned char* asset_get(const char* name, unsigned int* out_len) {
    for (int i = 0; i < g_assets_count; i++) {
        if (strcmp(g_assets[i].name, name) == 0) {
            if (out_len) *out_len = *g_assets[i].len;
            return g_assets[i].data;
        }
    }
    if (out_len) *out_len = 0;
    return 0;
}
