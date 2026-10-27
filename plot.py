import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

parser = argparse.ArgumentParser(description="График I/Q из PCM")
parser.add_argument("file", nargs="?", default="rxdata.pcm")
parser.add_argument("--start", type=int, default=0)
parser.add_argument("--count", type=int, default=20000)
parser.add_argument("--save", type=Path)
args = parser.parse_args()

if args.start < 0 or args.count < 1:
    parser.error("Проверь начало и длину участка")
if args.save and args.save.suffix.lower() != ".png":
    parser.error("Картинку нужно сохранять в .png")

iq = np.memmap(args.file, dtype="<i2", mode="r").reshape(-1, 2)
data = iq[args.start:args.start + args.count]
if len(data) == 0:
    parser.error("Этот участок за пределами файла")

n = np.arange(len(data))
plt.figure(figsize=(10, 4))
plt.plot(n, data[:, 0], "b", linewidth=0.7, label="I")
plt.plot(n, data[:, 1], "r", linewidth=0.7, alpha=0.7, label="Q")
plt.xlabel("Номер отсчёта")
plt.ylabel("Амплитуда")
plt.title("Принятый сигнал")
plt.grid()
plt.legend()
plt.tight_layout()

if args.save:
    args.save.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(args.save, dpi=120)
else:
    plt.show()
