/* El Tesoro del Pirata - Multi-Mapa v4
   g++ -std=c++11 -o tesoro_2d main_2d.cpp -lraylib -lopengl32 -lgdi32 -lwinmm */

#define _USE_MATH_DEFINES
#include "raylib.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <climits>
#include <algorithm>
#include <fstream>
#include <sstream>
using namespace std;
#ifndef PI
#define PI 3.14159265f
#endif

// ============================================================
// LAYOUT
// ============================================================
const int WIN_W=1100,WIN_H=700;
const int LEFT_W=190,MAP_W=580,MAP_X=190,RIGH_X=770,RIGH_W=330;
const int MAP_H=490,NODE_R=17,MAX_N=20,HASH_SZ=23,INICIO=0;
const int NUM_MAPS=5;
const float ANIM_SPD=0.55f;
const float SPR_FPS =0.16f;   // segundos por frame de sprite (~6fps suave)

// ============================================================
// SPRITES ANIMADOS
// ============================================================
struct SpriteInfo{
    Texture2D tex;
    int fw,fh;   // tamaño de cada frame en el sheet (px)
    int wx,wy;   // origen de la animacion walk en el sheet (px)
    int nf;      // cantidad de frames del walk
    float dh;    // alto de renderizado en pantalla (px)
    Color tint;  // tinte al dibujar (permite recolorear sprites LPC)
    bool ok;
    SpriteInfo():fw(1),fh(1),wx(0),wy(0),nf(1),dh(40.f),tint(WHITE),ok(false){}
};
SpriteInfo gSpr[5];
float gSprT=0.f;
int   gSprF=0;

// Musica
const char*MUSIC_FILES[6]={
    "Great Fairy's Fountain   The Legend of Zelda  Ocarina of Time - RetroLevel.mp3",
    "Peter Pan - Main Title Music (The Second Star to the Right) - Sam Novak.mp3",
    "Lord of the Rings Main Theme - Only best part - Gustav Rauhala.mp3",
    "Smash Mouth - All Star - SmashMouthVEVO.mp3",
    "Star Wars Main Theme (Full) - Coltsrock56.mp3",
    "Labrinth - Mount Everest (Official Video) - LabrinthVEVO.mp3"
};

// ============================================================
// COLORES
// ============================================================
const Color C_BG={8,12,22,255},C_PANEL={12,10,8,255},C_BOX={20,18,14,255};
const Color C_PARCH={210,185,125,255},C_WHITE={235,235,235,255};
const Color C_GRAY={140,140,140,255},C_DGRAY={55,55,55,255};
const Color C_NODE={40,100,50,255},C_VISIT={55,55,55,255};
const Color C_CUR={255,200,0,255},C_TREAS={200,25,25,255};
const Color C_ADJ={100,190,60,255},C_SEL={0,230,200,255};
const Color C_HINT={0,200,100,255},C_SOLVE={60,160,255,255};
const Color C_PATH={255,150,0,255},C_EDGE={110,90,55,180},C_EPATH={255,170,0,255};
const Color MAP_ACC[5]={{60,140,60,255},{150,110,40,255},{110,60,170,255},{40,60,150,255},{180,60,180,255}};
const Color MAP_BG[5]={{14,35,25,255},{35,25,12,255},{25,12,35,255},{8,12,28,255},{25,10,40,255}};
const int   MAP_TES[5]={5,5,5,5,19}; // nodo tesoro por mapa

// ============================================================
// [5] LISTA ENLAZADA
// ============================================================
struct NL{int v;NL*n;NL(int vv):v(vv),n(nullptr){}};
class Lista{
public:
    NL*h;int sz;
    Lista():h(nullptr),sz(0){}
    Lista(const Lista&o):h(nullptr),sz(0){for(NL*a=o.h;a;a=a->n)pushB(a->v);}
    Lista(Lista&&o)noexcept:h(o.h),sz(o.sz){o.h=nullptr;o.sz=0;}
    Lista&operator=(const Lista&o){if(this!=&o){clear();for(NL*a=o.h;a;a=a->n)pushB(a->v);}return*this;}
    Lista&operator=(Lista&&o)noexcept{if(this!=&o){clear();h=o.h;sz=o.sz;o.h=nullptr;o.sz=0;}return*this;}
    ~Lista(){clear();}
    void pushF(int v){NL*n=new NL(v);n->n=h;h=n;sz++;}
    void pushB(int v){NL*n=new NL(v);if(!h)h=n;else{NL*a=h;while(a->n)a=a->n;a->n=n;}sz++;}
    void clear(){while(h){NL*t=h;h=h->n;delete t;}sz=0;}
    bool empty()const{return h==nullptr;}
    vector<int>vec()const{vector<int>r;for(NL*a=h;a;a=a->n)r.push_back(a->v);return r;}
};

// ============================================================
// [4] TABLA HASH
// ============================================================
struct UD{int id;string nom,desc,pista;bool vis,tes;
    UD():id(-1),vis(false),tes(false){}
    UD(int i,const string&n,const string&d,const string&p,bool t):id(i),nom(n),desc(d),pista(p),vis(false),tes(t){}};
struct HE{int k;UD d;HE*n;HE(int kk,const UD&dd):k(kk),d(dd),n(nullptr){}};
class Hash{
    HE*T[HASH_SZ];
    int H(int k)const{return(int)((unsigned)(k*2654435761u)%HASH_SZ);}
public:
    Hash(){fill(T,T+HASH_SZ,(HE*)nullptr);}
    ~Hash(){for(int i=0;i<HASH_SZ;i++){HE*a=T[i];while(a){HE*t=a;a=a->n;delete t;}}}
    void put(int k,const UD&d){HE*e=new HE(k,d);e->n=T[H(k)];T[H(k)]=e;}
    UD*get(int k){for(HE*a=T[H(k)];a;a=a->n)if(a->k==k)return&a->d;return nullptr;}
    void reset(){for(int i=0;i<HASH_SZ;i++)for(HE*a=T[i];a;a=a->n)a->d.vis=false;}
};

// ============================================================
// [1] GRAFO + [3] BFS/DFS/Dijkstra
// ============================================================
struct Edge{int to,w;Edge(int t,int ww):to(t),w(ww){}};
class Graph{
public:
    int n;vector<Edge>adj[MAX_N];
    Graph():n(0){}
    void setN(int nn){n=nn;}
    void addE(int u,int v,int w){adj[u].push_back(Edge(v,w));adj[v].push_back(Edge(u,w));}
    bool adjTo(int u,int v)const{for(auto&e:adj[u])if(e.to==v)return true;return false;}
    Lista bfs(int s,int t,vector<int>&ord){
        bool vis[MAX_N]={};int par[MAX_N];fill(par,par+n,-1);
        queue<int>q;q.push(s);vis[s]=true;bool f=false;
        while(!q.empty()){int u=q.front();q.pop();ord.push_back(u);
            if(u==t){f=true;break;}
            for(auto&e:adj[u])if(!vis[e.to]){vis[e.to]=true;par[e.to]=u;q.push(e.to);}}
        Lista c;if(!f)return c;int x=t;while(x!=-1){c.pushF(x);x=par[x];}return c;}
    Lista dfs(int s,int t,vector<int>&ord){
        bool vis[MAX_N]={};int par[MAX_N];fill(par,par+n,-1);
        stack<pair<int,int>>stk;stk.push({s,-1});bool f=false;
        while(!stk.empty()){int u=stk.top().first,p=stk.top().second;stk.pop();
            if(vis[u])continue;vis[u]=true;par[u]=p;ord.push_back(u);
            if(u==t){f=true;break;}
            for(int i=(int)adj[u].size()-1;i>=0;i--)
                if(!vis[adj[u][i].to])stk.push({adj[u][i].to,u});}
        Lista c;if(!f)return c;int x=t;while(x!=-1){c.pushF(x);x=par[x];}return c;}
    Lista dijkstra(int s,int t,int*cost=nullptr){
        int dist[MAX_N],par[MAX_N];bool vis[MAX_N]={};
        fill(dist,dist+n,INT_MAX);fill(par,par+n,-1);dist[s]=0;
        priority_queue<pair<int,int>,vector<pair<int,int>>,greater<pair<int,int>>>pq;
        pq.push({0,s});
        while(!pq.empty()){int d=pq.top().first,u=pq.top().second;pq.pop();
            if(vis[u])continue;vis[u]=true;if(u==t)break;
            for(auto&e:adj[u])if(!vis[e.to]&&dist[u]+e.w<dist[e.to]){
                dist[e.to]=dist[u]+e.w;par[e.to]=u;pq.push({dist[e.to],e.to});}}
        if(cost)*cost=(dist[t]==INT_MAX)?-1:dist[t];
        Lista c;if(dist[t]==INT_MAX)return c;int x=t;while(x!=-1){c.pushF(x);x=par[x];}return c;}
    int pathW(const Lista&c)const{
        auto v=c.vec();int tot=0;
        for(int i=0;i+1<(int)v.size();i++)
            for(auto&e:adj[v[i]])if(e.to==v[i+1]){tot+=e.w;break;}
        return tot;}
};

// ============================================================
// [2] ARBOL DE DECISIONES
// ============================================================
struct TN{string kw,lbl;int id;TN*L,*R;
    TN(const string&k,const string&l,int i=-1):kw(k),lbl(l),id(i),L(nullptr),R(nullptr){}};
class DecTree{
    void freeT(TN*n){if(!n)return;freeT(n->L);freeT(n->R);delete n;}
    int walk(TN*n,const string&p)const{
        if(!n)return 0;if(n->id>=0)return n->id;
        return p.find(n->kw)!=string::npos?walk(n->L,p):walk(n->R,p);}
public:
    TN*root;DecTree():root(nullptr){}~DecTree(){freeT(root);}
    void build(const char**kws,const int*dsts,int nn,int def=0){
        freeT(root);root=_b(kws,dsts,nn,def);}
    TN*_b(const char**kws,const int*dsts,int nn,int def){
        if(nn==0)return new TN("","def",def);
        TN*nd=new TN(kws[0],kws[0]);nd->L=new TN("","->",dsts[0]);
        nd->R=_b(kws+1,dsts+1,nn-1,def);return nd;}
    int eval(string p)const{
        transform(p.begin(),p.end(),p.begin(),::tolower);return walk(root,p);}
};

// ============================================================
// DATOS DE LOS 5 MAPAS
// ============================================================
const char*MAP_NAME[5]={"Neverland","Tierra Media","Mundo de Shrek","Galaxia Star Wars","Euphoria"};
const char*MAP_SDESC[5]={
    "Peter Pan te guia al tesoro pirata",
    "El Portador lleva el Anillo al Monte Doom",
    "Shrek busca el tesoro del Bosque Encantado",
    "Yoda descifra el mapa estelar hacia el Templo del Primer Jedi en Ahch-To",
    "Rue busca escapar a traves de las vias del tren hacia la libertad"};
