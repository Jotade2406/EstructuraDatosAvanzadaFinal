/*
 * ============================================================
 *  EL DICCIONARIO DEL HABLA CAMBA
 *  Santa Cruz de la Sierra - Bolivia
 * ============================================================
 *  Juego estilo Bookworm + Gestion de diccionario
 *  Backend: Tabla Hash 28 posiciones + Lista Enlazada/BST
 *
 *  Pantallas:
 *    MENU       -> Jugar / Diccionario / Descripcion
 *    JUEGO      -> Forma palabras cambas en el tablero 7x4
 *    DICCIONARIO-> Buscar / Agregar / Eliminar palabras
 *    DESCRIPCION-> Resumen del juego y estructuras de datos
 *    GAME OVER  -> Puntuacion final
 * ============================================================
 */

#include <raylib.h>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <map>
#include <cstring>
using namespace std;

// ============================================================
// FUENTE GLOBAL (TTF del sistema para mejor legibilidad)
// ============================================================
static Font gFont;
static bool gFontLoaded = false;

// Wrapper de DrawText usando fuente TTF si esta cargada
void TXT(const char* text, int x, int y, int fs, Color col) {
    if(gFontLoaded)
        DrawTextEx(gFont, text, {(float)x,(float)y}, (float)fs, 1.f, col);
    else
        TXT(text, x, y, fs, col);
}

int TXTW(const char* text, int fs) {
    if(gFontLoaded)
        return (int)MeasureTextEx(gFont, text, (float)fs, 1.f).x;
    return TXTW(text, fs);
}

// ============================================================
// [1] ENTRADA
// ============================================================
struct Entrada {
    string palabra, significado;
    bool esNueva = false; // agregada en sesion actual
    Entrada(){}
    Entrada(const string& p,const string& s,bool n=false)
        :palabra(p),significado(s),esNueva(n){}
};

// ============================================================
// [2] NODO LISTA ENLAZADA (colisiones base)
//     El BST reemplaza la lista para mejor rendimiento O(log n)
// ============================================================
struct NodoLista {
    Entrada dato;
    NodoLista* sig;
    NodoLista(const string& p,const string& s,bool n=false)
        :dato(p,s,n),sig(nullptr){}
};

// ============================================================
// [3] NODO BST (mejora de rendimiento sobre lista enlazada)
//     Busqueda: lista O(k) -> BST O(log k) por bucket
// ============================================================
struct NodoBST {
    Entrada dato;
    NodoBST *izq, *der;
    NodoBST(const string& p,const string& s,bool n=false)
        :dato(p,s,n),izq(nullptr),der(nullptr){}
};

class BST {
    string norm(const string& s) const {
        string r=s; transform(r.begin(),r.end(),r.begin(),::toupper); return r;
    }
    NodoBST* ins(NodoBST* n,const string& p,const string& s,bool nu){
        if(!n){cant++;return new NodoBST(p,s,nu);}
        string k1=norm(p),k2=norm(n->dato.palabra);
        if(k1<k2) n->izq=ins(n->izq,p,s,nu);
        else if(k1>k2) n->der=ins(n->der,p,s,nu);
        else { n->dato.significado=s; } // actualizar
        return n;
    }
    NodoBST* bus(NodoBST* n,const string& p) const {
        if(!n) return nullptr;
        string k1=norm(p),k2=norm(n->dato.palabra);
        if(k1==k2) return n;
        return k1<k2 ? bus(n->izq,p) : bus(n->der,p);
    }
    NodoBST* minN(NodoBST* n){while(n->izq)n=n->izq;return n;}
    NodoBST* del(NodoBST* n,const string& p){
        if(!n) return nullptr;
        string k1=norm(p),k2=norm(n->dato.palabra);
        if(k1<k2) n->izq=del(n->izq,p);
        else if(k1>k2) n->der=del(n->der,p);
        else {
            if(!n->izq){NodoBST* t=n->der;delete n;cant--;return t;}
            if(!n->der){NodoBST* t=n->izq;delete n;cant--;return t;}
            NodoBST* s=minN(n->der);
            n->dato=s->dato;
            n->der=del(n->der,s->dato.palabra);
        }
        return n;
    }
    void lib(NodoBST* n){if(!n)return;lib(n->izq);lib(n->der);delete n;}
    void inord(NodoBST* n,vector<Entrada>& v) const {
        if(!n)return;inord(n->izq,v);v.push_back(n->dato);inord(n->der,v);
    }
public:
    NodoBST* raiz; int cant;
    BST():raiz(nullptr),cant(0){}
    ~BST(){lib(raiz);}
    void insertar(const string& p,const string& s,bool nu=false){raiz=ins(raiz,p,s,nu);}
    NodoBST* buscar(const string& p) const {return bus(raiz,p);}
    bool eliminar(const string& p){if(!buscar(p))return false;raiz=del(raiz,p);return true;}
    vector<Entrada> inorden() const {vector<Entrada> v;inord(raiz,v);return v;}
};

// ============================================================
// [4] TABLA HASH - 28 posiciones, BST por colision
// ============================================================
const int TAM=28;
class TablaHash {
    BST tabla[TAM];
    int total=0;
    vector<Entrada> nuevas; // palabras agregadas en sesion

    int hashFn(const string& p) const {
        string s=p; transform(s.begin(),s.end(),s.begin(),::toupper);
        unsigned long long h=5381;
        for(unsigned char c:s) h=((h<<5)+h)+c;
        return (int)(h%TAM);
    }
public:
    int ultimoBucket=-1;

    // Cargar desde archivo (esNueva=false)
    int cargar(const string& ruta){
        ifstream f(ruta); if(!f.is_open()) return -1;
        int cnt=0; string ln;
        while(getline(f,ln)){
            if(ln.empty()||ln[0]=='#') continue;
            if(!ln.empty()&&ln.back()=='\r') ln.pop_back();
            size_t s=ln.find('|'); if(s==string::npos) continue;
            string p=ln.substr(0,s), sig=ln.substr(s+1);
            while(!p.empty()&&p.front()==' ') p.erase(0,1);
            while(!p.empty()&&p.back()==' ') p.pop_back();
            while(!sig.empty()&&sig.front()==' ') sig.erase(0,1);
            if(p.empty()||sig.empty()) continue;
            insertar(p,sig,false); cnt++;
        }
        f.close(); return cnt;
    }

    // Insertar (esNueva=true = agregada por usuario)
    bool insertar(const string& p,const string& s,bool esNueva=true){
        int i=hashFn(p);
        bool ex=(tabla[i].buscar(p)!=nullptr);
        tabla[i].insertar(p,s,esNueva);
        if(!ex){ total++; if(esNueva) nuevas.push_back(Entrada(p,s,true)); }
        return !ex;
    }

    // Buscar
    Entrada* buscar(const string& p){
        int i=hashFn(p); ultimoBucket=i;
        NodoBST* n=tabla[i].buscar(p);
        return n ? &n->dato : nullptr;
    }

    // Eliminar de memoria (no del archivo)
    bool eliminar(const string& p){
        int i=hashFn(p);
        if(tabla[i].eliminar(p)){ total--; return true; }
        return false;
    }

    // Guardar palabras nuevas al final del archivo
    bool guardarNuevas(const string& ruta){
        if(nuevas.empty()) return true;
        ofstream f(ruta,ios::app); if(!f.is_open()) return false;
        f << "# ============================================================\n";
        f << "# PALABRAS NUEVAS\n";
        f << "# ============================================================\n";
        for(auto& e:nuevas) f << e.palabra << "|" << e.significado << "\n";
        f.close(); nuevas.clear(); return true;
    }

    vector<Entrada> todas() const {
        vector<Entrada> v;
        for(int i=0;i<TAM;i++){auto b=tabla[i].inorden();for(auto& e:b)v.push_back(e);}
        return v;
    }
    int bucketSize(int i) const { return (i>=0&&i<TAM)?tabla[i].cant:0; }
    int getTotal() const { return total; }
    int getNuevas() const { return (int)nuevas.size(); }
};

// ============================================================
// CONSTANTES DEL JUEGO
// ============================================================
const int WIN_W=1100, WIN_H=700;
const int GCOLS=7, GROWS=4;  // 7x4 = 28 tiles = 28 buckets!
const int TILE=82, GAP=6;
const int GX=452, GY=92;
const int MAX_LIVES=5;

