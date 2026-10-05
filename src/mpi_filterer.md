# mpi_filterer - Filtros con Memoria Distribuida (Diseño 4)

Este programa paraleliza la aplicación de filtros espaciales mediante **paso de mensajes con MPI** sobre contenedores Docker, dividiendo la imagen en bandas horizontales asignadas a diferentes nodos/procesos.

## Ejecución con Docker

### En Windows:
```cmd
run_docker_mpi.bat
```

O manualmente:
```bash
docker build -t netpbm_mpi .
docker run --rm -v "%cd%:/app" netpbm_mpi mpirun -np 4 ./mpi_filterer images/sulfur.pgm sulfur_mpi.pgm
```

### En Linux / macOS / WSL:
```bash
chmod +x run_docker_mpi.sh
./run_docker_mpi.sh
```

## Compilación Nativa (si OpenMPI está instalado)

```bash
mpic++ -Wall -Wextra -O2 -Iheaders -o mpi_filterer src/mpi_filterer.cpp src/Filter.cpp src/Image.cpp
mpirun -np 4 ./mpi_filterer images/sulfur.pgm sulfur_mpi.pgm
```
