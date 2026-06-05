/*
 ****************************************************************
 *   EL TESORO DEL PIRATA  -  Neverland Edition
 *   Desafio Final  |  Estructuras de Datos
 ****************************************************************
 *  Estructuras implementadas:
 *   [1] Grafo          - Lista de adyacencia ponderada
 *   [2] Arbol          - Arbol de decisiones (interpretar pistas)
 *   [3] Cola / Pila    - BFS con cola,  DFS con pila
 *   [4] Tabla Hash     - Almacen de datos de ubicaciones
 *   [5] Lista Enlazada - Ruta optima / historial de exploracion
 *   [6] Archivos       - Lectura de mapa/pistas, guardado de ruta
 *   [+] Diccionario    - Habla popular de Santa Cruz, Bolivia
 ****************************************************************
 *  Compilar:
 *    g++ -std=c++11 -o tesoro main.cpp
 *    (MinGW / GCC / MSVC todos admitidos)
 ****************************************************************
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <climits>
#include <algorithm>
#include <cstring>

using namespace std;

// ============================================================
// CONSTANTES GLOBALES
// ============================================================
const int MAX_N   = 20;   // max nodos en el grafo
const int HASH_SZ = 23;   // tamano tabla hash (primo)
const int INICIO  = 0;    // nodo inicial del juego

// ============================================================
// ============================================================
//   [5]  LISTA ENLAZADA  - camino / historial
// ============================================================
// ============================================================
struct NodoL {
    int      val;
    NodoL*   sig;
    NodoL(int v) : val(v), sig(nullptr) {}
};

class Lista {
public:
    NodoL* cabeza;
    int    tam;

    Lista() : cabeza(nullptr), tam(0) {}

    ~Lista() { limpiar(); }

    // Agrega al final
    void pushBack(int v) {
        NodoL* n = new NodoL(v);
        if (!cabeza) { cabeza = n; }
        else {
            NodoL* a = cabeza;
            while (a->sig) a = a->sig;
            a->sig = n;
        }
        tam++;
    }

    // Agrega al inicio (util para reconstruir camino)
    void pushFront(int v) {
        NodoL* n = new NodoL(v);
        n->sig   = cabeza;
        cabeza   = n;
        tam++;
    }

    void limpiar() {
        while (cabeza) { NodoL* t = cabeza; cabeza = cabeza->sig; delete t; }
        tam = 0;
    }

    bool vacia() const { return cabeza == nullptr; }

    // Devuelve contenido como vector para iterar comodamente
    vector<int> aVector() const {
        vector<int> r;
        for (NodoL* a = cabeza; a; a = a->sig) r.push_back(a->val);
        return r;
    }
};

// ============================================================
// ============================================================
//   [4]  TABLA HASH  - datos de cada ubicacion
// ============================================================
// ============================================================
struct DatosUbic {
    int    id;
    string nombre;
    string descripcion;
    string pista;
    bool   visitado;
    bool   esTesoro;

    DatosUbic() : id(-1), visitado(false), esTesoro(false) {}
    DatosUbic(int i, const string& n, const string& d,
              const string& p, bool t)
        : id(i), nombre(n), descripcion(d), pista(p),
          visitado(false), esTesoro(t) {}
};

struct EntradaH {
    int       clave;
    DatosUbic dato;
    EntradaH* sig;
    EntradaH(int c, const DatosUbic& d) : clave(c), dato(d), sig(nullptr) {}
};

class TablaHash {
    EntradaH* tabla[HASH_SZ];

    // Funcion hash multiplicativa (Knuth)
    int hash(int c) const {
        return (int)((unsigned)(c * 2654435761u) % HASH_SZ);
    }

public:
    TablaHash() { fill(tabla, tabla + HASH_SZ, (EntradaH*)nullptr); }

    ~TablaHash() {
        for (int i = 0; i < HASH_SZ; i++) {
            EntradaH* a = tabla[i];
            while (a) { EntradaH* t = a; a = a->sig; delete t; }
        }
    }

    void insertar(int c, const DatosUbic& d) {
        int       idx = hash(c);
        EntradaH* n   = new EntradaH(c, d);
        n->sig         = tabla[idx];
        tabla[idx]     = n;
    }

    DatosUbic* buscar(int c) {
        for (EntradaH* a = tabla[hash(c)]; a; a = a->sig)
            if (a->clave == c) return &a->dato;
        return nullptr;
    }

    void resetVisitados() {
        for (int i = 0; i < HASH_SZ; i++)
            for (EntradaH* a = tabla[i]; a; a = a->sig)
                a->dato.visitado = false;
    }
};

// ============================================================
// ============================================================
//   [1]  GRAFO  - lista de adyacencia ponderada
// ============================================================
// ============================================================
struct Arista {
    int dest, peso;
    Arista(int d, int p) : dest(d), peso(p) {}
};

class Grafo {
public:
    int            n;
    vector<Arista> adj[MAX_N];

    Grafo() : n(0) {}

    void setN(int numN) { n = numN; }

    void agregarArista(int u, int v, int p) {
        adj[u].push_back(Arista(v, p));
        adj[v].push_back(Arista(u, p));
    }

    // ---- [3] BFS - busqueda en anchura (usa queue) ----
    Lista bfs(int ini, int obj, TablaHash& th) {
        bool   vis[MAX_N] = {};
        int    pad[MAX_N];
        fill(pad, pad + n, -1);

        queue<int> cola;
        cola.push(ini);
        vis[ini] = true;

        cout << "\n  [BFS] Orden de visita:\n  ";
        bool encontrado = false;

        while (!cola.empty()) {
            int u = cola.front(); cola.pop();

            DatosUbic* d = th.buscar(u);
            cout << (d ? d->nombre : "?");
            if (u == obj) { encontrado = true; cout << " [TESORO!]\n"; break; }
            cout << " -> ";

            for (int i = 0; i < (int)adj[u].size(); i++) {
                int v = adj[u][i].dest;
                if (!vis[v]) {
                    vis[v]  = true;
                    pad[v]  = u;
                    cola.push(v);
                }
            }
        }

        Lista camino;
        if (!encontrado) { cout << "\n"; return camino; }

        // Reconstruir camino desde destino hasta inicio
        int cur = obj;
        while (cur != -1) { camino.pushFront(cur); cur = pad[cur]; }
        return camino;
    }

    // ---- [3] DFS - busqueda en profundidad (usa stack de pares) ----
    Lista dfs(int ini, int obj, TablaHash& th) {
        bool   vis[MAX_N] = {};
        int    pad[MAX_N];
        fill(pad, pad + n, -1);

        // stack guarda {nodo, padre} para rastreo correcto
        stack< pair<int,int> > pila;
        pila.push(make_pair(ini, -1));

        cout << "\n  [DFS] Orden de visita:\n  ";
        bool encontrado = false;

        while (!pila.empty()) {
            int u   = pila.top().first;
            int par = pila.top().second;
            pila.pop();

            if (vis[u]) continue;
            vis[u]  = true;
            pad[u]  = par;

            DatosUbic* d = th.buscar(u);
            cout << (d ? d->nombre : "?");
            if (u == obj) { encontrado = true; cout << " [TESORO!]\n"; break; }
            cout << " -> ";

            // Invertir para mantener orden natural
            for (int i = (int)adj[u].size() - 1; i >= 0; i--) {
                int v = adj[u][i].dest;
                if (!vis[v])
                    pila.push(make_pair(v, u));
            }
        }

        Lista camino;
        if (!encontrado) { cout << "\n"; return camino; }

        int cur = obj;
        while (cur != -1) { camino.pushFront(cur); cur = pad[cur]; }
        return camino;
    }

    // ---- Dijkstra - camino optimo ponderado ----
    Lista dijkstra(int ini, int obj, int* costoFinal = nullptr) {
        int  dist[MAX_N], pad[MAX_N];
        bool vis[MAX_N] = {};
        fill(dist, dist + n, INT_MAX);
        fill(pad,  pad  + n, -1);
        dist[ini] = 0;

        // min-heap: {distancia, nodo}
        priority_queue< pair<int,int>,
                        vector< pair<int,int> >,
                        greater< pair<int,int> > > pq;
        pq.push(make_pair(0, ini));

        while (!pq.empty()) {
            int d = pq.top().first;
            int u = pq.top().second;
            pq.pop();

            if (vis[u]) continue;
            vis[u] = true;
            if (u == obj) break;

            for (int i = 0; i < (int)adj[u].size(); i++) {
                int v = adj[u][i].dest;
                int p = adj[u][i].peso;
                if (!vis[v] && dist[u] + p < dist[v]) {
                    dist[v] = dist[u] + p;
                    pad[v]  = u;
                    pq.push(make_pair(dist[v], v));
                }
            }
        }

        if (costoFinal) *costoFinal = (dist[obj] == INT_MAX) ? -1 : dist[obj];

        Lista camino;
        if (dist[obj] == INT_MAX) return camino;
        int cur = obj;
        while (cur != -1) { camino.pushFront(cur); cur = pad[cur]; }
        return camino;
    }

    // Calcular costo real de un camino (Lista)
    int costoCamino(const Lista& camino) const {
        vector<int> v = camino.aVector();
        int total = 0;
        for (int i = 0; i + 1 < (int)v.size(); i++) {
            for (int j = 0; j < (int)adj[v[i]].size(); j++) {
                if (adj[v[i]][j].dest == v[i + 1]) {
                    total += adj[v[i]][j].peso;
                    break;
                }
            }
        }
        return total;
    }
};

// ============================================================
// ============================================================
//   [2]  ARBOL DE DECISIONES  - interpretar pistas
// ============================================================
// ============================================================
struct NodoA {
    string keyword;     // palabra clave a buscar en la pista
    string etiqueta;    // descripcion del nodo
    int    ubicId;      // >= 0 = hoja (destino sugerido); -1 = interno
    NodoA* iz;
    NodoA* de;

    NodoA(const string& k, const string& e, int u = -1)
        : keyword(k), etiqueta(e), ubicId(u), iz(nullptr), de(nullptr) {}
};

class ArbolDecisiones {
    void liberar(NodoA* n) {
        if (!n) return; liberar(n->iz); liberar(n->de); delete n;
    }

    void mostrar(NodoA* n, int nivel, const string& rama) const {
        if (!n) return;
        string pre(nivel * 4, ' ');
        if (n->ubicId >= 0)
            cout << pre << rama << "[HOJA] -> " << n->etiqueta
                 << "  (nodo " << n->ubicId << ")\n";
        else
            cout << pre << rama << "? " << n->etiqueta << " ?\n";
        if (n->iz || n->de) {
            mostrar(n->iz, nivel + 1, "SI: ");
            mostrar(n->de, nivel + 1, "NO: ");
        }
    }

    int recorrer(NodoA* n, const string& pista) const {
        if (!n) return 0;
        if (n->ubicId >= 0) return n->ubicId;
        if (pista.find(n->keyword) != string::npos)
            return recorrer(n->iz, pista);
        return recorrer(n->de, pista);
    }

public:
    NodoA* raiz;
    ArbolDecisiones() : raiz(nullptr) {}
    ~ArbolDecisiones() { liberar(raiz); }

    /*
     * Arbol de decision basado en palabras clave de las pistas:
     *
     *             [calavera?]
     *            /           \
     *        nodo 5         [sirena?]
     *       (TESORO)        /        \
     *                    nodo 3   [cocodrilo?]
     *                  (Sirenas)  /            \
     *                          nodo 4       [pirata?]
     *                        (Croco.)       /        \
     *                                    nodo 1    [indio?]
     *                                 (JollyRog)  /       \
     *                                           nodo 6   nodo 0
     *                                         (Indio)  (Playa)
     */
    void construir() {
        raiz       = new NodoA("calavera", "La pista menciona calavera/craneo?");
        raiz->iz   = new NodoA("", "Roca Calavera - EL TESORO!", 5);

        raiz->de   = new NodoA("sirena", "La pista menciona sirenas/voces?");
        raiz->de->iz = new NodoA("", "Laguna de las Sirenas", 3);

        raiz->de->de   = new NodoA("cocodrilo", "La pista menciona cocodrilo/tictac?");
        raiz->de->de->iz = new NodoA("", "Arroyo del Cocodrilo", 4);

        raiz->de->de->de   = new NodoA("pirata", "La pista menciona pirata/barco/jolly?");
        raiz->de->de->de->iz = new NodoA("", "El Jolly Roger", 1);

        raiz->de->de->de->de   = new NodoA("indio", "La pista menciona indio/tribu/jefe?");
        raiz->de->de->de->de->iz = new NodoA("", "Campamento Indio", 6);
        raiz->de->de->de->de->de = new NodoA("", "Playa del Desembarco", 0);
    }

    // Devuelve el id de ubicacion sugerida segun keywords en la pista
    int interpretar(string pista) const {
        transform(pista.begin(), pista.end(), pista.begin(), ::tolower);
        return recorrer(raiz, pista);
    }

    void mostrarArbol() const {
        cout << "\n  === Arbol de Decisiones ===\n\n";
        mostrar(raiz, 1, "");
        cout << "\n";
    }
};

