// benchmark: bst con punteros vs cache-oblivious static tree (vEB layout)
// compila con: g++ -O2 -std=c++17 -o benchmark cache-oblivious.cpp

#include <chrono>
#include <climits>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

using namespace std;
using clk = chrono::high_resolution_clock;

// ======================== BST con punteros ========================

struct Nodo {
  int dato;
  Nodo *izq, *der;
  Nodo(int v) : dato(v), izq(nullptr), der(nullptr) {}
};

Nodo *construir_bst(const vector<int> &arr, int l, int r) {
  if (l > r)
    return nullptr;
  int m = l + (r - l) / 2;
  Nodo *n = new Nodo(arr[m]);
  n->izq = construir_bst(arr, l, m - 1);
  n->der = construir_bst(arr, m + 1, r);
  return n;
}

bool buscar_bst(Nodo *raiz, int val) {
  while (raiz) {
    if (val == raiz->dato)
      return true;
    raiz = (val < raiz->dato) ? raiz->izq : raiz->der;
  }
  return false;
}

void liberar_bst(Nodo *raiz) {
  if (!raiz)
    return;
  liberar_bst(raiz->izq);
  liberar_bst(raiz->der);
  delete raiz;
}

// ======================== vEB layout (arbol perfecto) ========================

static const int NINGUNO = -1;

struct VebTree {
  // Cada nodo guarda valor + indices de hijos (todo contiguo en memoria)
  struct NodoV {
    int val;
    int izq, der; // indices hijo en el arreglo nodes[]
  };

  vector<NodoV> nodes; // arreglo en orden vEB
  int n;               // tamanio (2^h - 1)
  int raiz;            // indice de la raiz en nodes[]

  // --- Paso 1: Eytzinger (BFS) layout desde arreglo ordenado ---
  void eytzinger(const vector<int> &sorted, vector<int> &bfs, int i, int &k) {
    if (i >= n)
      return;
    eytzinger(sorted, bfs, 2 * i + 1, k);
    bfs[i] = sorted[k++];
    eytzinger(sorted, bfs, 2 * i + 2, k);
  }

  // --- Paso 2: Permutacion vEB ---
  // order[pos++] = bfs_index  (orden en que aparecen nodos BFS en el layout vEB)
  void veb_perm(vector<int> &order, int bfs_root, int &pos, int h) {
    if (h <= 0 || bfs_root >= n)
      return;
    if (h == 1) {
      order[pos++] = bfs_root;
      return;
    }
    int h_top = (h + 1) / 2;
    int h_bot = h - h_top;
    int num_bots = 1 << h_top;

    // Colocar top tree (primeros h_top niveles)
    veb_perm(order, bfs_root, pos, h_top);

    // Colocar cada bottom tree
    for (int i = 0; i < num_bots; i++) {
      int bot_bfs = bfs_root * (1 << h_top) + ((1 << h_top) - 1) + i;
      if (bot_bfs < n)
        veb_perm(order, bot_bfs, pos, h_bot);
    }
  }

  void construir(const vector<int> &sorted_orig) {
    int orig_n = (int)sorted_orig.size();

    // Padding a arbol perfecto (2^h - 1 nodos)
    int h = 1;
    while ((1 << h) - 1 < orig_n)
      h++;
    n = (1 << h) - 1;

    vector<int> sorted(n);
    for (int i = 0; i < orig_n; i++)
      sorted[i] = sorted_orig[i];
    for (int i = orig_n; i < n; i++)
      sorted[i] = INT_MAX;

    // 1) BFS layout
    vector<int> bfs(n);
    int k = 0;
    eytzinger(sorted, bfs, 0, k);

    // 2) Permutacion vEB
    vector<int> order(n);
    int pos = 0;
    veb_perm(order, 0, pos, h);

    // Invertir: bfs_to_veb[bfs_idx] = veb_idx
    vector<int> bfs_to_veb(n);
    for (int i = 0; i < n; i++)
      bfs_to_veb[order[i]] = i;

    // 3) Construir nodos en orden vEB con hijos precomputados
    nodes.resize(n);
    for (int i = 0; i < n; i++) {
      int vi = bfs_to_veb[i];
      nodes[vi].val = bfs[i];
      int lb = 2 * i + 1, rb = 2 * i + 2;
      nodes[vi].izq = (lb < n) ? bfs_to_veb[lb] : NINGUNO;
      nodes[vi].der = (rb < n) ? bfs_to_veb[rb] : NINGUNO;
    }
    raiz = bfs_to_veb[0];
  }

  bool buscar(int val) const {
    int pos = raiz;
    while (pos != NINGUNO) {
      const NodoV &nd = nodes[pos];
      if (val == nd.val)
        return true;
      pos = (val < nd.val) ? nd.izq : nd.der;
    }
    return false;
  }
};

// ======================== Medicion de tiempos ========================

