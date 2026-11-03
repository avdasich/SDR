# SDR

Лабораторные работы с ADALM-Pluto.

| Неделя | Работа |
|---|---|
| 1–2 | [FM-приёмник](Week_1-2/README.md) |
| 3 | [SoapySDR](Week_3/README.md) |
| 4 | [Чтение PCM и график I/Q](Week_4/README.md) |
| 5 | [Приём и передача на двух Pluto](Week_5/README.md) |

В корне лежат `main.c` и `CMakeLists.txt`. Зависимости описаны в [BUILD.md](BUILD.md).

```bash
cmake -S . -B build
cmake --build build -j2
SoapySDRUtil --find
```

Подставить URI своих плат и запустить сначала приёмник, затем передатчик:

```bash
./build/sdr rx USB_URI_RX rx_new.pcm
./build/sdr tx USB_URI_TX
```

Остановка — Ctrl+C. Передатчик формирует 4 мс I=Q=2000 и 300 мс нулей.

График записи:

```bash
python3 plot.py rxdata.pcm --start 37063000 --count 8000
```

PCM остаются локально и не добавляются в Git.