// ============================================================
// ESTRUCTURAS DE DATOS DEL JUEGO
// ============================================================
struct Tile {
    char letra=' ';
    bool sel=false, usada=false;
    float shakeT=0, flashT=0;
    bool flashOk=false;
};

enum Scr { SC_MENU, SC_GAME, SC_DICT, SC_DESC, SC_OVER, SC_WIN };
const int WIN_SCORE = 20000; // puntos para ganar

struct WordEntry { string palabra; int pts; string def; };

struct GState {
    Tile grid[GROWS][GCOLS];
    string palabra;
    vector<pair<int,int>> orden;
    int score=0, lives=MAX_LIVES, combo=0;
    string msgWord, msgDef;
    bool msgOk=false;
    float msgT=0;
    int lastBucket=-1, lastBucketSz=0;
    bool showHash=false;
    float hashT=0;
    string hintWord;
    int hintRevealed=0;
    float hintT=0;
    string garantizada;
    vector<WordEntry> historial; // palabras encontradas en la sesion
    int histScroll=0;            // scroll del historial
};

struct DictState {
    string search;
    vector<Entrada> filtered;
    int scroll=0, selected=-1;
    bool addMode=false;
    string newWord, newDef;
    int addField=0; // 0=word, 1=def
    string statusMsg;
    float statusT=0;
    bool deleteConfirm=false;
};

// ============================================================
// GENERACION DE GRID CON PALABRA GARANTIZADA
// ============================================================
static mt19937 rng((unsigned)time(nullptr));

char letraRnd(){
    static const string pool=
        "AAAAAAEEEEEIIIIOOOOUUU"
        "CCCCHHHRRRRSSSSTTTTNNNMMMLLL"
        "BBBDDDFFFGGGPPPVVVJJJ"
        "QXYZ";
    return pool[rng()%pool.size()];
}

// Verifica si una palabra puede formarse con los tiles disponibles
bool puedeFormar(const string& word, Tile grid[GROWS][GCOLS]){
    map<char,int> need, have;
    for(char c:word) need[c]++;
    for(int r=0;r<GROWS;r++) for(int c=0;c<GCOLS;c++)
        if(!grid[r][c].usada) have[grid[r][c].letra]++;
    for(auto& [ch,cnt]:need)
        if(have[ch]<cnt) return false;
    return true;
}

// Busca una pista entre todas las palabras del diccionario
string buscarPista(GState& g, TablaHash& dic){
    vector<Entrada> todas=dic.todas();
    shuffle(todas.begin(),todas.end(),rng);
    for(auto& e:todas){
        string p=e.palabra;
        if(p.find(' ')!=string::npos) continue;
        if(p.size()<4||p.size()>10) continue;
        bool ok=true;
        for(char c:p) if(!isalpha((unsigned char)c)){ok=false;break;}
        if(ok && puedeFormar(p,g.grid)) return p;
    }
    return "";
}

void generarGrid(GState& g, TablaHash& dic){
    // Obtener palabras validas (sin espacios, 4-9 letras, solo A-Z)
    vector<string> cands;
    for(auto& e:dic.todas()){
        string p=e.palabra;
        if(p.find(' ')!=string::npos) continue;
        if(p.size()<4||p.size()>9) continue;
        bool ok=true;
        for(char c:p) if(!isalpha((unsigned char)c)){ok=false;break;}
        if(ok) cands.push_back(p);
    }
    shuffle(cands.begin(),cands.end(),rng);

    // La primera palabra es la garantizada
    string letras;
    g.garantizada="";
    for(auto& w:cands){
        if((int)w.size()<=GROWS*GCOLS){
            g.garantizada=w; letras=w; break;
        }
    }

    // Agregar mas palabras sin exceder 28 letras
    for(auto& w:cands){
        if(w==g.garantizada) continue;
        if((int)(letras.size()+w.size())<=GROWS*GCOLS) letras+=w;
        if((int)letras.size()>=GROWS*GCOLS-4) break;
    }

    // Rellenar con letras aleatorias hasta 28
    while((int)letras.size()<GROWS*GCOLS) letras+=letraRnd();
    letras=letras.substr(0,GROWS*GCOLS);
    shuffle(letras.begin(),letras.end(),rng);

    // Colocar en grid
    int idx=0;
    for(int r=0;r<GROWS;r++) for(int c=0;c<GCOLS;c++){
        g.grid[r][c]=Tile();
        g.grid[r][c].letra=letras[idx++];
    }

    g.palabra=""; g.orden.clear();
    g.hintWord=""; g.hintRevealed=0; g.hintT=0;
}

// ============================================================
// UTILIDADES DE DIBUJO
// ============================================================
void wrapText(const string& txt,int x,int y,int maxW,int fs,Color col){
    if(txt.empty()) return;
    istringstream ss(txt); string word,line; int cy=y;
    while(ss>>word){
        string test=line.empty()?word:line+" "+word;
        if(TXTW(test.c_str(),fs)>maxW){
            if(!line.empty()){TXT(line.c_str(),x,cy,fs,col);cy+=fs+4;line=word;}
            else{TXT(word.c_str(),x,cy,fs,col);cy+=fs+4;line="";}
        } else line=test;
    }
    if(!line.empty()) TXT(line.c_str(),x,cy,fs,col);
}

void drawBtn(int x,int y,int w,int h,const char* lbl,Color bg,Color brd,int fs=14){
    DrawRectangleRounded({(float)x,(float)y,(float)w,(float)h},0.25f,8,bg);
    DrawRectangleRoundedLinesEx({(float)x,(float)y,(float)w,(float)h},0.25f,8,2,brd);
    int lw=TXTW(lbl,fs);
    TXT(lbl,x+w/2-lw/2,y+h/2-fs/2,fs,WHITE);
}

bool btnClick(int x,int y,int w,int h,Vector2 m,bool lclick){
    return lclick&&m.x>=x&&m.x<x+w&&m.y>=y&&m.y<y+h;
}

void drawTile(Tile& t,int x,int y,int ordIdx,bool hover){
    float sx=0;
    if(t.shakeT>0) sx=sinf(t.shakeT*35)*5*(t.shakeT/0.5f);
    int tx=x+(int)sx,ty=y;
    Color bg,brd,tc;
    if(t.usada){ bg={50,40,26,200};brd={75,60,40,200};tc={80,70,50,160}; }
    else if(t.sel){
        if(t.flashT>0&&t.flashOk) bg={(unsigned char)(60+(int)(t.flashT/0.5f*40)),(unsigned char)(170+(int)(t.flashT/0.5f*50)),60,255};
        else if(t.flashT>0&&!t.flashOk) bg={(unsigned char)(190+(int)(t.flashT/0.5f*55)),(unsigned char)(50*(t.flashT/0.5f)),40,255};
        else bg={230,185,40,255};
        brd={255,235,80,255}; tc={35,15,0,255};
    } else if(hover){ bg={195,160,88,255};brd={220,185,110,255};tc={35,15,0,255}; }
    else { bg={165,125,72,255};brd={125,90,48,255};tc={35,15,0,255}; }
    DrawRectangle(tx+3,ty+4,TILE,TILE,{0,0,0,65});
    DrawRectangleRounded({(float)tx,(float)ty,(float)TILE,(float)TILE},0.18f,8,bg);
    DrawRectangleRoundedLinesEx({(float)tx,(float)ty,(float)TILE,(float)TILE},0.18f,8,t.sel?3:2,brd);
    if(!t.usada){
        char str[2]={t.letra,0};
        int fw=TXTW(str,42);
        // Sombra para legibilidad
        TXT(str,tx+TILE/2-fw/2+2,ty+TILE/2-21+2,42,{0,0,0,110});
        // Letra: blanca en tiles normales, oscura en seleccionadas (fondo dorado)
        Color lc=t.sel?Color{30,15,0,255}:Color{255,252,220,255};
        TXT(str,tx+TILE/2-fw/2,ty+TILE/2-21,42,lc);
    }
    if(t.sel&&!t.usada&&ordIdx>=0){
        char num[4]; snprintf(num,sizeof(num),"%d",ordIdx+1);
        TXT(num,tx+TILE-18,ty+4,14,{255,255,255,230});
    }
}

