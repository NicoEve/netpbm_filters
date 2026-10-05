#include "../headers/Filter.h"
#include <cstring>
#include <cstdlib>
#include <cmath>

// ---------------------------------------------------------------------
// BlurFilter (Filtro de suavizado por promedio 3x3)
// ---------------------------------------------------------------------
const char* BlurFilter::getName() const {
    return "blur";
}

bool BlurFilter::apply(const Image& src, Image& dst) const {
    if (!src.isValid()) return false;

    int width = src.getWidth();
    int height = src.getHeight();
    int channels = src.getChannels();
    int max_c = src.getMaxColor();

    if (!dst.init(width, height, channels, max_c, src.getMagic())) {
        return false;
    }

    const int* src_pixels = src.getPixels();
    int* dst_pixels = dst.getPixels();

    const float kernel_weight = 1.0f / 9.0f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < channels; c++) {
                float sum = 0.0f;
                float weight_sum = 0.0f;

                for (int ky = -1; ky <= 1; ky++) {
                    int ny = y + ky;
                    if (ny < 0 || ny >= height) continue;

                    for (int kx = -1; kx <= 1; kx++) {
                        int nx = x + kx;
                        if (nx < 0 || nx >= width) continue;

                        int idx = (ny * width + nx) * channels + c;
                        sum += src_pixels[idx] * kernel_weight;
                        weight_sum += kernel_weight;
                    }
                }

                int result = (weight_sum > 0.0f) ? static_cast<int>((sum / weight_sum) + 0.5f) : 0;
                if (result < 0) result = 0;
                if (result > max_c) result = max_c;

                dst_pixels[(y * width + x) * channels + c] = result;
            }
        }
    }
    return true;
}

// ---------------------------------------------------------------------
// LaplaceFilter (Filtro Laplaciano de deteccion de bordes 3x3)
// ---------------------------------------------------------------------
const char* LaplaceFilter::getName() const {
    return "laplace";
}

bool LaplaceFilter::apply(const Image& src, Image& dst) const {
    if (!src.isValid()) return false;

    int width = src.getWidth();
    int height = src.getHeight();
    int channels = src.getChannels();
    int max_c = src.getMaxColor();

    if (!dst.init(width, height, channels, max_c, src.getMagic())) {
        return false;
    }

    const int* src_pixels = src.getPixels();
    int* dst_pixels = dst.getPixels();

    // Kernel Laplaciano estandar
    const int laplace_k[3][3] = {
        { 0,  1,  0 },
        { 1, -4,  1 },
        { 0,  1,  0 }
    };

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < channels; c++) {
                int sum = 0;

                for (int ky = -1; ky <= 1; ky++) {
                    int ny = y + ky;
                    if (ny < 0) ny = 0;
                    else if (ny >= height) ny = height - 1;

                    for (int kx = -1; kx <= 1; kx++) {
                        int nx = x + kx;
                        if (nx < 0) nx = 0;
                        else if (nx >= width) nx = width - 1;

                        int idx = (ny * width + nx) * channels + c;
                        sum += src_pixels[idx] * laplace_k[ky + 1][kx + 1];
                    }
                }

                // En deteccion de bordes se toma el valor absoluto para destacar gradientes positivos y negativos
                int result = (sum < 0) ? -sum : sum;
                if (result > max_c) result = max_c;

                dst_pixels[(y * width + x) * channels + c] = result;
            }
        }
    }
    return true;
}

// ---------------------------------------------------------------------
// SharpenFilter (Filtro de realce de nitidez 3x3)
// ---------------------------------------------------------------------
const char* SharpenFilter::getName() const {
    return "sharpen";
}

bool SharpenFilter::apply(const Image& src, Image& dst) const {
    if (!src.isValid()) return false;

    int width = src.getWidth();
    int height = src.getHeight();
    int channels = src.getChannels();
    int max_c = src.getMaxColor();

    if (!dst.init(width, height, channels, max_c, src.getMagic())) {
        return false;
    }

    const int* src_pixels = src.getPixels();
    int* dst_pixels = dst.getPixels();

    // Kernel de Realce (Suma de pesos = 1 para preservar el brillo global)
    const int sharpen_k[3][3] = {
        {  0, -1,  0 },
        { -1,  5, -1 },
        {  0, -1,  0 }
    };

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < channels; c++) {
                int sum = 0;

                for (int ky = -1; ky <= 1; ky++) {
                    int ny = y + ky;
                    if (ny < 0) ny = 0;
                    else if (ny >= height) ny = height - 1;

                    for (int kx = -1; kx <= 1; kx++) {
                        int nx = x + kx;
                        if (nx < 0) nx = 0;
                        else if (nx >= width) nx = width - 1;

                        int idx = (ny * width + nx) * channels + c;
                        sum += src_pixels[idx] * sharpen_k[ky + 1][kx + 1];
                    }
                }

                int result = sum;
                if (result < 0) result = 0;
                if (result > max_c) result = max_c;

                dst_pixels[(y * width + x) * channels + c] = result;
            }
        }
    }
    return true;
}

// ---------------------------------------------------------------------
// Factoria de Filtros
// ---------------------------------------------------------------------
Filter* createFilterByName(const char* name) {
    if (!name) return nullptr;

    if (std::strcmp(name, "blur") == 0 || std::strcmp(name, "suavizado") == 0) {
        return new BlurFilter();
    }
    if (std::strcmp(name, "laplace") == 0 || std::strcmp(name, "laplaciano") == 0) {
        return new LaplaceFilter();
    }
    if (std::strcmp(name, "sharpen") == 0 || std::strcmp(name, "sharpening") == 0 || std::strcmp(name, "realce") == 0) {
        return new SharpenFilter();
    }
    return nullptr;
}
