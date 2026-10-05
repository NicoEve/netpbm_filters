# Filterer - Filtros Secuenciales (Diseño 2)

Este programa en C++ permite aplicar filtros espaciales de convolución (`blur`, `laplace`, `sharpen`) sobre imágenes en formato PGM (escala de grises) y PPM (color RGB), midiendo el tiempo de CPU y el tiempo total de ejecución.

## Compilación

Desde la raíz del repositorio:
```bash
g++ -Wall -Wextra -O2 -Iheaders -o filterer src/filterer.cpp src/Filter.cpp src/Image.cpp
```

O en Windows:
```powershell
g++ -Wall -Wextra -O2 -Iheaders -o filterer.exe src/filterer.cpp src/Filter.cpp src/Image.cpp
```

## Ejecución

### Filtro individual
```bash
./filterer images/fruit.ppm fruit_blur.ppm --f blur
./filterer images/lena.pgm lena_laplace.pgm --f laplace
./filterer images/puj.ppm puj_sharpen.ppm --f sharpen
```

### Múltiples filtros encadenados
```bash
./filterer images/fruit.ppm resultado.ppm --f blur --f sharpen
```