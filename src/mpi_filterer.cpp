#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <mpi.h>
#include "../headers/Image.h"
#include "../headers/Filter.h"

#define MAX_FILTERS 16

void printUsage(const char* prog) {
    std::cout << "===================================================================\n";
    std::cout << "  Filtros con Memoria Distribuida (MPI + Docker) - Diseno 4\n";
    std::cout << "  Paso de mensajes entre nodos descomponiendo la imagen por bandas\n";
    std::cout << "===================================================================\n";
    std::cout << "Uso:\n";
    std::cout << "  mpirun -np <num_nodos> " << prog << " <input_file> <output_file> [--f <filtro1> ...]\n\n";
    std::cout << "Filtros disponibles:\n";
    std::cout << "  - blur       : Suavizado por promedio 3x3\n";
    std::cout << "  - laplace    : Deteccion de bordes laplaciano 3x3\n";
    std::cout << "  - sharpen    : Realce de nitidez 3x3\n";
    std::cout << "  (Si no se especifica --f, se aplican los 3 filtros en cadena)\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  mpirun -np 4 " << prog << " images/sulfur.ppm sulfur_mpi.ppm\n";
    std::cout << "  mpirun -np 4 " << prog << " images/damma.ppm damma_mpi.ppm --f blur\n";
    std::cout << "  mpirun -np 2 " << prog << " images/sulfur.pgm sulfur_mpi.pgm --f sharpen\n";
    std::cout << "===================================================================\n";
}

