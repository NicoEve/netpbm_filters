# th_filterer - Filtros Paralelos con Pthreads (Diseño 3)

Este programa paraleliza la aplicación de filtros sobre imágenes NetPBM (PGM/PPM) dividiendo la imagen en **cuatro cuadrantes geométricos independientes**:
1. Arriba-Izquierda
2. Arriba-Derecha
3. Abajo-Izquierda
4. Abajo-Derecha

Cada región es procesada de manera concurrente por un hilo POSIX (`pthread`).

## Compilación

```bash
g++ -Wall -Wextra -O2 -Iheaders -o th_filterer src/th_filterer.cpp src/Filter.cpp src/Image.cpp -lpthread
```

O en Windows:
```powershell
g++ -Wall -Wextra -O2 -Iheaders -o th_filterer.exe src/th_filterer.cpp src/Filter.cpp src/Image.cpp -lpthread
```

## Ejecución

```bash
# Formato solicitado en el enunciado:
./th_filterer images/fruit.pgm fruit_blur2.ppm --f blur

# Pruebas con damma y sulfur:
./th_filterer images/damma.pgm damma_blur.pgm --f blur
./th_filterer images/sulfur.ppm sulfur_sharpen.ppm --f sharpen
```
