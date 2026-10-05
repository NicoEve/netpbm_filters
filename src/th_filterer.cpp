#include <iostream>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <chrono>
#include <pthread.h>
#include "../headers/Image.h"
#include "../headers/Filter.h"

#define NUM_REGIONS 4
#define MAX_FILTERS 16

// Estructura para pasar parametros a cada hilo (sin uso de std::vector ni std::string)
struct RegionTask {
    int id;
    const char* region_name;
    const Image* src;
    Image* dst;
    const Filter* filter;
    int x_start;
    int x_end;
    int y_start;
    int y_end;
};

// Funcion ejecutada por cada hilo POSIX (pthread)
void* threadFilterRegion(void* arg) {
    RegionTask* task = static_cast<RegionTask*>(arg);
    if (task && task->filter && task->src && task->dst) {
        task->filter->applyRegion(*(task->src), *(task->dst),
                                  task->x_start, task->x_end,
                                  task->y_start, task->y_end);
    }
    return nullptr;
}

void printUsage(const char* prog) {
    std::cout << "===================================================================\n";
    std::cout << "  Filtros con Memoria Compartida (Pthreads) - Diseno 3\n";
    std::cout << "  Paralelizacion dividiendo la imagen en 4 cuadrantes independientes\n";
    std::cout << "===================================================================\n";
    std::cout << "Uso:\n";
    std::cout << "  " << prog << " <input_file> <output_file> --f <filtro1> [--f <filtro2> ...]\n\n";
    std::cout << "Filtros disponibles:\n";
    std::cout << "  - blur       : Suavizado por promedio 3x3\n";
    std::cout << "  - laplace    : Deteccion de bordes laplaciano 3x3\n";
    std::cout << "  - sharpen    : Realce de nitidez 3x3\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  " << prog << " images/fruit.pgm fruit_blur2.ppm --f blur\n";
    std::cout << "  " << prog << " images/damma.pgm damma_laplace.pgm --f laplace\n";
    std::cout << "  " << prog << " images/sulfur.ppm sulfur_sharpen.ppm --f sharpen\n";
    std::cout << "===================================================================\n";
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

    if (filter_count == 0) {
        std::cerr << "[ERROR] Debe especificar al menos un filtro con --f <nombre>\n";
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
        std::cerr << "[ERROR] No se pudo cargar la imagen.\n";
        for (int i = 0; i < filter_count; i++) delete filters[i];
        return 1;
    }

    int width = current_image.getWidth();
    int height = current_image.getHeight();
    int channels = current_image.getChannels();
    int mid_x = width / 2;
    int mid_y = height / 2;

    std::cerr << "[INFO] Imagen cargada: " << width << "x" << height 
              << " (" << current_image.getMagic() << ", " << channels << " canal(es))\n";
    std::cerr << "[INFO] Descomposicion en 4 cuadrantes (Hilos Pthreads):\n";
    std::cerr << "  - Hilo 0 (Arriba-Izquierda): x=[0, " << mid_x << "), y=[0, " << mid_y << ")\n";
    std::cerr << "  - Hilo 1 (Arriba-Derecha):   x=[" << mid_x << ", " << width << "), y=[0, " << mid_y << ")\n";
    std::cerr << "  - Hilo 2 (Abajo-Izquierda):  x=[0, " << mid_x << "), y=[" << mid_y << ", " << height << ")\n";
    std::cerr << "  - Hilo 3 (Abajo-Derecha):    x=[" << mid_x << ", " << width << "), y=[" << mid_y << ", " << height << ")\n";

    // 2. Medir tiempos de ejecucion del filtrado paralelo con Pthreads
    std::clock_t cpu_start = std::clock();
    auto wall_start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < filter_count; i++) {
        Image filtered_image;
        if (!filtered_image.init(width, height, channels, current_image.getMaxColor(), current_image.getMagic())) {
            std::cerr << "[ERROR] No se pudo inicializar la imagen de destino.\n";
            for (int k = 0; k < filter_count; k++) delete filters[k];
            return 1;
        }

        pthread_t threads[NUM_REGIONS];
        RegionTask tasks[NUM_REGIONS] = {
            { 0, "Arriba-Izquierda", &current_image, &filtered_image, filters[i], 0, mid_x, 0, mid_y },
            { 1, "Arriba-Derecha",   &current_image, &filtered_image, filters[i], mid_x, width, 0, mid_y },
            { 2, "Abajo-Izquierda",  &current_image, &filtered_image, filters[i], 0, mid_x, mid_y, height },
            { 3, "Abajo-Derecha",    &current_image, &filtered_image, filters[i], mid_x, width, mid_y, height }
        };

        // Crear 4 hilos para procesar las 4 regiones concurrentemente
        for (int t = 0; t < NUM_REGIONS; t++) {
            int rc = pthread_create(&threads[t], nullptr, threadFilterRegion, &tasks[t]);
            if (rc != 0) {
                std::cerr << "[ERROR] Error al crear el hilo " << t << " (codigo " << rc << ")\n";
                for (int k = 0; k < filter_count; k++) delete filters[k];
                return 1;
            }
        }

        // Sincronizar (esperar que los 4 hilos terminen su region)
        for (int t = 0; t < NUM_REGIONS; t++) {
            pthread_join(threads[t], nullptr);
        }

        current_image = filtered_image;
    }

    std::clock_t cpu_end = std::clock();
    auto wall_end = std::chrono::high_resolution_clock::now();

    double cpu_time = static_cast<double>(cpu_end - cpu_start) / CLOCKS_PER_SEC;
    std::chrono::duration<double> wall_time = wall_end - wall_start;

    std::cerr << "\n===================================================================\n";
    std::cerr << "  RESULTADOS DE RENDIMIENTO (DISEÑO 3 - PTHREADS 4 REGIONES)\n";
    std::cerr << "===================================================================\n";
    std::cerr << "  Tiempo en CPU:          " << cpu_time << " segundos (suma de todos los hilos)\n";
    std::cerr << "  Tiempo total ejecucion: " << wall_time.count() << " segundos (Wall-Clock)\n";
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

    std::cerr << "[SUCCESS] Filtrado paralelo con Pthreads completado exitosamente.\n";
    return 0;
}