// ============================================================
// ============================================================
//   DICCIONARIO DEL HABLA CRUZENA (Santa Cruz, Bolivia)
// ============================================================
// ============================================================
class DiccionarioCruzeno {
    struct Par { string esp; string cruzeno; };
    vector<Par> vocab;

public:
    DiccionarioCruzeno() {
        // Clave: espanol pirata -> traduccion cruzena
        const char* data[][2] = {
            {"amigo",     "pana"},
            {"genial",    "joya"},
            {"excelente", "machin"},
            {"dinero",    "cobre"},
            {"vamos",     "vamos pues no mas"},
            {"rapido",    "rapido no mas"},
            {"tonto",     "sonso"},
            {"tesoro",    "cosita linda ese"},
            {"pirata",    "corsario no mas"},
            {"barco",     "canoa grandota esa"},
            {"isla",      "pedacito de tierra"},
            {"mapa",      "papelito ese"},
            {"pista",     "dato no mas"},
            {"camino",    "sendero aquel"},
            {"buscar",    "tatear"},
            {"encontrar", "topar"},
            {"miedo",     "cana"},
            {"valiente",  "sin cana"},
            {"mucho",     "harto"},
            {"poco",      "un toque"},
            {"aqui",      "aca no mas"},
            {"ahi",       "alla pues"},
            {"hablar",    "conversar"},
            {"correr",    "salir corriendo"},
            {"escondido", "tapeado"},
            {"problema",  "trancazo"},
            {"facil",     "facilito no mas"},
            {"dificil",   "jodido eso"},
            {"seguro",    "seguramente pues"},
            {"nunca",     "ni a palos"}
        };
        for (int i = 0; i < 30; i++)
            vocab.push_back({data[i][0], data[i][1]});
    }

