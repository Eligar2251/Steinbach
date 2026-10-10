#!/bin/bash
# Автотесты логики и интерфейса игры (Linux, gcc).
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build

LOGIC_SRC="src/rng.c src/names.c src/worldgen.c src/items.c src/units.c src/towns.c \
           src/econ.c src/contracts.c src/events.c src/travel.c src/save.c src/game_logic.c"

echo "== Тесты игровой логики =="
gcc -std=c11 -Wall -O1 -o build/test_core tests/test_core.c $LOGIC_SRC -lm
./build/test_core

echo "== Смоук-тест интерфейса (заглушка raylib) =="
[ -f src/assets_gen.c ] || python3 tools/embed_assets.py > src/assets_gen.c
gcc -std=c11 -O1 -I third_party/raylib_src -I src -o build/test_ui \
    tests/test_ui.c tests/stub_raylib.c src/ui.c src/render.c src/assets_lookup.c \
    src/assets_gen.c $LOGIC_SRC -lm
./build/test_ui

echo "== Все тесты пройдены =="
