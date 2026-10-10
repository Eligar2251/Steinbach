#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Встраивает текстуры (PNG) и шрифты (TTF) в C-исходник.
Запуск: python3 tools/embed_assets.py > src/assets_gen.c
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEX = os.path.join(ROOT, "assets", "textures")
FONTS = os.path.join(ROOT, "assets", "fonts")

def c_ident(name):
    s = "ASSET_" + name.upper().replace(".", "_").replace("-", "_").replace(" ", "_")
    return s

def emit(path, varname):
    data = open(path, "rb").read()
    print(f"const unsigned char {varname}[{len(data)}] = {{")
    for i in range(0, len(data), 20):
        chunk = data[i:i + 20]
        print("    " + ",".join(f"0x{b:02x}" for b in chunk) + ",")
    print("};")
    print(f"const unsigned int {varname}_LEN = {len(data)};")
    print()

def main():
    print("/* assets_gen.c — СГЕНЕРИРОВАНО автоматически (tools/embed_assets.py). */")
    print("#include \"assets_gen.h\"")
    print()

    entries = []
    for fn in sorted(os.listdir(TEX)):
        if not fn.endswith(".png"):
            continue
        var = c_ident(fn)
        emit(os.path.join(TEX, fn), var)
        entries.append((fn[:-4], var))

    font_vars = {
        "DejaVuSans.ttf": "ASSET_FONT_SANS",
        "DejaVuSans-Bold.ttf": "ASSET_FONT_SANS_BOLD",
        "DejaVuSerif-Bold.ttf": "ASSET_FONT_SERIF_BOLD",
    }
    for fn, var in font_vars.items():
        p = os.path.join(FONTS, fn)
        if os.path.exists(p):
            emit(p, var)
            entries.append((var.lower().replace("asset_", ""), var))

    print("const EmbeddedAsset g_assets[] = {")
    for name, var in entries:
        print(f"    {{ \"{name}\", {var}, &{var}_LEN }},")
    print("};")
    print(f"const int g_assets_count = {len(entries)};")

if __name__ == "__main__":
    main()
