#include <iostream>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <chrono>
#include "../headers/Image.h"
#include "../headers/Filter.h"

#define MAX_FILTERS 16

void printUsage(const char* prog) {
    std::cout << "============================================================\n";
    std::cout << "  Filtros de Imagenes NetPBM (PGM / PPM) - Diseno 2 Secuencial\n";
    std::cout << "============================================================\n";
    std::cout << "Uso:\n";
    std::cout << "  " << prog << " <input_file> <output_file> --f <filtro1> [--f <filtro2> ...]\n\n";
    std::cout << "Filtros disponibles:\n";
    std::cout << "  - blur       : Suavizado por promedio 3x3\n";
    std::cout << "  - laplace    : Deteccion de bordes laplaciano 3x3\n";
    std::cout << "  - sharpen    : Realce de nitidez 3x3\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  " << prog << " images/fruit.ppm fruit_blur.ppm --f blur\n";
    std::cout << "  " << prog << " images/lena.pgm lena_laplace.pgm --f laplace\n";
    std::cout << "  " << prog << " images/puj.ppm puj_sharpen.ppm --f sharpen\n";
    std::cout << "  " << prog << " images/fruit.ppm out.ppm --f blur --f sharpen\n";
    std::cout << "============================================================\n";
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "[ERROR] Argumentos insuficientes.\n";
        printUsage(argv[0]);
        return 1;
    }

    const char* input_path = argv[1];
    const char* output_path = argv[2];

    Filter* filters[MAX_FILTERS];
    int filter_count = 0;

    // Procesar argumentos de filtros
    for (int i = 3; i < argc; i++) {
        if (std::strcmp(argv[i], "--f") == 0 || std::strcmp(argv[i], "-f") == 0) {
            if (i + 1 < argc) {
                i++;
                // Soportar filtros separados por coma o individuales
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
                        std::cerr << "[ADVERTENCIA] Se alcanzo el maximo de filtros soportados (" << MAX_FILTERS << ").\n";
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

    if (filter_count == 0) {
        std::cerr << "[ERROR] Debe especificar al menos un filtro valido con --f <nombre>\n";
        printUsage(argv[0]);
        return 1;
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

    std::cerr << "[INFO] Imagen cargada: " << current_image.getWidth() << "x" << current_image.getHeight()
              << " (" << current_image.getMagic() << ", " << current_image.getChannels() << " canal(es))\n";

    // 2. Medir tiempos de ejecucion del filtrado
    std::cerr << "[INFO] Aplicando " << filter_count << " filtro(s) secuencialmente...\n";
    for (int i = 0; i < filter_count; i++) {
        std::cerr << "  - [" << (i + 1) << "] " << filters[i]->getName() << "\n";
    }

    std::clock_t cpu_start = std::clock();
    auto wall_start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < filter_count; i++) {
        Image filtered_image;
        if (!filters[i]->apply(current_image, filtered_image)) {
            std::cerr << "[ERROR] Fallo al aplicar el filtro: " << filters[i]->getName() << "\n";
            for (int k = 0; k < filter_count; k++) delete filters[k];
            return 1;
        }
        current_image = filtered_image; // Copia segura gracias a la Regla de los 3
    }

    std::clock_t cpu_end = std::clock();
    auto wall_end = std::chrono::high_resolution_clock::now();

    double cpu_time = static_cast<double>(cpu_end - cpu_start) / CLOCKS_PER_SEC;
    std::chrono::duration<double> wall_time = wall_end - wall_start;

    std::cerr << "\n============================================================\n";
    std::cerr << "  RESULTADOS DE RENDIMIENTO (DISEÑO 2 - SECUENCIAL)\n";
    std::cerr << "============================================================\n";
    std::cerr << "  Tiempo en CPU:          " << cpu_time << " segundos\n";
    std::cerr << "  Tiempo total ejecucion: " << wall_time.count() << " segundos\n";
    std::cerr << "============================================================\n\n";

    // 3. Guardar imagen resultante
    std::cerr << "[INFO] Guardando resultado en: " << output_path << "..." << std::endl;
    bool saved = false;
    if (std::strcmp(output_path, "-") == 0) {
        saved = current_image.save(stdout);
    } else {
        saved = current_image.saveToFile(output_path);
    }

    // 4. Liberar filtros
    for (int i = 0; i < filter_count; i++) {
        delete filters[i];
    }

    if (!saved) {
        std::cerr << "[ERROR] Fallo al guardar la imagen procesada.\n";
        return 1;
    }

    std::cerr << "[SUCCESS] Filtrado completado exitosamente.\n";
    return 0;
}