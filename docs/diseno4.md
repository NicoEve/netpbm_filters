# Diseño 4: Memoria Distribuida (MPI + Docker)

## 1. Descripción General
El **Diseño 4** implementa el procesamiento de imágenes mediante el paradigma de **memoria distribuida** utilizando el estándar **MPI (Message Passing Interface)** en un entorno simulado y reproducible con **Docker**.

A diferencia de la memoria compartida, en memoria distribuida cada proceso (nodo/host) posee su propio espacio de direcciones de memoria completamente aislado. Los nodos coordinan la ejecución y comparten información exclusivamente mediante **paso de mensajes explícito**.

---

## 2. Esquema de Paso de Mensajes y Descomposición

### 2.1 Descomposición por Bandas Horizontales (1D Block Decomposition)
Para una imagen de $H$ filas y un comunicador de $P$ nodos (ranks $0, 1, \dots, P-1$):
- Cada nodo recibe un bloque contiguo de filas:
  $$\text{filas\_por\_nodo} = \left\lfloor \frac{H}{P} \right\rfloor$$
  Los residuos $H \pmod P$ se reparten equitativamente entre los primeros nodos para garantizar balanceo de carga (*load balancing*).
- El nodo $r$ es responsable exclusivamente de calcular las filas en el intervalo $[y_{\text{inicio}}, y_{\text{fin}})$.

```
   ┌────────────────────────────────────────────────────────┐
   │ Nodo 0: Filas [0, y1)          (Cálculo Local)         │
   ├────────────────────────────────────────────────────────┤
   │ Nodo 1: Filas [y1, y2)         (Cálculo Local)         │
   ├────────────────────────────────────────────────────────┤
   │ Nodo 2: Filas [y2, y3)         (Cálculo Local)         │
   ├────────────────────────────────────────────────────────┤
   │ Nodo 3: Filas [y3, H)          (Cálculo Local)         │
   └────────────────────────────────────────────────────────┘
```

### 2.2 Protocolo de Comunicación
1. **Paso 1: Transmisión de Metadatos (`MPI_Bcast`)**:
   El Nodo 0 (Master) lee la imagen de entrada y transmite mediante difusión colectiva (`MPI_Bcast`) un arreglo con las dimensiones ($W, H$), canales ($C$), valor máximo de color y formato (`P2` o `P3`).
2. **Paso 2: Difusión de Píxeles Fuente (`MPI_Bcast`)**:
   El búfer de píxeles original se difunde a todos los nodos. Esto permite que cada nodo disponga de la información de vecindad ($3 \times 3$) requerida para el cálculo de los bordes de su banda sin necesidad de intercambios complejos de celdas fantasma (*halo rows*) en cada iteración.
3. **Paso 3: Cómputo Local y Medición**:
   Cada nodo calcula únicamente su banda asignada $[y_{\text{inicio}}, y_{\text{fin}})$ midiendo de manera independiente su **tiempo de CPU** (`std::clock()`) y su **tiempo de reloj de pared** (`MPI_Wtime()`).
4. **Paso 4: Recolección Colectiva Vectorial (`MPI_Gatherv`)**:
   Cada nodo envía únicamente su banda procesada. El Nodo 0 ensambla las bandas de todos los nodos en la imagen final utilizando desplazamientos calculados (`displs`).
5. **Paso 5: Reporte de Tiempos por Nodo (`MPI_Gather`)**:
   Los tiempos de CPU y Wall-Clock individuales de cada nodo son enviados al Nodo 0, el cual imprime la tabla consolidada de desempeño.
6. **Paso 6: Escritura del Archivo Final**:
   El Nodo 0 guarda la imagen resultante en disco.

---

## 3. Entorno Simulado con Docker

Para garantizar que la solución funcione de manera idéntica en cualquier sistema operativo sin requerir la instalación manual de OpenMPI en la máquina anfitriona, se suministra:
- **`Dockerfile`**: Basado en `ubuntu:22.04` con `build-essential`, `openmpi-bin` y `libopenmpi-dev`.
- **`docker-compose.yml`**: Configuración para orquestar la simulación.
- **`run_docker_mpi.bat` y `run_docker_mpi.sh`**: Scripts de un solo clic para construir y ejecutar.

---

## 4. Compilación y Ejecución

### 4.1 En Contenedor Docker (Recomendado)
```bash
# Construir la imagen
docker build -t netpbm_mpi .

# Ejecución sobre sulfur.pgm con 4 nodos (aplica blur, laplace y sharpen):
docker run --rm -v "%cd%:/app" netpbm_mpi mpirun -np 4 ./mpi_filterer images/sulfur.pgm sulfur_mpi.pgm

# Ejecución sobre damma.ppm con 4 nodos:
docker run --rm -v "%cd%:/app" netpbm_mpi mpirun -np 4 ./mpi_filterer images/damma.ppm damma_mpi.ppm --f blur
```

### 4.2 En Entorno Nativo con OpenMPI instalado
```bash
mpic++ -Wall -Wextra -O2 -Iheaders -o mpi_filterer src/mpi_filterer.cpp src/Filter.cpp src/Image.cpp

# Ejecutar con 4 nodos:
mpirun -np 4 ./mpi_filterer images/sulfur.pgm sulfur_mpi.pgm
```

---

## 5. Medición de Tiempos por Nodo

Ejemplo de reporte generado automáticamente por la aplicación MPI:

```
===================================================================
  RESULTADOS DE RENDIMIENTO (DISEÑO 4 - MPI MEMORIA DISTRIBUIDA)
===================================================================
  Total Nodos MPI: 4
  Filtros aplicados: 3 (blur, laplace, sharpen)
-------------------------------------------------------------------
  Nodo   Host                 Filas Asignadas   CPU Time (s)   Wall Time (s)
-------------------------------------------------------------------
  0      mpi-master           [   0 -  250)        0.00912 s      0.00908 s
  1      mpi-master           [ 250 -  500)        0.00895 s      0.00891 s
  2      mpi-master           [ 500 -  750)        0.00889 s      0.00885 s
  3      mpi-master           [ 750 - 1000)        0.00904 s      0.00901 s
-------------------------------------------------------------------
  Tiempo Global de Ejecucion (Wall-Clock): 0.01245 segundos
===================================================================
```
