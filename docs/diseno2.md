# Diseño 2: Versión Secuencial (Filtros de Procesamiento de Imágenes)

## 1. Descripción General
El **Diseño 2** implementa el procesamiento y aplicación secuencial de filtros espaciales basados en convolución 3x3 sobre imágenes en formato NetPBM:
- **PGM (P2)**: 1 canal (escala de grises).
- **PPM (P3)**: 3 canales (color RGB).

Permite aplicar uno o múltiples filtros encadenados mediante la bandera `--f <filtro>`, midiendo de forma precisa el **tiempo de CPU** y el **tiempo total de ejecución (Wall-clock)** del procesamiento para establecer la línea base (*baseline*) de rendimiento antes de la paralelización.

---

## 2. Filtros Implementados y Fundamento Matemático

Cada filtro opera calculando la convolución discreta 2D de un kernel $K$ de $3 \times 3$ sobre la vecindad de cada píxel $(x, y)$ para cada canal $c$:

$$P'_{c}(x, y) = \sum_{ky=-1}^{1} \sum_{kx=-1}^{1} P_c(x+kx, y+ky) \cdot K[ky+1][kx+1]$$

### 2.1 Suavizado (Blur)
Aplica un filtro de promedio (*box blur*) con pesos uniformes de $1/9$:
$$K_{\text{blur}} = \frac{1}{9} \begin{pmatrix} 1 & 1 & 1 \\ 1 & 1 & 1 \\ 1 & 1 & 1 \end{pmatrix}$$
- **Tratamiento de bordes**: Para evitar oscurecer los bordes de la imagen, se normaliza dividiendo únicamente entre la suma de pesos de los píxeles válidos dentro de los límites de la imagen.

### 2.2 Laplace (Detección de Bordes)
Aplica el operador Laplaciano discreto de segundas derivadas para resaltar cambios abruptos de intensidad:
$$K_{\text{laplace}} = \begin{pmatrix} 0 & 1 & 0 \\ 1 & -4 & 1 \\ 0 & 1 & 0 \end{pmatrix}$$
- **Tratamiento de bordes y rango**: Se replican los píxeles en los bordes y se toma el valor absoluto $|S|$ del resultado de la convolución para resaltar transiciones tanto de claro a oscuro como de oscuro a claro, acotando el valor al rango $[0, \text{max\_color}]$.

### 2.3 Realce (Sharpening)
Aumenta la nitidez combinando la imagen original con la resta de su versión difuminada (máscara de enfoque):
$$K_{\text{sharpen}} = \begin{pmatrix} 0 & -1 & 0 \\ -1 & 5 & -1 \\ 0 & -1 & 0 \end{pmatrix}$$
- **Propiedad**: La suma total de los coeficientes es $1$ ($5 - 1 - 1 - 1 - 1 = 1$), lo que preserva el brillo medio global de la imagen mientras realza los bordes y texturas.

---

## 3. Arquitectura Orientada a Objetos y Restricciones

### 3.1 Jerarquía Polimórfica de Filtros
- **Clase base abstracta `Filter`** ([headers/Filter.h](file:///c:/Users/Usuario/OneDrive/Desktop/Universidad/ProgramacionParalela/netpbm_filters/headers/Filter.h)):
  Define la interfaz con métodos virtuales puros `getName()` y `apply(const Image& src, Image& dst)`.
- **Clases concretas**:
  - `BlurFilter`
  - `LaplaceFilter`
  - `SharpenFilter`
- **Factoría**: `createFilterByName(const char* name)` desacopla la instanciación de los nombres recibidos por la línea de comandos.

### 3.2 Cumplimiento de Restricciones del Curso
- **Sin `std::vector`**: Los filtros activos se gestionan en un arreglo estático de punteros `Filter* filters[MAX_FILTERS]`, y los datos de imagen usan arreglos dinámicos contiguos `int*`.
- **Sin `std::string`**: Se emplean arreglos de caracteres `char[]`, `const char*` y funciones C seguras (`strncpy`, `strcmp`, `strtok`).
- **Gestión de memoria**: Cada filtro instanciado dinámicamente es liberado explícitamente al culminar la ejecución, asegurando cero fugas de memoria.

---

## 4. Medición de Tiempos de Ejecución

Se miden dos métricas complementarias para el bloque de filtrado:
1. **Tiempo de CPU (`std::clock`)**: Tiempo real que el procesador dedicó a ejecutar instrucciones de este proceso.
2. **Tiempo Total de Ejecución (`std::chrono::high_resolution_clock`)**: Tiempo de reloj de pared (*wall-clock time*) transcurrido desde el inicio hasta el fin del procesamiento del filtro.

---

## 5. Tabla de Rendimiento Secuencial (Baseline)

Pruebas ejecutadas con optimización `-O2` en procesador local:

| Imagen | Formato | Dimensiones | Total Píxeles | Filtro | Tiempo CPU (s) | Tiempo Total (s) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **lena** | PGM (P2) | 512 x 512 | 262,144 | `blur` | 0.0030 s | 0.0036 s |
| **lena** | PGM (P2) | 512 x 512 | 262,144 | `laplace` | 0.0020 s | 0.0028 s |
| **lena** | PGM (P2) | 512 x 512 | 262,144 | `sharpen` | 0.0030 s | 0.0026 s |
| **fruit** | PPM (P3) | 900 x 450 | 1,215,000 | `blur` | 0.0150 s | 0.0149 s |
| **fruit** | PPM (P3) | 900 x 450 | 1,215,000 | `laplace` | 0.0130 s | 0.0133 s |
| **fruit** | PPM (P3) | 900 x 450 | 1,215,000 | `sharpen` | 0.0140 s | 0.0143 s |
| **puj** | PPM (P3) | 1920 x 600 | 3,456,000 | `blur` | 0.0420 s | 0.0415 s |
| **puj** | PPM (P3) | 1920 x 600 | 3,456,000 | `laplace` | 0.0390 s | 0.0393 s |
| **puj** | PPM (P3) | 1920 x 600 | 3,456,000 | `sharpen` | 0.0400 s | 0.0399 s |

---

## 6. Compilación y Ejecución

### Compilación
```bash
g++ -Wall -Wextra -O2 -Iheaders -o filterer src/filterer.cpp src/Filter.cpp src/Image.cpp
```

### Ejecución
```bash
# Filtro blur a fruit (como indica el enunciado):
./filterer images/fruit.ppm fruit_blur.ppm --f blur

# Filtro laplace a lena:
./filterer images/lena.pgm lena_laplace.pgm --f laplace

# Filtro sharpen a puj:
./filterer images/puj.ppm puj_sharpen.ppm --f sharpen

# Múltiples filtros encadenados:
./filterer images/fruit.ppm out.ppm --f blur --f sharpen
```
