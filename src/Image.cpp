#include "../headers/Image.h"
#include <cstdlib>
#include <cstring>
#include <cctype>

Image::Image()
    : width(0), height(0), max_color(255), channels(0), total_values(0), pixels(nullptr) {
    magic[0] = '\0';
}

Image::Image(int w, int h, int ch, int max_c, const char* magic_str)
    : width(w), height(h), max_color(max_c), channels(ch), total_values(w * h * ch), pixels(nullptr) {
    magic[0] = '\0';
    if (magic_str) {
        std::strncpy(magic, magic_str, 3);
        magic[3] = '\0';
    }
    allocateMemory(total_values);
}

Image::Image(const Image& other)
    : width(other.width), height(other.height), max_color(other.max_color),
      channels(other.channels), total_values(other.total_values), pixels(nullptr) {
    std::strncpy(magic, other.magic, 4);
    magic[3] = '\0';
    allocateMemory(total_values);
    if (pixels && other.pixels) {
        for (int i = 0; i < total_values; i++) {
            pixels[i] = other.pixels[i];
        }
    }
}

Image& Image::operator=(const Image& other) {
    if (this != &other) {
        freeMemory();
        width = other.width;
        height = other.height;
        max_color = other.max_color;
        channels = other.channels;
        total_values = other.total_values;
        std::strncpy(magic, other.magic, 4);
        magic[3] = '\0';

        allocateMemory(total_values);
        if (pixels && other.pixels) {
            for (int i = 0; i < total_values; i++) {
                pixels[i] = other.pixels[i];
            }
        }
    }
    return *this;
}

Image::~Image() {
    freeMemory();
}

void Image::allocateMemory(int count) {
    if (count > 0) {
        pixels = new int[count]();
    } else {
        pixels = nullptr;
    }
}

void Image::freeMemory() {
    if (pixels) {
        delete[] pixels;
        pixels = nullptr;
    }
}

bool Image::isValid() const {
    return pixels != nullptr && width > 0 && height > 0 && total_values > 0;
}

ImageType Image::getType() const {
    if (std::strcmp(magic, "P2") == 0) return IMAGE_TYPE_PGM;
    if (std::strcmp(magic, "P3") == 0) return IMAGE_TYPE_PPM;
    return IMAGE_TYPE_UNKNOWN;
}

const char* Image::getMagic() const { return magic; }
int Image::getWidth() const { return width; }
int Image::getHeight() const { return height; }
int Image::getMaxColor() const { return max_color; }
int Image::getChannels() const { return channels; }
int Image::getTotalValues() const { return total_values; }
const int* Image::getPixels() const { return pixels; }
int* Image::getPixels() { return pixels; }

int Image::getPixel(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height || !pixels) return 0;
    return pixels[y * width + x];
}

void Image::setPixel(int x, int y, int value) {
    if (x >= 0 && x < width && y >= 0 && y < height && pixels) {
        pixels[y * width + x] = value;
    }
}

int Image::getPixel(int x, int y, int channel) const {
    if (x < 0 || x >= width || y < 0 || y >= height ||
        channel < 0 || channel >= channels || !pixels) {
        return 0;
    }
    return pixels[(y * width + x) * channels + channel];
}

void Image::setPixel(int x, int y, int channel, int value) {
    if (x >= 0 && x < width && y >= 0 && y < height &&
        channel >= 0 && channel < channels && pixels) {
        pixels[(y * width + x) * channels + channel] = value;
    }
}

int Image::getValue(int index) const {
    if (index >= 0 && index < total_values && pixels) {
        return pixels[index];
    }
    return 0;
}

void Image::setValue(int index, int value) {
    if (index >= 0 && index < total_values && pixels) {
        pixels[index] = value;
    }
}

bool Image::init(int w, int h, int ch, int max_c, const char* magic_str) {
    freeMemory();
    width = w;
    height = h;
    channels = ch;
    max_color = max_c;
    total_values = w * h * ch;
    magic[0] = '\0';
    if (magic_str) {
        std::strncpy(magic, magic_str, 3);
        magic[3] = '\0';
    }
    allocateMemory(total_values);
    return isValid();
}

bool Image::skipWhitespaceAndComments(FILE* f) {
    if (!f) return false;
    int c;
    while ((c = std::fgetc(f)) != EOF) {
        if (c == '#') {
            // Comentario opcional: saltar hasta el salto de línea
            while ((c = std::fgetc(f)) != EOF && c != '\n');
        } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' && c != '\f') {
            std::ungetc(c, f);
            return true;
        }
    }
    return false;
}

