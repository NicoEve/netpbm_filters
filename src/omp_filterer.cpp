#include <iostream>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <chrono>
#include <omp.h>
#include "../headers/Image.h"
#include "../headers/Filter.h"

#define MAX_FILTERS 16

void printUsage(const char* prog) {
    std::cout << "===================================================================\n";
    std::cout << "  Filtros con Memoria Compartida (OpenMP) - Diseno 3\n";
    std::cout << "===================================================================\n";
    std::cout << "Uso:\n";
    std::cout << "  " << prog << " <input_file> <output_file> [--f <filtro1> ...]\n\n";
    std::cout << "Filtros disponibles:\n";
    std::cout << "  - blur       : Suavizado por promedio 3x3\n";
    std::cout << "  - laplace    : Deteccion de bordes laplaciano 3x3\n";
    std::cout << "  - sharpen    : Realce de nitidez 3x3\n";
    std::cout << "  (Por defecto si no se pasa --f: se aplica 'blur')\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  " << prog << " images/sulfur.pgm sulfur_N.pgm\n";
    std::cout << "  " << prog << " images/damma.ppm damma_blur.ppm --f blur\n";
    std::cout << "  " << prog << " images/sulfur.ppm sulfur_laplace.ppm --f laplace\n";
    std::cout << "===================================================================\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "[ERROR] Argumentos insuficientes.\n";
        printUsage(argv[0]);
        return 1;
    }

    const char* input_path = argv[1];
    const char* output_path = argv[2];

    Filter* filters[MAX_FILTERS];
    int filter_count = 0;

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
                        } else {
                            std::cerr << "[ADVERTENCIA] Filtro desconocido: '" << token << "'. Omitiendo.\n";
                        }
                    } else {
                        break;
                    }
                    token = std::strtok(nullptr, ",");
                }
            } else {
                std::cerr << "[ERROR] Falta el nombre del filtro despues de " << argv[i] << "\n";
                printUsage(argv[0]);
                return 1;
            }
        }
    }

    // Si no se paso ningun filtro especifico con --f, por defecto aplicamos blur
    if (filter_count == 0) {
        filters[filter_count++] = new BlurFilter();
    }

    // 1. Cargar imagen de entrada
    Image current_image;
    std::cerr << "[INFO] Cargando imagen desde: " << input_path << "..." << std::endl;
    bool loaded = false;
    if (std::strcmp(input_path, "-") == 0) {
        loaded = current_image.load(stdin);
    } else {
        loaded = current_image.loadFromFile(input_path);
    }

    if (!loaded || !current_image.isValid()) {
        std::cerr << "[ERROR] No se pudo cargar la imagen de entrada.\n";
        for (int i = 0; i < filter_count; i++) delete filters[i];
        return 1;
    }

    int max_threads = omp_get_max_threads();
    std::cerr << "[INFO] Imagen cargada: " << current_image.getWidth() << "x" << current_image.getHeight()
              << " (" << current_image.getMagic() << ", " << current_image.getChannels() << " canal(es))\n";
    std::cerr << "[INFO] Hilos OpenMP disponibles: " << max_threads << "\n";

    // 2. Medir tiempos de ejecucion del filtrado paralelo con OpenMP
    std::clock_t cpu_start = std::clock();
    double omp_start = omp_get_wtime();

    for (int i = 0; i < filter_count; i++) {
        Image filtered_image;
        if (!filters[i]->applyOMP(current_image, filtered_image)) {
            std::cerr << "[ERROR] Fallo al aplicar el filtro con OpenMP: " << filters[i]->getName() << "\n";
            for (int k = 0; k < filter_count; k++) delete filters[k];
            return 1;
        }
        current_image = filtered_image;
    }

    std::clock_t cpu_end = std::clock();
    double omp_end = omp_get_wtime();

    double cpu_time = static_cast<double>(cpu_end - cpu_start) / CLOCKS_PER_SEC;
    double wall_time = omp_end - omp_start;

    std::cerr << "\n===================================================================\n";
    std::cerr << "  RESULTADOS DE RENDIMIENTO (DISEÑO 3 - OPENMP PARALELO)\n";
    std::cerr << "===================================================================\n";
    std::cerr << "  Hilos activos:          " << max_threads << "\n";
    std::cerr << "  Tiempo en CPU:          " << cpu_time << " segundos (suma de todos los hilos)\n";
    std::cerr << "  Tiempo total ejecucion: " << wall_time << " segundos (Wall-Clock)\n";
    std::cerr << "===================================================================\n\n";

    // 3. Guardar imagen resultante
    std::cerr << "[INFO] Guardando resultado en: " << output_path << "..." << std::endl;
    bool saved = false;
    if (std::strcmp(output_path, "-") == 0) {
        saved = current_image.save(stdout);
    } else {
        saved = current_image.saveToFile(output_path);
    }

    for (int i = 0; i < filter_count; i++) {
        delete filters[i];
    }

    if (!saved) {
        std::cerr << "[ERROR] Fallo al guardar la imagen procesada.\n";
        return 1;
    }

    std::cerr << "[SUCCESS] Filtrado paralelo con OpenMP completado exitosamente.\n";
    return 0;
}
