#ifndef IMAGE_H
#define IMAGE_H

#include <cstdio>

/*
 * Clase Image
 * Representa una imagen en formato NetPBM (PGM - P2 o PPM - P3).
 * Sigue las directrices del proyecto:
 * - Paradigma Orientado a Objetos (modular y extensible).
 * - Uso exclusivo de arreglos dinámicos (int*) y punteros a char (char*).
 * - Sin uso de std::vector ni std::string.
 */

enum ImageType {
    IMAGE_TYPE_UNKNOWN = 0,
    IMAGE_TYPE_PGM = 2, // P2: Escala de grises (1 canal)
    IMAGE_TYPE_PPM = 3  // P3: Color RGB (3 canales)
};

class Image {
private:
    char magic[4];       // Número mágico: "P2" o "P3"
    int width;           // Ancho de la imagen en píxeles
    int height;          // Alto de la imagen en píxeles
    int max_color;       // Valor máximo de intensidad (ej. 255)
    int channels;        // 1 para PGM (P2), 3 para PPM (P3)
    int total_values;    // width * height * channels
    int* pixels;         // Arreglo dinámico con los valores de los píxeles

    // Métodos auxiliares internos de memoria
    void allocateMemory(int count);
    void freeMemory();

    // Métodos auxiliares de lectura de tokens y números respetando comentarios (#)
    static bool skipWhitespaceAndComments(FILE* f);
    static bool readToken(FILE* f, char* token, int max_len);
    static bool readInt(FILE* f, int* value);

public:
    // Constructores y Destructor (Regla de los 3)
    Image();
    Image(int w, int h, int ch, int max_c, const char* magic_str);
    Image(const Image& other);            // Constructor de copia
    Image& operator=(const Image& other); // Operador de asignación
    ~Image();                             // Destructor

    // Getters y utilidades
    const char* getMagic() const;
    int getWidth() const;
    int getHeight() const;
    int getMaxColor() const;
    int getChannels() const;
    int getTotalValues() const;
    bool isValid() const;
    ImageType getType() const;

    // Acceso a píxeles
    const int* getPixels() const;
    int* getPixels();

    // Acceso para PGM (1 canal)
    int getPixel(int x, int y) const;
    void setPixel(int x, int y, int value);

    // Acceso para PPM (3 canales: 0=R, 1=G, 2=B)
    int getPixel(int x, int y, int channel) const;
    void setPixel(int x, int y, int channel, int value);

    // Acceso directo por índice plano
    int getValue(int index) const;
    void setValue(int index, int value);

    // Operaciones de entrada y salida
    bool load(FILE* in);
    bool loadFromFile(const char* filepath);
    bool save(FILE* out) const;
    bool saveToFile(const char* filepath) const;

    // Inicializar imagen vacía con dimensiones
    bool init(int w, int h, int ch, int max_c, const char* magic_str);
};

#endif // IMAGE_H
