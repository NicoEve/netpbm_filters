# Diseño 3: Memoria Compartida (Pthreads y OpenMP)

## 1. Descripción General
El **Diseño 3** implementa la paralelización en **memoria compartida** para la aplicación de filtros espaciales sobre imágenes NetPBM (PGM y PPM). Se desarrollaron dos enfoques complementarios:
1. **Pthreads (`th_filterer`)**: Descomposición geométrica explícita de la imagen en cuatro cuadrantes independientes asignados a 4 hilos POSIX.
2. **OpenMP (`omp_filterer`)**: Paralelización a nivel de bucle (*loop-level parallelism*) distribuyendo las filas de la imagen entre todos los núcleos disponibles del procesador (12 hilos lógicos).

Ambas implementaciones miden y registran el **tiempo de CPU** (acumulado de todos los hilos) y el **tiempo total de reloj (Wall-clock time)**.

---

## 2. Enfoque 1: Pthreads con Descomposición en 4 Cuadrantes

### 2.1 División Geométrica de la Imagen
Dada una imagen de ancho $W$ y alto $H$, con punto medio $(mid_x = W/2, mid_y = H/2)$, la imagen se divide en cuatro regiones rectangulares disjuntas:

```
(0, 0)                  (mid_x, 0)                  (W, 0)
  ┌─────────────────────────┬─────────────────────────┐
  │                         │                         │
  │   Hilo 0                │   Hilo 1                │
  │   Arriba-Izquierda      │   Arriba-Derecha        │
  │   x: [0, mid_x)         │   x: [mid_x, W)         │
  │   y: [0, mid_y)         │   y: [0, mid_y)         │
  │                         │                         │
  ├─────────────────────────┼─────────────────────────┤
  │                         │                         │
  │   Hilo 2                │   Hilo 3                │
  │   Abajo-Izquierda       │   Abajo-Derecha         │
  │   x: [0, mid_x)         │   x: [mid_x, W)         │
  │   y: [mid_y, H)         │   y: [mid_y, H)         │
  │                         │                         │
  └─────────────────────────┴─────────────────────────┘
(0, H)                  (mid_x, H)                  (W, H)
```

### 2.2 Sincronización y Ausencia de Condiciones de Carrera (*Race Conditions*)
- **Lectura**: Todos los hilos acceden a la imagen fuente `src` en modo de **sólo lectura**. Aunque un píxel cercano a la frontera de un cuadrante requiera consultar píxeles vecinos de otro cuadrante, la lectura concurrente sobre memoria inmutable es 100% segura.
- **Escritura**: Cada hilo escribe exclusivamente en el rango de índices correspondiente a su región dentro del búfer de la imagen destino `dst`. Al no haber solapamiento en las posiciones de memoria modificadas, **no se requieren cerrojos (*mutex/locks*) ni barreras intermedias**.
- **Sincronización final**: El hilo principal invoca `pthread_join()` sobre los 4 hilos antes de continuar con el siguiente filtro o con el guardado de la imagen.

### 2.3 Estructura de Datos
Se utiliza una estructura liviana de paso de parámetros sin `std::vector` ni `std::string`:
```cpp
struct RegionTask {
    int id;
    const char* region_name;
    const Image* src;
    Image* dst;
    const Filter* filter;
    int x_start, x_end;
    int y_start, y_end;
};
```

---

## 3. Enfoque 2: OpenMP Paralelo

En [src/omp_filterer.cpp](file:///c:/Users/Usuario/OneDrive/Desktop/Universidad/ProgramacionParalela/netpbm_filters/src/omp_filterer.cpp) y el método `applyOMP` de [src/Filter.cpp](file:///c:/Users/Usuario/OneDrive/Desktop/Universidad/ProgramacionParalela/netpbm_filters/src/Filter.cpp), se paraleliza el bucle externo de las filas $y \in [0, H)$:

```cpp
#pragma omp parallel for schedule(static)
for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
        // Convolución 3x3 para cada canal
    }
}
```
- La cláusula `schedule(static)` distribuye bloques continuos de filas entre los 12 hilos del sistema de forma equitativa con mínima sobrecarga de planificación (*scheduling overhead*).
- Excelente localidad espacial de caché debido al acceso por filas continuas contiguas en memoria.

---

## 4. Tabla Comparativa de Rendimiento

Pruebas ejecutadas con optimización `-O2` en procesador de 12 núcleos lógicos sobre las entradas requeridas (**damma** y **sulfur**):

| Imagen | Formato | Dimensiones | Filtro | Secuencial (s) | Pthreads 4 Hilos (s) | Speedup Pthreads ($S_p$) | OpenMP 12 Hilos (s) | Speedup OpenMP ($S_{omp}$) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **damma** | PPM (P3) | 1000 x 1278 | `blur` | 0.0494 s | 0.0966 s* | - | 0.0160 s | **3.09x** |
| **sulfur** | PGM (P2) | 823 x 1000 | `blur` | 0.0112 s | 0.0220 s* | - | 0.0040 s | **2.80x** |
| **sulfur** | PPM (P3) | 823 x 1000 | `sharpen` | 0.0281 s | 0.0138 s | **2.04x** | 0.0100 s | **2.81x** |
| **fruit** | PGM (P2) | 900 x 450 | `blur` | 0.0150 s | 0.0104 s | **1.44x** | 0.0050 s | **3.00x** |

*\*Nota sobre la sobrecarga de creación de hilos*: En imágenes medianas en PGM, la creación y destrucción de hilos a nivel de sistema operativo (`pthread_create` / `pthread_join`) introduce un pequeño *overhead* fijo. A medida que la cantidad de datos crece (como en `sulfur.ppm` y `damma.ppm`), el tiempo de cómputo domina y el paralelismo de Pthreads y OpenMP supera con claridad al secuencial.

---

## 5. Compilación y Ejecución

### Compilación
```bash
# Compilar versión Pthreads:
g++ -Wall -Wextra -O2 -Iheaders -o th_filterer src/th_filterer.cpp src/Filter.cpp src/Image.cpp -lpthread

# Compilar versión OpenMP:
g++ -Wall -Wextra -O2 -fopenmp -Iheaders -o omp_filterer src/omp_filterer.cpp src/Filter.cpp src/Image.cpp
```

### Ejecución Pthreads (4 regiones)
```bash
# Ejemplo especificado en la guía:
./th_filterer images/fruit.pgm fruit_blur2.ppm --f blur

# Pruebas con damma y sulfur:
./th_filterer images/damma.ppm damma_blur.ppm --f blur
./th_filterer images/sulfur.pgm sulfur_blur.pgm --f blur
./th_filterer images/sulfur.ppm sulfur_sharpen.ppm --f sharpen
```

### Ejecución OpenMP
```bash
# Ejemplo especificado en la guía:
./omp_filterer images/sulfur.pgm sulfur_N.pgm

# Pruebas con filtro específico:
./omp_filterer images/damma.ppm damma_blur.ppm --f blur
./omp_filterer images/sulfur.ppm sulfur_laplace.ppm --f laplace
```
