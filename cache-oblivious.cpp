#include <chrono>
#include <climits>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

using namespace std;
using clk = chrono::high_resolution_clock;

// BST

struct Nodo {
  int dato;
  Nodo *izq, *der;
  Nodo(int v) : dato(v), izq(nullptr), der(nullptr) {}
};

// construye bst balanceado desde arreglo ordenado
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

//  vEB layout
// construir layout BFS (Eytzinger) via recorrido in-order
// permutar BFS -> vEB recursivamente
// guardar nodos como structs {val, izq, der} en orden vEB

static const int NINGUNO = -1;

struct VebTree {
  struct NodoV {
    int val;
    int izq, der;
  };

  vector<NodoV> nodes; // arreglo en orden vEB
  int n;               // tamaño (2^h - 1)
  int raiz;            // indice de la raiz en nodes[]


  // recorrido in-order del arbol implicito -> llena posiciones BFS
  void eytzinger(const vector<int> &sorted, vector<int> &bfs, int i, int &k) {
    if (i >= n)
      return;
    eytzinger(sorted, bfs, 2 * i + 1, k);
    bfs[i] = sorted[k++];
    eytzinger(sorted, bfs, 2 * i + 2, k);
  }

  //  permutacion vEB 
  // genera order[pos++] = bfs_index
  // para un subarbol de altura h con raiz en bfs_root:
  //  top tree=  primeros ceil(h/2) niveles
  //   bottom trees =  arboles colgando de las hojas del top
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

    // colocar top tree recursivamente en orden vEB
    veb_perm(order, bfs_root, pos, h_top);

    // Colocar cada bottom tree
    // Los hijos del nivel h_top del subarbol con raiz bfs_root
    // estan en BFS indices: bfs_root * 2^h_top + (2^h_top - 1) + i
    for (int i = 0; i < num_bots; i++) {
      int bot_bfs = bfs_root * (1 << h_top) + ((1 << h_top) - 1) + i;
      if (bot_bfs < n)
        veb_perm(order, bot_bfs, pos, h_bot);
    }
  }

  void construir(const vector<int> &sorted_orig) {
    int orig_n = (int)sorted_orig.size();

    // padding a arbol perfecto (2^h - 1 nodos)
    int h = 1;
    while ((1 << h) - 1 < orig_n)
      h++;
    n = (1 << h) - 1;

    vector<int> sorted(n);
    for (int i = 0; i < orig_n; i++)
      sorted[i] = sorted_orig[i];
    for (int i = orig_n; i < n; i++)
      sorted[i] = INT_MAX; // centinelas

    //BFS layout
    vector<int> bfs(n);
    int k = 0;
    eytzinger(sorted, bfs, 0, k);

    //permutacion vEB: order[veb_pos] = bfs_index
    vector<int> order(n);
    int pos = 0;
    veb_perm(order, 0, pos, h);

    // invertir: bfs_to_veb[bfs_idx] = veb_idx
    vector<int> bfs_to_veb(n);
    for (int i = 0; i < n; i++)
      bfs_to_veb[order[i]] = i;

    // construir nodos en orden vEB con hijos precomputados
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

  //buqueda: solo seguir izq/der como en un BST normal
  // pero los accesos a nodes[] siguen el layout vEB -> cache-friendly
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

// medicion

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

// experimento

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

  // datos ordenados sin repeticion (numeros pares)
  vector<int> datos(n_elems);
  for (int i = 0; i < n_elems; i++)
    datos[i] = i * 2;

  // consultas aleatorias: mezcla de presentes y ausentes
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
  return 0;
}
