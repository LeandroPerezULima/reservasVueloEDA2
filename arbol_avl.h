#ifndef ARBOL_AVL_H
#define ARBOL_AVL_H

#include "reserva.h"

struct NodoAVL {
    Reserva* r;          // puntero a la reserva (compartida con la hash)
    int altura;          // altura de una hoja = 1
    NodoAVL* izq;
    NodoAVL* der;

    NodoAVL(Reserva* x) {
        r = x;
        altura = 1;
        izq = nullptr;
        der = nullptr;
    }
};

// AVL ordenado por la clave compuesta fecha + hora + idReserva.
// El arbol NO es duenio de las Reserva: solo libera sus propios nodos.
class ArbolAVL {
private:
    NodoAVL* raiz;
    int n;
    bool huboCambio;     // true si insertarRec / eliminarRec modifico el arbol
    int visitados;       // nodos visitados en la ultima consulta por rango

    // ================= ALTURA Y FACTOR DE BALANCE =================
    int altura(NodoAVL* x) {
        if (x == nullptr) { return 0; }
        return x->altura;
    }

    void actualizarAltura(NodoAVL* x) {
        int hi = altura(x->izq);
        int hd = altura(x->der);
        if (hi > hd) { x->altura = 1 + hi; }
        else         { x->altura = 1 + hd; }
    }

    // factor de balance = altura izquierda - altura derecha
    int factor(NodoAVL* x) {
        if (x == nullptr) { return 0; }
        return altura(x->izq) - altura(x->der);
    }

    // ================= LAS 2 ROTACIONES BASICAS =================
    NodoAVL* rotarDerecha(NodoAVL* y) {
        NodoAVL* x = y->izq;
        y->izq = x->der;
        x->der = y;
        actualizarAltura(y);
        actualizarAltura(x);
        return x;
    }

    NodoAVL* rotarIzquierda(NodoAVL* x) {
        NodoAVL* y = x->der;
        x->der = y->izq;
        y->izq = x;
        actualizarAltura(x);
        actualizarAltura(y);
        return y;
    }

    // ================= LOS 4 CASOS DE DESBALANCE =================
    NodoAVL* casoLL(NodoAVL* x) { return rotarDerecha(x); }
    NodoAVL* casoRR(NodoAVL* x) { return rotarIzquierda(x); }

    NodoAVL* casoLR(NodoAVL* x) {
        x->izq = rotarIzquierda(x->izq);
        return rotarDerecha(x);
    }

    NodoAVL* casoRL(NodoAVL* x) {
        x->der = rotarDerecha(x->der);
        return rotarIzquierda(x);
    }

    NodoAVL* balancear(NodoAVL* x) {
        actualizarAltura(x);
        int fb = factor(x);

        if (fb > 1) {
            if (factor(x->izq) >= 0) { return casoLL(x); }
            return casoLR(x);
        }
        if (fb < -1) {
            if (factor(x->der) <= 0) { return casoRR(x); }
            return casoRL(x);
        }
        return x;
    }

    // ================= INSERTAR =================
    NodoAVL* insertarRec(NodoAVL* x, Reserva* r) {
        if (x == nullptr) {
            huboCambio = true;
            return new NodoAVL(r);
        }
        int c = compararClave(r, x->r);
        if (c < 0)      { x->izq = insertarRec(x->izq, r); }
        else if (c > 0) { x->der = insertarRec(x->der, r); }
        else            { return x; }                       // clave repetida
        return balancear(x);
    }

    // ================= ELIMINAR =================
    NodoAVL* minimoNodo(NodoAVL* x) {
        while (x->izq != nullptr) { x = x->izq; }
        return x;
    }

