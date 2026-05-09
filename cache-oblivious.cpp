

#include <chrono>
#include <iostream>
#include <random>
#include <vector>

using namespace std;
using clk = chrono::high_resolution_clock;

//  BST

struct Nodo {
  int dato;
  Nodo *izq;
  Nodo *der;
  Nodo(int v) : dato(v), izq(nullptr), der(nullptr) {}
};

// construye bst balanceado desde arreglo ordenado
Nodo *construir_bst(const vector<int> &arr, int l, int r) {
  if (l > r)
    return nullptr;
  int m = (l + r) / 2;
  Nodo *nodo = new Nodo(arr[m]);
  nodo->izq = construir_bst(arr, l, m - 1);
  nodo->der = construir_bst(arr, m + 1, r);
  return nodo;
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

int log2_entero(int n) {
  int h = 0;
  while (n > 1) {
    n >>= 1;
    h++;
  }
  return h;
}

struct VebTree {
  vector<int> datos;
  int n;

  // llena datos[] con el layout veb recursivamente
  // sorted[sl..sr] va a datos[dl..]
  void _build(const vector<int> &sorted, int sl, int sr, int dl) {
    if (sl > sr)
      return;
    if (sl == sr) {
      datos[dl] = sorted[sl];
      return;
    }

    int cnt = sr - sl + 1;
    int h = log2_entero(cnt);       // altura del subarbol
    int h_top = (h + 1) / 2;        // altura del top tree
    int tam_top = (1 << h_top) - 1; // nodos en el top tree

    // primero colocamos el top tree
    _build(sorted, sl, sl + tam_top - 1, dl);

    // luego los bottom trees uno tras otro
    int num_bot = tam_top + 1;
    int n_bot = cnt - tam_top;
    int tam_base = n_bot / num_bot;
    int extra = n_bot % num_bot; // los primeros 'extra' bottoms tienen +1

    int src = sl + tam_top; // indice fuente en sorted
    int dst = dl + tam_top; // indice destino en datos

    for (int i = 0; i < num_bot && src <= sr; i++) {
      int tam_i = tam_base + (i < extra ? 1 : 0);
      if (tam_i == 0)
        continue;
      _build(sorted, src, src + tam_i - 1, dst);
      src += tam_i;
      dst += tam_i;
    }
  }

  void construir(const vector<int> &sorted) {
    n = sorted.size();
    datos.resize(n);
    if (n > 0)
      _build(sorted, 0, n - 1, 0);
  }

  // devuelve la raiz del subarbol veb en datos[dl..dr]
  // bajamos siempre al top tree hasta llegar a un solo nodo
  int get_raiz(int dl, int dr) const {
    while (dl < dr) {
      int cnt = dr - dl + 1;
      int h = log2_entero(cnt);
      int h_top = (h + 1) / 2;
      int tam_top = (1 << h_top) - 1;
      dr = dl + tam_top - 1; // subimos al top
    }
    return datos[dl];
  }

  // busqueda navegando el layout veb sin punteros
  bool _search(int val, int dl, int dr) const {
    while (dl <= dr) {
      int cnt = dr - dl + 1;
      if (cnt == 1)
        return datos[dl] == val;

      int h = log2_entero(cnt);
      int h_top = (h + 1) / 2;
      int tam_top = (1 << h_top) - 1;

      // la raiz separa entre menores y mayores
      int raiz_val = get_raiz(dl, dl + tam_top - 1);

      if (val == raiz_val)
        return true;

      int n_bot = cnt - tam_top;
      int num_bot = tam_top + 1;
      int tam_base = n_bot / num_bot;
      int extra = n_bot % num_bot;
      int tam0 = tam_base + (extra > 0 ? 1 : 0); // tamanio primer bottom

      if (val < raiz_val) {
        // buscar en el top y luego en el primer bottom (hijo izq)
        bool en_top = _search(val, dl, dl + tam_top - 1);
        if (en_top)
          return true;
        if (tam0 == 0)
          return false;
        dl = dl + tam_top;
        dr = dl + tam0 - 1;
      } else {
        // buscar en el top y luego en los bottoms del lado derecho
        bool en_top = _search(val, dl, dl + tam_top - 1);
        if (en_top)
          return true;
        int dst = dl + tam_top + tam0;
        int rem = n_bot - tam0;
        if (rem <= 0)
          return false;
        dl = dst;
        dr = dst + rem - 1;
      }
    }
    return false;
  }

  bool buscar(int val) const { return _search(val, 0, n - 1); }
};

//   tiempos

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

//  Experimento

void experimento(int n_elems, int n_consult, int T) {
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

  double t_bst = 0.0;
  double t_veb = 0.0;

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

  cout << "  BST promedio   : " << prom_bst << " ms\n";
  cout << "  VEB promedio   : " << prom_veb << " ms\n";
  cout << "  Speedup VEB/BST: " << prom_bst / prom_veb << "x\n";
}

int main() {
  const int T = 5;

  cout << "Benchmark: BST con punteros vs Cache-Oblivious Static Tree\n";
  cout << "Repeticiones: " << T << "\n";

  experimento(10'000, 10'000, T);
  experimento(100'000, 100'000, T);
  experimento(1'000'000, 1'000'000, T);

  return 0;
}