// ============================================================
// PANTALLA: MENU PRINCIPAL
// ============================================================
void drawMenu(int sel, bool musicOn, float escHold){
    ClearBackground({18,11,4,255});

    // Letras de fondo decorativas
    const char* bgLts="CAMBAOSCHURIQUIDICCIONARIOJICHICHICHA";
    for(int i=0;i<(int)strlen(bgLts);i++){
        int x=(i%10)*115+30, y=(i/10)*130+70;
        char ltr[2]={bgLts[i],0};
        TXT(ltr,x,y,90,{35,24,10,255});
    }

    // Panel central
    DrawRectangleRounded({220,90,660,490},0.06f,8,{26,16,5,235});
    DrawRectangleRoundedLinesEx({220,90,660,490},0.06f,8,3,{145,105,42,255});

    // Titulo
    const char* t1="EL DICCIONARIO";
    const char* t2="DEL HABLA CAMBA";
    TXT(t1,WIN_W/2-TXTW(t1,44)/2,112,44,{228,188,62,255});
    TXT(t2,WIN_W/2-TXTW(t2,38)/2,162,38,{188,148,48,255});
    DrawLine(250,208,850,208,{115,80,34,200});
    TXT("Santa Cruz de la Sierra - Bolivia",
             WIN_W/2-TXTW("Santa Cruz de la Sierra - Bolivia",15)/2,214,15,{155,128,82,255});

    // Opciones de menu
    const char* opts[]={"JUGAR","DICCIONARIO","DESCRIPCION"};
    Color bgs[]={
        {50,135,58,255},{60,80,155,255},{120,60,20,255}
    };
    Color brds[]={
        {90,190,95,255},{90,125,210,255},{175,100,50,255}
    };
    for(int i=0;i<3;i++){
        bool active=(i==sel);
        int oy=255+i*82;
        Color bg=active?bgs[i]:Color{40,28,14,255};
        Color brd=active?brds[i]:Color{85,60,30,180};
        DrawRectangleRounded({260,(float)oy,580,64},0.25f,8,bg);
        DrawRectangleRoundedLinesEx({260,(float)oy,580,64},0.25f,8,active?3:2,brd);
        if(active){ TXT(">",270,oy+20,22,{255,235,80,255}); }
        int lw=TXTW(opts[i],24);
        TXT(opts[i],WIN_W/2-lw/2,oy+20,24,WHITE);
    }

    DrawLine(250,503,850,503,{115,80,34,200});

    // Controles
    TXT("[ARRIBA/ABAJO] Navegar   [ENTER] Seleccionar   [ESC 5s] Cerrar",
             WIN_W/2-TXTW("[ARRIBA/ABAJO] Navegar   [ENTER] Seleccionar   [ESC 5s] Cerrar",12)/2,512,12,{115,95,62,255});

    // Musica toggle
    const char* mLabel=musicOn?"MUSICA: ON":"MUSICA: OFF";
    Color mCol=musicOn?Color{45,135,55,255}:Color{120,45,45,255};
    drawBtn(WIN_W-155,12,143,34,mLabel,mCol,{100,85,50,255},12);

    // Hash Table info
    TXT("Hash Table (28 posiciones) + BST",
             WIN_W/2-TXTW("Hash Table (28 posiciones) + BST",11)/2,WIN_H-20,11,{85,68,42,255});

    // Barra ESC
    if(escHold>0.3f){
        float p=min(1.f,escHold/5.f);
        DrawRectangle(20,WIN_H-12,460,8,{50,35,18,255});
        DrawRectangle(20,WIN_H-12,(int)(460*p),8,{210,60,60,255});
        TXT("Manteniendo ESC para cerrar...",22,WIN_H-26,11,{200,90,90,255});
    }
}

// ============================================================
// PANTALLA: DESCRIPCION
// ============================================================
void drawDesc(Vector2 m, bool lclick, Scr& scr){
    ClearBackground({18,11,4,255});
    DrawRectangleRounded({60,40,WIN_W-120,WIN_H-80},0.04f,8,{28,18,6,235});
    DrawRectangleRoundedLinesEx({60,40,(float)(WIN_W-120),(float)(WIN_H-80)},0.04f,8,3,{145,105,42,255});

    TXT("DESCRIPCION DEL JUEGO",
             WIN_W/2-TXTW("DESCRIPCION DEL JUEGO",22)/2,58,22,{228,188,62,255});
    DrawLine(90,88,WIN_W-90,88,{115,80,34,200});

    int px=90,py=98;
    wrapText("El Diccionario del Habla Camba es un juego educativo basado en el habla popular de Santa Cruz de la Sierra, Bolivia. Forma palabras del oriente boliviano usando las letras del tablero. Cada palabra correcta muestra su definicion y te da puntos.",
             px,py,WIN_W-200,13,{195,170,128,255}); py+=80;

    TXT("COMO SE JUEGA:",px,py,15,{228,188,62,255}); py+=22;
    const char* how[]={"- Haz clic en letras del tablero para formarlas en orden",
                        "- Presiona ENTER o el boton ENVIAR para verificar la palabra",
                        "- Si la palabra existe en el diccionario: puntos y definicion",
                        "- Si no existe: pierdes una vida (tienes 5 vidas)",
                        "- Usa PISTA para revelar una palabra posible del tablero",
                        "- MEZCLAR regenera el tablero (siempre hay una palabra posible)"};
    for(auto& h:how){TXT(h,px+10,py,12,{175,150,108,255});py+=18;}
    py+=10;

    TXT("ESTRUCTURAS DE DATOS IMPLEMENTADAS:",px,py,15,{228,188,62,255}); py+=22;
    const char* structs[]={
        "[1] Tabla Hash    - 28 posiciones, funcion hash DJB2 multiplicativa",
        "[2] Lista Enlazada- Manejo base de colisiones en cada bucket",
        "[3] Arbol BST     - Mejora colisiones: O(n) lista -> O(log n) arbol",
        "[4] Archivo       - Carga palabras desde diccionario.txt al inicio",
        "[5] Buscar        - hash(palabra) -> bucket -> BST search",
        "[6] Agregar       - insert en BST del bucket, guarda en archivo",
        "[7] Eliminar      - delete en BST, tabla reorganizada automaticamente"};
    for(auto& s:structs){TXT(s,px+10,py,12,{155,132,94,255});py+=18;}
    py+=10;

    TXT("PUNTUACION:",px,py,15,{228,188,62,255}); py+=22;
    TXT("4 letras=100pts  5=250pts  6=500pts  7=900pts  8+=1500pts  x COMBO",px+10,py,12,{155,132,94,255});
    py+=24;
    TXT("El COMBO multiplica los puntos por palabras consecutivas correctas.",px+10,py,12,{155,132,94,255});

    if(btnClick(WIN_W/2-120,WIN_H-76,240,48,m,lclick)) scr=SC_MENU;
    drawBtn(WIN_W/2-120,WIN_H-76,240,48,"VOLVER AL MENU",{80,50,130,255},{130,95,190,255});
    TXT("[ESC] Volver",WIN_W/2-TXTW("[ESC] Volver",12)/2,WIN_H-22,12,{100,82,54,255});
}

// ============================================================
// PANTALLA: DICCIONARIO (buscar / agregar / eliminar)
// ============================================================
void updateDictFilter(DictState& ds, TablaHash& dic){
    ds.filtered.clear();
    string q=ds.search;
    transform(q.begin(),q.end(),q.begin(),::toupper);
    for(auto& e:dic.todas()){
        string p=e.palabra;
        transform(p.begin(),p.end(),p.begin(),::toupper);
        if(q.empty()||p.find(q)!=string::npos||e.significado.find(ds.search)!=string::npos)
            ds.filtered.push_back(e);
    }
    sort(ds.filtered.begin(),ds.filtered.end(),[](const Entrada& a,const Entrada& b){
        string pa=a.palabra,pb=b.palabra;
        transform(pa.begin(),pa.end(),pa.begin(),::toupper);
        transform(pb.begin(),pb.end(),pb.begin(),::toupper);
        return pa<pb;
    });
    ds.scroll=max(0,min(ds.scroll,(int)ds.filtered.size()-1));
}