    string traducir(const string& palabra) const {
        string lower = palabra;
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (int i = 0; i < (int)vocab.size(); i++)
            if (vocab[i].esp == lower) return vocab[i].cruzeno;
        return palabra;
    }

    void mostrar() const {
        cout << "\n  ============================================\n";
        cout << "   Diccionario del Habla Cruzena\n";
        cout << "   (Para entenderse con los piratas orientales)\n";
        cout << "  ============================================\n";
        cout << "  Espanol Pirata         Santa Cruz, Bolivia\n";
        cout << "  ----------------------------------------\n";
        for (int i = 0; i < (int)vocab.size(); i++) {
            int pad = (int)(22 - vocab[i].esp.size());
            cout << "  " << vocab[i].esp;
            for (int j = 0; j < pad; j++) cout << ' ';
            cout << "-> " << vocab[i].cruzeno << "\n";
        }
        cout << "\n";
    }
};

// ============================================================
// ============================================================
//   [6]  ARCHIVOS  - generar, cargar, guardar
// ============================================================
// ============================================================

// Genera mapa.txt y pistas.txt con el mapa de Neverland
void generarArchivos() {
    // ----- mapa.txt -----
    {
        ofstream f("mapa.txt");
        f << "# ================================================\n"
          << "# Mapa de Neverland | El Tesoro del Pirata\n"
          << "# ================================================\n"
          << "# Formato nodo:   <id> <Nombre_guion_bajo>  [TESORO]\n"
          << "# Formato arista: <orig> <dest> <peso_en_leguas>\n"
          << "# ================================================\n\n"
          << "NODOS 12\n"
          << "0  Playa_del_Desembarco\n"
          << "1  El_Jolly_Roger\n"
          << "2  Punto_Cojituerto\n"
          << "3  Laguna_de_las_Sirenas\n"
          << "4  Arroyo_del_Cocodrilo\n"
          << "5  Roca_Calavera  TESORO\n"
          << "6  Campamento_Indio\n"
          << "7  El_Arbol_Colgante\n"
          << "8  Cala_Canibal\n"
          << "9  Guarida_de_Perdidos\n"
          << "10 Montanas_Nevadas\n"
          << "11 Lago_Interior\n\n"
          << "ARISTAS\n"
          << "0  1   3\n"   // Playa      <-> Jolly Roger
          << "0  2   5\n"   // Playa      <-> Punto Cojituerto
          << "0  6   7\n"   // Playa      <-> Campamento Indio
          << "1  3   4\n"   // JollyRoger <-> Laguna Sirenas
          << "1  8   6\n"   // JollyRoger <-> Cala Canibal
          << "2  3   3\n"   // Cojituerto <-> Laguna Sirenas
          << "2  4   5\n"   // Cojituerto <-> Arroyo Croco
          << "3  5   8\n"   // Sirenas    <-> Roca Calavera (TESORO)
          << "3  11  4\n"   // Sirenas    <-> Lago Interior
          << "4  5   6\n"   // Croco      <-> Roca Calavera (TESORO)
          << "4  9   4\n"   // Croco      <-> Guarida Perdidos
          << "5  10  7\n"   // Calavera   <-> Montanas
          << "6  7   3\n"   // Indio      <-> Arbol Colgante
          << "6  9   5\n"   // Indio      <-> Guarida Perdidos
          << "7  10  4\n"   // Arbol      <-> Montanas
          << "8  11  5\n"   // Canibal    <-> Lago
          << "9  11  3\n"   // Perdidos   <-> Lago
          << "10 11  6\n";  // Montanas   <-> Lago
        cout << "  [+] mapa.txt generado\n";
    }

    // ----- pistas.txt -----
    {
        ofstream f("pistas.txt");
        f << "# Pistas de cada ubicacion\n"
          << "# Formato: <id> \"<texto>\"\n\n"
          << "0  \"La arena dorada te recibe, pirata. El barco pirata al norte y el cabo del hombre de palo esperan. El camino al tesoro comienza aqui.\"\n"
          << "1  \"El Jolly Roger cruje con la marea. El capitan dice: las sirenas al este conocen el secreto de la roca calavera. Sigue sus voces cantarinas.\"\n"
          << "2  \"El cabo del cojituerto huele a sal y aventura. Las sirenas al norte guardan un secreto, y el cocodrilo al este hace tictac... tictac...\"\n"
          << "3  \"Las sirenas cantan dulcemente: solo los valientes alcanzan la roca calavera. El craneo de piedra guarda el tesoro. El lago interior queda al sur.\"\n"
          << "4  \"El cocodrilo te observa con ojos amarillos. Tictac, tictac. Susurra: la calavera de roca esta cerca, pirata. Los ninos perdidos viven al este.\"\n"
          << "5  \"*** ENCONTRASTE EL TESORO! *** Bajo el ojo izquierdo de la calavera, donde la luna llena ilumina a medianoche, yace el cofre del gran pirata.\"\n"
          << "6  \"El jefe indio habla: el arbol colgante al norte guarda sabiduria ancestral, y los ninos perdidos al este conocen atajos secretos de la isla.\"\n"
          << "7  \"Los cuervos graznan desde el arbol: sube a las montanas nevadas para ver toda Neverland. Desde las cumbres, el ojo de la calavera sera visible.\"\n"
          << "8  \"La Cala Canibal es peligrosa, pirata. Huye rapido al lago interior. No hay tesoro aqui, solo peligro.\"\n"
          << "9  \"Los ninos perdidos juegan sin cesar. Peter Pan dice: el lago interior conecta todo. Desde ahi puedes alcanzar cualquier lugar de Neverland.\"\n"
          << "10 \"Desde las Montanas Nevadas ves todo Neverland. Al sur, el ojo de la calavera mira al cielo. El tesoro esta muy cerca, baja al lago y asciende a la roca.\"\n"
          << "11 \"El Lago Interior es el corazon de Neverland. Las aguas reflejan el camino: la roca calavera esta al norte, donde los rios mueren y los secretos nacen.\"\n";
        cout << "  [+] pistas.txt generado\n";
    }
}

