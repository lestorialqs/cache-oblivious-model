# Cache-Oblivious Static Tree — Benchmark

Comparacion de tiempos de busqueda entre un **BST con punteros** y un
**arbol estatico con layout van Emde Boas (vEB)**.

## Compilacion y ejecucion

```bash
g++ -O2 -std=c++17 -o benchmark cache-oblivious.cpp   # Linux / macOS
./benchmark

g++ -static -O2 -std=c++17 -o benchmark.exe cache-oblivious.cpp   # Windows (MSYS2)
.\benchmark.exe
```

## Resultados

| N (elementos) | Consultas | BST (ms) | vEB (ms) | Speedup |
|:-------------:|:---------:|:--------:|:--------:|:-------:|
| 1000 | 100000 | 2.57 | 3.38 | 0.76x |
| 10000 | 100000 | 5.25 | 5.56 | 0.94x |
| 100000 | 100000 | 8.32 | 7.34 | 1.13x |
| 1000000 | 1000000 | 207.64 | 123.05 | 1.69x |
| 5000000 | 1000000 | 434.68 | 228.24 | 1.90x |

> Cada medicion es el promedio de 5 repeticiones.

<img width="1271" height="316" alt="image" src="https://github.com/user-attachments/assets/5e742108-7980-4000-9ee8-b44eb1b743b6" />


## Por que el layout vEB es mas rapido?

- **Localidad espacial**: los nodos padre e hijo quedan contiguos en
  memoria, reduciendo *cache misses*.
- **Arreglo compacto**: cada nodo ocupa 12 bytes (valor + 2 indices)
  vs ~24 bytes del BST con punteros (dato + 2 punteros de 8 bytes).
- **Sin dispersion de heap**: el BST asigna cada nodo con `new`,
  esparciendo datos por toda la RAM. El vEB usa un solo `vector`
  contiguo.
- **Menos trafico de cache**: a medida que N crece, el arbol ya no
  cabe en cache L2/L3 y el layout vEB minimiza los accesos a RAM.
