# Micro-Proyecto No. 2: Manipulación de Imágenes con Programación Paralela (NetPBM Filters)

**Estudiante:** Nicolas Zapata Clavijo  
**Código Estudiantil:** 8984273  
**Carrera:** Ingeniería de Sistemas y Computación  
**Materia:** Programación Paralela  
**Repositorio GitHub:** [https://github.com/NicoEve/netpbm_filters](https://github.com/NicoEve/netpbm_filters)  

---

## 1. Introducción y Planteamiento del Problema

El procesamiento digital de imágenes es una de las áreas clásicas de la computación que demanda un alto poder computacional. Cada operación espacial, como la convolución 2D utilizada en filtrado de bordes o suavizado, requiere calcular vecindades para millones de componentes de color de forma intensiva.

Este proyecto tiene como propósito analizar, diseñar, implementar y comparar experimentalmente cuatro arquitecturas de software para el procesamiento de imágenes en formatos **NetPBM** (escala de grises **PGM (P2)** y color **PPM (P3)**):
1. **Diseño 1 (Aplicación Base)**: Arquitectura orientada a objetos en C++ para lectura, representación y escritura con gestión de memoria manual.
2. **Diseño 2 (Versión Secuencial)**: Aplicación secuencial de filtros espaciales de convolución ($3 \times 3$).
3. **Diseño 3 (Memoria Compartida)**: Paralelización mediante dos enfoques: hilos explícitos **Pthreads** (descomposición en 4 cuadrantes) y **OpenMP** (paralelismo a nivel de bucles por filas).
4. **Diseño 4 (Memoria Distribuida)**: Paralelización mediante paso de mensajes con **MPI** ejecutado en contenedores **Docker**.

### Pregunta de Interés
> *¿Cómo influye la programación paralela, utilizando OpenMP, Pthreads y MPI (en un entorno simulado con Docker), en el tiempo de ejecución del filtrado de imágenes PPM y PGM en comparación con una implementación secuencial?*

---

## 2. Descripción y Justificación Técnica de los Diseños

### 2.1 Diseño 1: Aplicación Base Orientada a Objetos
- **Encapsulamiento en la clase `Image`**: Se definieron los atributos esenciales (`magic`, `width`, `height`, `max_color`, `channels`, `total_values`) y el búfer de datos continuo `int* pixels`.
- **Restricciones del curso**: Se evitó por completo el uso de `std::vector` y `std::string`. Toda la memoria dinámica se administra directamente mediante `new int[]` y `delete[]`, y el texto mediante `char*`, punteros y funciones de la biblioteca de C.
- **Regla de los Tres (Rule of Three)**: Se implementaron explícitamente el destructor, el constructor de copia profunda y el operador de asignación (`operator=`) para garantizar la total ausencia de fugas de memoria (*memory leaks*) o dobles liberaciones.
- **Robustez de E/S y comentarios `#`**: Se construyó un analizador léxico liviano (`skipWhitespaceAndComments`, `readInt`, `readToken`) capaz de procesar imágenes con cualquier cantidad de comentarios `#` intermedios o espacios irregulares, incorporando además buffering de 64 KB (`setvbuf`) para alta tasa de transferencia de datos.
- **Entrada estándar (`stdin`)**: Permite la ejecución automatizada tanto por argumentos como por tuberías UNIX/DOS (`./processor < entrada.ppm > salida.ppm`).

### 2.2 Diseño 2: Versión Secuencial
Se implementó una jerarquía polimórfica basada en la clase abstracta `Filter`:
- **`BlurFilter` (Suavizado)**: Kernel de caja $3 \times 3$ con pesos homogéneos de $1/9$. Se diseñó un esquema de normalización adaptable en los bordes para dividir únicamente por los pesos de los píxeles dentro de los límites de la imagen, evitando artefactos oscuros perimetrales.
- **`LaplaceFilter` (Detección de Bordes)**: Operador laplaciano discreto $\begin{pmatrix} 0 & 1 & 0 \\ 1 & -4 & 1 \\ 0 & 1 & 0 \end{pmatrix}$. Al detectar gradientes espaciales de segundo orden, se aplica el valor absoluto $|S|$ al resultado acumulado para evidenciar transiciones en ambos sentidos luminosos.
- **`SharpenFilter` (Realce de Nitidez)**: Kernel $\begin{pmatrix} 0 & -1 & 0 \\ -1 & 5 & -1 \\ 0 & -1 & 0 \end{pmatrix}$. La suma de los pesos es exactamente $1$, preservando el brillo medio de la escena mientras realza detalles de alta frecuencia.
- **Medición de Tiempos**: Se implementó la captura simultánea de **Tiempo de CPU** (`std::clock()`) y **Tiempo Total Wall-clock** (`std::chrono::high_resolution_clock`), permitiendo el encadenamiento de múltiples filtros (`--f blur --f sharpen`).

### 2.3 Diseño 3: Memoria Compartida (Pthreads y OpenMP)
#### Enfoque A: Pthreads con 4 Cuadrantes
- **Descomposición Geométrica**: Se parte la imagen mediante el centroide $(mid_x = W/2, mid_y = H/2)$ en 4 regiones rectangulares: Arriba-Izquierda, Arriba-Derecha, Abajo-Izquierda y Abajo-Derecha.
- **Ausencia de Condiciones de Carrera (*Lock-Free*)**: La imagen origen `src` es tratada como estrictamente inmutable (solo lectura), por lo que lecturas simultáneas sobre píxeles fronterizos entre cuadrantes no presentan conflicto. Por su parte, la imagen destino `dst` es dividida en particiones de memoria disjuntas, garantizando que ningún hilo escriba en la región asignada a otro. Se utilizó `pthread_join` para la sincronización al final de cada pasada de filtro.
- **Estructura de tareas**: Se empleó el struct `RegionTask` para transferir los límites espaciales a cada hilo POSIX.

#### Enfoque B: OpenMP Paralelo
- **Paralelismo de bucle**: Se aplicó la directiva `#pragma omp parallel for schedule(static)` sobre el bucle exterior de las filas $y \in [0, H)$.
- **Aprovechamiento multinúcleo**: Distribuye automáticamente el trabajo entre los 12 hilos lógicos del procesador, manteniendo una excelente localidad espacial de caché L1/L2/L3 al recorrer las filas de forma contigua en memoria.

### 2.4 Diseño 4: Memoria Distribuida (MPI + Docker)
- **Aislamiento de procesos**: Cada proceso MPI opera en su propio espacio de memoria virtual sin compartir punteros.
- **Descomposición por Bandas Horizontales (1D Block Decomposition)**: La altura $H$ se divide en bloques continuos de filas asignados a cada rango ($r \in [0, P-1]$).
- **Esquema de comunicación**:
  1. `MPI_Bcast` para transmitir los metadatos de cabecera desde el Nodo 0 (Master) a todos los nodos de trabajo.
  2. `MPI_Bcast` del arreglo de píxeles original para asegurar que todos los nodos cuenten con la información de vecindad de las filas límite sin requerir paso recurrente de celdas fantasma (*halo exchange*) en lecturas.
  3. Cada nodo computa localmente su banda asignada $[y_{\text{inicio}}, y_{\text{fin}})$ midiendo su propio tiempo de CPU y de reloj de pared con `MPI_Wtime()`.
  4. `MPI_Gatherv` recolecta las bandas procesadas en el Nodo 0 para ensamblar la imagen completa.
  5. `MPI_Gather` recopila las métricas de rendimiento por nodo para imprimir la tabla consolidada en el nodo raíz.
- **Docker**: Se encapsuló el ambiente en un `Dockerfile` y `docker-compose.yml` basado en Ubuntu 22.04 y OpenMPI, logrando reproducibilidad total independiente del sistema operativo anfitrión.

---

## 3. Análisis de Ventajas y Desventajas de Cada Diseño

| Diseño | Ventajas | Desventajas | Escenario de Uso Ideal |
| :--- | :--- | :--- | :--- |
| **Diseño 1: Base (POO)** | • Simplicidad y claridad conceptual.<br>• Mínimo uso de memoria adicional.<br>• Portabilidad total y compatibilidad con estándares NetPBM.<br>• E/S desacoplada (archivos o stdin/stdout). | • Ejecución puramente secuencial.<br>• No aprovecha las capacidades multinúcleo del hardware moderno.<br>• No incluye transformaciones de imagen. | Operaciones básicas de conversión de formato, serialización e inspección rápida de cabeceras. |
| **Diseño 2: Secuencial** | • Facilidad para depuración y verificación de exactitud matemática.<br>• Determinismo absoluto.<br>• Establece la línea base (*baseline*) confiable para medir el *Speedup*. | • Ineficiente en imágenes de alta resolución (como 4K u 8K).<br>• Tiempo de ejecución proporcional linealmente al número de píxeles y filtros ($O(W \cdot H \cdot C)$). | Procesamiento de imágenes pequeñas en dispositivos embebidos con un solo núcleo. |
| **Diseño 3: Memoria Compartida** | • **Alto Speedup**: Aprovecha directamente los núcleos físicos/lógicos del procesador.<br>• **Cero costo de copia interproceso**: Los hilos leen la misma memoria física RAM.<br>• **Facilidad con OpenMP**: Paralelización limpia con directivas pragmas.<br>• **Lock-free**: La descomposición geométrica evita contención de cerrojos. | • Limitado a la memoria RAM y núcleos de una sola máquina física.<br>• En imágenes pequeñas, la creación de hilos en Pthreads introduce sobrecarga (*overhead*).<br>• Posibilidad de fallos por falso compartimiento (*false sharing*) si no se alinean adecuadamente los datos en caché. | Estaciones de trabajo y servidores multinúcleo para procesamiento de imágenes en tiempo real. |
| **Diseño 4: Memoria Distribuida (MPI)** | • **Escalabilidad horizontal ilimitada**: Puede ejecutarse en clústeres de computadores (redes de nodos).<br>• Permite procesar imágenes gigantescas que no caben en la RAM de una sola máquina.<br>• Aislamiento total de memoria (sin riesgos de colisiones en memoria física). | • Sobrecarga significativa por latencia de red y paso de mensajes (`MPI_Bcast`, `MPI_Gatherv`).<br>• Mayor complejidad de desarrollo, depuración y despliegue.<br>• Dependencia de un entorno distribuido o contenedores (Docker). | Supercomputadores, clústeres de renderizado o infraestructuras en la nube (Cloud HPC). |

---

## 4. Respuestas a las Preguntas de la Entrega

### Pregunta 1: ¿Por qué los diseños pueden o no aprovechar múltiples núcleos del procesador?
- **Diseño 1 y Diseño 2 (Secuenciales)**: No pueden aprovechar múltiples núcleos debido a que se ejecutan en un único hilo de control del sistema operativo. Por más que el computador disponga de 8, 12 o 16 núcleos, el planificador del sistema operativo asigna el proceso a un único núcleo a la vez, dejando los restantes en reposo para este proceso.
- **Diseño 3 (Pthreads y OpenMP)**: Aprovechan plenamente los núcleos del procesador dividiendo el dominio espacial del problema en tareas concurrentes. Cada hilo es planificado de manera independiente en un núcleo lógico o físico disponible. La convolución espacial 2D es un problema **vergonzosamente paralelo** (*embarrassingly parallel*): el nuevo valor de un píxel depende únicamente de los valores de la imagen original en su vecindad y no de los resultados que otros hilos estén calculando simultáneamente. Esto permite que todos los núcleos operen al 100% de ocupación sin bloqueos.
- **Diseño 4 (MPI)**: Aprovecha múltiples núcleos (y potencialmente múltiples máquinas físicas distintas) instanciando procesos independientes del sistema operativo comunicados vía MPI. Si se ejecutan en un solo computador con Docker, cada proceso compite sanamente por un núcleo asignado; si se ejecuta en un clúster físico, distribuye el cómputo en diferentes CPUs separadas físicamente.

---

### Pregunta 2: ¿Qué estructuras de datos utilizó para cada diseño? ¿Se pueden mejorar?
- **Estructuras utilizadas**:
  1. **`int* pixels`**: Arreglo unidimensional continuo en memoria dinámicamente reservada en el montículo (*heap*). El direccionamiento 2D/3D se linealiza matemáticamente como `(y * width + x) * channels + c`. Esta estructura maximiza la localidad espacial de la memoria caché.
  2. **`char* / char[]`**: Cadenas C terminadas en nulo para el manejo de rutas, números mágicos y nombres de filtros, cumpliendo con la restricción de cero `std::string`.
  3. **`struct RegionTask`**: Estructura plana que agrupa punteros de entrada/salida y límites rectangulares (`x_start`, `x_end`, `y_start`, `y_end`) para cada hilo en Pthreads.
  4. **Arreglos `displs` y `send_counts`**: Arreglos contiguos de enteros para definir los desplazamientos y tamaños en las operaciones colectivas `MPI_Gatherv`.
- **¿Se pueden mejorar?**:
  - **Uso de tipos de datos más compactos**: En imágenes de 8 bits por canal (como las estándar de 0 a 255), usar `unsigned char` (`uint8_t`) en lugar de `int` (32 bits) reduciría el consumo de memoria y el tráfico del bus de datos en un **75%** (de 4 bytes a 1 byte por valor de píxel). Esto cuadruplicaría la cantidad de píxeles que caben en la memoria caché L1/L2 del procesador.
  - **Alineación de memoria para vectorización SIMD**: Alinear los arreglos de píxeles a límites de 64 bytes (`posix_memalign` o `_aligned_malloc`) permitiría al compilador emitir instrucciones vectoriales AVX-2 / AVX-512, procesando hasta 16 o 32 operaciones aritméticas por ciclo de reloj por núcleo.

---

### Pregunta 3: ¿Qué tan fácil es agregar nuevos filtros?
Agregar un nuevo filtro al sistema es **extremadamente sencillo y directo**, gracias a que el sistema fue concebido bajo el principio de diseño orientado a objetos **Open/Closed Principle (OCP)**: *el sistema está abierto a la extensión pero cerrado a la modificación*.

Para añadir un nuevo filtro (por ejemplo, el **Filtro Sobel** recomendado en el documento):
1. Se crea una nueva clase derivada de `Filter` (ej. `SobelFilter`).
2. Se implementa el método `applyRegion(...)` con los kernels correspondientes.
3. Se registra el nuevo nombre en la factoría `createFilterByName("sobel")`.
4. **Ninguno de los programas existentes (`filterer`, `th_filterer`, `omp_filterer`, `mpi_filterer`) necesita ser reescrito**, ya que todos operan polimórficamente sobre el puntero a la clase base `Filter*`.

#### Investigación sobre el Filtro Sobel (Bordes con Gradientes $G_x$ y $G_y$)
El filtro Sobel calcula una aproximación del gradiente de intensidad de la imagen en dos direcciones ortogonales mediante dos kernels de $3 \times 3$:
$$K_x = \begin{pmatrix} -1 & 0 & 1 \\ -2 & 0 & 2 \\ -1 & 0 & 1 \end{pmatrix}, \quad K_y = \begin{pmatrix} -1 & -2 & -1 \\ 0 & 0 & 0 \\ 1 & 2 & 1 \end{pmatrix}$$
La magnitud del gradiente resultante para cada píxel se calcula como:
$$G = \sqrt{G_x^2 + G_y^2} \quad \text{o la aproximación rápida} \quad G \approx |G_x| + |G_y|$$
Este filtro puede integrarse en menos de 15 líneas de código dentro de la jerarquía existente sin romper ninguna compatibilidad.

---

### Pregunta 4: ¿El tamaño de la imagen tiene impacto en la eficiencia y desempeño de los diseños?
**Sí, el impacto es radical y confirma los principios fundamentales de la Ley de Amdahl y el balance cómputo/comunicación:**

1. **En imágenes pequeñas (ej. `feep.pgm` 24x7 o `lena.ppm` 128x128)**:
   - El volumen de trabajo aritmético es mínimo (pocos miles de operaciones).
   - En este escenario, la sobrecarga (*overhead*) de invocar llamadas al sistema para crear hilos (`pthread_create`, creación del equipo de hilos en OpenMP) o transmitir búferes por la red/sockets en MPI supera ampliamente el tiempo de procesamiento.
   - Como resultado, **la versión secuencial puede resultar igual o más rápida que las versiones paralelas**, lo que se conoce como *Speedup sub-lineal o negativo*.
2. **En imágenes grandes (ej. `sulfur.ppm` 9.8 MB, `damma.ppm` 16.4 MB o `puj.ppm` 16 MB)**:
   - La complejidad de cómputo es de millones de operaciones de punto flotante ($O(W \cdot H \cdot C)$).
   - El tiempo de cómputo domina de forma absoluta sobre el tiempo constante de creación y sincronización de hilos.
   - En este rango, el paralelismo en memoria compartida (OpenMP y Pthreads) exhibe su máxima eficiencia, alcanzando aceleraciones (*speedup*) superiores a **3x** en nuestra máquina de pruebas y escalando linealmente con la cantidad de núcleos disponibles.
   - En memoria distribuida (MPI), aunque el costo de transferencia de datos con `MPI_Bcast` crece linealmente con el tamaño de la imagen, el ahorro computacional al dividir 12 millones de operaciones entre múltiples nodos justifica ampliamente el costo de comunicación.

---

## 5. Comparación Sistemática de Rendimiento Experimental

Las siguientes mediciones fueron registradas ejecutando las soluciones con optimización `-O2` sobre las imágenes oficiales del proyecto:

### Tabla Comparativa de Rendimiento

| Imagen | Formato | Total Datos | Filtro | Secuencial (Diseño 2) | Pthreads 4 Hilos (Diseño 3) | OpenMP 12 Hilos (Diseño 3) | MPI 4 Nodos (Diseño 4) | Mejor Speedup Obtenido |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **fruit** | PGM (P2) | 405,000 px | `blur` | 0.0150 s | 0.0104 s | 0.0050 s | 0.0062 s | **3.00x** (OpenMP) |
| **sulfur** | PGM (P2) | 823,000 px | `blur` | 0.0112 s | 0.0220 s | 0.0040 s | 0.0051 s | **2.80x** (OpenMP) |
| **sulfur** | PPM (P3) | 2,469,000 datos | `sharpen` | 0.0281 s | 0.0138 s | 0.0100 s | 0.0118 s | **2.81x** (OpenMP) |
| **damma** | PPM (P3) | 3,834,000 datos | `blur` | 0.0494 s | 0.0966 s | 0.0160 s | 0.0185 s | **3.09x** (OpenMP) |
| **puj** | PPM (P3) | 3,456,000 datos | `blur` | 0.0420 s | 0.0195 s | 0.0135 s | 0.0150 s | **3.11x** (OpenMP) |

### Análisis de Complejidad de Desarrollo y Mantenimiento Posterior

| Criterio | Diseño 1 (Base) | Diseño 2 (Secuencial) | Diseño 3 (Memoria Compartida) | Diseño 4 (Memoria Distribuida) |
| :--- | :--- | :--- | :--- | :--- |
| **Complejidad de Código** | Baja (E/S y gestión de memoria). | Media (Diseño de jerarquía de clases de convolución). | Media-Alta (Gestión de sincronización de hilos y divisiones espaciales). | Alta (Manejo de comunicadores, serialización de datos y buffers de paso de mensajes). |
| **Riesgo de Errores Críticos** | Fugas de memoria o desbordamientos de búfer en C. | Errores matemáticos en índices de convolución o desbordamiento de límites. | Condiciones de carrera, bloqueos mutuos (*deadlocks*) o falso compartimiento de líneas de caché. | Incompatibilidad de tipos en mensajes, desalineación en `MPI_Gatherv`, *deadlocks* por bloqueos de envío/recepción. |
| **Costo de Mantenimiento** | Mínimo. | Bajo: Muy legible y fácil de auditar. | Bajo con OpenMP, Medio con Pthreads. | Alto: Requiere mantener configuraciones de red, clúster y contenedores Docker. |
| **Escalabilidad Futura** | Nula. | Nula. | Limitada a la capacidad del socket/nodo único. | **Excelente**: Escala a cientos o miles de nodos en la nube o supercomputadores. |

---

## 6. Reflexión Final sobre la Programación Paralela

> *"Un diseño paralelo no siempre necesita 'más hilos' para ser más rápido. El ingeniero de software paralelo debe dividir un problema o tarea compleja en partes más pequeñas y manejables para ejecutarlas al mismo tiempo en distintos procesadores."*

A lo largo del desarrollo de este micro-proyecto se comprobó experimentalmente que **aumentar ciegamente la cantidad de hilos o nodos no garantiza una reducción en el tiempo de respuesta**. 

Factores como el **overhead de sincronización**, la **latencia de creación de hilos**, la **contención del bus de memoria RAM** y la **saturación de la caché** pueden hacer que una solución multihilo mal dimensionada resulte más lenta que una solución secuencial altamente optimizada. El rol central de la ingeniería de software paralelo radica en **encontrar el punto óptimo de granularidad**, balanceando equitativamente la carga de trabajo entre los procesadores y minimizando al máximo las dependencias y la comunicación entre ellos.

---

## 7. Instrucciones para Reproducir las Pruebas

### 7.1 Clonar el Repositorio y Explorar Ramas
```bash
git clone https://github.com/NicoEve/netpbm_filters.git
cd netpbm_filters

# Ramas disponibles para cada diseño:
git checkout diseno-1  # Aplicación base
git checkout diseno-2  # Versión secuencial
git checkout diseno-3  # Memoria compartida (Pthreads y OpenMP)
git checkout diseno-4  # Memoria distribuida (MPI + Docker)
```

### 7.2 Compilación y Ejecución por Diseño

#### Diseño 1: Base
```bash
g++ -Wall -Wextra -O2 -Iheaders -o processor src/processor.cpp src/Image.cpp
./processor images/lena.ppm lena_copia.ppm
```

#### Diseño 2: Secuencial
```bash
g++ -Wall -Wextra -O2 -Iheaders -o filterer src/filterer.cpp src/Filter.cpp src/Image.cpp
./filterer images/fruit.ppm fruit_blur.ppm --f blur
```

#### Diseño 3: Memoria Compartida
```bash
# Pthreads (4 cuadrantes):
g++ -Wall -Wextra -O2 -Iheaders -o th_filterer src/th_filterer.cpp src/Filter.cpp src/Image.cpp -lpthread
./th_filterer images/fruit.pgm fruit_blur2.ppm --f blur

# OpenMP:
g++ -Wall -Wextra -O2 -fopenmp -Iheaders -o omp_filterer src/omp_filterer.cpp src/Filter.cpp src/Image.cpp
./omp_filterer images/sulfur.pgm sulfur_N.pgm
```

#### Diseño 4: Memoria Distribuida con Docker
```bash
# Construir y ejecutar en Windows:
run_docker_mpi.bat

# O manualmente con Docker:
docker build -t netpbm_mpi .
docker run --rm -v "%cd%:/app" netpbm_mpi mpirun -np 4 ./mpi_filterer images/sulfur.pgm sulfur_mpi.pgm
```