const char*MAP_FILE[5]={"Mapas/Neverland/mapa.txt","Mapas/TierraMedia/mapa.txt","Mapas/Shrek/mapa.txt","Mapas/StarWars/mapa.txt","Mapas/Euphoria/mapa.txt"};
const char*CLUE_FILE[5]={"Mapas/Neverland/pistas.txt","Mapas/TierraMedia/pistas.txt","Mapas/Shrek/pistas.txt","Mapas/StarWars/pistas.txt","Mapas/Euphoria/pistas.txt"};
const char*OUT_FILE[5]={"Mapas/Neverland/ruta.txt","Mapas/TierraMedia/ruta.txt","Mapas/Shrek/ruta.txt","Mapas/StarWars/ruta.txt","Mapas/Euphoria/ruta.txt"};

Vector2 MPOS[5][20]={
    {{350,460},{135,378},{290,398},{170,293},{370,328},{260,180},{510,412},{474,306},{48,312},{534,203},{256,76},{418,113},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
    {{115,428},{250,343},{425,233},{355,368},{395,183},{540,423},{315,443},{485,333},{165,288},{195,388},{235,443},{425,303},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
    {{58,438},{225,338},{365,256},{160,216},{440,373},{525,166},{78,316},{300,436},{445,453},{215,126},{350,126},{550,296},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
    {{70,440},{185,415},{78,235},{278,288},{408,218},{545,105},{372,392},{505,368},{148,358},{268,428},{418,145},{512,448},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
    {{290,80},{90,140},{460,110},{85,310},{280,210},{440,250},{540,350},{145,380},{390,380},{240,280},{520,160},{180,230},{350,360},{75,450},{500,420},{350,150},{155,480},{480,240},{320,420},{520,30}}
};
const char*MLBL[5][20]={
    {"Playa","JollyRgr","Cojituerto","Sirenas","Croco","TESORO!","Indio","Arbol","Canibal","Perdidos","Montanas","Lago","","","","","","","",""},
    {"Comarca","Bree","Rivendell","Moria","Lothlorien","MORDOR!","Rohan","M.Tirith","Isengard","Fangorn","Ab.Helm","AmonHen","","","","","","","",""},
    {"Pantano","Duloc","T.Fiona","FarFarAway","C.Dragon","BOSQUE!","Worcest.","JackHorner","S.Lorenzo","MamaLuna","GiantBean","Barco","","","","","","","",""},
    {"Tatooine","Jakku","Jedha","Coruscant","Bespin","AHCH-TO!","E.Muerte","Endor","Dagobah","Mustafar","Kamino","Crait","","","","","","","",""},
    {"Bano","Parque","Motel","Hab.Rue","Laberinto","Centro","CampoFt","TiendaFez","CasaCas","Teatro","CasaFez","CasaMad","CasaNat","Iglesia","CasaLaur","Agencia","EscArte","TerrenoCons","CasaTexas","VIAS_TREN!"}
};
const char*MKWDS[5][10]={
    {"calavera","sirena","cocodrilo","pirata","indio","","","","",""},
    {"fuego","elfo","moria","jinete","ciudad","ent","","","",""},
    {"bosque","fiona","dragon","farquaad","pirata","","","","",""},
    {"lava","jedi","nube","ewok","clon","","","","",""},
    {"rue","adiccion","droga","tren","oscuridad","escape","libertad","tren","",""}};
int MKDST[5][10]={{5,3,4,1,6,0,0,0,0,0},{5,4,3,6,7,9,0,0,0,0},{5,2,4,3,11,0,0,0,0,0},{9,8,4,7,10,0,0,0,0,0},{19,3,2,19,7,19,19,19,0,0}};
int MKCNT[5]={5,6,5,5,7};

// ============================================================
// [6] ARCHIVOS - generacion
// ============================================================
void genNeverland(){
    {ofstream f(MAP_FILE[0]);f<<"NODOS 12\n0 Playa_del_Desembarco\n1 El_Jolly_Roger\n2 Punto_Cojituerto\n3 Laguna_de_las_Sirenas\n4 Arroyo_del_Cocodrilo\n5 Roca_Calavera TESORO\n6 Campamento_Indio\n7 El_Arbol_Colgante\n8 Cala_Canibal\n9 Guarida_de_Perdidos\n10 Montanas_Nevadas\n11 Lago_Interior\n\nARISTAS\n0 1 3\n0 2 5\n0 6 7\n1 3 4\n1 8 6\n2 3 3\n2 4 5\n3 5 8\n3 11 4\n4 5 6\n4 9 4\n5 10 7\n6 7 3\n6 9 5\n7 10 4\n8 11 5\n9 11 3\n10 11 6\n";}
    {ofstream f(CLUE_FILE[0]);f<<"0 \"La arena dorada te recibe. El barco pirata al norte, el cabo de palo al noroeste.\"\n1 \"El Jolly Roger cruje. Las sirenas conocen el secreto de la roca calavera. Sigue sus voces.\"\n2 \"Sirenas al norte, el cocodrilo al este hace tictac...\"\n3 \"Las sirenas cantan: solo los valientes alcanzan la roca calavera. El lago queda al sur.\"\n4 \"El cocodrilo te observa. Tictac, tictac. La calavera de roca esta muy cerca, pirata.\"\n5 \"ENCONTRASTE EL TESORO! Bajo el ojo de la calavera yace el cofre del gran pirata.\"\n6 \"El jefe indio: el arbol al norte y los ninos perdidos al este conocen atajos.\"\n7 \"Los cuervos graznan: sube a las montanas para ver el ojo de la calavera.\"\n8 \"Cala Canibal es peligrosa. Huye al lago interior rapidamente.\"\n9 \"Los ninos perdidos: el lago conecta toda la isla. Puedes ir a cualquier lugar.\"\n10 \"Desde las montanas ves toda Neverland. El ojo de la calavera mira al sur.\"\n11 \"El Lago Interior, corazon de Neverland. La roca calavera esta al norte.\"\n";}
}
void genTierraMedia(){
    {ofstream f(MAP_FILE[1]);f<<"NODOS 12\n0 La_Comarca\n1 Bree\n2 Rivendell\n3 Moria\n4 Lothlorien\n5 Mordor TESORO\n6 Rohan\n7 Minas_Tirith\n8 Isengard\n9 Fangorn\n10 Abismo_de_Helm\n11 Amon_Hen\n\nARISTAS\n0 1 5\n0 9 8\n1 2 6\n1 3 7\n2 4 3\n2 3 5\n3 5 9\n3 11 4\n4 11 5\n5 7 6\n6 7 4\n6 10 3\n8 9 3\n8 10 5\n9 6 4\n10 6 3\n11 5 7\n";}
    {ofstream f(CLUE_FILE[1]);f<<"0 \"La Comarca, tu hogar hobbit. Bree al norte es tu primer paso en la gran aventura.\"\n1 \"En Bree, el Pasoso te espera. Rivendell guarda sabiduria, Moria oculta peligro.\"\n2 \"Los elfos de Rivendell: el Anillo debe ir al fuego de Mordor. Lothlorien te guia.\"\n3 \"Las minas oscuras de Moria. El fuego de Mordor esta al este.\"\n4 \"En el Bosque Dorado, Galadriel susurra: Solo el fuego puede destruirlo.\"\n5 \"EL MONTE DOOM! El Anillo Unico sera destruido en el fuego eterno de Mordor.\"\n6 \"Rohan, tierra de jinetes. Minas Tirith al este busca tu ayuda.\"\n7 \"Minas Tirith, la ciudad blanca. Mordor esta al este de las montanas.\"\n8 \"Isengard cayo. Los Ents de Fangorn marchan al este contigo.\"\n9 \"Los Ents de Fangorn marchan. Rohan al sur, el camino a Mordor.\"\n10 \"Los jinetes de Rohan van a Minas Tirith y luego al este oscuro.\"\n11 \"En Amon Hen escuchaste el llamado del Anillo. El fuego de Mordor espera.\"\n";}
}
void genShrek(){
    {ofstream f(MAP_FILE[2]);f<<"NODOS 12\n0 Pantano_de_Shrek\n1 Duloc\n2 Torre_de_Fiona\n3 Far_Far_Away\n4 Cueva_del_Dragon\n5 Bosque_Encantado TESORO\n6 Worcestershire\n7 Jack_Horner_Factory\n8 San_Lorenzo\n9 Mama_Lunas_House\n10 Giant_Bean\n11 Barco_Pirata\n\nARISTAS\n0 1 5\n0 6 4\n0 7 6\n1 2 5\n1 4 7\n2 3 4\n2 5 8\n3 9 5\n3 10 6\n4 11 4\n6 9 6\n7 8 3\n8 11 5\n9 10 4\n10 5 5\n11 5 7\n";}
    {ofstream f(CLUE_FILE[2]);f<<"0 \"El pantano de Shrek huele a cebollas y magia. Duloc al este, Worcestershire al norte.\"\n1 \"Duloc, el reino de Lord Farquaad. La Torre de Fiona al norte. La cueva del dragon tambien.\"\n2 \"Torre de la Princesa. Fiona espera. Far Far Away al norte. El bosque encantado al este...\"\n3 \"Far Far Away, reino real. Mama Luna al norte conoce los caminos, Giant Bean tambien.\"\n4 \"La Cueva del Dragon! La dragon enamorada del Asno guarda secretos del bosque.\"\n5 \"EL BOSQUE ENCANTADO! El tesoro de las hadas yace aqui, entre hongos magicos.\"\n6 \"Worcestershire, pueblo tranquilo. Mama Luna conoce los caminos del bosque encantado.\"\n7 \"Jack Horner acumula artefactos magicos. San Lorenzo esta al sur.\"\n8 \"San Lorenzo, donde vive el Gato con Botas. El barco pirata navega cerca.\"\n9 \"Mama Luna sabe cosas. Giant Bean al este, camino al bosque encantado.\"\n10 \"La Giant Bean sube hasta las nubes. El bosque encantado esta justo al oeste.\"\n11 \"El Barco Pirata. Los piratas dicen que el bosque encantado guarda el mayor tesoro.\"\n";}
}
void genStarWars(){
    {ofstream f(MAP_FILE[3]);
     f<<"NODOS 12\n"
       "0 Tatooine_Cantina_Mos_Eisley\n"
       "1 Jakku_Cementerio_Estelar\n"
       "2 Jedha_Ruinas_Ciudad_Santa\n"
       "3 Coruscant_Nivel_1313\n"
       "4 Bespin_Ciudad_Nubes\n"
       "5 Ahch-To_Templo_Primer_Jedi TESORO\n"
       "6 Estrella_de_la_Muerte\n"
       "7 Endor_Aldea_Ewok\n"
       "8 Dagobah_Pantano_Yoda\n"
       "9 Mustafar_Castillo_Vader\n"
       "10 Kamino_Ciudad_Tipoca\n"
       "11 Crait_Llanuras_Sal\n"
       "\nARISTAS\n"
       "0 1 5\n0 2 7\n0 8 6\n"
       "1 3 8\n"
       "2 6 5\n"
       "3 4 4\n3 6 6\n3 10 5\n"
       "4 5 9\n4 7 5\n4 8 7\n"
       "6 9 4\n"
       "7 5 8\n7 11 6\n"
       "8 9 5\n"
       "9 5 7\n"
       "10 11 5\n"
       "11 5 6\n";}
    {ofstream f(CLUE_FILE[3]);
     f<<"0 \"Tatooine, dos soles y arena infinita. La Cantina de Mos Eisley guarda secretos. Un cazarrecompensas susurra: los pantanos de Dagobah conocen el camino.\"\n"
       "1 \"Jakku, cementerio de Destructores Estelares. Entre la chatarra hay fragmentos de un mapa estelar. Coruscant, en el Nivel 1313, tiene la pieza que falta.\"\n"
       "2 \"Jedha, ciudad santa destruida por el Imperio. Un guardian rebelde grabo las coordenadas de la Estrella de la Muerte antes de morir entre las ruinas.\"\n"
       "3 \"Coruscant, Nivel 1313. Entre contrabandistas y luces de neon hay tres rutas: las nubes de Bespin, la Estrella de la Muerte imperial, y los clones de Kamino.\"\n"
       "4 \"Bespin, Ciudad de las Nubes. En la mesa de Sabacc un piloto susurra las coordenadas de Ahch-To. Tambien conoce los caminos a Endor y a Dagobah.\"\n"
       "5 \"AHCH-TO! En la cima de la isla, el Templo del Primer Jedi. El mapa estelar esta completo. Los textos sagrados del origen de la Fuerza descansan aqui.\"\n"
       "6 \"La Estrella de la Muerte. En el hangar de detencion un piloto rebelde grabo las coordenadas de Mustafar en el panel de control antes de ser capturado.\"\n"
       "7 \"Endor, la luna de los Ewok. Celebran la caida del Imperio. Los nativos conocen rutas hacia Ahch-To y hacia las llanuras de sal de Crait.\"\n"
       "8 \"Dagobah. El maestro Yoda en el pantano: la lava de Mustafar guarda verdades oscuras del lado Sith. Sigue el camino luminoso de la Fuerza hacia Ahch-To.\"\n"
       "9 \"Mustafar, planeta de lava negra. En el castillo de Darth Vader hay una camara secreta. Quien sobreviva el calor encontrara el camino a Ahch-To.\"\n"
       "10 \"Kamino, estacion de clonacion bajo tormentas eternas. Los registros imperiales apuntan a Crait, el ultimo refugio de la Resistencia en el borde de la galaxia.\"\n"
       "11 \"Crait, llanuras de sal carmesi. Al pisarlas el mineral rojo brilla bajo tus pies. Un piloto herido tiene las ultimas coordenadas: el Templo Jedi en Ahch-To.\"\n";}
}
void genEuphoria(){
    {ofstream f(MAP_FILE[4]);
     f<<"NODOS 20\n"
       "0 Bano_East_Highland\n"
       "1 Parque_Atracciones\n"
       "2 Motel_Cal_Jules\n"
       "3 Habitacion_Rue\n"
       "4 Laberinto_Espejos\n"
       "5 Centro_Comercial\n"
       "6 Campo_Futbol\n"
       "7 Tienda_Fezco\n"
       "8 Casa_Cassie_Lexi\n"
       "9 Teatro_Escuela\n"
       "10 Casa_Fezco\n"
       "11 Casa_Maddy\n"
       "12 Casa_Nate\n"
       "13 Iglesia_Alucinacion\n"
       "14 Casa_Laurie\n"
       "15 Agencia_Hollywood\n"
       "16 Escuela_Arte_Jules\n"
       "17 Terreno_Construccion\n"
       "18 Casa_Familia_Texas\n"
       "19 Vias_Tren TESORO\n"
       "\nARISTAS\n"
       "0 1 3\n0 2 5\n0 7 6\n"
       "1 4 2\n1 5 4\n"
       "2 3 3\n2 6 7\n"
       "3 13 5\n"
       "4 8 4\n"
       "5 11 3\n5 15 8\n"
       "6 12 5\n"
       "7 10 2\n7 14 6\n"
       "8 9 3\n"
       "9 16 4\n"
       "10 17 5\n"
       "11 18 7\n"
       "12 19 4\n"
       "13 19 6\n"
       "14 19 5\n"
       "15 19 9\n"
       "16 19 8\n"
       "17 19 6\n"
       "18 19 4\n";}
    {ofstream f(CLUE_FILE[4]);f<<"0 \"El bano de East Highland. Donde los secretos explotan entre las paredes mojadas. Fezco controla el flujo aqui cerca.\"\n1 \"Parque de atracciones. Donde todo comenzo a desmoronarse en la S1. El laberinto de espejos refleja la realidad distorsionada.\"\n2 \"Motel de citas. Donde Cal y Jules perpetuaban sus crimenes. Las paredes guardan traumas irrecuperables.\"\n3 \"La habitacion de Rue. Donde la adiccion consume cada respiro. Las luces rojas pulsean como un latido moribundo.\"\n4 \"Laberinto de espejos. Donde la mente se pierde en sus propios reflejos. La locura visual de Rue hecha tangible.\"\n5 \"Centro comercial. Donde Cassie y Maddy evadian sus miserias con ropa cara. Ahora solo queda el vacio.\"\n6 \"Campo de futbol. Territorio de McKay y sus traumas de rendimiento. El sacrificio pesa sobre cada atardecer.\"\n7 \"Tienda de Fezco. Centro de operaciones y tensión perpetua. Ashtray vigila desde las sombras.\"\n8 \"Casa de Cassie y Lexi. Donde la prima se esconde en el bano para colapsar silenciosamente. El caos domestico.\"\n9 \"Teatro de la escuela. Donde Lexi expone la verdad cruda: Our Life. El espejo que nadie queria ver.\"\n10 \"Casa de Fezco. Lugar del tiroteo brutal que cambio todo. Ashtray pago el precio.\"\n11 \"Casa de Maddy. Donde ella juega de madre entre sus secretos. Los vestidos caros no tapan el vacio.\"\n12 \"Casa de Nate. Donde la toxicidad permea cada habitacion con su padre. La prisión de su propia sangre.\"\n13 \"Iglesia de la alucinacion. Donde Rue abraza a Labrinth en su mente. El ultimo refugio antes del abismo.\"\n14 \"Casa de Laurie. Donde Rue arrastra su deuda masiva. La esclavitud que devora almas.\"\n15 \"Agencia de talentos en Hollywood. Donde Maddy construye su futuro entre ilusiones. El brillo que oculta el dolor.\"\n16 \"Escuela de arte de Jules. Donde ella intenta pintar un futuro lejos de todo. El arte como escape final.\"\n17 \"Terreno de construccion de Nate. Proyecto fallido rodeado por la mafia armenia. El sueno quebrado.\"\n18 \"Casa de la familia religiosa en Texas. Donde Rue se refugia en su viaje de regreso. La gracia en el caos.\"\n19 \"VIAS DEL TREN. El puente definitivo. El lugar del quiebre original donde todo comenzo a colapsar. La unica salida. Pero no hay libertad, solo el final del viaje.\"\n";}
}
void genMapFiles(int m){if(m==0)genNeverland();else if(m==1)genTierraMedia();else if(m==2)genShrek();else if(m==3)genStarWars();else genEuphoria();}

// ============================================================
// [6] CARGAR DATOS
// ============================================================
bool loadData(Graph&g,Hash&h,int&tes,int mapId){
    const char*mf=MAP_FILE[mapId],*cf=CLUE_FILE[mapId];
    ifstream fm(mf);if(!fm.is_open()){genMapFiles(mapId);fm.open(mf);if(!fm.is_open())return false;}
    int nn=0;vector<string>names(MAX_N);vector<bool>isTes(MAX_N,false);
    tes=-1;bool ae=false;string ln;
    while(getline(fm,ln)){
        if(ln.empty()||ln[0]=='#')continue;
        istringstream ss(ln);string tk;ss>>tk;
        if(tk=="NODOS"){ss>>nn;continue;}if(tk=="ARISTAS"){ae=true;continue;}
        if(ae){int u,v,w;istringstream s2(ln);if(s2>>u>>v>>w)g.addE(u,v,w);}
        else{int id;string nm,ex;istringstream s2(ln);if(!(s2>>id>>nm))continue;
            replace(nm.begin(),nm.end(),'_',' ');names[id]=nm;
            if(s2>>ex&&ex=="TESORO"){isTes[id]=true;tes=id;}}}
    fm.close();g.setN(nn);
    ifstream fp(cf);vector<string>pistas(MAX_N);
    if(fp.is_open()){while(getline(fp,ln)){if(ln.empty()||ln[0]=='#')continue;
        istringstream ss(ln);int id;ss>>id;
        size_t a=ln.find('"'),b=ln.rfind('"');
        if(a!=string::npos&&b!=a)pistas[id]=ln.substr(a+1,b-a-1);}fp.close();}
    for(int i=0;i<nn;i++){
        string d=isTes[i]?"*** "+names[i]+" - TESORO ***":names[i];
        h.put(i,UD(i,names[i],d,pistas[i],isTes[i]));}
    return true;
}
void saveRoute(const Lista&c,Hash&h,int cost,const string&met,int mid){
    const char*mapN[5]={"neverland","tierramedia","shrek","starwars","euphoria"};
    const char*mapD[5]={"Neverland","TierraMedia","Shrek","StarWars","Euphoria"};
    string folder="Mapas/";folder+=mapD[mid];folder+="/";
    int num=1;
    while(true){string fn=folder+"ruta_"+mapN[mid]+"_"+to_string(num)+".txt";ifstream ch(fn);if(!ch.good()){ch.close();break;}ch.close();num++;}
    string fpath=folder+"ruta_"+mapN[mid]+"_"+to_string(num)+".txt";
    ofstream f(fpath);if(!f.is_open())return;
    f<<"======================================\n  RUTA AL TESORO - "<<MAP_NAME[mid]<<"\n======================================\nMetodo: "<<met<<"\n\n";
    auto v=c.vec();for(int i=0;i<(int)v.size();i++){UD*d=h.get(v[i]);f<<"  Paso "<<(i+1)<<": ["<<(d?d->nom:"?")<<"]\n";if(d&&!d->pista.empty())f<<"  Pista: \""<<d->pista<<"\"\n\n";}
    if(cost>0)f<<"Costo total: "<<cost<<"\n";f<<"======================================\n";f.close();}

// ============================================================
// ESTADO GLOBAL
// ============================================================
Music  gTracks[6];
int    gCurTrack=0;
bool   gMusicOn=true;
int    gMenuSel=0;   // 0..3 main menu
int    gMapSel=0;    // 0..4 map select
float  gEscHold=0;   // segundos manteniendo ESC
Graph  gPreviewG[5]; // grafos pre-cargados para preview

void switchTrack(int idx){
    if(gCurTrack==idx)return;
    StopMusicStream(gTracks[gCurTrack]);gCurTrack=idx;
    PlayMusicStream(gTracks[gCurTrack]);
    SetMusicVolume(gTracks[gCurTrack],gMusicOn?0.7f:0.0f);}
void toggleMusic(){gMusicOn=!gMusicOn;SetMusicVolume(gTracks[gCurTrack],gMusicOn?0.7f:0.0f);}

// ============================================================
// ESTADO DE JUEGO
// ============================================================
enum Screen{SC_MAIN,SC_MAPSEL,SC_DESC,SC_GAME,SC_PLAY,SC_SOLVE,SC_WIN};
enum SMode{BFS_M,DFS_M,DIJ_M};
struct Ctx{
    Screen scr;SMode smode;
    int player,tesoro,hint,selAdj,activeMap;
    vector<int>order,path;int step;float atimer;bool done;int cost;
    Lista hist;int twLen;float twTimer;float blinkT;bool blinkOn;
    Graph*g;Hash*h;DecTree*tree;
    Ctx():scr(SC_MAIN),smode(BFS_M),player(0),tesoro(-1),hint(-1),selAdj(0),activeMap(0),
          step(0),atimer(0),done(false),cost(0),twLen(0),twTimer(0),blinkT(0),blinkOn(true),
          g(nullptr),h(nullptr),tree(nullptr){}
};

// ============================================================
// CARGA DE SPRITES
// ============================================================
// pre=true: strip ya tiene transparencia, no aplicar ImageColorReplace
static void loadSpr(int id,const char*p,int fw,int fh,int wx,int wy,int nf,float dh,bool pre=false,Color tint=WHITE){
    Image img=LoadImage(p);
    if(!img.data)return;
    ImageFormat(&img,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    if(!pre){Color bg=GetImageColor(img,0,0);ImageColorReplace(&img,bg,BLANK);}
    gSpr[id].tex=LoadTextureFromImage(img);
    SetTextureFilter(gSpr[id].tex,TEXTURE_FILTER_POINT);
    UnloadImage(img);
    gSpr[id].fw=fw;gSpr[id].fh=fh;gSpr[id].wx=wx;gSpr[id].wy=wy;
    gSpr[id].nf=nf;gSpr[id].dh=dh;gSpr[id].tint=tint;gSpr[id].ok=true;
}
void loadSprites(){
    // Todos LPC walk-down (frontal, como Frodo), 64x64, 9 frames
    // [0] Peter Pan  -> orco atletico + tinte verde vibrante
    loadSpr(0,"sprites/vader_lpc.png",    64,64,0,0,9,70.f,true,{80,255,90,255});
    // [1] Frodo      -> LPC original, colores naturales
    loadSpr(1,"sprites/frodo_strip.png",  64,64,0,0,9,70.f,true,WHITE);
    // [2] Shrek      -> orco rey verde, color natural
    loadSpr(2,"sprites/shrek_lpc.png",    64,64,0,0,9,70.f,true,WHITE);
    // [3] Star Wars  -> orco con tinte azul imperial oscuro
    loadSpr(3,"sprites/vader_lpc.png",    64,64,0,0,9,70.f,true,{40,80,180,255});
    // [4] Rue/Euphoria -> encapuchada (hoodie) tinte rosa-lila, silueta femenina
    loadSpr(4,"sprites/peterpan_lpc.png", 64,64,0,0,9,70.f,true,{255,120,210,255});
}
void unloadSprites(){
    for(int i=0;i<5;i++)if(gSpr[i].ok){UnloadTexture(gSpr[i].tex);gSpr[i].ok=false;}
}

// ============================================================
// PERSONAJES  (primitivas Raylib)
// ============================================================
// Peter Pan
void drawPeterPan(float cx,float cy,float sz){
    float t=(float)GetTime();
    // sombra
    DrawEllipse((int)cx,(int)(cy+sz*0.9f),(int)(sz*0.6f),(int)(sz*0.15f),{0,0,0,80});
    // cuerpo tunica verde
    DrawRectangle((int)(cx-sz*0.22f),(int)(cy+sz*0.25f),(int)(sz*0.44f),(int)(sz*0.55f),{34,130,34,255});
    // brazo izq (volando)
    DrawLineEx({cx-sz*0.22f,cy+sz*0.35f},{cx-sz*0.75f,cy+sz*0.1f},sz*0.12f,{34,130,34,255});
    // brazo der
    DrawLineEx({cx+sz*0.22f,cy+sz*0.35f},{cx+sz*0.75f,cy+sz*0.1f},sz*0.12f,{34,130,34,255});
    // piernas
    DrawLineEx({cx-sz*0.1f,cy+sz*0.8f},{cx-sz*0.2f,cy+sz*1.1f},sz*0.1f,{210,160,110,255});
    DrawLineEx({cx+sz*0.1f,cy+sz*0.8f},{cx+sz*0.2f,cy+sz*1.1f},sz*0.1f,{210,160,110,255});
    // cara
    DrawCircle((int)cx,(int)(cy+sz*0.05f),(int)(sz*0.28f),{255,220,170,255});
    // ojos
    DrawCircle((int)(cx-sz*0.1f),(int)(cy-sz*0.02f),(int)(sz*0.06f),{40,40,180,255});
    DrawCircle((int)(cx+sz*0.1f),(int)(cy-sz*0.02f),(int)(sz*0.06f),{40,40,180,255});
    // sonrisa
    DrawCircleLines((int)cx,(int)(cy+sz*0.12f),(int)(sz*0.12f),{150,80,50,255});
    DrawRectangle((int)(cx-sz*0.12f),(int)(cy+sz*0.0f),(int)(sz*0.24f),(int)(sz*0.12f),{255,220,170,220});
    // sombrero (triangulo verde puntiagudo)
    DrawTriangle({cx,cy-sz*0.6f},{cx-sz*0.32f,cy-sz*0.22f},{cx+sz*0.32f,cy-sz*0.22f},{34,130,34,255});
    DrawRectangle((int)(cx-sz*0.3f),(int)(cy-sz*0.25f),(int)(sz*0.6f),(int)(sz*0.07f),{20,100,20,255});
    // estrella en sombrero (brilla)
    float stBr=(sinf(t*3)+1)*0.5f;
    DrawCircle((int)(cx+sz*0.18f),(int)(cy-sz*0.32f),(int)(sz*0.07f),{255,255,(unsigned char)(100+100*stBr),255});
}

// El Anillo Unico
void drawRing(float cx,float cy,float sz){
    float t=(float)GetTime();
    // Sombra
    DrawEllipse((int)cx,(int)(cy+sz*0.85f),(int)(sz*0.55f),(int)(sz*0.12f),{0,0,0,80});
    // Glow exterior
    for(int i=4;i>=1;i--)
        DrawRing({cx,cy},(int)(sz*0.42f),(int)(sz*0.7f+i*4),0,360,40,{200,160,20,(unsigned char)(20*i)});
    // Anillo dorado
    DrawRing({cx,cy},sz*0.42f,sz*0.7f,0,360,40,{220,185,30,255});
    // Borde exterior
    DrawRing({cx,cy},sz*0.69f,sz*0.73f,0,360,40,{255,220,80,255});
    // Interior oscuro
    DrawCircle((int)cx,(int)cy,(int)(sz*0.42f),{30,20,10,200});
    // Inscripcion (brillo rojo-fuego que pulsa)
    float glA=(sinf(t*2.5f)+1)*0.5f;
    DrawRing({cx,cy},sz*0.35f,sz*0.42f,0,360,30,{220,60,0,(unsigned char)(glA*180)});
    // Texto en el interior del anillo
    DrawText("ONE",(int)(cx-MeasureText("ONE",9)/2),(int)(cy-5),9,{200,150,0,(unsigned char)(100+100*glA)});
}

// Shrek el Ogro
void drawShrek(float cx,float cy,float sz){
    // sombra
    DrawEllipse((int)cx,(int)(cy+sz*0.95f),(int)(sz*0.65f),(int)(sz*0.15f),{0,0,0,80});
    // cuerpo (camisa)
    DrawRectangle((int)(cx-sz*0.35f),(int)(cy+sz*0.5f),(int)(sz*0.7f),(int)(sz*0.55f),{160,120,70,255});
    // brazos
    DrawCircle((int)(cx-sz*0.55f),(int)(cy+sz*0.55f),(int)(sz*0.22f),{100,160,50,255});
    DrawCircle((int)(cx+sz*0.55f),(int)(cy+sz*0.55f),(int)(sz*0.22f),{100,160,50,255});
    // cabeza grande
    DrawEllipse((int)cx,(int)cy,(int)(sz*0.52f),(int)(sz*0.58f),{100,160,50,255});
    // orejas-cuernos
    DrawCircle((int)(cx-sz*0.48f),(int)(cy-sz*0.28f),(int)(sz*0.18f),{90,148,45,255});
    DrawCircle((int)(cx+sz*0.48f),(int)(cy-sz*0.28f),(int)(sz*0.18f),{90,148,45,255});
    // bordes orejas
    DrawCircleLines((int)(cx-sz*0.48f),(int)(cy-sz*0.28f),(int)(sz*0.18f),{70,120,35,255});
    DrawCircleLines((int)(cx+sz*0.48f),(int)(cy-sz*0.28f),(int)(sz*0.18f),{70,120,35,255});
    // ojos blancos
    DrawEllipse((int)(cx-sz*0.2f),(int)(cy-sz*0.12f),(int)(sz*0.15f),(int)(sz*0.18f),WHITE);
    DrawEllipse((int)(cx+sz*0.2f),(int)(cy-sz*0.12f),(int)(sz*0.15f),(int)(sz*0.18f),WHITE);
    // pupilas
    DrawCircle((int)(cx-sz*0.2f),(int)(cy-sz*0.08f),(int)(sz*0.08f),{60,100,30,255});
    DrawCircle((int)(cx+sz*0.2f),(int)(cy-sz*0.08f),(int)(sz*0.08f),{60,100,30,255});
    // nariz bulbosa
    DrawEllipse((int)cx,(int)(cy+sz*0.1f),(int)(sz*0.16f),(int)(sz*0.12f),{85,140,40,255});
    DrawCircle((int)(cx-sz*0.08f),(int)(cy+sz*0.1f),(int)(sz*0.04f),{70,120,30,255});
    DrawCircle((int)(cx+sz*0.08f),(int)(cy+sz*0.1f),(int)(sz*0.04f),{70,120,30,255});
    // sonrisa grande
    DrawText(":D",(int)(cx-sz*0.22f),(int)(cy+sz*0.28f),(int)(sz*0.35f),{50,90,25,255});
}

// Darth Vader
void drawVader(float cx,float cy,float sz){
    float t=(float)GetTime();
    float glR=(sinf(t*3)+1)*0.5f;
    // sombra
    DrawEllipse((int)cx,(int)(cy+sz*1.1f),(int)(sz*0.5f),(int)(sz*0.12f),{0,0,0,100});
    // capa (triangulo negro)
    DrawTriangle({cx-sz*0.5f,cy+sz*1.1f},{cx+sz*0.5f,cy+sz*1.1f},{cx,cy+sz*0.4f},{15,15,15,255});
    // cuerpo
    DrawRectangle((int)(cx-sz*0.32f),(int)(cy+sz*0.4f),(int)(sz*0.64f),(int)(sz*0.7f),{20,20,20,255});
    // panel de control en el pecho
    DrawRectangle((int)(cx-sz*0.22f),(int)(cy+sz*0.5f),(int)(sz*0.44f),(int)(sz*0.22f),{35,35,35,255});
    DrawCircle((int)(cx-sz*0.13f),(int)(cy+sz*0.6f),(int)(sz*0.055f),{220,30,30,255});
    DrawCircle((int)(cx+sz*0.0f),(int)(cy+sz*0.6f),(int)(sz*0.055f),{30,220,30,255});
    DrawCircle((int)(cx+sz*0.13f),(int)(cy+sz*0.6f),(int)(sz*0.055f),{30,80,220,255});
    // cuello
    DrawRectangle((int)(cx-sz*0.12f),(int)(cy+sz*0.28f),(int)(sz*0.24f),(int)(sz*0.14f),{25,25,25,255});
    // casco (dombo negro)
    DrawEllipse((int)cx,(int)(cy-sz*0.1f),(int)(sz*0.38f),(int)(sz*0.44f),{15,15,15,255});
    // placa facial
    DrawRectangle((int)(cx-sz*0.25f),(int)(cy+sz*0.08f),(int)(sz*0.5f),(int)(sz*0.22f),{22,22,22,255});
    // rejilla boca
    for(int i=0;i<5;i++)
        DrawRectangle((int)(cx-sz*0.2f+i*sz*0.09f),(int)(cy+sz*0.18f),(int)(sz*0.05f),(int)(sz*0.08f),{10,10,10,255});
    // ojos rojos que brillan
    DrawCircle((int)(cx-sz*0.14f),(int)(cy-sz*0.04f),(int)(sz*0.08f),{200,0,0,(unsigned char)(180+75*glR)});
    DrawCircle((int)(cx+sz*0.14f),(int)(cy-sz*0.04f),(int)(sz*0.08f),{200,0,0,(unsigned char)(180+75*glR)});
    // sable de luz ROJO
    float sabX=cx+sz*0.55f;
    DrawLineEx({sabX,cy+sz*0.55f},{sabX,cy-sz*1.2f},sz*0.1f,{200,0,0,255});
    DrawLineEx({sabX,cy+sz*0.55f},{sabX,cy-sz*1.2f},sz*0.04f,{255,150,150,200});
    DrawCircle((int)sabX,(int)(cy+sz*0.55f),(int)(sz*0.1f),{50,50,50,255});
}

void drawMapChar(int mapId,float cx,float cy,float sz){
    SpriteInfo&s=gSpr[mapId];
    if(s.ok){
        int f=gSprF%s.nf;
        Rectangle src={(float)(s.wx+f*s.fw),(float)s.wy,(float)s.fw,(float)s.fh};
        float dh=s.dh, dw=dh*(float)s.fw/(float)s.fh;
        Rectangle dst={cx-dw*.5f, cy-dh, dw, dh};
        DrawTexturePro(s.tex,src,dst,{0,0},0.f,s.tint);
        return;
    }
    // Fallback: primitivas Raylib
    if(mapId==0)drawPeterPan(cx,cy,sz);
    else if(mapId==1)drawRing(cx,cy,sz);
    else if(mapId==2)drawShrek(cx,cy,sz);
    else drawVader(cx,cy,sz);
}

// ============================================================
// HELPERS
// ============================================================
bool inPath(const vector<int>&p,int v){for(auto x:p)if(x==v)return true;return false;}
bool edgeInPath(const vector<int>&p,int u,int v){
    for(int i=0;i+1<(int)p.size();i++)if((p[i]==u&&p[i+1]==v)||(p[i]==v&&p[i+1]==u))return true;return false;}
void wrapDraw(const string&txt,int x,int y,int mw,int sz,Color col,int mc=-1){
    if(txt.empty())return;
    string show=mc<0?txt:txt.substr(0,min(mc,(int)txt.size()));
    istringstream ss(show);string word,line;int ly=y;
    while(ss>>word){string test=line.empty()?word:(line+" "+word);
        if(MeasureText(test.c_str(),sz)>mw&&!line.empty()){DrawText(line.c_str(),x,ly,sz,col);ly+=sz+3;line=word;}
        else line=test;}if(!line.empty())DrawText(line.c_str(),x,ly,sz,col);}
void drawGlow(Vector2 p,float r,Color col){
    for(int i=3;i>=1;i--)DrawCircleV(p,r+(float)(i*5),{col.r,col.g,col.b,(unsigned char)(20*i)});
    DrawCircleV(p,r,col);}

// Escalar MPOS a pantalla de juego
Vector2 mposToScreen(int m,int i){
    return {MPOS[m][i].x+MAP_X, MPOS[m][i].y};}
// Escalar MPOS a area de preview (px,py=origen, pw,ph=tamano)
Vector2 mposToPreview(int m,int i,float px,float py,float pw,float ph){
    return {px+MPOS[m][i].x*pw/580.f, py+MPOS[m][i].y*ph/490.f};}

// Mouse convertido a espacio de diseno 1100x700
Vector2 mouseDes(){
    Vector2 r=GetMousePosition();
    return {r.x*WIN_W/(float)GetScreenWidth(),r.y*WIN_H/(float)GetScreenHeight()};}

// ============================================================
// MINI GRAFO (preview)
// ============================================================
void drawMiniGraph(int mapId,float px,float py,float pw,float ph,float nodeR=7){
    Graph&g=gPreviewG[mapId];
    Color acc=MAP_ACC[mapId];
    // Fondo
    DrawRectangle((int)px,(int)py,(int)pw,(int)ph,{0,0,0,80});
    DrawRectangleLinesEx({px,py,pw,ph},1,{acc.r,acc.g,acc.b,80});
    // Aristas
    for(int i=0;i<g.n;i++)for(auto&e:g.adj[i])if(e.to>i){
        Vector2 A=mposToPreview(mapId,i,px,py,pw,ph);
        Vector2 B=mposToPreview(mapId,e.to,px,py,pw,ph);
        DrawLineEx(A,B,1.5f,{acc.r,acc.g,acc.b,120});}
    // Nodos
    for(int i=0;i<g.n;i++){
        Vector2 p=mposToPreview(mapId,i,px,py,pw,ph);
        bool tes=(i==5); // tesoro siempre nodo 5
        DrawCircleV(p,nodeR,tes?C_TREAS:acc);
        if(tes)DrawText("$",(int)(p.x-4),(int)(p.y-5),10,{255,220,0,255});
        // Etiqueta pequeña
        int tw=MeasureText(MLBL[mapId][i],7);
        DrawText(MLBL[mapId][i],(int)(p.x-tw/2),(int)(p.y+nodeR+2),7,{200,180,130,200});}
}

// ============================================================
// ESC COUNTDOWN OVERLAY
// ============================================================
void drawEscOverlay(float hold){
    if(hold<0.3f)return;
    int dots=min(5,(int)hold);
    DrawRectangle(0,WIN_H-44,WIN_W,44,{0,0,0,210});
    DrawRectangle(0,WIN_H-45,WIN_W,1,{180,50,50,200});
    // Barra de progreso
    float prog=hold/5.f;
    DrawRectangle(0,WIN_H-4,(int)(WIN_W*prog),4,{200,50,50,255});
    string msg="Mantener ESC para cerrar  ";
    for(int i=0;i<dots;i++)msg+=".  ";
    int tw=MeasureText(msg.c_str(),15);
    DrawText(msg.c_str(),WIN_W/2-tw/2,WIN_H-32,15,{220,110,110,255});}

// ============================================================
// PANEL IZQUIERDO (algoritmos)
// ============================================================
void drawLeftPanel(Ctx&c){
    DrawRectangle(0,0,LEFT_W,WIN_H,{10,8,6,255});
    DrawRectangle(LEFT_W-2,0,2,WIN_H,{60,48,30,200});
    Color acc=MAP_ACC[c.activeMap];
    DrawText("MAPA",8,10,10,{120,100,65,255});
    wrapDraw(MAP_NAME[c.activeMap],8,24,LEFT_W-16,12,acc);
    DrawLine(8,46,LEFT_W-10,46,{50,40,25,200});
    int y=56;
    const char*opts[]={"[1] Explorar","[2] BFS","[3] DFS","[4] Dijkstra"};
    for(int i=0;i<4;i++){
        bool active=(c.scr==SC_PLAY&&i==0)||
                    (c.scr==SC_SOLVE&&((i==1&&c.smode==BFS_M)||(i==2&&c.smode==DFS_M)||(i==3&&c.smode==DIJ_M)));
        Color tc=active?C_CUR:(c.scr>=SC_GAME?acc:C_GRAY);
        if(active){DrawRectangle(4,y-3,LEFT_W-12,20,{30,25,15,180});DrawRectangleLinesEx({4,(float)(y-3),(float)(LEFT_W-12),20},1,acc);}
        DrawText(opts[i],10,y,13,tc);y+=28;}
    DrawLine(8,y,LEFT_W-10,y,{50,40,25,200});y+=10;
    DrawText("[5] Arbol",10,y,12,C_GRAY);y+=24;
    DrawText("[ESC] Atras",10,y,12,{90,80,60,255});
    if(c.scr==SC_WIN){DrawLine(8,WIN_H-60,LEFT_W-10,WIN_H-60,{50,40,25,200});
        DrawText("VICTORIA!",10,WIN_H-48,14,C_TREAS);DrawText("[ENTER]",10,WIN_H-28,11,{100,190,255,255});}
}

// ============================================================
// AREA DEL MAPA
// ============================================================
void drawMapArea(Ctx&c){
    Color bg=MAP_BG[c.activeMap];Color acc=MAP_ACC[c.activeMap];
    DrawRectangle(MAP_X,0,MAP_W,MAP_H,bg);
    DrawRectangle(MAP_X+40,30,500,420,{(unsigned char)min(255,bg.r+8),(unsigned char)min(255,bg.g+18),(unsigned char)min(255,bg.b+8),255});
    DrawText(MAP_NAME[c.activeMap],MAP_X+8,6,16,acc);
    Graph*g=c.g;float bob=(float)sin(GetTime()*2.5f)*3.f;
    // Aristas
    for(int i=0;i<g->n;i++)for(auto&e:g->adj[i])if(e.to>i){
        bool onP=c.done&&edgeInPath(c.path,i,e.to);
        int selN=(c.scr==SC_PLAY&&(int)g->adj[c.player].size()>c.selAdj)?g->adj[c.player][c.selAdj].to:-1;
        bool isSel=c.scr==SC_PLAY&&((i==c.player&&e.to==selN)||(e.to==c.player&&i==selN));
        Color ec=isSel?C_SEL:onP?C_EPATH:C_EDGE;
        Vector2 A=mposToScreen(c.activeMap,i),B=mposToScreen(c.activeMap,e.to);
        DrawLineEx(A,B,onP?3.f:2.f,ec);
        Vector2 mid={(A.x+B.x)/2,(A.y+B.y)/2};
        DrawText(TextFormat("%d",e.w),(int)mid.x-5,(int)mid.y-7,9,{170,145,85,185});}
    // Nodos
    for(int i=0;i<g->n;i++){
        Vector2 base=mposToScreen(c.activeMap,i);
        bool vis=false;for(int k=0;k<c.step&&k<(int)c.order.size();k++)if(c.order[k]==i){vis=true;break;}
        Color nc=C_NODE;
        if(c.scr==SC_PLAY||c.scr==SC_WIN){
            if(i==c.player)nc=C_CUR;else if(c.h->get(i)&&c.h->get(i)->tes)nc=C_TREAS;
            else if(i==c.hint)nc=C_HINT;
            else{int selN=(int)g->adj[c.player].size()>c.selAdj?g->adj[c.player][c.selAdj].to:-1;
                if(c.scr==SC_PLAY&&g->adjTo(c.player,i))nc=(i==selN)?C_SEL:C_ADJ;
                else if(c.h->get(i)&&c.h->get(i)->vis)nc=C_VISIT;}}
        else if(c.scr==SC_SOLVE||c.scr==SC_GAME){
            UD*d=c.h?c.h->get(i):nullptr;
            if(d&&d->tes)nc=C_TREAS;else if(c.done&&inPath(c.path,i))nc=C_PATH;
            else if(!c.done&&c.step>0&&i==c.order[c.step-1])nc=C_SOLVE;
            else if(vis)nc=C_VISIT;}
        // Dibujar personaje en nodo actual (SC_PLAY)
        if(i==c.player&&c.scr==SC_PLAY){
            drawGlow({base.x,base.y+bob},(float)NODE_R,C_CUR);
            drawMapChar(c.activeMap,base.x,base.y-NODE_R*0.5f+bob,NODE_R*1.1f);
            Vector2 tri[]={{base.x-8,base.y-NODE_R-14+bob},{base.x+8,base.y-NODE_R-14+bob},{base.x,base.y-NODE_R-2+bob}};
            DrawTriangle(tri[0],tri[1],tri[2],C_CUR);
        }else if(c.h&&c.h->get(i)&&c.h->get(i)->tes){
            drawGlow(base,(float)NODE_R,C_TREAS);DrawText("$",(int)base.x-4,(int)base.y-7,14,{255,220,0,255});
        }else{DrawCircleV(base,(float)NODE_R,nc);DrawCircleLines((int)base.x,(int)base.y,(float)NODE_R,{190,190,190,130});}
        int tw=MeasureText(MLBL[c.activeMap][i],10);
        DrawText(MLBL[c.activeMap][i],(int)base.x-tw/2,(int)base.y+NODE_R+3,10,C_PARCH);
        DrawText(TextFormat("%d",i),(int)base.x+NODE_R-5,(int)base.y-NODE_R,8,{120,120,120,160});}
    // Menu de movimiento / leyenda
    DrawRectangle(MAP_X,MAP_H,MAP_W,WIN_H-MAP_H,C_BOX);
    DrawRectangleLinesEx({(float)MAP_X,(float)MAP_H,(float)MAP_W,(float)(WIN_H-MAP_H)},1,{60,50,35,200});
    if(c.scr==SC_PLAY){
        UD*d=c.h->get(c.player);int px=MAP_X+14,py=MAP_H+10;
        DrawText(d?d->nom.c_str():"?",px,py,13,C_CUR);py+=17;
        DrawLine(px,py,MAP_X+MAP_W-14,py,C_DGRAY);py+=8;
        DrawText("A donde ir?",px,py,12,{170,145,90,255});py+=17;
        for(int i=0;i<(int)g->adj[c.player].size();i++){
            int v=g->adj[c.player][i].to;UD*dv=c.h->get(v);bool sel=(i==c.selAdj);
            Color tc=sel?C_SEL:C_ADJ;if(sel&&c.blinkOn)DrawText(">",(int)px,(int)py,13,C_SEL);
            DrawText(dv?dv->nom.c_str():"?",px+16,py,13,tc);
            DrawText(TextFormat("%d leg",g->adj[c.player][i].w),px+265,py,11,sel?C_SEL:C_GRAY);py+=16;}
        DrawText("[Up/Down] Sel  [ENTER/Z] Mover  [H] Pista  [ESC] Atras",MAP_X+14,WIN_H-18,10,C_DGRAY);
        if(c.hint>=0){UD*dh=c.h->get(c.hint);if(dh)DrawText(TextFormat("->%s",dh->nom.c_str()),MAP_X+MAP_W-180,WIN_H-18,10,C_HINT);}
    }else{
        // Leyenda
        int lx=MAP_X+8,ly=MAP_H+12;
        Color lc[]={C_CUR,C_TREAS,C_PATH,C_SOLVE,C_ADJ,C_VISIT};
        const char*ll[]={"Posicion","Tesoro","Ruta","Visitando","Ir aqui","Visitado"};
        for(int i=0;i<6;i++){DrawCircle(lx+7,ly+7,7,lc[i]);DrawText(ll[i],lx+18,ly+1,11,C_GRAY);lx+=MeasureText(ll[i],11)+32;}
        if(c.scr==SC_SOLVE){DrawText("[ESP] Saltar  [ENTER] Menu",MAP_X+MAP_W/2-100,WIN_H-18,11,C_DGRAY);}
    }
}

// ============================================================
// PANEL DERECHO
// ============================================================
void drawRightPanel(Ctx&c){
    DrawRectangle(RIGH_X,0,RIGH_W,WIN_H,C_PANEL);DrawRectangle(RIGH_X,0,2,WIN_H,{50,40,25,200});
    int px=RIGH_X+14,pw=RIGH_W-24,py=14;Color acc=MAP_ACC[c.activeMap];
    if(c.scr==SC_GAME){
        DrawText("SELECCIONA",px,py,11,{140,115,65,255});py+=16;DrawLine(px,py,px+pw,py,C_DGRAY);py+=10;
        DrawText("Usa el panel izq.",px,py,11,C_GRAY);py+=14;DrawText("para explorar.",px,py,11,C_GRAY);py+=28;
        wrapDraw(MAP_SDESC[c.activeMap],px,py,pw,11,acc);py+=50;DrawLine(px,py,px+pw,py,C_DGRAY);py+=10;
        const char*es[]={"Grafo - Adyacencia","Arbol - Decision","Cola/Pila - BFS/DFS","Hash - Datos","Lista - Ruta","Archivos - I/O"};
        for(int i=0;i<6;i++){DrawText(es[i],px,py,11,C_GRAY);py+=14;}
    }else if(c.scr==SC_PLAY){
        UD*d=c.h->get(c.player);
        DrawText("UBICACION",px,py,11,{140,115,65,255});py+=16;DrawLine(px,py,px+pw,py,C_DGRAY);py+=8;
        if(d){DrawText(d->nom.c_str(),px,py,13,C_CUR);py+=20;wrapDraw(d->desc,px,py,pw,10,C_GRAY);py+=32;
            DrawLine(px,py,px+pw,py,C_DGRAY);py+=8;DrawText("PISTA:",px,py,11,{210,180,70,255});py+=15;
            wrapDraw(d->pista,px,py,pw,11,{205,200,172,255},c.twLen);
            if(c.twLen<(int)d->pista.size()&&c.blinkOn)DrawText("_",px,py+56,12,{200,195,168,255});py+=90;}
        DrawLine(px,py,px+pw,py,C_DGRAY);py+=8;
        DrawText(TextFormat("Nodos: %d | Leg: %d",c.hist.sz-1,c.g->pathW(c.hist)),px,py,11,C_GRAY);
    }else if(c.scr==SC_SOLVE){
        const char*t=c.smode==BFS_M?"BFS":c.smode==DFS_M?"DFS":"DIJKSTRA";
        DrawText(t,px,py,16,C_CUR);py+=22;DrawLine(px,py,px+pw,py,C_DGRAY);py+=10;
        if(!c.done){DrawText("Explorando...",px,py,13,C_SOLVE);py+=20;
            DrawText(TextFormat("Paso %d/%d",min(c.step,(int)c.order.size()),(int)c.order.size()),px,py,12,C_GRAY);py+=18;
            if(c.step>0&&c.step<=(int)c.order.size()){UD*d=c.h->get(c.order[c.step-1]);
                DrawText("Visitando:",px,py,12,{170,145,90,255});py+=14;if(d)DrawText(d->nom.c_str(),px,py,13,C_SOLVE);}
        }else{DrawText("RUTA HALLADA!",px,py,13,{80,240,80,255});py+=20;
            if(c.cost>0){DrawText(TextFormat("Costo: %d",c.cost),px,py,12,C_PARCH);py+=18;}
            DrawText("Camino:",px,py,12,{170,145,90,255});py+=14;
            for(int i=0;i<(int)c.path.size();i++){UD*d=c.h->get(c.path[i]);
                DrawText(TextFormat("%d. %s",i+1,d?d->nom.c_str():"?"),px,py,11,C_PATH);py+=13;}
            py+=6;DrawText("Guardado en archivo",px,py,10,C_DGRAY);}
    }else if(c.scr==SC_WIN){
        DrawText("VICTORIA!",px,py,20,C_TREAS);py+=28;DrawLine(px,py,px+pw,py,C_DGRAY);py+=10;
        UD*d=c.h->get(c.tesoro);wrapDraw(d?d->pista:"",px,py,pw,12,{220,215,175,255});py+=80;
        DrawLine(px,py,px+pw,py,C_DGRAY);py+=10;DrawText("TU RUTA:",px,py,12,{170,145,90,255});py+=14;
        auto v=c.hist.vec();for(int i=0;i<(int)v.size();i++){UD*di=c.h->get(v[i]);
            DrawText(TextFormat("%d. %s",i+1,di?di->nom.c_str():"?"),px,py,11,C_PATH);py+=13;}
        py+=8;DrawText(TextFormat("Costo: %d",c.g->pathW(c.hist)),px,py,12,C_PARCH);py+=18;
        DrawText("[ENTER] Menu principal",px,py,11,{100,190,255,255});}
}

// ============================================================
// MENU PRINCIPAL (navegacion con flechas)
// ============================================================
const char*MAIN_OPTS[4]={"Elegir Mapa","Descripcion del juego","Silenciar / Activar musica","Salir"};
const Color MAIN_OPT_COL[4]={{100,190,60,255},{210,185,125,255},{140,140,140,255},{110,110,110,255}};

void drawMainMenu(){
    DrawRectangle(0,0,WIN_W,WIN_H,C_BG);
    srand(42);
    for(int i=0;i<80;i++){int sx=rand()%WIN_W,sy=rand()%WIN_H;float br=0.4f+(float)sin(GetTime()*1.5f+(float)i)*0.4f;
        DrawCircle(sx,sy,1,{(unsigned char)(200*br),(unsigned char)(200*br),(unsigned char)(255*br),200});}
    srand((unsigned)time(nullptr));
    int bx=WIN_W/2-260,by=WIN_H/2-250,bw=520,bh=500;
    DrawRectangle(bx,by,bw,bh,{18,14,10,240});DrawRectangleLinesEx({(float)bx,(float)by,(float)bw,(float)bh},2,C_PARCH);
    float t=(float)GetTime();
    Color tc={(unsigned char)(220+30*(float)sin(t)),(unsigned char)(175+20*(float)sin(t+1)),(unsigned char)(45+30*(float)sin(t+2)),255};
    const char*t1="BUSQUEDA DE TESORO";
    DrawText(t1,bx+bw/2-MeasureText(t1,32)/2,by+22,32,tc);
    const char*sub="Estructuras de Datos Avanzada";
    DrawText(sub,bx+bw/2-MeasureText(sub,13)/2,by+62,13,C_PARCH);
    DrawLine(bx+30,by+86,bx+bw-30,by+86,{90,70,40,200});
    // Opciones con cursor de seleccion
    int oy=by+104;
    for(int i=0;i<4;i++){
        bool sel=(i==gMenuSel);
        if(sel){DrawRectangle(bx+20,oy-6,bw-40,38,{35,28,15,200});
            DrawRectangleLinesEx({(float)(bx+20),(float)(oy-6),(float)(bw-40),38},1,MAIN_OPT_COL[i]);}
        Color ic=sel?C_CUR:MAIN_OPT_COL[i];
        if(sel&&(int)(GetTime()*2)%2==0)DrawText(">",(int)(bx+32),(int)(oy+4),18,C_CUR);
        DrawText(MAIN_OPTS[i],bx+54,oy+4,17,ic);
        // Icono musica si es opcion 2
        if(i==2)DrawText(gMusicOn?"[ON]":"[OFF]",bx+bw-90,oy+4,13,gMusicOn?C_HINT:C_TREAS);
        oy+=50;}
    DrawLine(bx+30,oy+6,bx+bw-30,oy+6,{70,55,30,140});
    const char*pie="Grafo | Arbol | Hash | Lista | Cola/Pila | Archivos";
    DrawText(pie,bx+bw/2-MeasureText(pie,10)/2,oy+16,10,{80,68,48,255});
    DrawText("[UP/DOWN] Navegar   [ENTER] Seleccionar   [ESC 5s] Cerrar",
        WIN_W/2-MeasureText("[UP/DOWN] Navegar   [ENTER] Seleccionar   [ESC 5s] Cerrar",11)/2,WIN_H-22,11,C_DGRAY);}

// ============================================================
// ELEGIR MAPA (flechas + enter + click + preview)
// ============================================================
void drawMapSelect(){
    DrawRectangle(0,0,WIN_W,WIN_H,C_BG);
    // Panel izquierdo: lista de mapas
    DrawRectangle(0,0,480,WIN_H,{10,8,6,255});
    DrawRectangle(479,0,1,WIN_H,{60,50,35,200});
    DrawText("ELEGIR MAPA",20,18,22,C_PARCH);
    DrawText("[UP/DOWN] Navegar  [ENTER] Jugar  [ESC] Volver",20,WIN_H-22,10,C_DGRAY);
    static const char*mus[5]={"Peter Pan - Main Title","Lord of the Rings Theme","All Star - Smash Mouth","Star Wars Main Theme","Euphoria - All of Us"};
    int oy=50;
    for(int i=0;i<NUM_MAPS;i++){
        bool sel=(i==gMapSel);
        Color acc=MAP_ACC[i];
        Color boxBg=sel?Color{(unsigned char)min(255,acc.r/3+20),(unsigned char)min(255,acc.g/3+20),(unsigned char)min(255,acc.b/3+20),230}:Color{15,12,8,200};
        DrawRectangle(14,oy,450,112,boxBg);
        DrawRectangleLinesEx({14,(float)oy,450,112},sel?2:1,sel?acc:Color{50,40,25,180});
        drawMapChar(i,(float)(14+50),(float)(oy+56),26.f);
        DrawText(TextFormat("[%d]",i+1),112,oy+8,10,sel?C_CUR:C_DGRAY);
        DrawText(MAP_NAME[i],112,oy+20,16,sel?C_CUR:acc);
        wrapDraw(MAP_SDESC[i],112,oy+42,320,10,sel?C_WHITE:C_GRAY);
        DrawText(mus[i],112,oy+86,9,{100,85,55,200});
        if(sel&&(int)(GetTime()*2)%2==0)DrawText(">>>",430,oy+44,16,C_CUR);
        oy+=116;}
    // Panel derecho: preview del mapa seleccionado
    DrawRectangle(480,0,WIN_W-480,WIN_H,{8,6,4,255});
    Color acc=MAP_ACC[gMapSel];
    // Titulo
    DrawText(MAP_NAME[gMapSel],490,18,22,acc);
    DrawLine(490,46,WIN_W-14,46,{acc.r,acc.g,acc.b,150});
    // Personaje grande (animado)
    drawMapChar(gMapSel,(float)(WIN_W-155),(float)(160),(float)(70));
    // Mini grafo del mapa
    drawMiniGraph(gMapSel,490.f,56.f,370.f,310.f,9.f);
    // Descripcion
    wrapDraw(MAP_SDESC[gMapSel],490,380,WIN_W-510,13,C_WHITE);
    // Hint de inicio
    DrawText(TextFormat("Inicio: nodo 0  |  Tesoro: nodo %d",MAP_TES[gMapSel]),490,420,11,C_GRAY);
    // Boton ENTER
    if((int)(GetTime()*2)%2==0){
        DrawRectangle(490,WIN_H-55,280,38,{(unsigned char)(acc.r/4),(unsigned char)(acc.g/4),(unsigned char)(acc.b/4),200});
        DrawRectangleLinesEx({490,(float)(WIN_H-55),280,38},2,acc);
        DrawText(TextFormat("ENTER / CLICK -> Jugar %s",MAP_NAME[gMapSel]),500,WIN_H-43,12,acc);}
}

// ============================================================
// DESCRIPCION
// ============================================================
void drawDesc(){
    DrawRectangle(0,0,WIN_W,WIN_H,C_BG);
    int bx=60,by=30,bw=WIN_W-120,bh=WIN_H-60;
    DrawRectangle(bx,by,bw,bh,{18,14,10,230});DrawRectangleLinesEx({(float)bx,(float)by,(float)bw,(float)bh},2,C_PARCH);
    int px=bx+30,pw=bw-60,py=by+20;
    DrawText("DESCRIPCION DEL JUEGO",px,py,20,C_PARCH);py+=32;DrawLine(px,py,px+pw,py,{80,65,40,200});py+=16;
    wrapDraw("El Tesoro del Pirata implementa estructuras de datos en un juego de aventura. Explora mundos de fantasia usando grafos, arboles, BFS, DFS y Dijkstra. Cada mundo tiene 12 ubicaciones (nodos) conectadas por caminos (aristas) con costos diferentes.",px,py,pw,12,C_GRAY);py+=60;
    DrawText("ESTRUCTURAS:",px,py,13,{210,180,70,255});py+=18;
    const char*s[]={"[1] Grafo        - Lista de adyacencia ponderada, 12 nodos por mapa",
        "[2] Arbol        - Arbol de decision interpreta keywords de las pistas",
        "[3] Cola / Pila  - BFS usa queue<int>, DFS usa stack<pair<int,int>>",
        "[4] Tabla Hash   - Almacena nombre, descripcion, pista y estado visitado",
        "[5] Lista        - Lista enlazada con copy/move para guardar rutas","[6] Archivos     - Lee mapa/pistas, guarda ruta en archivo de salida"};
    for(int i=0;i<6;i++){DrawText(s[i],px,py,11,C_GRAY);py+=15;}py+=10;
    DrawText("LOS 5 MUNDOS:",px,py,13,{210,180,70,255});py+=18;
    for(int i=0;i<NUM_MAPS;i++){DrawText(TextFormat("[%d] %s",i+1,MAP_NAME[i]),px,py,12,MAP_ACC[i]);py+=14;
        wrapDraw(MAP_SDESC[i],px+16,py,pw-16,11,C_GRAY);py+=18;}
    DrawText("[ESC] Volver",WIN_W/2-MeasureText("[ESC] Volver",13)/2,WIN_H-40,13,C_DGRAY);}

// ============================================================
// LOGICA
// ============================================================
void loadMap(Ctx&c,int mapId,Graph&g,Hash&h,DecTree&tree){
    g=Graph();h=Hash();int tes=-1;
    if(!loadData(g,h,tes,mapId))return;
    tree.build(MKWDS[mapId],MKDST[mapId],MKCNT[mapId],0);
    c.activeMap=mapId;c.tesoro=tes;c.g=&g;c.h=&h;c.tree=&tree;
    c.scr=SC_GAME;
    switchTrack(mapId+1);}

void startSolve(Ctx&c,SMode m){
    c.scr=SC_SOLVE;c.smode=m;c.step=0;c.atimer=0;c.done=false;
    c.order.clear();c.path.clear();c.cost=0;c.h->reset();
    Lista cam;const char*met="";
    if(m==BFS_M){cam=c.g->bfs(INICIO,c.tesoro,c.order);c.cost=c.g->pathW(cam);met="BFS";}
    else if(m==DFS_M){cam=c.g->dfs(INICIO,c.tesoro,c.order);c.cost=c.g->pathW(cam);met="DFS";}
    else{cam=c.g->dijkstra(INICIO,c.tesoro,&c.cost);c.order=cam.vec();met="Dijkstra";}
    c.path=cam.vec();saveRoute(cam,*c.h,c.cost,met,c.activeMap);}

void startPlay(Ctx&c){
    c.scr=SC_PLAY;c.player=INICIO;c.hint=-1;c.selAdj=0;
    c.hist.clear();c.hist.pushB(INICIO);c.h->reset();c.twLen=0;c.twTimer=0;
    UD*d=c.h->get(INICIO);if(d)d->vis=true;}

void moveTo(Ctx&c,int dest){
    if(!c.g->adjTo(c.player,dest))return;
    c.player=dest;c.selAdj=0;c.hist.pushB(dest);c.hint=-1;c.twLen=0;c.twTimer=0;
    UD*d=c.h->get(dest);if(d)d->vis=true;
    if(dest==c.tesoro){c.path=c.hist.vec();saveRoute(c.hist,*c.h,c.g->pathW(c.hist),"Exploración Manual",c.activeMap);c.scr=SC_WIN;}}

// ESC back navigation
void escBack(Ctx&c){
    if(c.scr==SC_MAPSEL){c.scr=SC_MAIN;switchTrack(0);}
    else if(c.scr==SC_DESC){c.scr=SC_MAIN;}
    else if(c.scr==SC_GAME){c.scr=SC_MAPSEL;switchTrack(0);}
    else if(c.scr==SC_PLAY||c.scr==SC_SOLVE||c.scr==SC_WIN){c.scr=SC_GAME;}}

// Confirmar opcion del menu principal
void confirmMain(Ctx&c,Graph&g,Hash&h,DecTree&tree){
    if(gMenuSel==0)c.scr=SC_MAPSEL;
    else if(gMenuSel==1)c.scr=SC_DESC;
    else if(gMenuSel==2)toggleMusic();
    else c.scr=SC_MAIN;} // sale en el bucle

// ============================================================
// MAIN
// ============================================================
int main(){
    Graph g;Hash h;DecTree tree;int tesoro=-1;
    SetConfigFlags(FLAG_WINDOW_MAXIMIZED);
    InitWindow(WIN_W,WIN_H,"El Tesoro del Pirata - Multi-Mapa");
    SetExitKey(KEY_NULL);  // desactivar el cierre automatico con ESC de Raylib
    InitAudioDevice();SetTargetFPS(60);
    loadSprites();

    // Cargar musica (6 tracks)
    for(int i=0;i<6;i++){gTracks[i]=LoadMusicStream(MUSIC_FILES[i]);gTracks[i].looping=true;}
    PlayMusicStream(gTracks[0]);SetMusicVolume(gTracks[0],0.7f);

    // Pre-cargar los 5 grafos para preview
    for(int i=0;i<5;i++){Hash tmp;int tt;loadData(gPreviewG[i],tmp,tt,i);}

    // RenderTexture para escalar
    RenderTexture2D rt=LoadRenderTexture(WIN_W,WIN_H);

    Ctx ctx;ctx.g=&g;ctx.h=&h;ctx.tree=&tree;
    bool running=true;

    while(!WindowShouldClose()&&running){
        float dt=GetFrameTime();
        UpdateMusicStream(gTracks[gCurTrack]);
        ctx.blinkT+=dt;if(ctx.blinkT>0.5f){ctx.blinkT=0;ctx.blinkOn=!ctx.blinkOn;}
        gSprT+=dt;if(gSprT>=SPR_FPS){gSprT=0.f;gSprF++;}
        // Typewriter
        if(ctx.scr==SC_PLAY){UD*d=ctx.h->get(ctx.player);
            if(d&&ctx.twLen<(int)d->pista.size()){ctx.twTimer+=dt;if(ctx.twTimer>0.03f){ctx.twTimer=0;ctx.twLen++;}}}
        // Animacion algoritmo
        if(ctx.scr==SC_SOLVE&&!ctx.done){ctx.atimer+=dt;
            if(ctx.atimer>=ANIM_SPD){ctx.atimer=0;ctx.step++;if(ctx.step>=(int)ctx.order.size())ctx.done=true;}}

        // ESC hold para cerrar
        if(IsKeyDown(KEY_ESCAPE)){gEscHold+=dt;if(gEscHold>=5.f)running=false;}
        else{if(gEscHold>0.05f&&gEscHold<5.f)escBack(ctx);gEscHold=0;}
        if(IsKeyPressed(KEY_F11))ToggleFullscreen();

        // ---- INPUT ----
        switch(ctx.scr){
        case SC_MAIN:
            if(IsKeyPressed(KEY_UP))gMenuSel=(gMenuSel-1+4)%4;
            if(IsKeyPressed(KEY_DOWN))gMenuSel=(gMenuSel+1)%4;
            if(IsKeyPressed(KEY_ENTER)){
                if(gMenuSel==0)ctx.scr=SC_MAPSEL;
                else if(gMenuSel==1)ctx.scr=SC_DESC;
                else if(gMenuSel==2)toggleMusic();
                else running=false;}
            // Click en opciones
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                Vector2 m=mouseDes();int bx=WIN_W/2-260,oy=WIN_H/2-250+104;
                for(int i=0;i<4;i++){if(m.x>bx+20&&m.x<bx+500&&m.y>oy-6&&m.y<oy+32){gMenuSel=i;
                    if(i==0)ctx.scr=SC_MAPSEL;else if(i==1)ctx.scr=SC_DESC;
                    else if(i==2)toggleMusic();else running=false;break;}oy+=50;}}
            break;
        case SC_MAPSEL:
            if(IsKeyPressed(KEY_UP))gMapSel=(gMapSel-1+NUM_MAPS)%NUM_MAPS;
            if(IsKeyPressed(KEY_DOWN))gMapSel=(gMapSel+1)%NUM_MAPS;
            if(IsKeyPressed(KEY_ENTER))loadMap(ctx,gMapSel,g,h,tree);
            // Click en opciones del panel izquierdo
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                Vector2 m=mouseDes();
                if(m.x<480){int oy=50;for(int i=0;i<NUM_MAPS;i++){if(m.y>oy&&m.y<oy+112){gMapSel=i;break;}oy+=116;}}
                if(m.x>480&&m.y>WIN_H-70)loadMap(ctx,gMapSel,g,h,tree);}
            break;
        case SC_DESC:break;
        case SC_GAME:
            if(IsKeyPressed(KEY_ONE))startPlay(ctx);
            if(IsKeyPressed(KEY_TWO))startSolve(ctx,BFS_M);
            if(IsKeyPressed(KEY_THREE))startSolve(ctx,DFS_M);
            if(IsKeyPressed(KEY_FOUR))startSolve(ctx,DIJ_M);
            // Click en panel izquierdo
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                Vector2 m=mouseDes();int y=56;
                for(int i=0;i<4;i++){if(m.x<LEFT_W&&m.y>y-3&&m.y<y+17){
                    if(i==0)startPlay(ctx);else if(i==1)startSolve(ctx,BFS_M);
                    else if(i==2)startSolve(ctx,DFS_M);else startSolve(ctx,DIJ_M);break;}y+=28;}}
            break;
        case SC_PLAY:{
            int adjSz=(int)ctx.g->adj[ctx.player].size();
            if(IsKeyPressed(KEY_ONE))startPlay(ctx);
            if(IsKeyPressed(KEY_TWO))startSolve(ctx,BFS_M);
            if(IsKeyPressed(KEY_THREE))startSolve(ctx,DFS_M);
            if(IsKeyPressed(KEY_FOUR))startSolve(ctx,DIJ_M);
            if(adjSz>0){
                if(IsKeyPressed(KEY_UP)||IsKeyPressed(KEY_W))ctx.selAdj=(ctx.selAdj-1+adjSz)%adjSz;
                if(IsKeyPressed(KEY_DOWN)||IsKeyPressed(KEY_S))ctx.selAdj=(ctx.selAdj+1)%adjSz;
                if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_Z)||IsKeyPressed(KEY_RIGHT))
                    moveTo(ctx,ctx.g->adj[ctx.player][ctx.selAdj].to);}
            if(IsKeyPressed(KEY_H)){UD*d=ctx.h->get(ctx.player);ctx.hint=d?ctx.tree->eval(d->pista):-1;}
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                Vector2 m=mouseDes();
                for(int i=0;i<g.n;i++){Vector2 np=mposToScreen(ctx.activeMap,i);
                    if(CheckCollisionPointCircle(m,np,(float)NODE_R)&&ctx.g->adjTo(ctx.player,i)){moveTo(ctx,i);break;}}}
            break;}
        case SC_SOLVE:
            if(IsKeyPressed(KEY_SPACE)){ctx.step=(int)ctx.order.size();ctx.done=true;}
            if(IsKeyPressed(KEY_ENTER)&&ctx.done)ctx.scr=SC_GAME;
            if(IsKeyPressed(KEY_ONE))startPlay(ctx);
            if(IsKeyPressed(KEY_TWO))startSolve(ctx,BFS_M);
            if(IsKeyPressed(KEY_THREE))startSolve(ctx,DFS_M);
            if(IsKeyPressed(KEY_FOUR))startSolve(ctx,DIJ_M);
            break;
        case SC_WIN:
            if(IsKeyPressed(KEY_ENTER))ctx.scr=SC_GAME;break;
        }

        // ---- DIBUJO ----
        BeginTextureMode(rt);
        ClearBackground(C_BG);
        if(ctx.scr==SC_MAIN)drawMainMenu();
        else if(ctx.scr==SC_MAPSEL)drawMapSelect();
        else if(ctx.scr==SC_DESC)drawDesc();
        else{drawLeftPanel(ctx);drawMapArea(ctx);drawRightPanel(ctx);}
        // Overlay ESC
        drawEscOverlay(gEscHold);
        DrawText(TextFormat("%dfps",GetFPS()),WIN_W-40,4,10,C_DGRAY);
        EndTextureMode();

        BeginDrawing();ClearBackground(BLACK);
        DrawTexturePro(rt.texture,{0,0,(float)WIN_W,-(float)WIN_H},
            {0,0,(float)GetScreenWidth(),(float)GetScreenHeight()},{0,0},0.f,WHITE);
        EndDrawing();
    }

    unloadSprites();
    for(int i=0;i<6;i++)UnloadMusicStream(gTracks[i]);
    UnloadRenderTexture(rt);CloseAudioDevice();CloseWindow();
    return 0;
}
