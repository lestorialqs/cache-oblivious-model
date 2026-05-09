# Cache-Oblivious Static Tree — benchmark

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
| 1000 | 100000 | 2.45 | 3.25 | 0.75x |
| 10000 | 100000 | 4.45 | 4.76 | 0.94x |
| 100000 | 100000 | 7.69 | 7.44 | 1.03x |
| 1000000 | 1000000 | 188.03 | 112.79 | 1.67x |
| 5000000 | 1000000 | 371.09 | 202.19 | 1.84x |

> Cada medicion es el promedio de 5 repeticiones.

## El layout vEB es mas rapido porque : 

- **Localidad espacial**: los nodos padre e hijo quedan contiguos en
  memoria, reduciendo *cache misses*.
- **Arreglo compacto**: cada nodo ocupa 12 bytes (valor + 2 indices)
  vs ~24 bytes del BST con punteros (dato + 2 punteros de 8 bytes).
- **Sin dispersion de heap**: el BST asigna cada nodo con `new`,
  esparciendo datos por toda la RAM. El vEB usa un solo `vector`
  contiguo.
- **Menos trafico de cache**: a medida que N crece, el arbol ya no
  cabe en cache L2/L3 y el layout vEB minimiza los accesos a RAM.
