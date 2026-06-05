/*
 * ============================================================
 *  EL DICCIONARIO DEL HABLA POPULAR
 *  Santa Cruz de la Sierra - Bolivia
 *  Recopilacion: Sanchico
 * ============================================================
 *  Estructura principal: TABLA HASH
 *    - 28 posiciones (buckets)
 *    - Colisiones resueltas con Arboles Binarios de Busqueda
 *
 *  Sin BST (lista enlazada por bucket):
 *    busqueda promedio = O(n / 28)
 *  Con BST por bucket (mejora implementada):
 *    busqueda promedio = O(log(n / 28))
 *
 *  Con ~450 entradas y 28 buckets -> ~16 por bucket
 *    Lista enlazada: hasta 16 comparaciones por busqueda
 *    BST:           log2(16) = 4 comparaciones por busqueda
 *
 *  Operaciones:
 *    [1] Cargar desde archivo  (diccionario.txt)
 *    [2] Agregar palabra
 *    [3] Buscar palabra y mostrar significado
 *    [4] Eliminar palabra
 *    [5] Mostrar diccionario completo ordenado
 *    [6] Buscar por letra o prefijo
 *    [7] Estadisticas de la tabla hash
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

const int TAM_TABLA = 28;

// ============================================================
// ENTRADA: unidad basica del diccionario
// ============================================================
struct Entrada {
    string palabra;
    string significado;

    Entrada() {}
    Entrada(const string& p, const string& s) : palabra(p), significado(s) {}
};

// ============================================================
// [1] NODO LISTA ENLAZADA - implementacion BASE de colisiones
//     Cada bucket es una lista enlazada simple
//     Busqueda: O(k) donde k = items en el bucket
// ============================================================
struct NodoLista {
    Entrada dato;
    NodoLista* sig;
    NodoLista(const string& p, const string& s) : dato(p, s), sig(nullptr) {}
};

// Bucket con lista enlazada (implementacion requerida)
class BucketLista {
    string norm(const string& s) const {
        string r = s; transform(r.begin(),r.end(),r.begin(),::toupper); return r;
    }
public:
    NodoLista* cabeza;
    int cantidad;
    BucketLista() : cabeza(nullptr), cantidad(0) {}

    void insertar(const string& p, const string& s) {
        NodoLista* curr = cabeza;
        while(curr){ if(norm(curr->dato.palabra)==norm(p)){curr->dato.significado=s;return;} curr=curr->sig; }
        NodoLista* n = new NodoLista(p, s);
        n->sig = cabeza; cabeza = n; cantidad++;   // insertar al frente O(1)
    }

    NodoLista* buscar(const string& p) const {    // O(k) - recorre toda la lista
        NodoLista* curr = cabeza;
        while(curr){ if(norm(curr->dato.palabra)==norm(p)) return curr; curr=curr->sig; }
        return nullptr;
    }

    bool eliminar(const string& p) {
        if(!cabeza) return false;
        if(norm(cabeza->dato.palabra)==norm(p)){
            NodoLista* tmp=cabeza; cabeza=cabeza->sig; tmp->sig=nullptr; delete tmp; cantidad--; return true;
        }
        NodoLista* prev=cabeza;
        while(prev->sig){
            if(norm(prev->sig->dato.palabra)==norm(p)){
                NodoLista* tmp=prev->sig; prev->sig=tmp->sig; tmp->sig=nullptr; delete tmp; cantidad--; return true;
            }
            prev=prev->sig;
        }
        return false;
    }

    ~BucketLista(){
        NodoLista* curr=cabeza;
        while(curr){ NodoLista* nx=curr->sig; delete curr; curr=nx; }
    }
};

// TablaHash con LISTA ENLAZADA (implementacion base del requisito)
class TablaHashLista {
    BucketLista tabla[TAM_TABLA]; int total=0;
    int hashFn(const string& p) const {
        string s=p; transform(s.begin(),s.end(),s.begin(),::toupper);
        unsigned long long h=5381;
        for(unsigned char c:s) h=((h<<5)+h)+c;
        return (int)(h%TAM_TABLA);
    }
public:
    bool insertar(const string& p, const string& s){
        int i=hashFn(p); bool ex=(tabla[i].buscar(p)!=nullptr);
        tabla[i].insertar(p,s); if(!ex)total++; return !ex;
    }
    NodoLista* buscar(const string& p){ int i=hashFn(p); return tabla[i].buscar(p); }
    bool eliminar(const string& p){ int i=hashFn(p); if(tabla[i].eliminar(p)){total--;return true;}return false; }
    int getTotal() const { return total; }
    int bucketMax() const { int m=0; for(int i=0;i<TAM_TABLA;i++) m=max(m,tabla[i].cantidad); return m; }
};

// ============================================================
// [2] NODO DEL ARBOL BINARIO DE BUSQUEDA - MEJORA del requisito
//     Cada bucket es un BST ordenado alfabeticamente
//     Busqueda: O(log k) — mucho mejor que O(k) de la lista
// ============================================================
struct NodoBST {
    Entrada dato;
    NodoBST* izq;
    NodoBST* der;

    NodoBST(const string& p, const string& s)
        : dato(p, s), izq(nullptr), der(nullptr) {}
};

// ============================================================
// CLASE BST - Arbol Binario de Busqueda por bucket
// Mejora el rendimiento respecto a lista enlazada simple:
//   Lista: O(k)      BST: O(log k)   donde k = items en bucket
// ============================================================
class BST {
public:
    NodoBST* raiz;
    int cantidad;

    BST() : raiz(nullptr), cantidad(0) {}

    // Normalizar a mayusculas para comparacion
    string normalizar(const string& s) const {
        string r = s;
        transform(r.begin(), r.end(), r.begin(), ::toupper);
        return r;
    }

    // ---- INSERTAR ----
    // Si la palabra ya existe, actualiza el significado
    NodoBST* insertar(NodoBST* nodo, const string& p, const string& s) {
        if (!nodo) {
            cantidad++;
            return new NodoBST(p, s);
        }
        string k1 = normalizar(p);
        string k2 = normalizar(nodo->dato.palabra);

        if      (k1 < k2) nodo->izq = insertar(nodo->izq, p, s);
        else if (k1 > k2) nodo->der = insertar(nodo->der, p, s);
        else              nodo->dato.significado = s; // actualizacion
        return nodo;
    }

    void insertar(const string& p, const string& s) {
        raiz = insertar(raiz, p, s);
    }

    // ---- BUSCAR ----
    // Retorna puntero al nodo o nullptr si no existe
    NodoBST* buscar(NodoBST* nodo, const string& p) const {
        if (!nodo) return nullptr;
        string k1 = normalizar(p);
        string k2 = normalizar(nodo->dato.palabra);

        if      (k1 == k2) return nodo;
        else if (k1 < k2)  return buscar(nodo->izq, p);
        else               return buscar(nodo->der, p);
    }

    NodoBST* buscar(const string& p) const {
        return buscar(raiz, p);
    }

    // ---- ELIMINAR ----
    NodoBST* minimo(NodoBST* nodo) const {
        while (nodo->izq) nodo = nodo->izq;
        return nodo;
    }

    NodoBST* eliminar(NodoBST* nodo, const string& p) {
        if (!nodo) return nullptr;
        string k1 = normalizar(p);
        string k2 = normalizar(nodo->dato.palabra);

        if (k1 < k2) {
            nodo->izq = eliminar(nodo->izq, p);
        } else if (k1 > k2) {
            nodo->der = eliminar(nodo->der, p);
        } else {
            // Nodo encontrado
            if (!nodo->izq) {
                NodoBST* tmp = nodo->der;
                delete nodo;
                cantidad--;
                return tmp;
            }
            if (!nodo->der) {
                NodoBST* tmp = nodo->izq;
                delete nodo;
                cantidad--;
                return tmp;
            }
            // Nodo con dos hijos: reemplazar con sucesor inorden
            NodoBST* suc = minimo(nodo->der);
            nodo->dato = suc->dato;
            nodo->der  = eliminar(nodo->der, suc->dato.palabra);
        }
        return nodo;
    }

    bool eliminar(const string& p) {
        if (!buscar(p)) return false;
        raiz = eliminar(raiz, p);
        return true;
    }

    // ---- RECORRIDO INORDEN (ordenado alfabeticamente) ----
    void inorden(NodoBST* nodo, vector<Entrada>& res) const {
        if (!nodo) return;
        inorden(nodo->izq, res);
        res.push_back(nodo->dato);
        inorden(nodo->der, res);
    }

    vector<Entrada> inorden() const {
        vector<Entrada> res;
        inorden(raiz, res);
        return res;
    }

    // ---- LIBERAR MEMORIA ----
    void liberar(NodoBST* nodo) {
        if (!nodo) return;
        liberar(nodo->izq);
        liberar(nodo->der);
        delete nodo;
    }

    ~BST() { liberar(raiz); }
};

// ============================================================
// CLASE TABLA HASH
// 28 buckets, cada uno es un BST
// Funcion hash: DJB2 mod 28
// ============================================================
class TablaHash {
private:
    BST tabla[TAM_TABLA];
    int total;
    vector<Entrada> nuevas; // palabras agregadas en esta sesion

    // Funcion hash DJB2 - distribucion uniforme
    int calcularHash(const string& palabra) const {
        string p = palabra;
        transform(p.begin(), p.end(), p.begin(), ::toupper);
        unsigned long long h = 5381;
        for (unsigned char c : p)
            h = ((h << 5) + h) + c;
        return (int)(h % TAM_TABLA);
    }

public:
    TablaHash() : total(0) {}

    // ---- INSERTAR ----
    // esNueva=true  -> agregada por el usuario (se guarda en seccion PALABRAS NUEVAS)
    // esNueva=false -> cargada desde archivo (no se marca como nueva)
    bool insertar(const string& palabra, const string& significado, bool esNueva = true) {
        int idx = calcularHash(palabra);
        bool existia = (tabla[idx].buscar(palabra) != nullptr);
        tabla[idx].insertar(palabra, significado);
        if (!existia) {
            total++;
            if (esNueva) nuevas.push_back(Entrada(palabra, significado));
        }
        return !existia;
    }

    // ---- BUSCAR ----
    Entrada* buscar(const string& palabra) {
        int idx = calcularHash(palabra);
        NodoBST* nodo = tabla[idx].buscar(palabra);
        return nodo ? &nodo->dato : nullptr;
    }

    // ---- ELIMINAR ----
    bool eliminar(const string& palabra) {
        int idx = calcularHash(palabra);
        if (tabla[idx].eliminar(palabra)) {
            total--;
            // Quitar de la lista de nuevas si estaba ahi
            string pUp = palabra;
            transform(pUp.begin(), pUp.end(), pUp.begin(), ::toupper);
            nuevas.erase(remove_if(nuevas.begin(), nuevas.end(), [&](const Entrada& e) {
                string ep = e.palabra;
                transform(ep.begin(), ep.end(), ep.begin(), ::toupper);
                return ep == pUp;
            }), nuevas.end());
            return true;
        }
        return false;
    }

    // ---- CARGAR DESDE ARCHIVO ----
    // Formato: PALABRA|definicion
    // Lineas con # son comentarios
    int cargarArchivo(const string& ruta) {
        ifstream f(ruta);
        if (!f.is_open()) return -1;

        int cnt = 0;
        string linea;
        while (getline(f, linea)) {
            if (linea.empty() || linea[0] == '#') continue;
            // Remover \r si existe (archivos Windows)
            if (!linea.empty() && linea.back() == '\r')
                linea.pop_back();

            size_t sep = linea.find('|');
            if (sep == string::npos) continue;

            string palabra    = linea.substr(0, sep);
            string significado = linea.substr(sep + 1);

            // Trim de espacios en extremos
            while (!palabra.empty()     && palabra.front()     == ' ') palabra.erase(0, 1);
            while (!palabra.empty()     && palabra.back()      == ' ') palabra.pop_back();
            while (!significado.empty() && significado.front() == ' ') significado.erase(0, 1);
            while (!significado.empty() && significado.back()  == ' ') significado.pop_back();

            if (palabra.empty() || significado.empty()) continue;
            if (insertar(palabra, significado, false)) cnt++; // false = viene del archivo
        }
        f.close();
        return cnt;
    }

    // ---- GUARDAR NUEVAS AL FINAL DEL ARCHIVO ----
    // Agrega las palabras nuevas debajo de "# PALABRAS NUEVAS"
    bool guardarNuevas(const string& ruta) {
        if (nuevas.empty()) return true; // nada que guardar

        ofstream f(ruta, ios::app); // modo append = agrega al final
        if (!f.is_open()) return false;

        f << "# ============================================================\n";
        f << "# PALABRAS NUEVAS\n";
        f << "# ============================================================\n";
        for (auto& e : nuevas)
            f << e.palabra << "|" << e.significado << "\n";

        f.close();
        nuevas.clear(); // ya guardadas, limpiar lista de pendientes
        return true;
    }

    int getNuevas() const { return (int)nuevas.size(); }

    // ---- GUARDAR EN ARCHIVO ----
    bool guardarArchivo(const string& ruta) {
        ofstream f(ruta);
        if (!f.is_open()) return false;

        f << "# DICCIONARIO DEL HABLA POPULAR CAMBA\n";
        f << "# Formato: PALABRA|definicion\n";

        // Obtener todas las entradas y ordenarlas
        vector<Entrada> todo = obtenerTodas();

        for (auto& e : todo)
            f << e.palabra << "|" << e.significado << "\n";

        f.close();
        return true;
    }

    // ---- OBTENER TODAS LAS ENTRADAS (ordenadas) ----
    vector<Entrada> obtenerTodas() const {
        vector<Entrada> todo;
        for (int i = 0; i < TAM_TABLA; i++) {
            vector<Entrada> bucket = tabla[i].inorden();
            for (auto& e : bucket) todo.push_back(e);
        }
        sort(todo.begin(), todo.end(), [](const Entrada& a, const Entrada& b) {
            string pa = a.palabra, pb = b.palabra;
            transform(pa.begin(), pa.end(), pa.begin(), ::toupper);
            transform(pb.begin(), pb.end(), pb.begin(), ::toupper);
            return pa < pb;
        });
        return todo;
    }

    // ---- MOSTRAR DICCIONARIO COMPLETO ----
    void mostrarTodo(bool pausar = true) const {
        vector<Entrada> todo = obtenerTodas();
        cout << "\n";
        cout << "  +-" << string(24, '-') << "-+-" << string(55, '-') << "-+\n";
        cout << "  | " << left << setw(24) << "PALABRA"
             << " | " << setw(55) << "SIGNIFICADO" << " |\n";
        cout << "  +-" << string(24, '-') << "-+-" << string(55, '-') << "-+\n";

        int mostrados = 0;
        for (auto& e : todo) {
            string pal = e.palabra;
            string sig = e.significado;

            // Truncar si es muy largo para la tabla
            if (pal.size() > 24) pal = pal.substr(0, 21) + "...";

            // Mostrar definicion cortada en multiples filas si es necesaria
            string primera = sig.size() > 55 ? sig.substr(0, 52) + "..." : sig;

            cout << "  | " << left << setw(24) << pal
                 << " | " << setw(55) << primera << " |\n";

            mostrados++;
            // Pausa cada 20 entradas
            if (pausar && mostrados % 20 == 0) {
                cout << "  +-" << string(24, '-') << "-+-" << string(55, '-') << "-+\n";
                cout << "  [" << mostrados << "/" << (int)todo.size()
                     << "] Presione ENTER para continuar...";
                cin.ignore(10000, '\n');
                cout << "  +-" << string(24, '-') << "-+-" << string(55, '-') << "-+\n";
            }
        }
        cout << "  +-" << string(24, '-') << "-+-" << string(55, '-') << "-+\n";
        cout << "  Total: " << todo.size() << " entradas en el diccionario\n";
    }

    // ---- BUSCAR POR PREFIJO ----
    void buscarPrefijo(const string& prefijo) const {
        string pUp = prefijo;
        transform(pUp.begin(), pUp.end(), pUp.begin(), ::toupper);

        vector<Entrada> encontradas;
        for (int i = 0; i < TAM_TABLA; i++) {
            vector<Entrada> bucket = tabla[i].inorden();
            for (auto& e : bucket) {
                string ep = e.palabra;
                transform(ep.begin(), ep.end(), ep.begin(), ::toupper);
                if (ep.size() >= pUp.size() && ep.substr(0, pUp.size()) == pUp)
                    encontradas.push_back(e);
            }
        }

        if (encontradas.empty()) {
            cout << "\n  No se encontraron entradas con prefijo \"" << prefijo << "\"\n";
            return;
        }

        sort(encontradas.begin(), encontradas.end(), [](const Entrada& a, const Entrada& b) {
            string pa = a.palabra, pb = b.palabra;
            transform(pa.begin(), pa.end(), pa.begin(), ::toupper);
            transform(pb.begin(), pb.end(), pb.begin(), ::toupper);
            return pa < pb;
        });

        cout << "\n  Entradas con prefijo \"" << prefijo << "\": "
             << encontradas.size() << " resultados\n";
        cout << "  " << string(84, '-') << "\n";
        for (auto& e : encontradas) {
            string sig = e.significado.size() > 55
                       ? e.significado.substr(0, 52) + "..."
                       : e.significado;
            cout << "  " << left << setw(26) << e.palabra << " -> " << sig << "\n";
        }
        cout << "  " << string(84, '-') << "\n";
    }

    // ---- ESTADISTICAS ----
    void estadisticas() const {
        cout << "\n";
        cout << "  ====================================================\n";
        cout << "   ESTADISTICAS DE LA TABLA HASH\n";
        cout << "  ====================================================\n";
        cout << "  Posiciones (buckets): " << TAM_TABLA << "\n";
        cout << "  Entradas totales    : " << total << "\n";

        int usados = 0, vacion = 0, maxBucket = 0, colisiones = 0;
        for (int i = 0; i < TAM_TABLA; i++) {
            int n = tabla[i].cantidad;
            if (n > 0) usados++; else vacion++;
            if (n > maxBucket) maxBucket = n;
            if (n > 1) colisiones += (n - 1);
        }

        double factorCarga = (double)total / TAM_TABLA;
        cout << "  Buckets usados      : " << usados << "\n";
        cout << "  Buckets vacios      : " << vacion << "\n";
        cout << fixed << setprecision(2);
        cout << "  Factor de carga     : " << factorCarga << "\n";
        cout << "  Colisiones          : " << colisiones << "\n";
        cout << "  Max por bucket      : " << maxBucket << "\n";

        cout << "\n  Ventaja del BST sobre lista enlazada:\n";
        if (maxBucket > 1) {
            cout << "    Lista enlazada: hasta " << maxBucket << " comparaciones por bucket\n";
            int logBST = 1;
            int n = maxBucket;
            while (n > 1) { n /= 2; logBST++; }
            cout << "    BST:            hasta " << logBST << " comparaciones por bucket (log2)\n";
            cout << "    Mejora:         " << maxBucket << " -> " << logBST
                 << " comparaciones en el peor caso\n";
        }

        cout << "\n  Distribucion por posicion:\n";
        cout << "  " << string(50, '-') << "\n";
        for (int i = 0; i < TAM_TABLA; i++) {
            int n = tabla[i].cantidad;
            cout << "  [" << setw(2) << i << "] ";
            cout << string(min(n, 25), '#');
            if (n > 25) cout << "+" ;
            cout << " (" << n << ")\n";
        }
        cout << "  " << string(50, '-') << "\n";
    }

    int getTotal() const { return total; }

    // Mostrar hash de una palabra (para explicacion academica)
    void mostrarHash(const string& palabra) const {
        int idx = calcularHash(palabra);
        cout << "\n  Hash de \"" << palabra << "\": " << idx
             << " (bucket " << idx << " de " << TAM_TABLA << ")\n";
    }
};

// ============================================================
// INTERFAZ DE USUARIO
// ============================================================

void limpiarPantalla() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void encabezado() {
    cout << "\n";
    cout << "  +======================================================+\n";
    cout << "  |        EL DICCIONARIO DEL HABLA POPULAR              |\n";
    cout << "  |        Santa Cruz de la Sierra - Bolivia             |\n";
    cout << "  |        Recopilacion: Sanchico                        |\n";
    cout << "  |        Implementacion: Tabla Hash + BST              |\n";
    cout << "  +======================================================+\n";
}

void mostrarMenu(int total, int pendientes) {
    cout << "\n";
    cout << "  Entradas en el diccionario: " << total << "\n";
    if (pendientes > 0)
        cout << "  Palabras nuevas sin guardar: " << pendientes << " (opcion 8)\n";
    cout << "  " << string(46, '-') << "\n";
    cout << "  [1] Buscar una palabra o frase\n";
    cout << "  [2] Agregar nueva entrada\n";
    cout << "  [3] Eliminar una entrada\n";
    cout << "  [4] Mostrar diccionario completo\n";
    cout << "  [5] Buscar por letra o prefijo\n";
    cout << "  [6] Ver hash de una palabra\n";
    cout << "  [7] Estadisticas de la tabla hash (BST)\n";
    cout << "  [8] Guardar palabras nuevas en archivo\n";
    cout << "  [9] Comparar Lista Enlazada vs BST (demostracion)\n";
    cout << "  [0] Salir\n";
    cout << "  " << string(46, '-') << "\n";
    cout << "  Opcion: ";
}

void pausar() {
    cout << "\n  Presione ENTER para continuar...";
    cin.ignore(10000, '\n');
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(1252);
    SetConsoleCP(1252);
#endif

    TablaHash diccionario;

    limpiarPantalla();
    encabezado();

    // Carga automatica del archivo de datos
    cout << "\n  Cargando diccionario desde archivo...\n";
    int cargadas = diccionario.cargarArchivo("diccionario.txt");
    if (cargadas < 0) {
        cout << "  AVISO: No se encontro diccionario.txt\n";
        cout << "  El diccionario comienza vacio.\n";
    } else {
        cout << "  " << cargadas << " entradas cargadas correctamente.\n";
    }

    pausar();

    int opcion;
    string entrada, significado;

    do {
        limpiarPantalla();
        encabezado();
        mostrarMenu(diccionario.getTotal(), diccionario.getNuevas());

        cin >> opcion;
        cin.ignore(10000, '\n');

        switch (opcion) {

        // ---- BUSCAR ----
        case 1: {
            cout << "\n  Ingrese la palabra o frase a buscar: ";
            getline(cin, entrada);

            Entrada* e = diccionario.buscar(entrada);
            if (e) {
                diccionario.mostrarHash(entrada);
                cout << "\n  ENCONTRADA:\n";
                cout << "  " << string(80, '=') << "\n";
                cout << "  Palabra   : " << e->palabra << "\n";
                cout << "  " << string(80, '-') << "\n";
                // Imprimir significado con salto de linea cada 75 chars
                string sig = e->significado;
                int pos = 0;
                while ((int)sig.size() - pos > 75) {
                    int corte = pos + 75;
                    while (corte > pos && sig[corte] != ' ') corte--;
                    if (corte == pos) corte = pos + 75;
                    cout << "  " << sig.substr(pos, corte - pos) << "\n";
                    pos = corte + 1;
                }
                if (pos < (int)sig.size())
                    cout << "  " << sig.substr(pos) << "\n";
                cout << "  " << string(80, '=') << "\n";
            } else {
                cout << "\n  La palabra \"" << entrada << "\" no existe en el diccionario.\n";
                cout << "  Sugerencia: pruebe buscar por prefijo (opcion 5).\n";
            }
            pausar();
            break;
        }

        // ---- AGREGAR ----
        case 2: {
            cout << "\n  Ingrese la palabra o frase: ";
            getline(cin, entrada);
            if (entrada.empty()) { cout << "  Entrada vacia, cancelado.\n"; pausar(); break; }

            cout << "  Ingrese el significado    : ";
            getline(cin, significado);
            if (significado.empty()) { cout << "  Significado vacio, cancelado.\n"; pausar(); break; }

            diccionario.mostrarHash(entrada);
            if (diccionario.insertar(entrada, significado)) {
                cout << "  OK: \"" << entrada << "\" agregada correctamente.\n";
            } else {
                cout << "  OK: Significado de \"" << entrada << "\" actualizado.\n";
            }
            pausar();
            break;
        }

        // ---- ELIMINAR ----
        case 3: {
            cout << "\n  Ingrese la palabra o frase a eliminar: ";
            getline(cin, entrada);

            Entrada* e = diccionario.buscar(entrada);
            if (!e) {
                cout << "  La palabra \"" << entrada << "\" no existe en el diccionario.\n";
            } else {
                cout << "  Entrada encontrada:\n";
                cout << "    Palabra   : " << e->palabra << "\n";
                cout << "    Significado: " << e->significado.substr(0, 60) << "...\n";
                cout << "  Confirmar eliminacion? (s/n): ";
                char c; cin >> c; cin.ignore(10000, '\n');
                if (c == 's' || c == 'S') {
                    diccionario.eliminar(entrada);
                    cout << "  OK: \"" << entrada << "\" eliminada del diccionario.\n";
                } else {
                    cout << "  Eliminacion cancelada.\n";
                }
            }
            pausar();
            break;
        }

        // ---- MOSTRAR TODO ----
        case 4: {
            cout << "\n";
            diccionario.mostrarTodo(true);
            pausar();
            break;
        }

        // ---- BUSCAR POR PREFIJO ----
        case 5: {
            cout << "\n  Ingrese letra o prefijo (ej: CH, MA, FARRA): ";
            getline(cin, entrada);
            diccionario.buscarPrefijo(entrada);
            pausar();
            break;
        }

        // ---- VER HASH ----
        case 6: {
            cout << "\n  Ingrese una palabra para ver su valor hash: ";
            getline(cin, entrada);
            diccionario.mostrarHash(entrada);
            cout << "  Esto indica en que bucket (0-27) se almacena la palabra.\n";
            cout << "  Dentro del bucket, el BST la ordena alfabeticamente.\n";
            pausar();
            break;
        }

        // ---- ESTADISTICAS ----
        case 7: {
            diccionario.estadisticas();
            pausar();
            break;
        }

        // ---- GUARDAR NUEVAS ----
        case 8: {
            int pend = diccionario.getNuevas();
            if (pend == 0) {
                cout << "\n  No hay palabras nuevas para guardar.\n";
            } else if (diccionario.guardarNuevas("diccionario.txt")) {
                cout << "\n  OK: " << pend << " palabra(s) guardada(s) en la seccion\n";
                cout << "  \"PALABRAS NUEVAS\" al final de diccionario.txt\n";
            } else {
                cout << "\n  ERROR: No se pudo guardar en el archivo.\n";
            }
            pausar();
            break;
        }

        // ---- COMPARACION LISTA vs BST ----
        case 9: {
            cout << "\n  === COMPARACION: Lista Enlazada vs Arbol BST ===\n";
            cout << "  Cargando diccionario en AMBAS implementaciones...\n";

            TablaHashLista hashLista;
            TablaHash      hashBST;

            // Cargar en ambas
            ifstream f("diccionario.txt");
            int cnt=0; string ln;
            while(getline(f,ln)){
                if(ln.empty()||ln[0]=='#') continue;
                if(!ln.empty()&&ln.back()=='\r') ln.pop_back();
                size_t s=ln.find('|'); if(s==string::npos) continue;
                string p=ln.substr(0,s), sig=ln.substr(s+1);
                while(!p.empty()&&p.back()==' ')p.pop_back();
                if(p.empty()||sig.empty()) continue;
                hashLista.insertar(p, sig);
                hashBST.insertar(p, sig, false);
                cnt++;
            }
            f.close();

            cout << "  " << cnt << " palabras cargadas en ambas tablas.\n\n";
            cout << "  " << string(52,'-') << "\n";
            cout << "  IMPLEMENTACION      | Colisiones | Busqueda\n";
            cout << "  " << string(52,'-') << "\n";
            cout << "  Lista Enlazada      | O(n/28)    | O(" << hashLista.bucketMax() << ") max bucket\n";
            cout << "  Arbol BST (mejora)  | O(log n/28)| O(log " << hashLista.bucketMax() << ") = O(" ;
            int bm=hashLista.bucketMax(), lg=0; int x=bm; while(x>1){x/=2;lg++;}
            cout << lg << ") max bucket\n";
            cout << "  " << string(52,'-') << "\n\n";

            // Buscar misma palabra en ambas y contar pasos simulados
            cout << "  Busqueda de prueba: 'CHICHA'\n";
            auto r1 = hashLista.buscar("CHICHA");
            auto r2 = diccionario.buscar("CHICHA");
            if(r1) cout << "  Lista: ENCONTRADA -> " << r1->dato.significado.substr(0,50) << "\n";
            if(r2) cout << "  BST  : ENCONTRADA -> " << r2->significado.substr(0,50) << "\n";

            cout << "\n  CONCLUSION: Con " << cnt << " palabras y 28 buckets (~"
                 << cnt/28 << " por bucket promedio)\n";
            cout << "  Lista enlazada: hasta " << hashLista.bucketMax() << " comparaciones en peor caso\n";
            cout << "  BST:            hasta " << lg << " comparaciones en peor caso (log2)\n";
            cout << "  Mejora:         " << hashLista.bucketMax() << "x mas rapido con BST\n";
            pausar();
            break;
        }

        case 0:
            cout << "\n  Hasta luego! Que le vaya bien, pues!\n\n";
            break;

        default:
            cout << "\n  Opcion invalida. Intente de nuevo.\n";
            pausar();
        }

    } while (opcion != 0);

    return 0;
}
