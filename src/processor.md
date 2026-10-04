# Processor - Aplicación Base (Diseño 1)

Este programa en C++ permite leer, procesar y guardar archivos de imagen en formato PGM (P2 - escala de grises) y PPM (P3 - color RGB).

## Compilación

Desde la raíz del repositorio:
```bash
g++ -Wall -Wextra -O2 -Iheaders -o processor src/processor.cpp src/Image.cpp
```

O en Windows:
```powershell
g++ -Wall -Wextra -O2 -Iheaders -o processor.exe src/processor.cpp src/Image.cpp
```

## Ejecución

### 1. Con rutas de archivo
```bash
./processor images/lena.pgm lena_salida.pgm
./processor images/lena.ppm lena_salida.ppm
```

### 2. Desde entrada estándar (stdin)
```bash
./processor < images/lena.ppm > lena_salida.ppm
./processor - lena_salida.pgm < images/lena.pgm
```