double medir_bst(Nodo *raiz, const vector<int> &consultas) {
  auto ini = clk::now();
  volatile int suma = 0;
  for (int q : consultas)
    suma += buscar_bst(raiz, q);
  auto fin = clk::now();
  (void)suma;
  return chrono::duration<double, milli>(fin - ini).count();
}

double medir_veb(const VebTree &arbol, const vector<int> &consultas) {
  auto ini = clk::now();
  volatile int suma = 0;
  for (int q : consultas)
    suma += arbol.buscar(q);
  auto fin = clk::now();
  (void)suma;
  return chrono::duration<double, milli>(fin - ini).count();
}

// ======================== Experimento ========================

struct Resultado {
  int n_elems;
  int n_consult;
  double t_bst;
  double t_veb;
  double speedup;
};

Resultado experimento(int n_elems, int n_consult, int T) {
  cout << "\n=== n=" << n_elems << " | consultas=" << n_consult
       << " | repeticiones=" << T << " ===\n";

  // datos ordenados (numeros pares para tener hits y misses)
  vector<int> datos(n_elems);
  for (int i = 0; i < n_elems; i++)
    datos[i] = i * 2;

  // consultas aleatorias
  mt19937 rng(42);
  uniform_int_distribution<int> dist(0, n_elems * 2);
  vector<int> consultas(n_consult);
  for (int &q : consultas)
    q = dist(rng);

  double t_bst = 0.0, t_veb = 0.0;

  for (int t = 0; t < T; t++) {
    Nodo *raiz = construir_bst(datos, 0, n_elems - 1);
    VebTree veb;
    veb.construir(datos);

    t_bst += medir_bst(raiz, consultas);
    t_veb += medir_veb(veb, consultas);

    liberar_bst(raiz);
  }

  double prom_bst = t_bst / T;
  double prom_veb = t_veb / T;
  double speedup = prom_bst / prom_veb;

  cout << "  BST promedio   : " << prom_bst << " ms\n";
  cout << "  VEB promedio   : " << prom_veb << " ms\n";
  cout << "  Speedup VEB/BST: " << speedup << "x\n";

  return {n_elems, n_consult, prom_bst, prom_veb, speedup};
}

// ======================== Generar README ========================

void generar_readme(const vector<Resultado> &resultados) {
  ofstream out("README.md");
  out << "# Cache-Oblivious Static Tree — Benchmark\n\n";
  out << "Comparacion de tiempos de busqueda entre un **BST con punteros** y un\n";
  out << "**arbol estatico con layout van Emde Boas (vEB)**.\n\n";
  out << "## Compilacion y ejecucion\n\n";
  out << "```bash\n";
  out << "g++ -O2 -std=c++17 -o benchmark cache-oblivious.cpp\n";
  out << "./benchmark          # Linux / macOS\n";
  out << ".\\benchmark.exe      # Windows\n";
  out << "```\n\n";
  out << "## Resultados\n\n";
  out << "| N (elementos) | Consultas | BST (ms) | vEB (ms) | Speedup |\n";
  out << "|:-------------:|:---------:|:--------:|:--------:|:-------:|\n";
  for (auto &r : resultados) {
    out << "| " << r.n_elems << " | " << r.n_consult << " | "
        << fixed << setprecision(2) << r.t_bst << " | " << r.t_veb << " | "
        << setprecision(2) << r.speedup << "x |\n";
  }
  out << "\n> Cada medicion es el promedio de 5 repeticiones.\n\n";
  out << "## Por que el layout vEB es mas rapido?\n\n";
  out << "- **Localidad espacial**: los nodos padre e hijo quedan contiguos en\n";
  out << "  memoria, reduciendo *cache misses*.\n";
  out << "- **Arreglo compacto**: cada nodo ocupa 12 bytes (valor + 2 indices)\n";
  out << "  vs ~24 bytes del BST con punteros (dato + 2 punteros de 8 bytes).\n";
  out << "- **Sin dispersion de heap**: el BST asigna cada nodo con `new`,\n";
  out << "  esparciendo datos por toda la RAM. El vEB usa un solo `vector`\n";
  out << "  contiguo.\n";
  out << "- **Menos trafico de cache**: a medida que N crece, el arbol ya no\n";
  out << "  cabe en cache L2/L3 y el layout vEB minimiza los accesos a RAM.\n";
  out.close();
  cout << "\n[README.md generado con tabla de resultados]\n";
}

// ======================== Main ========================

int main() {
  const int T = 5;
  cout << "Benchmark: BST con punteros vs Cache-Oblivious Static Tree (vEB)\n";
  cout << "Repeticiones por prueba: " << T << "\n";

  vector<Resultado> resultados;
  resultados.push_back(experimento(1'000, 100'000, T));
  resultados.push_back(experimento(10'000, 100'000, T));
  resultados.push_back(experimento(100'000, 100'000, T));
  resultados.push_back(experimento(1'000'000, 1'000'000, T));
  resultados.push_back(experimento(5'000'000, 1'000'000, T));

  generar_readme(resultados);
  return 0;
}