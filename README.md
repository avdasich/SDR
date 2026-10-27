# SDR

Лабораторные работы с ADALM-Pluto.

| Неделя | Работа |
|---|---|
| 1–2 | [FM-приёмник](Week_1-2/README.md) |
| 3 | [SoapySDR](Week_3/README.md) |
| 4 | [Чтение PCM и график I/Q](Week_4/README.md) |

График записи строится через NumPy и Matplotlib:

```bash
python3 -m pip install -r requirements.txt
python3 plot.py rxdata.pcm --start 10000000 --count 10000
```

PCM остаются локально и не добавляются в Git.