const int DICT_ROW_H=30;
const int DICT_LIST_Y=140;
const int DICT_LIST_H=WIN_H-DICT_LIST_Y-80;
const int DICT_ROWS_VIS=DICT_LIST_H/DICT_ROW_H;

void drawDict(DictState& ds, TablaHash& dic, Scr& scr, Vector2 m, bool lclick, float dt){
    ClearBackground({18,11,4,255});
    DrawRectangle(0,0,WIN_W,WIN_H,{22,14,5,255});

    // Header
    DrawRectangle(0,0,WIN_W,55,{30,20,8,255});
    TXT("DICCIONARIO DEL HABLA CAMBA",20,15,22,{228,188,62,255});
    char cnt[48]; snprintf(cnt,sizeof(cnt),"%d palabras | Hash 28 buckets + BST",dic.getTotal());
    TXT(cnt,20,40,11,{150,125,80,255});

    // Botones header
    if(btnClick(WIN_W-160,10,150,34,m,lclick)) scr=SC_MENU;
    drawBtn(WIN_W-160,10,150,34,"< VOLVER",{70,45,120,255},{120,88,180,255},13);

    // Barra de busqueda
    DrawRectangle(20,65,WIN_W-220,40,{38,26,10,255});
    DrawRectangleLinesEx({20,65,(float)(WIN_W-220),40},2,{120,88,44,200});
    string disp=ds.search.empty()?"Escribe para buscar...":ds.search;
    Color searchCol=ds.search.empty()?Color{90,75,50,200}:Color{220,200,150,255};
    TXT(disp.c_str(),28,78,14,searchCol);
    // Cursor parpadeante
    if(!ds.search.empty()&&(int)(GetTime()*2)%2==0){
        int cw=TXTW(ds.search.c_str(),14);
        TXT("|",30+cw,78,14,{220,200,150,255});
    }

    // Info busqueda
    char fi[48]; snprintf(fi,sizeof(fi),"Mostrando %d resultados",(int)ds.filtered.size());
    TXT(fi,WIN_W-210,78,12,{130,108,72,255});

    // Botones operaciones
    int bx=20, by=112;
    bool canDel=(ds.selected>=0&&ds.selected<(int)ds.filtered.size());
    drawBtn(bx,by,175,26,"+ AGREGAR NUEVA",{40,110,50,255},{75,175,85,255},12);
    if(btnClick(bx,by,175,26,m,lclick)&&!ds.addMode){
        ds.addMode=true; ds.newWord=""; ds.newDef=""; ds.addField=0;
    }
    drawBtn(bx+185,by,185,26,"- ELIMINAR SELECCION",
            canDel?Color{150,40,40,255}:Color{60,38,38,255},{195,80,80,255},12);
    if(btnClick(bx+185,by,185,26,m,lclick)&&canDel&&!ds.addMode){
        if(ds.deleteConfirm){
            dic.eliminar(ds.filtered[ds.selected].palabra);
            updateDictFilter(ds,dic);
            ds.selected=-1; ds.deleteConfirm=false;
            ds.statusMsg="Palabra eliminada de la tabla hash.";
            ds.statusT=3.f;
        } else { ds.deleteConfirm=true; }
    }
    if(ds.deleteConfirm&&!canDel) ds.deleteConfirm=false;
    if(ds.deleteConfirm){
        TXT("! Clic de nuevo para confirmar eliminacion",bx+380,by+6,12,{220,120,60,255});
    }

    // Lista de palabras
    int listX=20,listW=WIN_W-40,rowH=DICT_ROW_H;
    DrawRectangle(listX,DICT_LIST_Y,listW,DICT_LIST_H,{28,18,6,255});
    DrawRectangleLinesEx({(float)listX,(float)DICT_LIST_Y,(float)listW,(float)DICT_LIST_H},2,{100,72,36,200});

    // Header de columnas
    DrawRectangle(listX,DICT_LIST_Y,listW,20,{42,28,10,255});
    TXT("PALABRA",listX+8,DICT_LIST_Y+4,12,{200,175,110,255});
    TXT("SIGNIFICADO",listX+230,DICT_LIST_Y+4,12,{200,175,110,255});

    // Filas visibles
    for(int i=0;i<DICT_ROWS_VIS&&(ds.scroll+i)<(int)ds.filtered.size();i++){
        int idx=ds.scroll+i;
        Entrada& e=ds.filtered[idx];
        int ry=DICT_LIST_Y+20+i*rowH;
        bool isSel=(idx==ds.selected);
        bool isHov=(m.y>=ry&&m.y<ry+rowH&&m.x>=listX&&m.x<listX+listW);

        Color rowBg=isSel?Color{60,42,16,255}:isHov?Color{38,26,10,255}:Color{0,0,0,0};
        if(rowBg.a>0) DrawRectangle(listX,ry,listW,rowH,rowBg);
        if(isSel) DrawRectangleLinesEx({(float)listX,(float)ry,(float)listW,(float)rowH},1,{180,140,55,200});

        // Nombre
        string pshow=e.palabra;
        if((int)pshow.size()>28) pshow=pshow.substr(0,25)+"...";
        Color namCol=isSel?Color{255,215,70,255}:e.esNueva?Color{100,200,100,255}:Color{215,195,145,255};
        TXT(pshow.c_str(),listX+8,ry+7,12,namCol);
        // Indicador nuevo
        if(e.esNueva) TXT("*",listX+200,ry+7,11,{100,200,100,255});

        // Separador vertical
        DrawLine(listX+220,ry,listX+220,ry+rowH,{80,58,28,150});

        // Definicion (truncada)
        string sigshow=e.significado;
        if((int)sigshow.size()>80) sigshow=sigshow.substr(0,77)+"...";
        TXT(sigshow.c_str(),listX+228,ry+7,11,{170,148,105,255});

        // Separador horizontal
        DrawLine(listX,ry+rowH-1,listX+listW,ry+rowH-1,{55,38,16,180});

        if(lclick&&isHov){ ds.selected=idx; ds.deleteConfirm=false; }
    }

    // Panel detalle si hay seleccion
    if(ds.selected>=0&&ds.selected<(int)ds.filtered.size()){
        Entrada& e=ds.filtered[ds.selected];
        int dy=WIN_H-72;
        DrawRectangle(20,dy,WIN_W-40,60,{32,20,8,255});
        DrawRectangleLinesEx({20,(float)dy,(float)(WIN_W-40),60},1,{110,80,36,200});
        TXT(e.palabra.c_str(),28,dy+6,15,{228,188,62,255});
        wrapText(e.significado,28,dy+26,WIN_W-80,11,{185,162,118,255});
    }

    // Scroll
    float wheel=GetMouseWheelMove();
    if(wheel!=0){ ds.scroll-=(int)wheel*3; ds.scroll=max(0,min(ds.scroll,(int)(int)ds.filtered.size()-DICT_ROWS_VIS)); }
    if(IsKeyPressed(KEY_UP)&&ds.scroll>0) ds.scroll--;
    if(IsKeyPressed(KEY_DOWN)&&ds.scroll<(int)ds.filtered.size()-DICT_ROWS_VIS) ds.scroll++;

    // Scrollbar visual
    if((int)ds.filtered.size()>DICT_ROWS_VIS){
        int sbH=DICT_LIST_H-20;
        float thumb=(float)DICT_ROWS_VIS/ds.filtered.size()*sbH;
        float thumbY=DICT_LIST_Y+20+(float)ds.scroll/ds.filtered.size()*sbH;
        DrawRectangle(WIN_W-18,DICT_LIST_Y+20,8,(int)sbH,{50,36,14,200});
        DrawRectangle(WIN_W-18,(int)thumbY,8,(int)thumb,{150,115,55,255});
    }

    // Input de busqueda (siempre activo cuando no estamos en addMode)
    if(!ds.addMode){
        int key=GetCharPressed();
        while(key>0){
            if(key>=32&&key<128&&(int)ds.search.size()<60)
                ds.search+=(char)key;
            key=GetCharPressed();
        }
        if(IsKeyPressed(KEY_BACKSPACE)&&!ds.search.empty()){
            ds.search.pop_back(); updateDictFilter(ds,dic);
        }
        if(key>0||IsKeyPressed(KEY_BACKSPACE)) updateDictFilter(ds,dic);
        // Trigger update on any char input
        static int prevLen=-1;
        if((int)ds.search.size()!=prevLen){ updateDictFilter(ds,dic); prevLen=(int)ds.search.size(); }
    }

    // Status message
    if(ds.statusT>0){
        ds.statusT-=dt;
        TXT(ds.statusMsg.c_str(),WIN_W/2-TXTW(ds.statusMsg.c_str(),13)/2,WIN_H-20,13,{120,200,120,255});
    }

    // ---- POPUP AGREGAR ----
    if(ds.addMode){
        // Fondo oscuro
        DrawRectangle(0,0,WIN_W,WIN_H,{0,0,0,150});

        int pw=650,ph=280,px2=WIN_W/2-pw/2,py2=WIN_H/2-ph/2;
        DrawRectangleRounded({(float)px2,(float)py2,(float)pw,(float)ph},0.08f,8,{32,20,8,255});
        DrawRectangleRoundedLinesEx({(float)px2,(float)py2,(float)pw,(float)ph},0.08f,8,3,{145,105,42,255});

        TXT("AGREGAR NUEVA ENTRADA",px2+pw/2-TXTW("AGREGAR NUEVA ENTRADA",18)/2,py2+14,18,{228,188,62,255});
        DrawLine(px2+20,py2+40,px2+pw-20,py2+40,{115,80,34,200});

        // Campo palabra
        bool wActive=(ds.addField==0);
        TXT("Palabra:",px2+20,py2+55,13,{190,165,115,255});
        DrawRectangle(px2+20,py2+74,pw-40,36,wActive?Color{45,32,12,255}:Color{32,22,8,255});
        DrawRectangleLinesEx({(float)(px2+20),(float)(py2+74),(float)(pw-40),36},wActive?2:1,
                             wActive?Color{200,165,65,255}:Color{100,75,35,200});
        TXT(ds.newWord.c_str(),px2+28,py2+83,14,{230,210,160,255});
        if(wActive&&(int)(GetTime()*2)%2==0){
            int cw=TXTW(ds.newWord.c_str(),14);
            TXT("|",px2+30+cw,py2+83,14,{230,210,160,255});
        }

        // Campo definicion
        bool dActive=(ds.addField==1);
        TXT("Significado:",px2+20,py2+118,13,{190,165,115,255});
        DrawRectangle(px2+20,py2+137,pw-40,60,dActive?Color{45,32,12,255}:Color{32,22,8,255});
        DrawRectangleLinesEx({(float)(px2+20),(float)(py2+137),(float)(pw-40),60},dActive?2:1,
                             dActive?Color{200,165,65,255}:Color{100,75,35,200});
        wrapText(ds.newDef,px2+28,py2+146,(int)(pw-60),12,{230,210,160,255});

        TXT("[TAB] Cambiar campo  [ENTER] Guardar  [ESC] Cancelar",
                 px2+pw/2-TXTW("[TAB] Cambiar campo  [ENTER] Guardar  [ESC] Cancelar",11)/2,py2+205,11,{130,108,72,255});

        // Botones
        if(btnClick(px2+20,py2+225,pw/2-30,40,m,lclick)||IsKeyPressed(KEY_ESCAPE)){
            ds.addMode=false;
        }
        drawBtn(px2+20,py2+225,pw/2-30,40,"CANCELAR",{120,40,40,255},{185,75,75,255});

        bool canSave=!ds.newWord.empty()&&!ds.newDef.empty();
        if(canSave&&(btnClick(px2+pw/2+10,py2+225,pw/2-30,40,m,lclick)||IsKeyPressed(KEY_ENTER))){
            // Convertir a mayusculas la palabra
            string wp=ds.newWord;
            transform(wp.begin(),wp.end(),wp.begin(),::toupper);
            dic.insertar(wp,ds.newDef,true);
            dic.guardarNuevas("diccionario.txt");
            updateDictFilter(ds,dic);
            ds.addMode=false;
            ds.statusMsg="\""+wp+"\" agregada y guardada en el archivo.";
            ds.statusT=4.f;
        }
        drawBtn(px2+pw/2+10,py2+225,pw/2-30,40,"GUARDAR",
                canSave?Color{40,110,50,255}:Color{30,60,35,255},{75,175,85,255});

        // Input del popup
        if(btnClick(px2+20,py2+74,pw-40,36,m,lclick)) ds.addField=0;
        if(btnClick(px2+20,py2+137,pw-40,60,m,lclick)) ds.addField=1;
        if(IsKeyPressed(KEY_TAB)) ds.addField=1-ds.addField;

        int key=GetCharPressed();
        while(key>0){
            if(key>=32&&key<128){
                if(ds.addField==0&&(int)ds.newWord.size()<50)
                    ds.newWord+=(char)key;
                else if(ds.addField==1&&(int)ds.newDef.size()<300)
                    ds.newDef+=(char)key;
            }
            key=GetCharPressed();
        }
        if(IsKeyPressed(KEY_BACKSPACE)){
            if(ds.addField==0&&!ds.newWord.empty()) ds.newWord.pop_back();
            else if(ds.addField==1&&!ds.newDef.empty()) ds.newDef.pop_back();
        }
    }
}

