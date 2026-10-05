#!/bin/bash
set -e

echo "==================================================================="
echo "  Ejecutando Diseno 4: MPI en Contenedor Docker"
echo "==================================================================="

echo "1. Construyendo la imagen de Docker (netpbm_mpi)..."
docker build -t netpbm_mpi .

echo ""
echo "2. Ejecutando simulacion MPI con 4 nodos sobre sulfur.pgm..."
docker run --rm -v "$(pwd):/app" netpbm_mpi mpirun -np 4 ./mpi_filterer images/sulfur.pgm sulfur_mpi.pgm

echo ""
echo "3. Ejecutando simulacion MPI con 4 nodos sobre damma.ppm..."
docker run --rm -v "$(pwd):/app" netpbm_mpi mpirun -np 4 ./mpi_filterer images/damma.ppm damma_mpi.ppm

echo ""
echo "==================================================================="
echo "  Simulacion completada exitosamente."
echo "==================================================================="