// Carga grafo y tabla hash desde los archivos
bool cargarArchivos(Grafo& g, TablaHash& th, int& tesoro) {
    ifstream fm("mapa.txt");
    if (!fm.is_open()) {
        cout << "  Archivos no encontrados. Generando archivos por defecto...\n";
        generarArchivos();
        fm.open("mapa.txt");
        if (!fm.is_open()) return false;
    }

    int            numN = 0;
    vector<string> nombres(MAX_N);
    vector<bool>   esTes(MAX_N, false);
    tesoro = -1;
    bool aristasFase = false;
    string linea;

    while (getline(fm, linea)) {
        if (linea.empty() || linea[0] == '#') continue;
        istringstream ss(linea);
        string tok; ss >> tok;

        if (tok == "NODOS")   { ss >> numN; continue; }
        if (tok == "ARISTAS") { aristasFase = true; continue; }

        if (aristasFase) {
            int u, v, p;
            istringstream ss2(linea);
            if (ss2 >> u >> v >> p) g.agregarArista(u, v, p);
        } else {
            int id; string nom, extra;
            istringstream ss2(linea);
            if (!(ss2 >> id >> nom)) continue;
            replace(nom.begin(), nom.end(), '_', ' ');
            nombres[id] = nom;
            if (ss2 >> extra && extra == "TESORO") { esTes[id] = true; tesoro = id; }
        }
    }
    fm.close();
    g.setN(numN);

    // Leer pistas
    ifstream fp("pistas.txt");
    vector<string> pistas(MAX_N);
    if (fp.is_open()) {
        while (getline(fp, linea)) {
            if (linea.empty() || linea[0] == '#') continue;
            istringstream ss(linea); int id; ss >> id;
            size_t a = linea.find('"'), b = linea.rfind('"');
            if (a != string::npos && b != a)
                pistas[id] = linea.substr(a + 1, b - a - 1);
        }
        fp.close();
    }

    const char* descs[] = {
        "La playa dorada donde desembarcaste en Neverland",
        "El legendario barco pirata El Jolly Roger",
        "El cabo donde el Capitan Garfio perdio su pierna",
        "Laguna encantada habitada por sirenas",
        "El arroyo donde acecha el cocodrilo que tictaquea",
        "*** ROCA CALAVERA - AQUI ESTA EL TESORO! ***",
        "Campamento de las tribus ancestrales de la isla",
        "El arbol retorcido donde anidan cuervos negros",
        "La peligrosa e inhospita Cala Canibal",
        "Guarida oculta de los Ninos Perdidos de Peter Pan",
        "Las heladas montanas que tocan las nubes de Neverland",
        "El sereno Lago Interior, corazon de la isla"
    };

    for (int i = 0; i < numN; i++) {
        string desc = (i < 12) ? descs[i] : nombres[i];
        th.insertar(i, DatosUbic(i, nombres[i], desc, pistas[i], esTes[i]));
    }
    return true;
}