// ============================================================
// PANTALLA: JUEGO
// ============================================================
void drawGame(GState& g, TablaHash& dic){
    ClearBackground({20,13,5,255});

    // Panel izquierdo
    DrawRectangle(0,0,418,WIN_H,{32,20,8,255});
    DrawRectangle(417,0,2,WIN_H,{110,78,35,200});
    // --- Titulo ---
    TXT("DICCIONARIO",   16, 12, 24, {228,188,62,255});
    TXT("DEL HABLA CAMBA",16,40, 15, {178,138,48,255});
    DrawLine(16,62,400,62,{100,72,34,180});

    // --- Vidas y puntaje (bien separados) ---
    TXT("Vidas:", 16, 70, 14, {165,142,104,255});
    for(int i=0;i<MAX_LIVES;i++){
        Color hc=i<g.lives?Color{218,55,55,255}:Color{72,42,42,200};
        TXT("v", 88+i*32, 68, 22, hc);
    }

    char sc[48]; snprintf(sc,sizeof(sc),"PUNTOS: %d",g.score);
    TXT(sc, 16, 100, 17, {218,198,88,255});

    if(g.combo>1){
        char cb[32]; snprintf(cb,sizeof(cb),"COMBO x%d  !!!",g.combo);
        TXT(cb, 16, 124, 14, {255,155,0,255});
    }
    DrawLine(16,146,400,146,{100,72,34,180});

    // --- Palabra formandose ---
    TXT("Formando:", 16, 154, 13, {155,130,88,255});

    int px=16, py=174;
    for(int i=0;i<(int)g.orden.size();i++){
        int r=g.orden[i].first, c=g.orden[i].second;
        char ltr[2]={g.grid[r][c].letra,0};
        Color bc=(g.msgT>0&&!g.msgOk)?Color{198,68,68,255}:Color{218,172,42,255};
        DrawRectangleRounded({(float)px,(float)py,32,32},0.2f,4,bc);
        int fw=TXTW(ltr,19); TXT(ltr,px+16-fw/2,py+7,19,{35,15,0,255});
        px+=36;
        if(px>386){px=16;py+=38;}
    }
    if(g.palabra.empty())
        TXT("(ninguna letra seleccionada)", 16, 178, 11, {100,85,58,180});

    DrawLine(16,222,400,222,{100,72,34,180});

    // --- Ultimo resultado ---
    if(g.msgT>5.f){
        float a=min(1.f,(g.msgT-5.f)/0.4f);
        if(g.msgOk){
            TXT(g.msgWord.c_str(), 16, 230, 15, {68,212,88,(unsigned char)(255*a)});
            wrapText(g.msgDef, 16, 252, 398, 12, {200,178,138,(unsigned char)(200*a)});
        } else {
            wrapText(g.msgWord, 16, 230, 398, 13, {218,68,68,(unsigned char)(255*a)});
        }
    }

    // --- Barra de progreso hacia victoria ---
    DrawLine(16,284,400,284,{100,72,34,180});
    float prog=min(1.f,(float)g.score/WIN_SCORE);
    DrawRectangle(16,292,384,12,{40,28,12,255});
    DrawRectangle(16,292,(int)(384*prog),12,prog>=1.f?Color{80,220,80,255}:Color{190,150,45,255});
    char progTxt[48]; snprintf(progTxt,sizeof(progTxt),"%d / %d pts para ganar",g.score,WIN_SCORE);
    TXT(progTxt, 16, 310, 13, {158,138,90,255});

    // --- TABLA HASH (siempre visible, panel izquierdo inferior) ---
    {
        int hy = WIN_H - 135;
        DrawLine(16, hy, 400, hy, {100,72,34,200});

        // Encabezado prominente
        DrawRectangle(16, hy+4, 384, 20, {45,30,10,255});
        TXT("TABLA HASH  (28 buckets + BST)", 20, hy+6, 13, {228,188,62,255});

        // Info del ultimo bucket buscado
        if(g.showHash && g.lastBucket>=0){
            char hb[64];
            snprintf(hb,sizeof(hb),"ultimo: bucket %d  |  %d items",g.lastBucket,g.lastBucketSz);
            TXT(hb, 16, hy+28, 12, {180,155,100,255});
        } else {
            TXT("busca una palabra para ver la actividad", 16, hy+28, 11, {120,100,68,200});
        }

        // Visualizacion de los 28 buckets — mas grandes y con numero en el activo
        // 2 filas de 14, cuadros de 26x18
        for(int i=0;i<TAM;i++){
            int bx = 16 + (i%14)*27;
            int by = hy + 45 + (i/14)*22;
            bool active = (g.showHash && i==g.lastBucket);
            Color bc = active ? Color{255,215,0,255} : Color{55,40,18,255};
            Color brd= active ? Color{255,255,80,255}: Color{90,65,30,200};
            DrawRectangle(bx, by, 25, 18, bc);
            DrawRectangleLinesEx({(float)bx,(float)by,25,18}, active?2:1, brd);
            if(active){
                char bi[4]; snprintf(bi,sizeof(bi),"%d",i);
                int bw=TXTW(bi,10);
                TXT(bi, bx+12-bw/2, by+3, 10, {10,5,0,255});
            }
        }
        DrawLine(16, hy+92, 400, hy+92, {100,72,34,200});
    }

    // GRID de letras
    TXT("FORMA UNA PALABRA CAMBA:",GX,GY-26,13,{178,152,94,255});
    Vector2 mouse=GetMousePosition();
    for(int r=0;r<GROWS;r++) for(int c=0;c<GCOLS;c++){
        int tx=GX+c*(TILE+GAP),ty=GY+r*(TILE+GAP);
        bool hover=(!g.grid[r][c].usada&&!g.grid[r][c].sel&&
                    mouse.x>=tx&&mouse.x<tx+TILE&&mouse.y>=ty&&mouse.y<ty+TILE);
        int ord=-1;
        for(int i=0;i<(int)g.orden.size();i++)
            if(g.orden[i].first==r&&g.orden[i].second==c){ord=i;break;}
        drawTile(g.grid[r][c],tx,ty,ord,hover);
    }

    // --- Botones ---
    int btnY=GY+GROWS*(TILE+GAP)+12;
    drawBtn(GX,     btnY,178,42,"ENVIAR [ENTER]",
            !g.palabra.empty()?Color{42,138,52,255}:Color{35,60,38,255},{75,185,85,255});
    drawBtn(GX+186, btnY,162,42,"BORRAR [BKSP]", {138,65,22,255},{185,108,52,255});
    drawBtn(GX+356, btnY,148,42,"LIMPIAR [DEL]", {80,42,130,255},{128,88,185,255});
    drawBtn(GX+512, btnY,128,42,"MEZCLAR",       {32,85,128,255},{58,138,178,255},12);

    // Boton PISTA (segunda fila, izquierda)
    int btnY2 = btnY + 50;
    drawBtn(GX, btnY2, 155, 36, "PISTA  [H]",
            !g.hintWord.empty()?Color{115,85,20,255}:Color{75,55,14,255},{175,140,50,255},13);

    // --- PISTA: caja aparte al lado del boton, no superpuesta ---
    if(g.hintT>0 && !g.hintWord.empty()){
        string hshow;
        if(g.hintRevealed==0)      hshow="Hay una palabra de "+to_string(g.hintWord.size())+" letras";
        else if(g.hintRevealed==1) hshow="Empieza con:  " + g.hintWord.substr(0,2) + "...";
        else                       hshow="La palabra es:  " + g.hintWord;

        // Caja de pista — panel derecho, misma fila que el boton
        int px2 = GX+165, py2 = btnY2-2;
        int pw2 = WIN_W - px2 - 10;
        float alpha = min(1.f, g.hintT / 0.5f);
        DrawRectangleRounded({(float)px2,(float)py2,(float)pw2,42},0.2f,6,
                              {50,35,8,(unsigned char)(220*alpha)});
        DrawRectangleRoundedLinesEx({(float)px2,(float)py2,(float)pw2,42},0.2f,6,2,
                              {200,165,55,(unsigned char)(255*alpha)});
        TXT("PISTA:", px2+10, py2+5, 12,{220,185,60,(unsigned char)(255*alpha)});
        TXT(hshow.c_str(), px2+10, py2+22, 13,{230,210,155,(unsigned char)(240*alpha)});
    }

    TXT("Clic en letras + ENTER para buscar | BKSP borra ultima | DEL limpia todo",
        GX, WIN_H-20, 11, {105,88,58,255});
}

