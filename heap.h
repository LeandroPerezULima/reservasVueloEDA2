#ifndef HEAP_H
#define HEAP_H

#include "reserva.h"

// Max-heap en arreglo: lista de espera de un vuelo.
// Prioridad: primero por categoria (Premium > Frecuente > Regular);
// en empate, el que llego antes (menor ordenLlegada) tiene mayor prioridad.
class MaxHeap {
private:
    Reserva** arr;
    int n;
    int cap;

    static int padre(int i)     { return (i - 1) / 2; }
    static int izquierdo(int i) { return 2 * i + 1; }
    static int derecho(int i)   { return 2 * i + 2; }

    // true si a tiene MAYOR prioridad que b
    static bool mayor(Reserva* a, Reserva* b) {
        int ca = rangoCategoria(a->categoria);
        int cb = rangoCategoria(b->categoria);
        if (ca != cb) { return ca > cb; }
        return a->ordenLlegada < b->ordenLlegada;
    }

    void intercambiar(int i, int j) {
        Reserva* t = arr[i];
        arr[i] = arr[j];
        arr[j] = t;
    }

    // Sube el elemento i mientras tenga mayor prioridad que su padre
    void heapifyUp(int i) {
        while (i > 0 && mayor(arr[i], arr[padre(i)])) {
            intercambiar(i, padre(i));
            i = padre(i);
        }
    }

    // Baja el elemento i intercambiandolo con su hijo de mayor prioridad
    void heapifyDown(int i) {
        while (true) {
            int m = i;
            int l = izquierdo(i);
            int r = derecho(i);
            if (l < n && mayor(arr[l], arr[m])) { m = l; }
            if (r < n && mayor(arr[r], arr[m])) { m = r; }
            if (m == i) { break; }
            intercambiar(i, m);
            i = m;
        }
    }

    void crecer() {
        int nuevaCap = cap * 2;
        Reserva** nuevo = new Reserva*[nuevaCap];
        for (int i = 0; i < n; i++) { nuevo[i] = arr[i]; }
        delete[] arr;
        arr = nuevo;
        cap = nuevaCap;
    }

public:
    MaxHeap() {
        cap = 16;
        n = 0;
        arr = new Reserva*[cap];
    }

    // copia profunda del arreglo (los Reserva* se comparten): sirve para
    // listar la espera en orden sin alterar el heap original
    MaxHeap(const MaxHeap& o) {
        cap = o.cap;
        n = o.n;
        arr = new Reserva*[cap];
        for (int i = 0; i < n; i++) { arr[i] = o.arr[i]; }
    }

    MaxHeap& operator=(const MaxHeap&) = delete;

    ~MaxHeap() {
        delete[] arr;
    }

    int tamano() const { return n; }
    bool vacio() const { return n == 0; }

    void insertar(Reserva* r) {
        if (n == cap) { crecer(); }
        arr[n] = r;
        n++;
        heapifyUp(n - 1);
    }

    Reserva* verMax() const {
        if (n == 0) { return nullptr; }
        return arr[0];
    }

    // EXTRACCION: saca y devuelve el de mayor prioridad
    Reserva* extraerMax() {
        if (n == 0) { return nullptr; }
        Reserva* max = arr[0];
        arr[0] = arr[n - 1];                  // el ultimo pasa a la raiz
        n--;
        if (n > 0) { heapifyDown(0); }
        return max;
    }

    // busqueda secuencial de la posicion de r (la espera de un vuelo es pequena)
    int buscarPosicion(Reserva* r) const {
        for (int i = 0; i < n; i++) {
            if (arr[i] == r) { return i; }
        }
        return -1;
    }

    // elimina el elemento de la posicion i (cualquiera), reordenando hacia
    // arriba o hacia abajo segun corresponda
    void eliminarEnPosicion(int i) {
        if (i < 0 || i >= n) { return; }
        arr[i] = arr[n - 1];
        n--;
        if (i < n) {
            if (i > 0 && mayor(arr[i], arr[padre(i)])) { heapifyUp(i); }
            else                                       { heapifyDown(i); }
        }
    }

    bool eliminar(Reserva* r) {
        int i = buscarPosicion(r);
        if (i < 0) { return false; }
        eliminarEnPosicion(i);
        return true;
    }

    // comprueba la propiedad del heap (ningun hijo supera a su padre)
    bool esValido() const {
        for (int i = 1; i < n; i++) {
            if (mayor(arr[i], arr[padre(i)])) { return false; }
        }
        return true;
    }
};

#endif
