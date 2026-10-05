# omp_filterer - Filtros Paralelos con OpenMP (Diseño 3)

Este programa paraleliza la aplicación de filtros espaciales sobre imágenes NetPBM utilizando la biblioteca de memoria compartida **OpenMP**, distribuyendo las filas de la imagen entre todos los hilos lógicos disponibles del procesador.

## Compilación

```bash
g++ -Wall -Wextra -O2 -fopenmp -Iheaders -o omp_filterer src/omp_filterer.cpp src/Filter.cpp src/Image.cpp
```

O en Windows:
```powershell
g++ -Wall -Wextra -O2 -fopenmp -Iheaders -o omp_filterer.exe src/omp_filterer.cpp src/Filter.cpp src/Image.cpp
```

## Ejecución

```bash
# Formato solicitado en el enunciado (aplica blur por defecto si se omite --f):
./omp_filterer images/sulfur.pgm sulfur_N.pgm

# Con filtro específico:
./omp_filterer images/damma.ppm damma_blur.ppm --f blur
./omp_filterer images/sulfur.ppm sulfur_laplace.ppm --f laplace
./omp_filterer images/sulfur.ppm sulfur_sharpen.ppm --f sharpen
```