// ============================================================
// PANTALLA: GAME OVER
// ============================================================
void drawOver(GState& g, Vector2 m, bool lclick, Scr& scr, TablaHash& dic){
    ClearBackground({18,11,4,255});
    DrawRectangleRounded({240,118,620,450},0.06f,8,{26,16,5,235});
    DrawRectangleRoundedLinesEx({240,118,620,450},0.06f,8,3,{142,58,28,255});
    const char* t=g.lives<=0?"SE ACABARON LAS VIDAS":"JUEGO TERMINADO";
    TXT(t,WIN_W/2-TXTW(t,34)/2,148,34,{218,80,52,255});
    char sc[64]; snprintf(sc,sizeof(sc),"Puntuacion final: %d puntos",g.score);
    TXT(sc,WIN_W/2-TXTW(sc,24)/2,200,24,{222,192,72,255});
    DrawLine(270,238,830,238,{112,72,28,200});
    const char* msg=g.score==0?"Sigue intentando!":g.score<500?"Buen camba!":g.score<1500?"Gran conocedor del habla!":"Sos un camba de verdad pues!";
    TXT(msg,WIN_W/2-TXTW(msg,16)/2,250,16,{175,152,108,255});
    if(btnClick(WIN_W/2-155,320,310,52,m,lclick)){
        g=GState(); generarGrid(g,dic); scr=SC_GAME;
    }
    drawBtn(WIN_W/2-155,320,310,52,"JUGAR DE NUEVO",{42,130,52,255},{82,188,88,255});
    if(btnClick(WIN_W/2-155,384,310,52,m,lclick)){
        g=GState(); generarGrid(g,dic); scr=SC_MENU;
    }
    drawBtn(WIN_W/2-155,384,310,52,"VOLVER AL MENU",{78,42,120,255},{128,88,175,255});
}

// ============================================================
// LOGICA DEL JUEGO
// ============================================================
void toggleTile(GState& g,int r,int c){
    Tile& t=g.grid[r][c];
    if(t.usada) return;
    if(t.sel){
        if(!g.orden.empty()&&g.orden.back().first==r&&g.orden.back().second==c){
            t.sel=false; g.orden.pop_back();
            if(!g.palabra.empty()) g.palabra.pop_back();
        }
    } else {
        if((int)g.palabra.size()>=14) return;
        t.sel=true; g.orden.push_back({r,c});
        g.palabra+=t.letra;
    }
}

void limpiar(GState& g){
    for(int r=0;r<GROWS;r++) for(int c=0;c<GCOLS;c++)
        if(!g.grid[r][c].usada) g.grid[r][c].sel=false;
    g.orden.clear(); g.palabra="";
}

