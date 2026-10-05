#ifndef FILTER_H
#define FILTER_H

#include "Image.h"

/*
 * Jerarquia de clases Filter (Paradigma Orientado a Objetos)
 * Soporta ejecucion secuencial, por regiones (Pthreads) y paralela (OpenMP).
 * Sin uso de std::vector ni std::string.
 */

class Filter {
public:
    virtual ~Filter() {}
    virtual const char* getName() const = 0;
    
    // Aplicacion secuencial sobre toda la imagen
    virtual bool apply(const Image& src, Image& dst) const = 0;

    // Aplicacion sobre una subregion rectangular [x_start, x_end) x [y_start, y_end)
    // Utilizado para la descomposicion en 4 cuadrantes con Pthreads
    virtual bool applyRegion(const Image& src, Image& dst, 
                             int x_start, int x_end, 
                             int y_start, int y_end) const = 0;

    // Aplicacion paralela con OpenMP
    virtual bool applyOMP(const Image& src, Image& dst) const = 0;
};

// Filtro de Suavizado (Blur - Promedio 3x3)
class BlurFilter : public Filter {
public:
    virtual const char* getName() const;
    virtual bool apply(const Image& src, Image& dst) const;
    virtual bool applyRegion(const Image& src, Image& dst, 
                             int x_start, int x_end, 
                             int y_start, int y_end) const;
    virtual bool applyOMP(const Image& src, Image& dst) const;
};

// Filtro de Laplace (Deteccion de bordes laplaciano 3x3)
class LaplaceFilter : public Filter {
public:
    virtual const char* getName() const;
    virtual bool apply(const Image& src, Image& dst) const;
    virtual bool applyRegion(const Image& src, Image& dst, 
                             int x_start, int x_end, 
                             int y_start, int y_end) const;
    virtual bool applyOMP(const Image& src, Image& dst) const;
};

// Filtro de Realce (Sharpening 3x3)
class SharpenFilter : public Filter {
public:
    virtual const char* getName() const;
    virtual bool apply(const Image& src, Image& dst) const;
    virtual bool applyRegion(const Image& src, Image& dst, 
                             int x_start, int x_end, 
                             int y_start, int y_end) const;
    virtual bool applyOMP(const Image& src, Image& dst) const;
};

// Factoria de creacion de filtros
Filter* createFilterByName(const char* name);

#endif // FILTER_H
