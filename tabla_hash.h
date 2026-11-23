#ifndef TABLA_HASH_H
#define TABLA_HASH_H

#include "reserva.h"

struct NodoHash {
    Reserva* r;
    NodoHash* sig;
};

// Tabla hash con encadenamiento. Clave: idReserva.
// La tabla es la DUENA de los objetos Reserva: su destructor los libera.
class TablaHash {
private:
    NodoHash** tabla;
    int cap;
    int n;

public:
    TablaHash(int capacidad = 10007) {     // 10007 es primo, cercano a 10,000
        cap = capacidad;
        n = 0;
        tabla = new NodoHash*[cap];
        for (int i = 0; i < cap; i++) { tabla[i] = nullptr; }
    }

    ~TablaHash() {
        for (int i = 0; i < cap; i++) {
            NodoHash* actual = tabla[i];
            while (actual != nullptr) {
                NodoHash* sig = actual->sig;
                delete actual->r;
                delete actual;
                actual = sig;
            }
        }
        delete[] tabla;
    }

    // funcion hash polinomial (djb2) sobre el string del ID, modulo cap
    int indice(const string& id) const {
        unsigned long h = 5381;
        for (size_t i = 0; i < id.size(); i++) {
            h = h * 33 + (unsigned char)id[i];
        }
        return (int)(h % cap);
    }

    // false si el ID ya existe (el que llama sigue siendo duenio de r)
    bool insertar(Reserva* r) {
        if (buscar(r->idReserva) != nullptr) { return false; }
        int idx = indice(r->idReserva);
        NodoHash* nuevo = new NodoHash();
        nuevo->r = r;
        nuevo->sig = tabla[idx];           // al inicio de la lista de ese indice
        tabla[idx] = nuevo;
        n++;
        return true;
    }

    Reserva* buscar(const string& id) const {
        NodoHash* actual = tabla[indice(id)];
        while (actual != nullptr) {
            if (actual->r->idReserva == id) { return actual->r; }
            actual = actual->sig;
        }
        return nullptr;
    }

    // quita el nodo Y libera la Reserva (llamar DESPUES de sacarla del arbol)
    bool eliminar(const string& id) {
        int idx = indice(id);
        NodoHash* actual = tabla[idx];
        NodoHash* anterior = nullptr;
        while (actual != nullptr) {
            if (actual->r->idReserva == id) {
                if (anterior == nullptr) { tabla[idx] = actual->sig; }
                else                     { anterior->sig = actual->sig; }
                delete actual->r;
                delete actual;
                n--;
                return true;
            }
            anterior = actual;
            actual = actual->sig;
        }
        return false;
    }

    int cantidad() const  { return n; }
    int capacidad() const { return cap; }
    double factorCarga() const { return (double)n / cap; }
    NodoHash* cabeza(int i) const { return tabla[i]; }

    int cadenaMasLarga() const {
        int maxLen = 0;
        for (int i = 0; i < cap; i++) {
            int len = 0;
            for (NodoHash* a = tabla[i]; a != nullptr; a = a->sig) { len++; }
            if (len > maxLen) { maxLen = len; }
        }
        return maxLen;
    }
};

#endif
