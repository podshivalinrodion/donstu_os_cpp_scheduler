# Использование: python3 plot_gantt.py gantt.csv gantt.png
import csv
import sys
import matplotlib
matplotlib.use("Agg")  # рисуем в файл, окно не нужно
import matplotlib.pyplot as plt

rows = list(csv.DictReader(open(sys.argv[1], encoding="utf-8")))
pids = sorted({int(r["pid"]) for r in rows if int(r["pid"]) > 0})
fig, ax = plt.subplots(figsize=(10, 1.5 + 0.5 * len(pids)))
for r in rows:
    pid, start, end = int(r["pid"]), int(r["start"]), int(r["end"])
    if pid == -1:  # простой процессора не рисуем
        continue
    if pid == -2:  # переключение контекста — серым
        ax.barh(0, end - start, left=start, color="lightgray", edgecolor="black")
        continue
    ax.barh(pids.index(pid) + 1, end - start, left=start, edgecolor="black")
ax.set_yticks(range(len(pids) + 1))
ax.set_yticklabels(["CS"] + ["P" + str(p) for p in pids])
ax.set_xlabel("такт")
ax.grid(axis="x", linestyle=":")
plt.tight_layout()
plt.savefig(sys.argv[2], dpi=150)
print("сохранено:", sys.argv[2])
