# ===================================================================
# Dockerfile para Entorno de Memoria Distribuida con MPI (Diseño 4)
# ===================================================================
FROM ubuntu:22.04

# Evitar prompts interactivos durante la instalacion
ENV DEBIAN_FRONTEND=noninteractive

# Instalar compilador C++, OpenMPI y herramientas necesarias
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    g++ \
    make \
    openmpi-bin \
    libopenmpi-dev \
    openssh-client \
    openssh-server \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Directorio de trabajo
WORKDIR /app

# Copiar el codigo fuente del repositorio
COPY . /app

# Compilar la aplicacion MPI con optimizaciones
RUN mpic++ -Wall -Wextra -O2 -Iheaders -o mpi_filterer src/mpi_filterer.cpp src/Filter.cpp src/Image.cpp

# Permite ejecutar mpirun como root de manera segura dentro del contenedor
ENV OMPI_ALLOW_RUN_AS_ROOT=1
ENV OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1

# Comando por defecto: ejecutar con 4 nodos sobre sulfur.pgm
CMD ["mpirun", "-np", "4", "./mpi_filterer", "images/sulfur.pgm", "sulfur_mpi_out.pgm"]