// Guarda la ruta encontrada en un archivo
void guardarRuta(const Lista& camino, TablaHash& th, int costo,
                 const string& metodo) {
    ofstream f("ruta_tesoro.txt");
    if (!f.is_open()) { cout << "  [!] No se pudo escribir ruta_tesoro.txt\n"; return; }

    f << "================================================\n"
      << "  EL TESORO DEL PIRATA - Ruta Encontrada\n"
      << "================================================\n\n"
      << "Metodo de busqueda: " << metodo << "\n\n"
      << "Camino al tesoro:\n\n";

    vector<int> v = camino.aVector();
    for (int i = 0; i < (int)v.size(); i++) {
        DatosUbic* d = th.buscar(v[i]);
        f << "  Paso " << (i + 1) << ": ["
          << (d ? d->nombre : "Desconocido") << "]\n";
        if (d && !d->pista.empty())
            f << "  Pista: \"" << d->pista << "\"\n\n";
    }

    if (costo > 0) f << "Costo total: " << costo << " leguas\n";
    f << "\n================================================\n";
    f.close();
    cout << "  [+] Ruta guardada en 'ruta_tesoro.txt'\n";
}

// ============================================================
// ============================================================
//   INTERFAZ DE USUARIO - pantalla y mapas ASCII
// ============================================================
// ============================================================