void enviar(GState& g,TablaHash& dic){
    if(g.palabra.empty()) return;
    Entrada* e=dic.buscar(g.palabra);
    g.lastBucket=dic.ultimoBucket;
    g.lastBucketSz=dic.bucketSize(dic.ultimoBucket);
    g.showHash=true; g.hashT=6.f;
    if(e){
        int len=(int)g.palabra.size();
        // Puntos escalados: palabras mas largas valen exponencialmente mas
        int pts=len<=4?100:len==5?300:len==6?700:len==7?1200:len==8?2000:3500;
        g.combo++; pts*=g.combo; g.score+=pts;
        // Guardar en historial
        g.historial.push_back({e->palabra,pts,e->significado});
        g.histScroll=0; // auto-scroll al tope (mas reciente)
        char buf[64]; snprintf(buf,sizeof(buf),"%s  [+%d pts]",e->palabra.c_str(),pts);
        g.msgWord=buf; g.msgDef=e->significado; g.msgOk=true; g.msgT=9.f;
        for(auto& p:g.orden){
            auto& t=g.grid[p.first][p.second];
            t.flashT=0.5f; t.flashOk=true; t.usada=true; t.sel=false;
        }
        g.orden.clear(); g.palabra="";
        g.hintWord=""; g.hintRevealed=0; g.hintT=0;
        // Regenerar si quedan pocas tiles
        int libres=0;
        for(int r=0;r<GROWS;r++) for(int c=0;c<GCOLS;c++) if(!g.grid[r][c].usada) libres++;
        if(libres<8) generarGrid(g,dic);
    } else {
        g.lives--; g.combo=0;
        char buf[64]; snprintf(buf,sizeof(buf),"\"%s\" no esta en el diccionario",g.palabra.c_str());
        g.msgWord=buf; g.msgDef=""; g.msgOk=false; g.msgT=3.f;
        for(auto& p:g.orden){g.grid[p.first][p.second].shakeT=0.5f;g.grid[p.first][p.second].flashT=0.5f;g.grid[p.first][p.second].flashOk=false;}
        limpiar(g);
        // lives==0: el main detecta y cambia a SC_OVER
    }
}

// ============================================================
// MAIN
// ============================================================
// ============================================================
// PANTALLA VICTORIA
// ============================================================
void drawWin(GState& g, Vector2 m, bool lclick, Scr& scr, TablaHash& dic){
    ClearBackground({10,20,8,255});
    // Fondo festivo
    for(int i=0;i<20;i++){
        Color cc={(unsigned char)(rng()%200+55),(unsigned char)(rng()%200+55),(unsigned char)(rng()%200+55),80};
        DrawCircle(rng()%WIN_W, rng()%WIN_H, 3+(rng()%8), cc);
    }
    DrawRectangleRounded({180,90,740,500},0.06f,8,{12,28,10,240});
    DrawRectangleRoundedLinesEx({180,90,740,500},0.06f,8,4,{80,220,80,255});

    TXT("GANASTE !",WIN_W/2-TXTW("GANASTE !",52)/2,112,52,{80,240,80,255});
    DrawLine(210,174,890,174,{60,180,60,200});

    char sc[64]; snprintf(sc,sizeof(sc),"Puntuacion: %d puntos",g.score);
    TXT(sc,WIN_W/2-TXTW(sc,28)/2,185,28,{220,220,80,255});

    char wc[48]; snprintf(wc,sizeof(wc),"Palabras encontradas: %d",(int)g.historial.size());
    TXT(wc,WIN_W/2-TXTW(wc,16)/2,225,16,{160,210,160,255});

    DrawLine(210,252,890,252,{60,180,60,180});

    // Mostrar historial de palabras
    int py=262, vis=0;
    for(int i=(int)g.historial.size()-1;i>=0&&vis<10;i--,vis++){
        auto& e=g.historial[i];
        char line[80]; snprintf(line,sizeof(line),"%-18s +%d pts",e.palabra.c_str(),e.pts);
        TXT(line,220,py,13,{140,210,140,255});
        py+=22;
    }

    if(btnClick(WIN_W/2-175,520,160,48,m,lclick)){
        g=GState(); generarGrid(g,dic); scr=SC_GAME;
    }
    drawBtn(WIN_W/2-175,520,160,48,"JUGAR DE NUEVO",{42,130,52,255},{82,188,88,255},13);

    if(btnClick(WIN_W/2+15,520,160,48,m,lclick)){
        g=GState(); generarGrid(g,dic); scr=SC_MENU;
    }
    drawBtn(WIN_W/2+15,520,160,48,"AL MENU",{78,42,120,255},{128,88,175,255},13);
}

// ============================================================
// HISTORIAL en el panel izquierdo del juego
// ============================================================
void drawHistorial(GState& g, int x, int y, int w, int h){
    int n=(int)g.historial.size();

    // --- Encabezado ---
    DrawLine(x, y-6, x+w, y-6, {100,72,34,180});
    char hdr[48]; snprintf(hdr,sizeof(hdr),"Palabras encontradas: %d",n);
    TXT(hdr, x, y, 14, {200,175,115,255});
    y += 22;

    if(n==0){
        TXT("(ninguna aun)", x+6, y+6, 13, {100,85,58,180});
        return;
    }

    // Cada entrada ocupa 3 lineas con espacio generoso
    const int rowH = 48;  // altura por entrada
    int visible = h / rowH;
    int maxScroll = max(0, n - visible);
    g.histScroll = max(0, min(g.histScroll, maxScroll));

    // Scroll con rueda del raton
    Vector2 m = GetMousePosition();
    if(m.x>=x && m.x<x+w && m.y>=y && m.y<y+h){
        float wheel = GetMouseWheelMove();
        if(wheel != 0)
            g.histScroll = max(0, min(maxScroll, (int)(g.histScroll - wheel*2)));
    }

    // Dibujar entradas (mas reciente primero)
    for(int i=0; i<visible && (n-1-g.histScroll-i)>=0; i++){
        int idx = n-1-g.histScroll-i;
        auto& e = g.historial[idx];
        int ry = y + i*rowH;

        // Fondo alternado suave
        Color bgRow = (i%2==0) ? Color{38,26,10,200} : Color{28,18,6,180};
        DrawRectangle(x, ry, w, rowH-2, bgRow);
        DrawLine(x, ry+rowH-2, x+w, ry+rowH-2, {70,52,24,150});

        // Linea 1: Palabra  +pts  (fs=14, bien separada)
        char pts[32]; snprintf(pts,sizeof(pts),"+%d",e.pts);
        int ptsW = TXTW(pts, 14);
        TXT(e.palabra.c_str(), x+6, ry+5,  14, {230,205,130,255});
        TXT(pts,               x+w-ptsW-6, ry+5, 14, {110,230,90,255});

        // Linea 2: Definicion corta (fs=12, con margen arriba)
        string dshort = e.def.size()>42 ? e.def.substr(0,39)+"..." : e.def;
        TXT(dshort.c_str(), x+6, ry+23, 12, {155,135,92,220});
    }

    // Flechas de scroll
    if(n > visible){
        if(g.histScroll < maxScroll)
            TXT("[scroll v]", x+w/2-TXTW("[scroll v]",11)/2, y+h-16, 11, {130,110,70,200});
        if(g.histScroll > 0)
            TXT("[scroll ^]", x+w/2-TXTW("[scroll ^]",11)/2, y-16, 11, {130,110,70,200});
    }
}

