# Proyecto Final — Estructuras de Datos Avanzada

Dos juegos en C++ con gráficos 2D usando **Raylib**.

---

## Requisitos

- **g++** (MinGW-w64 recomendado) — compilador C++11 o superior
- **Raylib** instalado y en el PATH del sistema (cabeceras + `.a`)
  - Descarga: https://www.raylib.com/  
  - Instalación rápida con winget: `winget install raylib`
  - O descargar el installer `raylib-X.X.X_win64_mingw-w64.exe` desde GitHub releases
- Windows 10/11 de 64 bits

---

## Proyecto 1 — Búsqueda del Tesoro

Juego de exploración 2D con 5 mundos (Neverland, Tierra Media, Shrek, Star Wars, Euphoria).  
Usa: Grafo, Árbol de decisiones, BFS/DFS/Dijkstra, Tabla Hash, Lista Enlazada, Archivos.

### Compilar y correr

```bash
cd "Busqueda tesoro"
g++ -std=c++11 -o tesoro_2d main_2d.cpp -lraylib -lopengl32 -lgdi32 -lwinmm
./tesoro_2d
```

### Controles

| Tecla | Acción |
|---|---|
| ←→ / Mouse | Navegar entre mundos |
| Enter / Click | Seleccionar |
| W A S D | Mover personaje en el mapa |
| 1 | Exploración manual |
| 2 | BFS automático |
| 3 | DFS automático |
| 4 | Dijkstra (camino más corto) |
| ESC (mantener 5s) | Cerrar el juego |

### Archivos generados

Cada vez que llegás al tesoro se genera un archivo de ruta en `Mapas/<Mundo>/ruta_<mundo>_N.txt`.

---

## Proyecto 2 — El Diccionario del Habla Popular Camba

Juego estilo Bookworm con palabras del habla popular de Santa Cruz, Bolivia.  
Usa: Tabla Hash (28 posiciones), Lista Enlazada, BST, Archivos.

### Compilar y correr

```bash
cd "El diccionario del habla popular"
g++ -std=c++11 -o juego juego.cpp -lraylib -lopengl32 -lgdi32 -lwinmm
./juego
```

Para la versión consola (comparación Lista vs BST):

```bash
g++ -std=c++11 -o diccionario main.cpp
./diccionario
```

### Controles — Juego gráfico

| Tecla | Acción |
|---|---|
| Click en letras | Seleccionar letra para formar palabra |
| Enter | Enviar palabra |
| BACKSPACE | Borrar última letra |
| DEL | Limpiar selección completa |
| H | Pista (muestra una palabra válida) |
| ESC | Volver al menú |
| ESC (mantener 5s) | Cerrar el juego |
| Scroll | Desplazar historial de palabras |

### Archivos generados

Al ganar se genera `resultado_diccionario_N.txt` con estadísticas de la tabla hash y palabras encontradas.

---

## Estructura de carpetas

```
ProFinalEd/
├── Busqueda tesoro/
│   ├── main_2d.cpp          ← código fuente 2D
│   ├── main.cpp             ← versión consola (referencia)
│   ├── sprites/             ← imágenes de personajes
│   ├── Mapas/               ← mapas y rutas generadas por mundo
│   └── *.mp3                ← música por mundo
├── El diccionario del habla popular/
│   ├── juego.cpp            ← código fuente gráfico
│   ├── main.cpp             ← versión consola con comparación Lista/BST
│   ├── diccionario.txt      ← base de datos de palabras cambas
│   ├── sprites/             ← imágenes de personajes
│   └── *.mp3                ← música de menú y juego
└── README.md
```

---

## Notas

- Los archivos `.exe` no están en el repositorio (ignorados por `.gitignore`).
- Los mapas de texto se auto-generan la primera vez que entrás a cada mundo.
- Las palabras nuevas que agregues desde el juego se guardan en `diccionario.txt` bajo la sección `# PALABRAS NUEVAS`.
