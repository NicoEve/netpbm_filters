#include <iostream>
#include <cstring>
#include <cstdio>
#include "../headers/Image.h"

void printUsage(const char* prog) {
    std::cout << "=====================================================\n";
    std::cout << "  Procesador de Imagenes NetPBM (PGM / PPM) - Diseno 1\n";
    std::cout << "=====================================================\n";
    std::cout << "Uso:\n";
    std::cout << "  1) Con archivos:\n";
    std::cout << "     " << prog << " <input_file.pgm|ppm> <output_file.pgm|ppm>\n\n";
    std::cout << "  2) Con redireccion de entrada estandar (stdin):\n";
    std::cout << "     " << prog << " < input_file.ppm > output_file.ppm\n\n";
    std::cout << "  3) Leyendo de stdin hacia archivo de salida:\n";
    std::cout << "     " << prog << " <output_file.pgm|ppm> < input_file.pgm\n";
    std::cout << "     " << prog << " - <output_file.pgm|ppm>\n";
    std::cout << "=====================================================\n";
}

int main(int argc, char* argv[]) {
    const char* input_path = nullptr;
    const char* output_path = nullptr;

    if (argc >= 3) {
        input_path = argv[1];
        output_path = argv[2];
    } else if (argc == 2) {
        // Podria ser: ./processor output.ppm (leyendo de stdin)
        // o ./processor input.ppm (escribiendo a stdout)
        // Por compatibilidad con ./processor output.ppm < input.ppm:
        output_path = argv[1];
    } else if (argc == 1) {
        // Redireccion total: ./processor < in > out
        input_path = nullptr; // stdin
        output_path = nullptr; // stdout
    }

    Image img;
    bool loaded = false;

    // Cargar imagen
    if (input_path && std::strcmp(input_path, "-") != 0) {
        std::cerr << "[INFO] Cargando imagen desde archivo: " << input_path << "..." << std::endl;
        loaded = img.loadFromFile(input_path);
    } else {
        std::cerr << "[INFO] Cargando imagen desde entrada estandar (stdin)..." << std::endl;
        loaded = img.load(stdin);
    }

    if (!loaded || !img.isValid()) {
        std::cerr << "[ERROR] Fallo al cargar la imagen." << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    // Informacion de la imagen cargada
    std::cerr << "[INFO] Imagen cargada exitosamente:\n";
    std::cerr << "  - Formato:      " << img.getMagic() 
              << (img.getType() == IMAGE_TYPE_PGM ? " (PGM - Escala de Grises)" : " (PPM - Color RGB)") << "\n";
    std::cerr << "  - Dimensiones:  " << img.getWidth() << " x " << img.getHeight() << " px\n";
    std::cerr << "  - Canales:      " << img.getChannels() << "\n";
    std::cerr << "  - Max Color:    " << img.getMaxColor() << "\n";
    std::cerr << "  - Total datos:  " << img.getTotalValues() << " valores de pixel\n";

    // Guardar imagen procesada
    bool saved = false;
    if (output_path && std::strcmp(output_path, "-") != 0) {
        std::cerr << "[INFO] Guardando imagen resultante en: " << output_path << "..." << std::endl;
        saved = img.saveToFile(output_path);
    } else {
        std::cerr << "[INFO] Escribiendo imagen resultante en salida estandar (stdout)..." << std::endl;
        saved = img.save(stdout);
    }

    if (!saved) {
        std::cerr << "[ERROR] Fallo al guardar la imagen." << std::endl;
        return 1;
    }

    std::cerr << "[SUCCESS] Procesamiento completado correctamente." << std::endl;
    return 0;
}