    // elimina el nodo cuya clave es la de r (NO hace delete de la Reserva)
    NodoAVL* eliminarRec(NodoAVL* x, Reserva* r) {
        if (x == nullptr) { return nullptr; }

        int c = compararClave(r, x->r);
        if (c < 0)      { x->izq = eliminarRec(x->izq, r); }
        else if (c > 0) { x->der = eliminarRec(x->der, r); }
        else {
            if (x->izq == nullptr || x->der == nullptr) {   // 0 o 1 hijo
                NodoAVL* hijo = x->izq;
                if (hijo == nullptr) { hijo = x->der; }
                delete x;                                   // solo el nodo
                huboCambio = true;
                return hijo;
            }
            NodoAVL* sucesor = minimoNodo(x->der);          // 2 hijos
            x->r = sucesor->r;
            x->der = eliminarRec(x->der, sucesor->r);
        }
        return balancear(x);
    }

    // ================= RECORRIDOS =================
    // recorrido en orden, se detiene al llegar a k elementos
    void primerosRec(NodoAVL* x, int k, int& cont, bool imprimir) {
        if (x == nullptr || cont >= k) { return; }
        primerosRec(x->izq, k, cont, imprimir);
        if (cont < k) {
            if (imprimir) { mostrarReserva(x->r); }
            cont++;
        }
        primerosRec(x->der, k, cont, imprimir);
    }

    // consulta por rango de fechas: DESCARTA las ramas fuera del rango
    void rangoRec(NodoAVL* x, const string& ini, const string& fin,
                  int& total, int& impresos, int maxMuestra, bool imprimir) {
        if (x == nullptr) { return; }
        visitados++;
        const string& f = x->r->fecha;

        if (f >= ini) {                      // si f < ini, todo el subarbol izq queda fuera
            rangoRec(x->izq, ini, fin, total, impresos, maxMuestra, imprimir);
        }
        if (f >= ini && f <= fin) {
            total++;
            if (imprimir && impresos < maxMuestra) {
                mostrarReserva(x->r);
                impresos++;
            }
        }
        if (f <= fin) {                      // si f > fin, todo el subarbol der queda fuera
            rangoRec(x->der, ini, fin, total, impresos, maxMuestra, imprimir);
        }
    }

    // verifica orden BST, alturas y factor de balance
    bool verificarRec(NodoAVL* x, Reserva*& previo, int& h) {
        if (x == nullptr) { h = 0; return true; }
        int hi = 0, hd = 0;
        if (!verificarRec(x->izq, previo, hi)) { return false; }
        if (previo != nullptr && compararClave(previo, x->r) >= 0) { return false; }
        previo = x->r;
        if (!verificarRec(x->der, previo, hd)) { return false; }
        h = 1 + (hi > hd ? hi : hd);
        if (h != x->altura) { return false; }
        int d = hi - hd;
        return d >= -1 && d <= 1;
    }

    void liberar(NodoAVL* x) {
        if (x == nullptr) { return; }
        liberar(x->izq);
        liberar(x->der);
        delete x;
    }

public:
    ArbolAVL() {
        raiz = nullptr;
        n = 0;
        huboCambio = false;
        visitados = 0;
    }

    ~ArbolAVL() {
        liberar(raiz);
    }

    bool insertar(Reserva* r) {
        huboCambio = false;
        raiz = insertarRec(raiz, r);
        if (huboCambio) { n++; }
        return huboCambio;
    }

    bool eliminar(Reserva* r) {
        huboCambio = false;
        raiz = eliminarRec(raiz, r);
        if (huboCambio) { n--; }
        return huboCambio;
    }

    int cantidad() const { return n; }
    int altura() { return altura(raiz); }
    int nodosVisitados() const { return visitados; }

    // primeras k reservas en orden cronologico; devuelve cuantas recorrio
    int listarPrimeros(int k, bool imprimir) {
        int cont = 0;
        primerosRec(raiz, k, cont, imprimir);
        return cont;
    }

    // reservas con fecha en [ini, fin]; devuelve la cantidad encontrada
    int consultarRango(const string& ini, const string& fin, int maxMuestra, bool imprimir) {
        int total = 0, impresos = 0;
        visitados = 0;
        rangoRec(raiz, ini, fin, total, impresos, maxMuestra, imprimir);
        return total;
    }

    bool esValido() {
        Reserva* previo = nullptr;
        int h = 0;
        return verificarRec(raiz, previo, h);
    }
};

#endif
