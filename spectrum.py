import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

parser = argparse.ArgumentParser(description="Спектры двух частей всплеска")
parser.add_argument("file", nargs="?", default="rxdata.pcm")
parser.add_argument("--save", type=Path)
args = parser.parse_args()
if args.save and args.save.suffix.lower() != ".png":
    parser.error("Картинку нужно сохранять в .png")

fs = 1_000_000
iq = np.memmap(args.file, dtype="<i2", mode="r").reshape(-1, 2)
plt.figure(figsize=(10, 4))

for start, label in [(37_064_800, "Первая часть"), (37_066_800, "Вторая часть")]:
    data = iq[start:start + 1000].astype(float)
    if len(data) != 1000:
        parser.error("Для этого файла нужно выбрать другие участки")
    signal = data[:, 0] + 1j * data[:, 1]
    signal -= signal.mean()
    spectrum = np.abs(np.fft.fftshift(np.fft.fft(signal * np.hanning(len(signal)))))
    frequency = np.fft.fftshift(np.fft.fftfreq(len(signal), 1 / fs))
    level = 20 * np.log10(np.maximum(spectrum / spectrum.max(), 1e-6))
    plt.plot(frequency / 1000, level, label=label)

plt.xlim(-15, 15)
plt.ylim(-50, 2)
plt.xlabel("Частота, кГц")
plt.ylabel("Уровень относительно максимума, дБ")
plt.title("Спектры двух частей сигнала")
plt.grid()
plt.legend()
plt.tight_layout()

if args.save:
    args.save.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(args.save, dpi=120)
else:
    plt.show()
