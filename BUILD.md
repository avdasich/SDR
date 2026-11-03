# Установка и сборка

Для программы нужны SoapySDR 0.8.1, libiio 0.24, libad9361-iio 0.3 и SoapyPlutoSDR с веткой `sdr_gadget_timestamping`.

Порядок установки библиотек и команды для Linux есть в [методичке](https://github.com/fzybot/sdrLessons/blob/master/thirdparty/README.md). Обычный драйвер Pluto может не поддерживать используемые временные метки.

После установки библиотек:

```bash
cmake -S . -B build
cmake --build build -j2
SoapySDRUtil --find
```

В результате получается `build/sdr`.

Для графиков:

```bash
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
.venv/bin/python plot.py rxdata.pcm --start 37063000 --count 8000
```

В `requirements.txt` только NumPy и Matplotlib.
