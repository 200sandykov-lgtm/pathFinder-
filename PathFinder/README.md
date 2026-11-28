# PathFinder

Сборка в UCRT64:

```bash
g++ -std=c++20 -O3 main.cpp common.cpp scanner.cpp index.cpp search.cpp -o pathfinder.exe
```

Отладка: замените `-O3` на `-g -O0`.

main — управление; scanner — снимок файловой системы; index — построение постоянного индекса; search — бинарный поиск; common — общие структуры и функции.

Формат данных и расположение файлов сохранены. Старый pathfinder.cpp не включайте в сборку.
