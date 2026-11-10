# SDR

Лабораторные работы с ADALM-Pluto.

| Неделя | Работа |
|---|---|
| 1–2 | [FM-приёмник](Week_1-2/README.md) |
| 3 | [SoapySDR](Week_3/README.md) |
| 4 | [Чтение PCM и график I/Q](Week_4/README.md) |
| 5 | [Приём и передача на двух Pluto](Week_5/README.md) |
| 6 | [Приём двухтонового сигнала](Week_6/README.md) |

В корне лежат `main.c` и `CMakeLists.txt`. Версия с двумя тонами — в `experiments/two_tone`.

Сборка:

```bash
cmake -S . -B build
cmake --build build -j2
```

Запуск с URI своих плат, сначала RX, потом TX:

```bash
SoapySDRUtil --find
./build/sdr rx USB_URI_RX rx_new.pcm
./build/sdr tx USB_URI_TX
```

Остановка — Ctrl+C. Зависимости описаны в [BUILD.md](BUILD.md).

Графики:

```bash
python3 plot.py rxdata.pcm --start 37063000 --count 8000
python3 spectrum.py rxdata.pcm
```

`plot.py` показывает I/Q выбранного участка. В `spectrum.py` выбраны два участка из этой записи; для другого файла их нужно поменять. Скрипты открывают окна Matplotlib, а с `--save имя.png` сохраняют картинку.

PCM остаются локально. Корневой `main.c` передаёт прямоугольный импульс, а запись `rxdata.pcm` по частотам и периоду совпадает с двухтоновой версией.