// Elimina \r y espacios finales (necesario al pipear input en Windows)
string trim(const string& s) {
    string r = s;
    while (!r.empty() && (r.back() == '\r' || r.back() == ' ')) r.pop_back();
    while (!r.empty() && (r.front() == ' '))                     r.erase(r.begin());
    return r;
}

void pausar() {
    string dummy;
    cout << "\n  Presiona ENTER para continuar...";
    getline(cin, dummy);
}

void limpiar() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void titulo() {
    cout << "\n";
    cout << "  +---------------------------------------------------------+\n";
    cout << "  |          ** EL TESORO DEL PIRATA **                     |\n";
    cout << "  |              Neverland Edition                          |\n";
    cout << "  |   Desafio Final  |  Estructuras de Datos                |\n";
    cout << "  +---------------------------------------------------------+\n\n";
}

// Mapa ASCII de Neverland (simplificado)
void mapaNeverland() {
    cout << "\n";
    cout << "  ===================================================\n";
    cout << "           MAPA DE NEVERLAND\n";
    cout << "  ===================================================\n\n";
    cout << "         [10] Montanas Nevadas\n";
    cout << "         /           |    \\\n";
    cout << "  [7] Arbol        [11] Lago ---[9] Perdidos---[8] Canibal\n";
    cout << "       |             |              |\n";
    cout << "  [6] Indio---+    [5]*TESORO    [4] Cocodrilo\n";
    cout << "              |   / |               |\n";
    cout << "              | [3] Sirenas --------+\n";
    cout << "              |  |   \\\n";
    cout << "              | [11]  [1] Jolly Roger\n";
    cout << "              |         |\n";
    cout << "            [2] Cojituerto\n";
    cout << "                   |\n";
    cout << "              [0] PLAYA (INICIO)\n\n";
    cout << "  * El peso de cada arista (en leguas) se muestra\n";
    cout << "    en la lista de conexiones a continuacion:\n\n";
}

