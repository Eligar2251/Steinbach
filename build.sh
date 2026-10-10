#!/bin/bash
# ==========================================================================
#  Сборка игры «Steinbach» в Windows-исполняемый файл (Steinbach.exe).
#
#  Требования: python3 (+ Pillow, numpy — только для генерации текстур),
#  компилятор Zig (pip install ziglang).
#
#  Запуск:  ./build.sh          →  release/Steinbach.exe
# ==========================================================================
set -euo pipefail
cd "$(dirname "$0")"

ZIG="${ZIG:-python3 -m ziglang}"
TARGET="x86_64-windows-gnu"
BUILD=build/win
mkdir -p "$BUILD" release

echo "== [1/4] Генерация текстур =="
python3 tools/gen_textures.py

echo "== [2/4] Встраивание ресурсов =="
python3 tools/embed_assets.py > src/assets_gen.c

echo "== [3/4] Сборка raylib (статическая библиотека) =="
RL_SRC=third_party/raylib_src
RL_OBJS=""
for f in rcore rshapes rtextures rtext rmodels raudio rglfw utils; do
    OBJ="$BUILD/rl_$f.o"
    if [ ! -f "$OBJ" ] || [ "$RL_SRC/$f.c" -nt "$OBJ" ]; then
        echo "   CC  $f.c"
        $ZIG cc -target "$TARGET" -O2 \
            -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 \
            -ffunction-sections -fdata-sections \
            -Wno-missing-braces -Wno-unused-but-set-variable \
            -I "$RL_SRC/external/glfw/include" \
            -c "$RL_SRC/$f.c" -o "$OBJ"
    fi
    RL_OBJS="$RL_OBJS $OBJ"
done
$ZIG ar rcs "$BUILD/libraylib.a" $RL_OBJS

echo "== [4/4] Компиляция игры → Steinbach.exe =="
GAME_SRCS="src/main.c src/ui.c src/render.c src/game_logic.c src/rng.c src/names.c \
           src/worldgen.c src/items.c src/units.c src/towns.c src/econ.c \
           src/contracts.c src/events.c src/travel.c src/save.c \
           src/assets_lookup.c src/assets_gen.c"
$ZIG cc -target "$TARGET" -O2 -std=gnu11 \
    -ffunction-sections -fdata-sections \
    -I "$RL_SRC" -I src \
    $GAME_SRCS "$BUILD/libraylib.a" \
    -o release/Steinbach.exe \
    -Wl,--subsystem,windows -Wl,--gc-sections -s \
    -lgdi32 -lwinmm -lopengl32 -lshell32 -lm

ls -la release/Steinbach.exe
echo "Готово: release/Steinbach.exe"