bool Image::readToken(FILE* f, char* token, int max_len) {
    if (!skipWhitespaceAndComments(f)) return false;
    int idx = 0;
    int c;
    while ((c = std::fgetc(f)) != EOF) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f' || c == '#') {
            std::ungetc(c, f);
            break;
        }
        if (idx < max_len - 1) {
            token[idx++] = static_cast<char>(c);
        }
    }
    token[idx] = '\0';
    return idx > 0;
}

bool Image::readInt(FILE* f, int* value) {
    if (!skipWhitespaceAndComments(f)) return false;
    int c = std::fgetc(f);
    if (c == EOF) return false;

    bool negative = false;
    if (c == '-') {
        negative = true;
        c = std::fgetc(f);
    } else if (c == '+') {
        c = std::fgetc(f);
    }

    if (c < '0' || c > '9') {
        std::ungetc(c, f);
        return false;
    }

    int val = 0;
    while (c >= '0' && c <= '9') {
        val = val * 10 + (c - '0');
        c = std::fgetc(f);
    }
    if (c != EOF) {
        std::ungetc(c, f);
    }

    *value = negative ? -val : val;
    return true;
}

bool Image::load(FILE* in) {
    if (!in) return false;

    // Aumentar buffer de entrada para acelerar I/O
    std::setvbuf(in, nullptr, _IOFBF, 65536);

    char read_magic[8];
    if (!readToken(in, read_magic, sizeof(read_magic))) {
        std::fprintf(stderr, "Error: No se pudo leer el numero magico.\n");
        return false;
    }

    int ch = 0;
    if (std::strcmp(read_magic, "P2") == 0) {
        ch = 1;
    } else if (std::strcmp(read_magic, "P3") == 0) {
        ch = 3;
    } else {
        std::fprintf(stderr, "Error: Formato no soportado '%s'. Debe ser P2 (PGM) o P3 (PPM).\n", read_magic);
        return false;
    }

    int w = 0, h = 0, max_c = 0;
    if (!readInt(in, &w) || !readInt(in, &h)) {
        std::fprintf(stderr, "Error: Dimensiones de la imagen invalidas.\n");
        return false;
    }

    if (!readInt(in, &max_c)) {
        std::fprintf(stderr, "Error: Valor maximo de color invalido.\n");
        return false;
    }

    if (w <= 0 || h <= 0 || max_c <= 0) {
        std::fprintf(stderr, "Error: Dimensiones o max_color fuera de rango: %dx%d, max=%d\n", w, h, max_c);
        return false;
    }

    init(w, h, ch, max_c, read_magic);

    for (int i = 0; i < total_values; i++) {
        if (!readInt(in, &pixels[i])) {
            std::fprintf(stderr, "Error: Fin de archivo inesperado leyendo pixel %d de %d.\n", i, total_values);
            freeMemory();
            return false;
        }
    }

    return true;
}

bool Image::loadFromFile(const char* filepath) {
    if (!filepath) return false;
    FILE* f = std::fopen(filepath, "r");
    if (!f) {
        std::fprintf(stderr, "Error: No se pudo abrir el archivo de entrada '%s'.\n", filepath);
        return false;
    }
    bool ok = load(f);
    std::fclose(f);
    return ok;
}

bool Image::save(FILE* out) const {
    if (!isValid() || !out) return false;

    // Aumentar buffer de salida para acelerar escritura
    std::setvbuf(out, nullptr, _IOFBF, 65536);

    std::fprintf(out, "%s\n", magic);
    std::fprintf(out, "%d %d\n", width, height);
    std::fprintf(out, "%d\n", max_color);

    if (channels == 1) {
        for (int i = 0; i < total_values; i++) {
            std::fprintf(out, "%d\n", pixels[i]);
        }
    } else {
        for (int i = 0; i < total_values; i += 3) {
            std::fprintf(out, "%d\n%d\n%d\n", pixels[i], pixels[i + 1], pixels[i + 2]);
        }
    }

    return true;
}

bool Image::saveToFile(const char* filepath) const {
    if (!filepath) return false;
    FILE* f = std::fopen(filepath, "w");
    if (!f) {
        std::fprintf(stderr, "Error: No se pudo crear el archivo de salida '%s'.\n", filepath);
        return false;
    }
    bool ok = save(f);
    std::fclose(f);
    return ok;
}