void mostrarMapa(Grafo& g, TablaHash& th, int actual = -1) {
    mapaNeverland();
    cout << "  --- Ubicaciones ---\n";
    for (int i = 0; i < g.n; i++) {
        DatosUbic* d = th.buscar(i);
        if (!d) continue;
        string extra = "";
        if (i == actual)   extra += " <<< ESTAS AQUI";
        if (d->esTesoro)   extra += " [TESORO!]";
        if (d->visitado)   extra += " (visitado)";
        cout << "  [" << i << "] " << d->nombre << extra << "\n";
    }
    cout << "\n  --- Conexiones (leguas) ---\n";
    for (int i = 0; i < g.n; i++) {
        DatosUbic* di = th.buscar(i);
        for (int j = 0; j < (int)g.adj[i].size(); j++) {
            if (g.adj[i][j].dest > i) {
                DatosUbic* dj = th.buscar(g.adj[i][j].dest);
                cout << "  " << (di ? di->nombre : "?")
                     << " <--[" << g.adj[i][j].peso << " leg]--> "
                     << (dj ? dj->nombre : "?") << "\n";
            }
        }
    }
    cout << "\n";
}

void mostrarUbicacion(int nodo, TablaHash& th) {
    DatosUbic* d = th.buscar(nodo);
    if (!d) return;
    cout << "\n  +--------------------------------------------------+\n";
    cout << "  | Ubicacion : " << d->nombre << "\n";
    cout << "  | " << d->descripcion << "\n";
    if (!d->pista.empty())
        cout << "  | PISTA: \"" << d->pista << "\"\n";
    if (d->esTesoro)
        cout << "  |  *** !!! AQUI ESTA EL TESORO !!! ***  \n";
    cout << "  +--------------------------------------------------+\n";
}

void mostrarCamino(const Lista& camino, TablaHash& th, int costo = -1) {
    if (camino.vacia()) { cout << "  No se encontro camino al tesoro.\n"; return; }
    vector<int> v = camino.aVector();
    cout << "\n  Camino (" << v.size() << " pasos):\n\n  ";
    for (int i = 0; i < (int)v.size(); i++) {
        DatosUbic* d = th.buscar(v[i]);
        cout << "[" << v[i] << "] " << (d ? d->nombre : "?");
        if (i + 1 < (int)v.size()) cout << "\n   --> ";
    }
    if (costo >= 0) cout << "\n\n  Costo total: " << costo << " leguas";
    cout << "\n";
}

// ============================================================
// ============================================================
//   MODO EXPLORACION MANUAL
// ============================================================
// ============================================================
void explorarManual(Grafo& g, TablaHash& th, int tesoro,
                    ArbolDecisiones& arbol, DiccionarioCruzeno& dic) {
    th.resetVisitados();
    int   actual = INICIO;
    Lista historial;
    historial.pushBack(actual);
    if (th.buscar(actual)) th.buscar(actual)->visitado = true;

    cout << "\n  === Modo Exploracion Manual ===\n";
    cout << "  Comandos: numero=mover  |  pista=ayuda  |  mapa=ver mapa  |  salir\n";

    while (true) {
        mostrarUbicacion(actual, th);

        // Condicion de victoria
        DatosUbic* d = th.buscar(actual);
        if (d && d->esTesoro) {
            cout << "\n  **** FELICIDADES, PIRATA! ****\n";
            cout << "  Encontraste el " << dic.traducir("tesoro") << "!\n";
            // Guardar ruta del historial
            guardarRuta(historial, th, g.costoCamino(historial), "Exploracion Manual");
            return;
        }

        // Mostrar opciones de movimiento
        cout << "\n  Caminos disponibles desde aqui:\n";
        for (int i = 0; i < (int)g.adj[actual].size(); i++) {
            int        v  = g.adj[actual][i].dest;
            int        p  = g.adj[actual][i].peso;
            DatosUbic* dv = th.buscar(v);
            cout << "  [" << i << "] " << (dv ? dv->nombre : "?")
                 << "  (" << p << " leguas)";
            if (dv && dv->visitado) cout << " (ya visitado)";
            cout << "\n";
        }

        cout << "\n  Tu eleccion: ";
        string inp;
        getline(cin, inp);
        inp = trim(inp);

        if (inp == "salir") { cout << "  Huiste de Neverland...\n"; return; }

        if (inp == "mapa")  { mostrarMapa(g, th, actual); continue; }

        if (inp == "pista" || inp == "ayuda") {
            string pistaActual = d ? d->pista : "";
            int suger = arbol.interpretar(pistaActual);
            DatosUbic* ds = th.buscar(suger);
            cout << "  El arbol de decisiones sugiere ir a: "
                 << (ds ? ds->nombre : "lugar desconocido") << "\n";
            continue;
        }

        // Intentar mover
        bool valido = true;
        for (int i = 0; i < (int)inp.size(); i++)
            if (inp[i] < '0' || inp[i] > '9') { valido = false; break; }

        if (valido && !inp.empty()) {
            int idx = atoi(inp.c_str());
            if (idx >= 0 && idx < (int)g.adj[actual].size()) {
                actual = g.adj[actual][idx].dest;
                historial.pushBack(actual);
                DatosUbic* da = th.buscar(actual);
                if (da) da->visitado = true;
            } else {
                cout << "  Opcion invalida.\n";
            }
        } else {
            cout << "  Comando no reconocido.\n";
        }
    }
}

