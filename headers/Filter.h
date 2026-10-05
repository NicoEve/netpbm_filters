#ifndef FILTER_H
#define FILTER_H

#include "Image.h"

/*
 * Jerarquia de clases Filter (Paradigma Orientado a Objetos)
 * Sigue las directrices del proyecto:
 * - Modular y extensible para soportar nuevos filtros y futuras paralelizaciones.
 * - Sin uso de std::vector ni std::string (punteros y arreglos de C).
 */

class Filter {
public:
    virtual ~Filter() {}
    virtual const char* getName() const = 0;
    virtual bool apply(const Image& src, Image& dst) const = 0;
};

// Filtro de Suavizado (Blur - Promedio 3x3)
class BlurFilter : public Filter {
public:
    virtual const char* getName() const;
    virtual bool apply(const Image& src, Image& dst) const;
};

// Filtro de Laplace (Deteccion de bordes laplaciano 3x3)
class LaplaceFilter : public Filter {
public:
    virtual const char* getName() const;
    virtual bool apply(const Image& src, Image& dst) const;
};

// Filtro de Realce (Sharpening 3x3)
class SharpenFilter : public Filter {
public:
    virtual const char* getName() const;
    virtual bool apply(const Image& src, Image& dst) const;
};

// Factoria de creacion de filtros
Filter* createFilterByName(const char* name);

#endif // FILTER_H
