/* assets_gen.h — доступ к встроенным ресурсам. */
#ifndef ASSETS_GEN_H
#define ASSETS_GEN_H

typedef struct {
    const char* name;
    const unsigned char* data;
    const unsigned int* len;
} EmbeddedAsset;

extern const EmbeddedAsset g_assets[];
extern const int g_assets_count;

const unsigned char* asset_get(const char* name, unsigned int* out_len);

#endif
