# Diseño 1: Aplicación Base (NetPBM Processor)

## 1. Descripción General
El **Diseño 1** implementa la arquitectura base orientada a objetos en C++ para la lectura, representación en memoria y escritura de imágenes en formatos NetPBM:
- **PGM (P2)**: Imágenes en escala de grises (1 canal por píxel).
- **PPM (P3)**: Imágenes a color RGB (3 canales por píxel).

El sistema opera de forma automática y no interactiva, soportando tanto el paso de rutas por línea de comandos como la redirección de entrada/salida estándar (`stdin` y `stdout`).

---

## 2. Decisiones de Diseño y Arquitectura

### 2.1 Paradigma Orientado a Objetos (POO)
Se diseñó la clase modular `Image` (declarada en `headers/Image.h` e implementada en `src/Image.cpp`), la cual encapsula:
- **Atributos**:
  - `magic`: Cadena C (`char[4]`) para el número mágico (`"P2"` o `"P3"`).
  - `width` y `height`: Dimensiones en píxeles.
  - `max_color`: Rango dinámico máximo de color/intensidad (generalmente 255).
  - `channels`: Número de componentes por píxel (1 para PGM, 3 para PPM).
  - `total_values`: Número total de enteros en el arreglo (`width * height * channels`).
  - `pixels`: Puntero al arreglo dinámico continuo de enteros (`int*`).
- **Métodos**:
  - **Constructores y Destructor**: Implementación de la *Regla de los Tres* (Constructor por defecto, Constructor con parámetros, Constructor de copia, Operador de asignación `=` y Destructor) para garantizar gestión de memoria libre de fugas (*memory leaks*).
  - **Accesores y Mutadores**: Métodos `getPixel(x, y)` / `setPixel(x, y, val)` para 1 canal y `getPixel(x, y, c)` / `setPixel(x, y, c, val)` para 3 canales con validación de límites.
  - **Operaciones de E/S**: `load`, `loadFromFile`, `save` y `saveToFile`.

### 2.2 Cumplimiento de Restricciones del Curso
- **Sin `std::vector`**: Toda la memoria de píxeles se gestiona con arreglos dinámicos contiguos de enteros (`new int[...]` y `delete[]`).
- **Sin `std::string`**: Se emplean únicamente arreglos de caracteres y punteros a carácter (`char*`, `const char*`) junto a funciones estándar seguras de C (`std::strncmp`, `std::strncpy`, `std::strlen`).

### 2.3 Manejo Robusto del Formato NetPBM y Comentarios
La especificación NetPBM permite comentarios que inician con `#` y se extienden hasta el final de la línea en cualquier lugar del encabezado. Para procesarlos de manera eficiente y confiable:
- Se implementó `skipWhitespaceAndComments(FILE* f)`, que descarta espacios, saltos de línea y líneas completas de comentarios iniciadas con `#`.
- Se implementó `readInt(FILE* f, int* value)` y `readToken(FILE* f, char* token, int maxLen)`, acelerando drásticamente el parseo y evitando fallos al encontrar comentarios o formatos con espaciados variables.
- Se configuró buffering de 64 KB (`setvbuf`) para minimizar las llamadas al sistema en operaciones de lectura y escritura de imágenes grandes (como `fruit.ppm`, `puj.ppm` y `damma.ppm`).

---

## 3. Estructura de Archivos
```
netpbm_filters/
├── headers/
│   └── Image.h         # Definición de la clase Image
├── src/
│   ├── Image.cpp       # Implementación de la clase Image
│   ├── processor.cpp   # Programa principal de la aplicación base
│   └── processor.md    # Resumen de compilación y ejecución de processor
├── docs/
│   └── diseno1.md      # Documentación detallada del Diseño 1
└── images/             # Imágenes de prueba (.pgm y .ppm)
```

---

## 4. Compilación

Para compilar el programa con optimización y advertencias habilitadas:

```bash
g++ -Wall -Wextra -O2 -Iheaders -o processor src/processor.cpp src/Image.cpp
```

En sistemas Windows (MinGW):
```powershell
g++ -Wall -Wextra -O2 -Iheaders -o processor.exe src/processor.cpp src/Image.cpp
```

---

## 5. Modos de Ejecución

### 5.1 Modo estándar con argumentos de archivo
```bash
./processor images/lena.pgm lena_copia.pgm
./processor images/lena.ppm lena_copia.ppm
```

### 5.2 Modo con redirección de entrada estándar (stdin / stdout)
```bash
./processor < images/lena.ppm > lena_copia.ppm
```

### 5.3 Modo mixto (stdin hacia archivo)
```bash
./processor - lena_copia.ppm < images/lena.ppm
```

---

## 6. Pruebas y Validación Realizadas
1. **Lectura y Escritura PGM (P2)**: Probado con `images/feep.pgm` (24x7) y `images/lena.pgm` (512x512). Se verificó la integridad de cabecera y datos de píxeles.
2. **Lectura y Escritura PPM (P3)**: Probado con `images/lena.ppm` (128x128) y `images/fruit.ppm` (900x450, 1.21 millones de valores).
3. **Redirección stdin/stdout**: Comprobada mediante tuberías y redirección `<` y `>`.
4. **Verificación de memoria**: Comportamiento de copia profunda probado mediante constructor de copia y operador de asignación.
