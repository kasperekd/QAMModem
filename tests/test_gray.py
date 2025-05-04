import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from itertools import combinations

df = pd.read_csv("constellation.csv")

def is_gray_code(a, b):
    xor = a ^ b
    return xor and not (xor & (xor - 1))

def index_to_bitstring(i, bits=4):
    return format(i, f'0{bits}b')

def bitstring_to_int(s):
    return int(s, 2)

points = df[['real', 'imag']].values
indices = df['index'].values
labels = [index_to_bitstring(i, bits=4 if len(df) == 16 else 6) for i in indices]

min_distance = np.inf
for i, j in combinations(range(len(points)), 2):
    dist = np.linalg.norm(points[i] - points[j])
    if dist < min_distance:
        min_distance = dist

edges = []
for i, j in combinations(range(len(points)), 2):
    if np.linalg.norm(points[i] - points[j]) <= min_distance + 1e-6:
        edges.append((i, j))

plt.figure(figsize=(8, 8))
plt.scatter(points[:, 0], points[:, 1], color='blue', zorder=2)

for i, (x, y) in enumerate(points):
    plt.text(x + 0.1, y + 0.1, labels[i], fontsize=9, ha='left', va='bottom', zorder=3)

for i, j in edges:
    x1, y1 = points[i]
    x2, y2 = points[j]
    b1 = bitstring_to_int(labels[i])
    b2 = bitstring_to_int(labels[j])

    if is_gray_code(b1, b2):
        color = 'green'
    else:
        color = 'red'

    plt.plot([x1, x2], [y1, y2], color=color, linewidth=1.2, alpha=0.7, zorder=1)

def is_gray(a, b):
    xor = a ^ b
    return bin(xor).count('1') == 1

bad_edges = 0
for i, j in edges:
    if not is_gray(bitstring_to_int(labels[i]), bitstring_to_int(labels[j])):
        bad_edges += 1

print(f"Неверных рёбер : {bad_edges}")

plt.title("Constellation with Gray Code Check")
plt.xlabel("I")
plt.ylabel("Q")
plt.grid(True, linestyle='--', alpha=0.5)
plt.axis('equal')
plt.show()