// ============================================================
// ============================================================
//   MENU PRINCIPAL
// ============================================================
// ============================================================
void menu(Grafo& g, TablaHash& th, int tesoro,
          ArbolDecisiones& arbol, DiccionarioCruzeno& dic) {
    string op;
    while (true) {
        limpiar();
        titulo();
        cout << "  [1] Ver mapa de Neverland\n";
        cout << "  [2] Explorar manualmente (modo juego)\n";
        cout << "  [3] Busqueda BFS  (en anchura)\n";
        cout << "  [4] Busqueda DFS  (en profundidad)\n";
        cout << "  [5] Camino optimo (Dijkstra)\n";
        cout << "  [6] Ver arbol de decisiones\n";
        cout << "  [7] Diccionario del habla cruzena\n";
        cout << "  [8] Interpretar una pista\n";
        cout << "  [0] Salir\n";
        cout << "\n  Opcion: ";
        getline(cin, op);
        op = trim(op);

        if (op == "0") break;

        limpiar();

        if (op == "1") {
            mostrarMapa(g, th);
            pausar();
        }
        else if (op == "2") {
            explorarManual(g, th, tesoro, arbol, dic);
            pausar();
        }
        else if (op == "3") {
            th.resetVisitados();
            cout << "\n  === BFS: Playa -> Roca Calavera ===\n";
            Lista camino = g.bfs(INICIO, tesoro, th);
            mostrarCamino(camino, th, g.costoCamino(camino));
            if (!camino.vacia())
                guardarRuta(camino, th, g.costoCamino(camino), "BFS");
            pausar();
        }
        else if (op == "4") {
            th.resetVisitados();
            cout << "\n  === DFS: Playa -> Roca Calavera ===\n";
            Lista camino = g.dfs(INICIO, tesoro, th);
            mostrarCamino(camino, th, g.costoCamino(camino));
            if (!camino.vacia())
                guardarRuta(camino, th, g.costoCamino(camino), "DFS");
            pausar();
        }
        else if (op == "5") {
            cout << "\n  === Dijkstra: camino optimo al tesoro ===\n";
            int costo = 0;
            Lista camino = g.dijkstra(INICIO, tesoro, &costo);
            mostrarCamino(camino, th, costo);
            if (!camino.vacia())
                guardarRuta(camino, th, costo, "Dijkstra");
            pausar();
        }
        else if (op == "6") {
            arbol.mostrarArbol();
            pausar();
        }
        else if (op == "7") {
            dic.mostrar();
            pausar();
        }
        else if (op == "8") {
            cout << "\n  Escribe la pista a interpretar:\n  > ";
            string pista;
            getline(cin, pista);
            pista = trim(pista);
            int suger = arbol.interpretar(pista);
            DatosUbic* d = th.buscar(suger);
            cout << "\n  El arbol de decisiones sugiere ir a: "
                 << (d ? d->nombre : "lugar desconocido") << "\n";
            pausar();
        }
        else {
            cout << "  Opcion no valida.\n";
            pausar();
        }
    }
}

// ============================================================
// MAIN
// ============================================================
int main() {
    Grafo     g;
    TablaHash th;
    int       tesoro = -1;

    cout << "\n  Cargando El Tesoro del Pirata...\n\n";

    if (!cargarArchivos(g, th, tesoro)) {
        cout << "  Error critico al cargar datos.\n";
        return 1;
    }

    if (tesoro == -1) {
        cout << "  [!] No se encontro el nodo TESORO en mapa.txt\n";
        return 1;
    }

    cout << "\n  Mapa cargado: " << g.n << " ubicaciones | Tesoro en nodo " << tesoro << "\n";

    ArbolDecisiones arbol;
    arbol.construir();

    DiccionarioCruzeno dic;

    pausar();
    menu(g, th, tesoro, arbol, dic);

    cout << "\n  Hasta la proxima, " << dic.traducir("amigo") << "!\n\n";
    return 0;
}
