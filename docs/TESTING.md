# Проверка прототипа — 2026-10-07

В рабочей среде собран **Godot 4.3 stable из исходников** для headless-проверок (Linux, без X11/Wayland, fallback TextServer). Движок, исходники и логи сборки находятся вне репозитория и не являются частью проекта. Обычный официальный Godot поддерживает более полный текстовый renderer.

## Выполнено

| Проверка | Результат |
|---|---|
| Импорт проекта и компиляция всех GDScript в Godot 4.3 | Без SCRIPT ERROR / Parse Error |
| `tests/SmokeTests.tscn` | **141 checks, 0 failures** |
| `tests/SceneTests.tscn` | **0 failures**, без ошибок скриптов |
| Старт `scenes/Main.tscn` / главное меню headless | Без ошибок скриптов |
| gdtoolkit: GDScript parse / gdlint / gdformat | Проходит; политика ранних возвратов в `gdlintrc` |
| Миграция и RLS-тест: PostgreSQL parser (pglast) | SQL синтаксис разобран |
| `git diff --check` | Без ошибок whitespace |

SmokeTests включает локальную БД сохранений (8 слотов, запись/чтение/удаление, отклонение невалидного state), 3 размера × 2/3/4 игрока, каталоги, движение, бой и контратаку, найм/tech gates, города/таверну, пять героев, перки, возрождение, сериализацию/локальное сохранение, malformed JSON, ходы трех ИИ, завершение партии, асинхронную очередь, повторный клик конца хода и отмену старого AI session. Дополнительно проверяется отклонение secret/service_role и принятие публичного anon ключа.

SceneTests создает карту/HUD, выбирает юнита/героя, открывает технологии, пересоздает игровую сцену с сохранением живых объектов, создает меню/лобби/настройки и экран результата. Это проверка жизненного цикла, **не визуальный screenshot QA**.

## Команды для повторения

```sh
godot --headless --editor --path . --import
godot --headless --path . tests/SmokeTests.tscn
godot --headless --path . tests/SceneTests.tscn
```

Также добавлен GitHub Actions workflow `.github/workflows/godot.yml`; он запускает импорт и оба набора тестов. В этой сессии workflow на GitHub не запускался (изменения не пушились).

Дополнительно проверено на диске этой среды: после прогона создан `~/.local/share/godot/app_userdata/HexEmpire/saves/hexempire_saves.json` со слотом `autosave` (карта 10, раунд 2) и резервной копией `.bak`.

## Пока не выполнено

- Применение SQL к реальной Supabase/Postgres, live RLS и Auth/REST roundtrip.
- Web export с официальными шаблонами и запуск WebAssembly в браузере.
- Визуальный UI QA, тесты на телефонах и desktop export.
- Android/iOS подпись, release-пакеты, длительные партии и баланс.

Для RLS: инструкция в `docs/SUPABASE.md`, отдельный файл `tests/supabase_rls.sql`. Успех SQL-парсера **не означает**, что миграции уже применены или политики проверены на сервере.