// ============================================================
// MAIN
// ============================================================
int main(){
    InitWindow(WIN_W,WIN_H,"El Diccionario del Habla Camba");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    InitAudioDevice();

    // Cargar fuente TTF del sistema para mejor legibilidad
    const char* fontPaths[]={
        "C:/Windows/Fonts/calibri.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/verdana.ttf",
        "C:/Windows/Fonts/tahoma.ttf"
    };
    for(auto& fp : fontPaths){
        if(FileExists(fp)){
            // Cargamos con suficientes glifos para español
            int codepoints[256]; for(int i=0;i<256;i++) codepoints[i]=i;
            gFont = LoadFontEx(fp, 48, codepoints, 256);
            SetTextureFilter(gFont.texture, TEXTURE_FILTER_BILINEAR);
            gFontLoaded = true;
            break;
        }
    }

    // ---- Musica del MENU (Watery Graves) ----
    Music menuBgm={}; bool hasMenuBgm=false;
    const char* menuTracks[]={
        "Watery Graves (In-Game) - Plants vs. Zombies Soundtrack (Official) - supershigi.mp3",
        "watery graves.mp3",
        "Watery Graves.mp3"
    };
    for(auto& tr:menuTracks){
        if(FileExists(tr)){
            menuBgm=LoadMusicStream(tr); menuBgm.looping=true;
            hasMenuBgm=true; break;
        }
    }

    // ---- Musica del JUEGO (Graze the Roof) ----
    Music gameBgm={}; bool hasGameBgm=false;
    const char* gameTracks[]={
        "Graze the Roof (In-Game) - Laura Shigihara.mp3",
        "graze the roof.mp3",
        "Graze the Roof.mp3"
    };
    for(auto& tr:gameTracks){
        if(FileExists(tr)){
            gameBgm=LoadMusicStream(tr); gameBgm.looping=true;
            hasGameBgm=true; break;
        }
    }

    bool musicOn=true;
    bool playingGame=false; // true = sonando musica del juego

    auto startMenuMusic=[&](){
        if(playingGame&&hasGameBgm) StopMusicStream(gameBgm);
        if(hasMenuBgm){ PlayMusicStream(menuBgm); SetMusicVolume(menuBgm,musicOn?0.55f:0.f); }
        playingGame=false;
    };
    auto startGameMusic=[&](){
        if(!playingGame&&hasMenuBgm) StopMusicStream(menuBgm);
        if(hasGameBgm){ PlayMusicStream(gameBgm); SetMusicVolume(gameBgm,musicOn?0.55f:0.f); }
        playingGame=true;
    };

    startMenuMusic();

    // Cargar diccionario
    TablaHash dic; dic.cargar("diccionario.txt");

    // Estados
    GState g; generarGrid(g,dic);
    DictState ds; updateDictFilter(ds,dic);
    Scr scr=SC_MENU;
    int menuSel=0;
    float escHold=0;
    bool running=true;

    while(!WindowShouldClose()&&running){
        float dt=GetFrameTime();
        if(!playingGame&&hasMenuBgm) UpdateMusicStream(menuBgm);
        if(playingGame&&hasGameBgm)  UpdateMusicStream(gameBgm);

        Vector2 mouse=GetMousePosition();
        bool lclick=IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        // ESC hold global → cerrar
        if(IsKeyDown(KEY_ESCAPE)) escHold+=dt; else escHold=0;
        if(escHold>=5.f) running=false;

        // ESC press → SIEMPRE ir atras / volver al menu
        if(IsKeyPressed(KEY_ESCAPE)){
            if(scr==SC_GAME){
                limpiar(g); scr=SC_MENU; startMenuMusic();
            }
            else if(scr==SC_DICT||scr==SC_DESC||scr==SC_WIN){
                scr=SC_MENU; startMenuMusic();
            }
            else if(scr==SC_OVER){
                g=GState(); generarGrid(g,dic); scr=SC_MENU; startMenuMusic();
            }
        }

        // Timers animacion en juego
        if(scr==SC_GAME){
            if(g.msgT>0) g.msgT-=dt;
            if(g.hintT>0) g.hintT-=dt;
            if(g.hashT>0){g.hashT-=dt;if(g.hashT<=0)g.showHash=false;}
            for(int r=0;r<GROWS;r++) for(int c=0;c<GCOLS;c++){
                auto& t=g.grid[r][c];
                if(t.shakeT>0)t.shakeT=max(0.f,t.shakeT-dt);
                if(t.flashT>0)t.flashT=max(0.f,t.flashT-dt);
            }
            if(g.lives<=0){scr=SC_OVER; startMenuMusic();}
            if(g.score>=WIN_SCORE){scr=SC_WIN; startMenuMusic();}
        }

        // ---- INPUT POR PANTALLA ----
        if(scr==SC_MENU){
            if(IsKeyPressed(KEY_UP))   menuSel=(menuSel-1+3)%3;
            if(IsKeyPressed(KEY_DOWN)) menuSel=(menuSel+1)%3;
            if(IsKeyPressed(KEY_ENTER)){
                if(menuSel==0){scr=SC_GAME; startGameMusic();}
                else if(menuSel==1){updateDictFilter(ds,dic);scr=SC_DICT;}
                else scr=SC_DESC;
            }
            int opts_y[]={255,337,419};
            for(int i=0;i<3;i++){
                if(lclick&&mouse.x>=260&&mouse.x<840&&mouse.y>=opts_y[i]&&mouse.y<opts_y[i]+64){
                    if(menuSel==i){
                        if(i==0){scr=SC_GAME; startGameMusic();}
                        else if(i==1){updateDictFilter(ds,dic);scr=SC_DICT;}
                        else scr=SC_DESC;
                    } else menuSel=i;
                }
            }
            // Musica toggle
            if(lclick&&mouse.x>=WIN_W-160&&mouse.x<WIN_W-10&&mouse.y>=10&&mouse.y<44){
                musicOn=!musicOn;
                if(hasMenuBgm) SetMusicVolume(menuBgm,musicOn?0.55f:0.f);
                if(hasGameBgm) SetMusicVolume(gameBgm,musicOn?0.55f:0.f);
            }
        }
        else if(scr==SC_GAME){
            if(lclick){
                for(int r=0;r<GROWS;r++) for(int c=0;c<GCOLS;c++){
                    int tx=GX+c*(TILE+GAP),ty=GY+r*(TILE+GAP);
                    if(mouse.x>=tx&&mouse.x<tx+TILE&&mouse.y>=ty&&mouse.y<ty+TILE)
                        toggleTile(g,r,c);
                }
                int btnY=GY+GROWS*(TILE+GAP)+12;
                if(mouse.y>=btnY&&mouse.y<btnY+42){
                    if(mouse.x>=GX&&mouse.x<GX+178)       enviar(g,dic);
                    if(mouse.x>=GX+186&&mouse.x<GX+348){
                        if(!g.orden.empty()){auto p=g.orden.back();g.orden.pop_back();g.grid[p.first][p.second].sel=false;if(!g.palabra.empty())g.palabra.pop_back();}
                    }
                    if(mouse.x>=GX+356&&mouse.x<GX+504)   limpiar(g);
                    if(mouse.x>=GX+512&&mouse.x<GX+640)   generarGrid(g,dic);
                }
                int btnY2=btnY+50;
                if(mouse.y>=btnY2&&mouse.y<btnY2+36&&mouse.x>=GX&&mouse.x<GX+140){
                    if(g.hintWord.empty()) g.hintWord=buscarPista(g,dic);
                    g.hintRevealed=min(2,g.hintRevealed+1); g.hintT=8.f;
                }
            }
            if(IsKeyPressed(KEY_ENTER))   enviar(g,dic);
            if(IsKeyPressed(KEY_H)){
                if(g.hintWord.empty()) g.hintWord=buscarPista(g,dic);
                g.hintRevealed=min(2,g.hintRevealed+1); g.hintT=8.f;
            }
            if(IsKeyPressed(KEY_BACKSPACE)){
                if(!g.orden.empty()){auto p=g.orden.back();g.orden.pop_back();g.grid[p.first][p.second].sel=false;if(!g.palabra.empty())g.palabra.pop_back();}
            }
            if(IsKeyPressed(KEY_DELETE)) limpiar(g);
        }

        // ---- DIBUJO ----
        BeginDrawing();
        if(scr==SC_MENU)       drawMenu(menuSel,musicOn,escHold);
        else if(scr==SC_GAME){
            drawGame(g,dic);
            // Historial sobre el panel izquierdo
            drawHistorial(g,16,330,398,WIN_H-330-145);
        }
        else if(scr==SC_DICT)  drawDict(ds,dic,scr,mouse,lclick,dt);
        else if(scr==SC_DESC)  drawDesc(mouse,lclick,scr);
        else if(scr==SC_WIN)   drawWin(g,mouse,lclick,scr,dic);
        else                   drawOver(g,mouse,lclick,scr,dic);
        DrawFPS(WIN_W-68,4);
        EndDrawing();
    }

    if(hasMenuBgm){StopMusicStream(menuBgm);UnloadMusicStream(menuBgm);}
    if(hasGameBgm){StopMusicStream(gameBgm);UnloadMusicStream(gameBgm);}
    if(gFontLoaded) UnloadFont(gFont);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