int main(int argc, char* argv[]) {
    // 1. Inicializacion del entorno MPI
    MPI_Init(&argc, &argv);

    int rank = 0;
    int world_size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len = 0;
    MPI_Get_processor_name(processor_name, &name_len);

    if (argc < 3) {
        if (rank == 0) {
            std::cerr << "[ERROR] Argumentos insuficientes.\n";
            printUsage(argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    const char* input_path = argv[1];
    const char* output_path = argv[2];

    Filter* filters[MAX_FILTERS];
    int filter_count = 0;

    // Parseo de argumentos de filtros
    for (int i = 3; i < argc; i++) {
        if (std::strcmp(argv[i], "--f") == 0 || std::strcmp(argv[i], "-f") == 0) {
            if (i + 1 < argc) {
                i++;
                char filter_arg[64];
                std::strncpy(filter_arg, argv[i], sizeof(filter_arg) - 1);
                filter_arg[sizeof(filter_arg) - 1] = '\0';

                char* token = std::strtok(filter_arg, ",");
                while (token != nullptr) {
                    if (filter_count < MAX_FILTERS) {
                        Filter* f = createFilterByName(token);
                        if (f) {
                            filters[filter_count++] = f;
                        } else if (rank == 0) {
                            std::cerr << "[ADVERTENCIA] Filtro desconocido: '" << token << "'. Omitiendo.\n";
                        }
                    } else {
                        break;
                    }
                    token = std::strtok(nullptr, ",");
                }
            }
        }
    }

    // Requerimiento del documento: si no se especifica --f, aplicar los 3 filtros (blur, laplace, sharpen)
    if (filter_count == 0) {
        filters[filter_count++] = new BlurFilter();
        filters[filter_count++] = new LaplaceFilter();
        filters[filter_count++] = new SharpenFilter();
    }

    // 2. Carga inicial de imagen en el Nodo 0 (Master)
    Image full_image;
    int meta[5]; // 0: width, 1: height, 2: channels, 3: max_color, 4: magic_code (2 o 3)

    if (rank == 0) {
        std::cerr << "[NODO " << rank << " (" << processor_name << ")] Cargando imagen: " << input_path << "...\n";
        bool loaded = false;
        if (std::strcmp(input_path, "-") == 0) {
            loaded = full_image.load(stdin);
        } else {
            loaded = full_image.loadFromFile(input_path);
        }

        if (!loaded || !full_image.isValid()) {
            std::cerr << "[ERROR] Nodo 0 no pudo cargar la imagen de entrada.\n";
            meta[0] = -1; // Señal de error
        } else {
            meta[0] = full_image.getWidth();
            meta[1] = full_image.getHeight();
            meta[2] = full_image.getChannels();
            meta[3] = full_image.getMaxColor();
            meta[4] = (full_image.getType() == IMAGE_TYPE_PGM) ? 2 : 3;

            std::cerr << "[NODO 0] Imagen cargada: " << meta[0] << "x" << meta[1]
                      << " (" << (meta[4] == 2 ? "P2 - PGM" : "P3 - PPM") << ", "
                      << meta[2] << " canales, " << world_size << " nodos MPI activos)\n";
        }
    }

    // Transmitir metadatos a todos los nodos
    MPI_Bcast(meta, 5, MPI_INT, 0, MPI_COMM_WORLD);

    if (meta[0] <= 0) {
        for (int i = 0; i < filter_count; i++) delete filters[i];
        MPI_Finalize();
        return 1;
    }

    int width = meta[0];
    int height = meta[1];
    int channels = meta[2];
    int max_color = meta[3];
    const char* magic_str = (meta[4] == 2) ? "P2" : "P3";
    int total_values = width * height * channels;

    // 3. Descomposicion por bandas horizontales (1D Block Decomposition)
    int rows_per_rank = height / world_size;
    int remainder = height % world_size;

    // Arreglos de conteos y desplazamientos para MPI_Gatherv
    int* send_counts = new int[world_size];
    int* displs = new int[world_size];

    int current_y = 0;
    for (int r = 0; r < world_size; r++) {
        int r_rows = rows_per_rank + (r < remainder ? 1 : 0);
        displs[r] = current_y * width * channels;
        send_counts[r] = r_rows * width * channels;
        current_y += r_rows;
    }

    int my_rows = rows_per_rank + (rank < remainder ? 1 : 0);
    int my_y_start = displs[rank] / (width * channels);
    int my_y_end = my_y_start + my_rows;

    // Buffer local de la imagen compartida para lectura
    int* broadcast_pixels = nullptr;
    if (rank == 0) {
        broadcast_pixels = full_image.getPixels();
    } else {
        broadcast_pixels = new int[total_values];
    }

    // Inicializar imagen local de destino para recolectar
    Image local_dst;
    local_dst.init(width, height, channels, max_color, magic_str);

    // Variables para medicion de tiempos por nodo
    double node_cpu_time = 0.0;
    double node_wall_time = 0.0;

    // Sincronizar nodos antes de comenzar medicion
    MPI_Barrier(MPI_COMM_WORLD);

    std::clock_t global_cpu_start = std::clock();
    double global_wall_start = MPI_Wtime();

    // 4. Aplicacion de los filtros distribuidos
    for (int f = 0; f < filter_count; f++) {
        // Enviar la imagen actual a todos los nodos de trabajo
        MPI_Bcast(broadcast_pixels, total_values, MPI_INT, 0, MPI_COMM_WORLD);

        Image local_src;
        local_src.init(width, height, channels, max_color, magic_str);
        std::memcpy(local_src.getPixels(), broadcast_pixels, total_values * sizeof(int));

        // Cada nodo mide su tiempo local de CPU y de reloj de pared
        std::clock_t local_cpu_start = std::clock();
        double local_wall_start = MPI_Wtime();

        // Aplicar el filtro UNICAMENTE sobre las filas asignadas a este nodo [my_y_start, my_y_end)
        filters[f]->applyRegion(local_src, local_dst, 0, width, my_y_start, my_y_end);

        std::clock_t local_cpu_end = std::clock();
        double local_wall_end = MPI_Wtime();

        node_cpu_time += static_cast<double>(local_cpu_end - local_cpu_start) / CLOCKS_PER_SEC;
        node_wall_time += (local_wall_end - local_wall_start);

        // Recolectar las bandas calculadas de cada nodo en el Nodo 0 (Master)
        MPI_Gatherv(local_dst.getPixels() + displs[rank],
                    send_counts[rank],
                    MPI_INT,
                    (rank == 0 ? broadcast_pixels : nullptr),
                    send_counts,
                    displs,
                    MPI_INT,
                    0,
                    MPI_COMM_WORLD);
    }

    std::clock_t global_cpu_end = std::clock();
    double global_wall_end = MPI_Wtime();

    double total_node_cpu = static_cast<double>(global_cpu_end - global_cpu_start) / CLOCKS_PER_SEC;
    double total_node_wall = global_wall_end - global_wall_start;

    // 5. Recoleccion y presentacion de tiempos de cada nodo
    double* all_cpu_times = nullptr;
    double* all_wall_times = nullptr;

    if (rank == 0) {
        all_cpu_times = new double[world_size];
        all_wall_times = new double[world_size];
    }

    MPI_Gather(&node_cpu_time, 1, MPI_DOUBLE, all_cpu_times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&node_wall_time, 1, MPI_DOUBLE, all_wall_times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cerr << "\n===================================================================\n";
        std::cerr << "  RESULTADOS DE RENDIMIENTO (DISEÑO 4 - MPI MEMORIA DISTRIBUIDA)\n";
        std::cerr << "===================================================================\n";
        std::cerr << "  Total Nodos MPI: " << world_size << "\n";
        std::cerr << "  Filtros aplicados: " << filter_count << "\n";
        std::cerr << "-------------------------------------------------------------------\n";
        std::cerr << "  Nodo   Host                 Filas Asignadas   CPU Time (s)   Wall Time (s)\n";
        std::cerr << "-------------------------------------------------------------------\n";
        for (int r = 0; r < world_size; r++) {
            int r_start = displs[r] / (width * channels);
            int r_rows = send_counts[r] / (width * channels);
            std::fprintf(stderr, "  %-6d %-20s [%4d - %4d)     %10.5f s   %10.5f s\n",
                         r, processor_name, r_start, r_start + r_rows,
                         all_cpu_times[r], all_wall_times[r]);
        }
        std::cerr << "-------------------------------------------------------------------\n";
        std::cerr << "  Tiempo Global de Ejecucion (Wall-Clock): " << total_node_wall << " segundos\n";
        std::cerr << "===================================================================\n\n";

        // 6. Guardar la imagen final ensamblada
        std::cerr << "[NODO 0] Guardando imagen resultante en: " << output_path << "...\n";
        // Replicar píxeles calculados en full_image
        std::memcpy(full_image.getPixels(), broadcast_pixels, total_values * sizeof(int));

        bool saved = false;
        if (std::strcmp(output_path, "-") == 0) {
            saved = full_image.save(stdout);
        } else {
            saved = full_image.saveToFile(output_path);
        }

        if (saved) {
            std::cerr << "[SUCCESS] Filtrado distribuido con MPI finalizado correctamente.\n";
        } else {
            std::cerr << "[ERROR] Fallo al guardar la imagen final en el Nodo 0.\n";
        }

        delete[] all_cpu_times;
        delete[] all_wall_times;
    }

    // Limpieza de memoria (sin fugas)
    if (rank != 0) {
        delete[] broadcast_pixels;
    }
    delete[] send_counts;
    delete[] displs;
    for (int i = 0; i < filter_count; i++) {
        delete filters[i];
    }

    MPI_Finalize();
    return 0;
}
