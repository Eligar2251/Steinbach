#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Генератор всех текстур игры «Steinbach» (стиль Battle Brothers).
Все арты рисуются процедурно: тайлы местности, иконки построек,
портреты наёмников, элементы интерфейса, заставка.

Запуск:  python3 tools/gen_textures.py
Выход:   assets/textures/*.png
"""
import os
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "assets", "textures")
os.makedirs(OUT, exist_ok=True)

RNG = np.random.default_rng(20261010)

# ---------------------------------------------------------------- шумы -----

def value_noise(w, h, scale, seed, octaves=1, persistence=0.5):
    """Мягкий value-noise на numpy (билинейная интерполяция решёток)."""
    rng = np.random.default_rng(seed)
    total = np.zeros((h, w), dtype=np.float32)
    amp = 1.0
    amp_sum = 0.0
    sc = scale
    for _ in range(octaves):
        gw = max(2, int(w / sc) + 2)
        gh = max(2, int(h / sc) + 2)
        grid = rng.random((gh, gw), dtype=np.float32)
        ys = np.linspace(0, gh - 1, h, dtype=np.float32)
        xs = np.linspace(0, gw - 1, w, dtype=np.float32)
        y0 = np.floor(ys).astype(int)
        x0 = np.floor(xs).astype(int)
        y1 = np.minimum(y0 + 1, gh - 1)
        x1 = np.minimum(x0 + 1, gw - 1)
        fy = (ys - y0)[:, None]
        fx = (xs - x0)[None, :]
        fy = fy * fy * (3 - 2 * fy)
        fx = fx * fx * (3 - 2 * fx)
        g00 = grid[np.ix_(y0, x0)]
        g01 = grid[np.ix_(y0, x1)]
        g10 = grid[np.ix_(y1, x0)]
        g11 = grid[np.ix_(y1, x1)]
        top = g00 * (1 - fx) + g01 * fx
        bot = g10 * (1 - fx) + g11 * fx
        total += amp * (top * (1 - fy) + bot * fy)
        amp_sum += amp
        amp *= persistence
        sc = max(2.0, sc / 2)
    return total / amp_sum


def hex2rgb(c):
    c = c.lstrip("#")
    return np.array([int(c[i:i + 2], 16) for i in (0, 2, 4)], dtype=np.float32)


def mix(a, b, t):
    """a,b: (3,) или (h,w,3); t: (h,w) в [0,1]."""
    if isinstance(a, str):
        a = hex2rgb(a)
    if isinstance(b, str):
        b = hex2rgb(b)
    if a.ndim == 1:
        a = a[None, None, :]
    if b.ndim == 1:
        b = b[None, None, :]
    if np.isscalar(t):
        t = np.full(a.shape[:2], float(t), dtype=np.float32)
    t = t[..., None]
    return a * (1 - t) + b * t


def to_img(arr):
    return Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), "RGB")


def to_rgba(arr, alpha):
    rgb = np.clip(arr, 0, 255).astype(np.uint8)
    a = np.clip(alpha, 0, 255).astype(np.uint8)
    return Image.fromarray(np.dstack([rgb, a]), "RGBA")


# --------------------------------------------------------------- тайлы -----
# Плоский стиль в духе Kenney (medieval-rts): ровные цвета, простые формы.

TILE = 64

def tile_shade(base, seed=0, veg=0):
    """Совместимость: плоский тайл с лёгким шумом (hex или уже готовое изображение)."""
    if isinstance(base, str):
        w = h = TILE
        img = np.zeros((h, w, 3), dtype=np.float32)
        img[:] = hex2rgb(base)
    else:
        img = np.array(base, dtype=np.float32)
        h, w = img.shape[:2]
    v = value_noise(w, h, 10, seed or 1, 2) - 0.5
    img += v[..., None] * 7
    return img

def save_tile(name, arr):
    to_img(arr).save(os.path.join(OUT, name))

def flat(base, seed=0):
    """Ровный цвет + лёгкие вариации, чтобы тайл не был «пластиковым»."""
    w = h = TILE
    img = np.zeros((h, w, 3), dtype=np.float32)
    img[:] = hex2rgb(base)
    v = value_noise(w, h, 10, seed or 1, 2) - 0.5
    img += v[..., None] * 7
    return img

def draw_on(arr, fn):
    img = to_img(arr).convert('RGBA')
    d = ImageDraw.Draw(img)
    fn(d)
    return np.array(img.convert('RGB'))

def gen_grass():
    arr = flat('#5aa84e', 31)
    def f(d):
        rng = np.random.default_rng(34)
        for _ in range(14):
            x, y = rng.integers(4, 60, 2)
            c = (46, 122, 58, 255) if rng.random() < 0.5 else (94, 172, 80, 255)
            d.line([x, y, x + int(rng.integers(-1, 2)), y - 3], fill=c, width=2)
    return draw_on(arr, f)

def gen_dirt():
    arr = flat('#b07f4e', 91)
    def f(d):
        rng = np.random.default_rng(92)
        for _ in range(10):
            x, y = rng.integers(4, 60, 2)
            r = int(rng.integers(1, 3))
            d.ellipse([x - r, y - r, x + r, y + r], fill=(139, 99, 62, 255))
    return draw_on(arr, f)

def gen_sand():
    arr = flat('#d9c48e', 21)
    def f(d):
        rng = np.random.default_rng(22)
        for _ in range(9):
            x, y = rng.integers(4, 60, 2)
            d.point((x, y), fill=(168, 143, 92, 255))
    return draw_on(arr, f)

def gen_water():
    arr = flat('#4f8fc4', 12)
    def f(d):
        for i, y in enumerate((14, 32, 50)):
            d.arc([6 + i * 4, y, 34 + i * 4, y + 12], 200, 340, fill=(110, 172, 214, 255), width=3)
            d.arc([30 - i * 3, y + 6, 58 - i * 3, y + 18], 200, 340, fill=(88, 152, 198, 255), width=3)
    return draw_on(arr, f)

def gen_deep():
    arr = flat('#3a6d9c', 11)
    def f(d):
        d.arc([10, 20, 50, 40], 200, 340, fill=(72, 130, 176, 255), width=3)
        d.arc([4, 38, 44, 58], 200, 340, fill=(72, 130, 176, 255), width=3)
    return draw_on(arr, f)

def gen_forest():
    """Трава с подлеском — деревья рисуются спрайтами поверх."""
    arr = flat('#4d8f42', 41)
    def f(d):
        rng = np.random.default_rng(43)
        for _ in range(12):
            x, y = rng.integers(4, 60, 2)
            d.ellipse([x - 3, y - 2, x + 3, y + 2], fill=(56, 110, 52, 255))
    return draw_on(arr, f)

def gen_hills():
    arr = flat('#8fa45c', 51)
    def f(d):
        d.arc([8, 26, 36, 50], 180, 360, fill=(122, 142, 78, 255), width=3)
        d.arc([28, 16, 60, 44], 180, 360, fill=(122, 142, 78, 255), width=3)
    return draw_on(arr, f)

def gen_mountain():
    arr = flat('#8c8c8c', 61)
    def f(d):
        d.polygon([(10, 52), (24, 18), (38, 52)], fill=(122, 122, 122, 255))
        d.polygon([(24, 18), (30, 30), (18, 30)], fill=(214, 222, 230, 255))
        d.polygon([(30, 56), (46, 22), (60, 56)], fill=(106, 106, 106, 255))
        d.polygon([(46, 22), (52, 34), (40, 34)], fill=(214, 222, 230, 255))
    return draw_on(arr, f)

def gen_swamp():
    arr = flat('#5d7a4c', 71)
    def f(d):
        d.ellipse([8, 14, 30, 30], fill=(72, 106, 78, 255))
        d.ellipse([34, 36, 58, 54], fill=(72, 106, 78, 255))
        d.line([18, 46, 18, 56], fill=(104, 128, 72, 255), width=2)
        d.line([46, 16, 46, 26], fill=(104, 128, 72, 255), width=2)
    return draw_on(arr, f)

def gen_snow():
    arr = flat('#dfe6ee', 81)
    def f(d):
        rng = np.random.default_rng(82)
        for _ in range(8):
            x, y = rng.integers(4, 60, 2)
            d.point((x, y), fill=(255, 255, 255, 255))
        d.arc([12, 30, 40, 52], 180, 360, fill=(196, 208, 222, 255), width=3)
    return draw_on(arr, f)

# --------------------------------------------------------------- иконки ----
# Иконки построек/локаций — с альфа-каналом.

def canvas(sz=96):
    img = Image.new("RGBA", (sz, sz), (0, 0, 0, 0))
    return img, ImageDraw.Draw(img)


def house(d, x, y, w, h, roof="#7a3b2e", wall="#b08a5f", dark="#5e3327"):
    d.rectangle([x, y, x + w, y + h], fill=wall, outline=(46, 32, 22, 255), width=2)
    d.polygon([(x - 3, y), (x + w + 3, y), (x + w // 2, y - h * 0.7)], fill=roof,
              outline=(46, 32, 22, 255))
    d.line([x - 3, y, x + w // 2, y - h * 0.7, x + w + 3, y], fill=dark, width=2)
    d.rectangle([x + w // 2 - 3, y + h // 2, x + w // 2 + 3, y + h], fill=(62, 42, 28, 255))
    d.rectangle([x + 3, y + 4, x + 3 + w // 3, y + 4 + h // 4], fill=(196, 176, 120, 255))


def icon_village():
    img, d = canvas()
    d.ellipse([6, 70, 90, 88], fill=(70, 92, 48, 255))
    house(d, 16, 42, 26, 24)
    house(d, 52, 38, 28, 28, roof="#864432")
    d.line([12, 66, 84, 62], fill=(110, 84, 54, 255), width=4)
    return img


def icon_town():
    img, d = canvas()
    d.ellipse([4, 72, 92, 90], fill=(70, 92, 48, 255))
    # частокол
    for x in range(8, 90, 8):
        d.line([x, 44, x, 72], fill=(96, 70, 44, 255), width=4)
        d.polygon([(x - 2, 44), (x + 2, 44), (x, 38)], fill=(96, 70, 44, 255))
    d.line([6, 58, 90, 58], fill=(110, 82, 52, 255), width=3)
    house(d, 28, 30, 30, 28, roof="#6e3629")
    house(d, 62, 38, 18, 20, roof="#7a3b2e")
    return img


def icon_city():
    img, d = canvas()
    d.ellipse([2, 74, 94, 92], fill=(70, 92, 48, 255))
    # стены
    d.rectangle([8, 48, 88, 78], fill=(122, 114, 102, 255), outline=(52, 48, 42, 255), width=2)
    for x in range(8, 89, 12):
        d.rectangle([x, 42, x + 8, 48], fill=(122, 114, 102, 255), outline=(52, 48, 42, 255), width=1)
    # башни
    for tx in (12, 68):
        d.rectangle([tx, 26, tx + 16, 78], fill=(134, 126, 114, 255), outline=(52, 48, 42, 255), width=2)
        d.polygon([(tx - 3, 26), (tx + 19, 26), (tx + 8, 12)], fill=(96, 52, 40, 255),
                  outline=(52, 32, 24, 255))
    # донжон
    d.rectangle([38, 18, 58, 78], fill=(146, 138, 126, 255), outline=(52, 48, 42, 255), width=2)
    d.rectangle([36, 12, 60, 20], fill=(122, 114, 102, 255), outline=(52, 48, 42, 255), width=2)
    d.rectangle([44, 58, 52, 78], fill=(62, 42, 28, 255))
    d.line([48, 12, 48, 2], fill=(52, 42, 28, 255), width=2)
    d.polygon([(48, 2), (64, 6), (48, 10)], fill=(160, 46, 40, 255))
    return img


def icon_castle():
    img, d = canvas()
    d.ellipse([2, 76, 94, 92], fill=(70, 92, 48, 255))
    d.rectangle([14, 40, 82, 80], fill=(128, 122, 112, 255), outline=(52, 48, 42, 255), width=2)
    for tx in (8, 38, 70):
        d.rectangle([tx, 20, tx + 18, 80], fill=(140, 134, 124, 255), outline=(52, 48, 42, 255), width=2)
        for c in range(3):
            d.rectangle([tx + c * 7, 14, tx + c * 7 + 5, 20], fill=(140, 134, 124, 255),
                        outline=(52, 48, 42, 255), width=1)
        d.rectangle([tx + 5, 30, tx + 13, 42], fill=(70, 58, 48, 255))
    d.rectangle([44, 58, 54, 80], fill=(62, 42, 28, 255))
    d.line([47, 14, 47, 2], fill=(52, 42, 28, 255), width=2)
    d.polygon([(47, 2), (66, 7), (47, 12)], fill=(160, 46, 40, 255))
    return img


def icon_ruins():
    img, d = canvas()
    d.ellipse([6, 72, 90, 90], fill=(70, 92, 48, 255))
    d.rectangle([18, 30, 34, 76], fill=(122, 116, 106, 255), outline=(58, 54, 48, 255), width=2)
    d.rectangle([56, 42, 72, 76], fill=(112, 106, 96, 255), outline=(58, 54, 48, 255), width=2)
    d.rectangle([30, 52, 60, 76], fill=(118, 112, 102, 255), outline=(58, 54, 48, 255), width=2)
    # сломанные верхушки
    d.polygon([(18, 30), (24, 22), (28, 30), (34, 26), (34, 34), (18, 34)], fill=(122, 116, 106, 255),
              outline=(58, 54, 48, 255))
    d.polygon([(56, 42), (62, 36), (68, 44), (72, 40), (72, 48), (56, 48)], fill=(112, 106, 96, 255),
              outline=(58, 54, 48, 255))
    # трещины + плющ
    d.line([24, 40, 28, 70], fill=(84, 80, 72, 255), width=1)
    d.line([62, 52, 66, 72], fill=(84, 80, 72, 255), width=1)
    d.line([20, 34, 22, 60], fill=(66, 96, 52, 200), width=2)
    return img


def icon_camp():
    img, d = canvas()
    d.ellipse([4, 72, 92, 90], fill=(70, 92, 48, 255))
    # палатка
    d.polygon([(46, 26), (18, 74), (74, 74)], fill=(122, 96, 66, 255), outline=(58, 42, 28, 255))
    d.polygon([(46, 30), (34, 72), (58, 72)], fill=(96, 72, 48, 255))
    d.polygon([(46, 46), (38, 72), (54, 72)], fill=(36, 26, 18, 255))
    # костёр
    d.ellipse([62, 62, 82, 74], fill=(90, 70, 48, 255))
    d.polygon([(72, 46), (66, 64), (78, 64)], fill=(214, 108, 42, 255))
    d.polygon([(72, 52), (68, 63), (76, 63)], fill=(240, 170, 64, 255))
    # череп на шесте
    d.line([16, 30, 16, 70], fill=(90, 70, 48, 255), width=3)
    d.ellipse([10, 18, 22, 32], fill=(216, 208, 188, 255), outline=(60, 52, 40, 255))
    d.rectangle([13, 30, 19, 36], fill=(216, 208, 188, 255), outline=(60, 52, 40, 255))
    d.point((13, 24), fill=(20, 16, 12, 255))
    d.point((18, 24), fill=(20, 16, 12, 255))
    return img


def icon_mine():
    img, d = canvas()
    d.ellipse([4, 72, 92, 90], fill=(70, 92, 48, 255))
    # холм
    d.polygon([(8, 78), (28, 34), (66, 34), (88, 78)], fill=(110, 102, 92, 255),
              outline=(58, 52, 46, 255))
    # вход
    d.pieslice([32, 46, 62, 82], 180, 360, fill=(40, 30, 22, 255), outline=(58, 52, 46, 255))
    # рельсы
    d.line([38, 82, 58, 82], fill=(120, 88, 52, 255), width=3)
    # кирка + кристалл
    d.line([18, 44, 34, 28], fill=(90, 74, 52, 255), width=3)
    d.arc([14, 18, 42, 40], 200, 340, fill=(160, 158, 150, 255), width=4)
    d.polygon([(76, 40), (82, 30), (88, 42), (80, 50)], fill=(168, 196, 216, 255),
              outline=(80, 110, 140, 255))
    return img


def icon_shrine():
    img, d = canvas()
    d.ellipse([6, 74, 90, 90], fill=(70, 92, 48, 255))
    d.rectangle([28, 66, 68, 80], fill=(130, 124, 114, 255), outline=(58, 52, 46, 255), width=2)
    d.rectangle([34, 24, 62, 68], fill=(142, 136, 126, 255), outline=(58, 52, 46, 255), width=2)
    d.polygon([(30, 24), (66, 24), (48, 8)], fill=(120, 114, 104, 255), outline=(58, 52, 46, 255))
    d.ellipse([42, 34, 54, 48], fill=(196, 176, 110, 255), outline=(110, 92, 48, 255))
    d.line([48, 52, 48, 64], fill=(196, 176, 110, 255), width=3)
    d.line([42, 58, 54, 58], fill=(196, 176, 110, 255), width=3)
    return img


def icon_party():
    img, d = canvas()
    # знамя отряда
    d.line([46, 12, 46, 86], fill=(96, 72, 46, 255), width=5)
    d.polygon([(48, 14), (88, 22), (48, 44)], fill=(150, 42, 38, 255), outline=(70, 22, 20, 255))
    d.ellipse([38, 6, 54, 20], fill=(188, 158, 82, 255), outline=(96, 72, 36, 255))
    # щит
    d.polygon([(24, 44), (44, 44), (44, 70), (34, 82), (24, 70)], fill=(122, 116, 106, 255),
              outline=(58, 52, 46, 255))
    d.line([34, 46, 34, 78], fill=(150, 42, 38, 255), width=3)
    d.line([26, 58, 42, 58], fill=(150, 42, 38, 255), width=3)
    return img


def icon_farm():
    img, d = canvas()
    d.ellipse([4, 72, 92, 90], fill=(70, 92, 48, 255))
    # амбар
    d.rectangle([20, 38, 58, 76], fill=(148, 82, 52, 255), outline=(58, 36, 26, 255), width=2)
    d.polygon([(16, 40), (40, 18), (62, 40)], fill=(96, 56, 38, 255), outline=(58, 36, 26, 255))
    d.rectangle([32, 54, 46, 76], fill=(70, 42, 28, 255))
    # поле
    for i in range(6):
        d.line([62 + i * 5, 52, 62 + i * 5, 80], fill=(176, 152, 82, 255), width=2)
    # солнце
    d.ellipse([70, 12, 86, 28], fill=(226, 190, 96, 255))
    return img


def icon_bandit():
    img, d = canvas()
    # череп с перевязью
    d.ellipse([22, 18, 74, 66], fill=(214, 206, 186, 255), outline=(60, 52, 40, 255), width=2)
    d.rectangle([34, 58, 62, 76], fill=(214, 206, 186, 255), outline=(60, 52, 40, 255), width=2)
    d.ellipse([32, 32, 44, 46], fill=(24, 18, 14, 255))
    d.ellipse([52, 32, 64, 46], fill=(24, 18, 14, 255))
    d.polygon([(46, 46), (42, 56), (50, 56)], fill=(24, 18, 14, 255))
    for x in (38, 46, 54):
        d.line([x, 60, x, 74], fill=(60, 52, 40, 255), width=1)
    # скрещённые кости
    d.line([10, 84, 86, 20], fill=(214, 206, 186, 255), width=7)
    d.line([10, 20, 86, 84], fill=(214, 206, 186, 255), width=7)
    return img


# -------------------------------------------------------------- портреты ---
# Пиксельные портреты наёмников 96x96.

SKINS = ["#e8c39e", "#d9a878", "#c08a5e", "#a4714c", "#e0b090", "#8a5a3c"]
HAIRS = ["#2b1c12", "#4a2e18", "#6b4423", "#8a5c28", "#b08040", "#5a5a5a", "#8a8a8a", "#c8b090"]


def gen_portrait(idx, helmet=False, beard=0.0, hood=False):
    rng = np.random.default_rng(500 + idx)
    sz = 96
    img, d = canvas(sz)
    bg = ["#4a3b2c", "#3c4450", "#54422f", "#42503c", "#4e3a3a"][idx % 5]
    d.rectangle([0, 0, sz, sz], fill=bg)
    # виньетка
    for i in range(12):
        d.rectangle([i, i, sz - i, sz - i], outline=None)
    skin = SKINS[int(rng.integers(0, len(SKINS)))]
    hair = HAIRS[int(rng.integers(0, len(HAIRS)))]
    cx = sz // 2
    # шея/плечи
    d.rectangle([cx - 14, 66, cx + 14, 82], fill=skin)
    d.polygon([(8, 96), (24, 72), (cx, 80), (72, 72), (88, 96)], fill=(70, 52, 40, 255))
    d.polygon([(30, 96), (cx, 78), (66, 96)], fill=(90, 68, 50, 255))
    # голова
    d.ellipse([cx - 22, 20, cx + 22, 72], fill=skin, outline=(60, 40, 28, 255))
    # уши
    d.ellipse([cx - 27, 42, cx - 17, 54], fill=skin, outline=(60, 40, 28, 255))
    d.ellipse([cx + 17, 42, cx + 27, 54], fill=skin, outline=(60, 40, 28, 255))
    # волосы
    if not helmet and not hood:
        d.pieslice([cx - 24, 12, cx + 24, 56], 180, 360, fill=hair)
        d.polygon([(cx - 24, 34), (cx - 18, 20), (cx - 10, 30), (cx - 2, 18), (cx + 8, 28),
                   (cx + 18, 18), (cx + 24, 34), (cx + 24, 24), (cx - 24, 24)], fill=hair)
    if helmet:
        d.pieslice([cx - 25, 12, cx + 25, 58], 180, 360, fill=(110, 110, 118, 255),
                   outline=(56, 56, 62, 255))
        d.rectangle([cx - 25, 34, cx + 25, 40], fill=(90, 90, 98, 255), outline=(56, 56, 62, 255))
        d.rectangle([cx - 3, 20, cx + 3, 40], fill=(130, 130, 140, 255))
    if hood:
        d.pieslice([cx - 30, 8, cx + 30, 70], 180, 360, fill=(70, 58, 48, 255))
        d.pieslice([cx - 24, 18, cx + 24, 66], 180, 360, fill=(92, 76, 60, 255))
    # брови + глаза
    brow = (40, 28, 18, 255)
    d.line([cx - 16, 42, cx - 6, 40], fill=brow, width=3)
    d.line([cx + 6, 40, cx + 16, 42], fill=brow, width=3)
    eye_w = int(rng.integers(3, 5))
    d.ellipse([cx - 14, 45, cx - 14 + eye_w + 3, 50], fill=(240, 238, 230, 255))
    d.ellipse([cx + 11 - 1, 45, cx + 11 + eye_w + 1, 50], fill=(240, 238, 230, 255))
    d.point((cx - 11, 47), fill=(30, 40, 60, 255))
    d.point((cx + 12, 47), fill=(30, 40, 60, 255))
    # нос
    d.line([cx, 48, cx - 2, 58], fill=(0, 0, 0, 70), width=2)
    d.line([cx - 2, 58, cx + 3, 58], fill=(0, 0, 0, 70), width=1)
    # рот
    mood = rng.random()
    if mood < 0.3:
        d.line([cx - 7, 64, cx + 7, 64], fill=(110, 60, 50, 255), width=2)
    elif mood < 0.7:
        d.arc([cx - 8, 58, cx + 8, 68], 20, 160, fill=(110, 60, 50, 255), width=2)
    else:
        d.arc([cx - 8, 60, cx + 8, 70], 200, 340, fill=(110, 60, 50, 255), width=2)
    # борода
    if rng.random() < beard:
        d.pieslice([cx - 22, 40, cx + 22, 86], 0, 180, fill=hair)
        d.rectangle([cx - 22, 46, cx + 22, 64], fill=hair)
        # прорисуем лицо поверх (глаза/нос/рот)
        d.ellipse([cx - 14, 45, cx - 14 + eye_w + 3, 50], fill=(240, 238, 230, 255))
        d.ellipse([cx + 10, 45, cx + 11 + eye_w + 1, 50], fill=(240, 238, 230, 255))
        d.point((cx - 11, 47), fill=(30, 40, 60, 255))
        d.point((cx + 12, 47), fill=(30, 40, 60, 255))
        d.line([cx, 48, cx - 2, 58], fill=(0, 0, 0, 70), width=2)
        d.line([cx - 7, 62, cx + 7, 62], fill=(110, 60, 50, 255), width=2)
        if not helmet and not hood:
            d.pieslice([cx - 24, 12, cx + 24, 56], 180, 360, fill=hair)
    # лёгкий шум
    arr = np.array(img).astype(np.float32)
    n = value_noise(sz, sz, 6, 600 + idx, 2)[..., None] - 0.5
    arr[..., :3] += n * 12
    img = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), "RGBA")
    return img


# ------------------------------------------------------------------ UI -----

def gen_wood():
    w, h = 128, 128
    ys, xs = np.mgrid[0:h, 0:w]
    grain = value_noise(w, h, 60, 71, 2) * 2 + value_noise(w, h, 4, 72, 3) * 0.5
    rings = np.sin(ys * 0.22 + grain * 7.0) * 0.5 + 0.5
    base = mix("#5e422a", "#42301c", rings)
    base = mix(base, "#6e5034", value_noise(w, h, 30, 73, 2) * 0.35)
    # щели досок
    for y in (32, 64, 96):
        base[y, :] *= 0.55
        base[y + 1, :] *= 0.75
    img = tile_shade(base, 74, 10)
    return to_img(img)


def gen_parchment():
    w, h = 128, 128
    base = mix("#d9c49a", "#c4aa7c", value_noise(w, h, 24, 81, 3))
    spots = value_noise(w, h, 8, 82, 2)
    base = mix(base, "#a88e62", np.clip((0.35 - spots) * 2.4, 0, 1) * 0.5)
    base = mix(base, "#e8d6ae", np.clip((spots - 0.62) * 2.4, 0, 1) * 0.5)
    # края темнее
    ys, xs = np.mgrid[0:h, 0:w]
    edge = np.minimum(np.minimum(xs, w - 1 - xs), np.minimum(ys, h - 1 - ys))
    vign = np.clip(edge / 18.0, 0, 1)
    base = mix(base, "#7a6242", (1 - vign) * 0.55)
    return to_img(tile_shade(base, 83, 8))


def gen_stone_ui():
    w, h = 128, 128
    base = mix("#5c5650", "#46403c", value_noise(w, h, 18, 91, 3))
    ys, xs = np.mgrid[0:h, 0:w]
    blocks = ((xs // 32) + (ys // 21)) % 2
    base = mix(base, "#524c46", blocks * 0.35)
    for y in range(0, h, 21):
        base[y, :] *= 0.6
    for row, y in enumerate(range(0, h, 21)):
        off = 16 if row % 2 else 0
        for x in range(off, w, 32):
            base[y:y + 21, x] *= 0.65
    return to_img(tile_shade(base, 92, 12))


def gen_icon(kind):
    img, d = canvas(48)
    col = (212, 192, 152, 255)
    dark = (52, 40, 28, 255)
    if kind == "hp":
        d.polygon([(24, 42), (6, 22), (6, 14), (13, 8), (21, 10), (24, 15),
                   (27, 10), (35, 8), (42, 14), (42, 22)], fill=(190, 62, 52, 255), outline=dark)
    elif kind == "fatigue":
        d.polygon([(16, 8), (10, 26), (20, 26), (12, 42), (36, 18), (24, 18), (32, 8)],
                  fill=(214, 168, 62, 255), outline=dark)
    elif kind == "resolve":
        d.polygon([(24, 6), (30, 18), (42, 20), (33, 29), (36, 42), (24, 35), (12, 42),
                   (15, 29), (6, 20), (18, 18)], fill=(202, 172, 92, 255), outline=dark)
    elif kind == "initiative":
        d.polygon([(28, 4), (12, 28), (22, 28), (18, 44), (36, 18), (26, 18), (38, 4)],
                  fill=(196, 196, 210, 255), outline=dark)
    elif kind == "matk":
        d.line([10, 40, 36, 10], fill=col, width=5)
        d.polygon([(32, 6), (42, 4), (40, 16)], fill=(200, 200, 210, 255), outline=dark)
        d.line([8, 36, 16, 44], fill=(160, 110, 60, 255), width=4)
    elif kind == "ratk":
        d.arc([6, 6, 42, 42], 300, 60, fill=(150, 104, 56, 255), width=4)
        d.line([38, 12, 14, 38], fill=(150, 104, 56, 255), width=3)
        d.line([10, 42, 40, 8], fill=col, width=2)
        d.polygon([(40, 8), (34, 10), (38, 14)], fill=col)
    elif kind == "mdef":
        d.polygon([(24, 4), (42, 12), (42, 26), (24, 44), (6, 26), (6, 12)],
                  fill=(120, 130, 150, 255), outline=dark)
        d.line([24, 10, 24, 36], fill=(190, 190, 205, 255), width=3)
    elif kind == "rdef":
        d.polygon([(24, 4), (42, 12), (42, 26), (24, 44), (6, 26), (6, 12)],
                  fill=(120, 150, 130, 255), outline=dark)
        d.line([12, 14, 36, 34], fill=(190, 210, 195, 255), width=2)
        d.line([36, 14, 12, 34], fill=(190, 210, 195, 255), width=2)
    elif kind == "armor_head":
        d.pieslice([8, 8, 40, 40], 180, 360, fill=(140, 140, 152, 255), outline=dark)
        d.rectangle([8, 24, 40, 34], fill=(120, 120, 132, 255), outline=dark)
        d.rectangle([18, 26, 30, 34], fill=(50, 50, 58, 255))
    elif kind == "armor_body":
        d.polygon([(14, 8), (34, 8), (42, 18), (38, 42), (10, 42), (6, 18)],
                  fill=(140, 140, 152, 255), outline=dark)
        d.line([24, 10, 24, 40], fill=(90, 90, 102, 255), width=2)
        for y in (18, 26, 34):
            d.line([10, y, 38, y], fill=(90, 90, 102, 255), width=1)
    elif kind == "coin":
        d.ellipse([8, 8, 40, 40], fill=(216, 178, 76, 255), outline=(130, 96, 32, 255), width=3)
        d.ellipse([15, 15, 33, 33], outline=(150, 112, 40, 255), width=2)
        d.line([24, 18, 24, 30], fill=(150, 112, 40, 255), width=2)
    elif kind == "food":
        d.polygon([(24, 6), (34, 22), (34, 38), (24, 44), (14, 38), (14, 22)],
                  fill=(190, 148, 78, 255), outline=dark)
        d.line([24, 8, 24, 42], fill=(150, 108, 56, 255), width=2)
    elif kind == "med":
        d.rectangle([18, 8, 30, 40], fill=(188, 70, 62, 255), outline=dark)
        d.rectangle([8, 18, 40, 30], fill=(188, 70, 62, 255), outline=dark)
    elif kind == "star":
        d.polygon([(24, 4), (30, 18), (44, 20), (33, 29), (36, 44), (24, 36), (12, 44),
                   (15, 29), (4, 20), (18, 18)], fill=(222, 190, 102, 255), outline=dark)
    elif kind == "day":
        d.ellipse([12, 12, 36, 36], fill=(226, 190, 96, 255), outline=dark)
        for a in range(8):
            ang = a * math.pi / 4
            x1 = 24 + math.cos(ang) * 13
            y1 = 24 + math.sin(ang) * 13
            x2 = 24 + math.cos(ang) * 19
            y2 = 24 + math.sin(ang) * 19
            d.line([x1, y1, x2, y2], fill=(226, 190, 96, 255), width=3)
    elif kind == "mood":
        d.ellipse([6, 6, 42, 42], fill=(216, 196, 128, 255), outline=dark)
        d.ellipse([15, 17, 20, 23], fill=(40, 30, 20, 255))
        d.ellipse([28, 17, 33, 23], fill=(40, 30, 20, 255))
        d.arc([15, 24, 33, 38], 0, 180, fill=(40, 30, 20, 255), width=3)
    elif kind == "weight":
        d.polygon([(16, 10), (32, 10), (38, 40), (10, 40)], fill=(150, 150, 162, 255), outline=dark)
        d.arc([17, 2, 31, 18], 180, 360, fill=dark, width=3)
    return img


def gen_title():
    """Заставка главного меню 1280x720 — рисованный пейзаж."""
    w, h = 1280, 720
    ys, xs = np.mgrid[0:h, 0:w]
    # небо
    t = np.clip(ys / (h * 0.62), 0, 1)
    sky = mix("#24324e", "#d98f5a", t)
    cloud = value_noise(w, h, 140, 101, 3)
    sky = mix(sky, "#e8b07a", np.clip((cloud - 0.55) * 1.6, 0, 1) * (0.55 - t * 0.35))
    # солнце
    sun_x, sun_y, sun_r = 640, 420, 120
    dd = ((xs - sun_x) ** 2 + (ys - sun_y) ** 2) ** 0.5
    glow = np.clip(1.0 - dd / (sun_r * 3.2), 0, 1) ** 2
    sky = mix(sky, "#ffd9a0", glow * 0.75)
    core = np.clip(1.0 - dd / sun_r, 0, 1)
    sky = mix(sky, "#ffecc8", core ** 0.7)
    arr = sky
    # слои гор
    def ridge(base_y, amp, colr, seed, sharp=1.6):
        prof = value_noise(w, 1, 220, seed, 4)[0]
        prof = base_y - (prof - 0.5) * amp
        mask = np.clip((ys - prof[None, :]) * 0.9, 0, 1)
        return mix(arr, colr, mask ** sharp)

    arr = ridge(430, 130, "#6a5670", 111, 1.2)
    arr = ridge(480, 150, "#54425c", 112, 1.1)
    arr = ridge(530, 120, "#3c3048", 113, 1.0)
    # лес силуэтами
    tree_row = value_noise(w, 1, 26, 114, 3)[0]
    prof = 570 - tree_row * 60
    mask = np.clip((ys - prof[None, :]) * 1.2, 0, 1)
    arr = mix(arr, "#221c2c", mask)
    # туман
    fog = value_noise(w, h, 100, 115, 3)
    fog_mask = np.clip((fog - 0.52) * 2.0, 0, 1) * np.clip((ys - 380) / 300.0, 0, 1)
    arr = mix(arr, "#c9a588", fog_mask * 0.35)
    # дорога
    xs1 = np.arange(w, dtype=np.float32)
    road_prof = 720 - np.clip((xs1 - 640) ** 2 / 2200.0, 0, 260) - value_noise(w, 1, 90, 116, 2)[0] * 40
    road_mask = np.clip(1 - np.abs(ys - road_prof[None, :]) / (28 + np.clip((ys - 560) / 8, 0, 24)), 0, 1)
    arr = mix(arr, "#4a3a30", road_mask * 0.85)
    img = to_img(arr)
    # тёмные края снизу для читаемости меню
    d = ImageDraw.Draw(img, "RGBA")
    for i in range(160):
        d.line([0, h - i, w, h - i], fill=(10, 8, 12, int(i * 1.1)))
    for i in range(120):
        d.line([0, i, w, i], fill=(10, 8, 12, int((120 - i) * 0.7)))
    return img


# ---------------------------------------------------------------- main -----

def main():
    tiles = {
        "t_deep.png": gen_deep(),
        "t_water.png": gen_water(),
        "t_sand.png": gen_sand(),
        "t_grass.png": gen_grass(),
        "t_forest.png": gen_forest(),
        "t_hills.png": gen_hills(),
        "t_mountain.png": gen_mountain(),
        "t_swamp.png": gen_swamp(),
        "t_snow.png": gen_snow(),
        "t_dirt.png": gen_dirt(),
    }
    for name, arr in tiles.items():
        save_tile(name, arr)

    icons = {
        "ic_village.png": icon_village(),
        "ic_town.png": icon_town(),
        "ic_city.png": icon_city(),
        "ic_castle.png": icon_castle(),
        "ic_ruins.png": icon_ruins(),
        "ic_camp.png": icon_camp(),
        "ic_mine.png": icon_mine(),
        "ic_shrine.png": icon_shrine(),
        "ic_party.png": icon_party(),
        "ic_farm.png": icon_farm(),
        "ic_bandit.png": icon_bandit(),
    }
    for name, img in icons.items():
        img.save(os.path.join(OUT, name))

    for i in range(12):
        helmet = i % 4 == 1
        hood = i % 5 == 2
        p = gen_portrait(i, helmet=helmet, hood=hood, beard=0.45)
        p.save(os.path.join(OUT, f"portrait_{i}.png"))

    gen_wood().save(os.path.join(OUT, "ui_wood.png"))
    gen_parchment().save(os.path.join(OUT, "ui_parchment.png"))
    gen_stone_ui().save(os.path.join(OUT, "ui_stone.png"))

    for kind in ["hp", "fatigue", "resolve", "initiative", "matk", "ratk", "mdef", "rdef",
                 "armor_head", "armor_body", "coin", "food", "med", "star", "day", "mood", "weight"]:
        gen_icon(kind).save(os.path.join(OUT, f"ic_{kind}.png"))

    gen_title().save(os.path.join(OUT, "title.png"))

    n = len([f for f in os.listdir(OUT) if f.endswith(".png")])
    print(f"OK: сгенерировано {n} текстур в {OUT}")


if __name__ == "__main__":
    main()
