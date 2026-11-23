#ifndef SISTEMA_H
#define SISTEMA_H

#include <fstream>
#include <sstream>
#include <cstdlib>
#include "reserva.h"
#include "tabla_hash.h"
#include "arbol_avl.h"
#include "heap.h"

const int MAX_VUELOS = 64;

struct Vuelo {
    string codigo, origen, destino, fecha, hora;
    int capacidad;
    int ocupados;        // reservas Confirmada + Pendiente + CheckIn
    MaxHeap espera;      // lista de espera de este vuelo

    Vuelo() {
        capacidad = 0;
        ocupados = 0;
    }
};

// codigos de retorno de las operaciones
const int OP_NO_EXISTE = 0;
const int OP_OK = 1;
const int OP_INVALIDA = 2;
const int OP_CASO_A = 2;     // cancelacion: libero asiento
const int OP_CASO_B = 3;     // cancelacion: estaba en espera
const int OP_YA_CANCELADA = 4;

class Sistema {
public:
    TablaHash hash;
    ArbolAVL arbol;
    Vuelo vuelos[MAX_VUELOS];
    int nVuelos;
    int contadorLlegada;

    Sistema() : hash(10007) {
        nVuelos = 0;
        contadorLlegada = 0;
    }

    // ---------- utilidades ----------
    int buscarVuelo(const string& codigo) {
        for (int i = 0; i < nVuelos; i++) {
            if (vuelos[i].codigo == codigo) { return i; }
        }
        return -1;
    }

    // separa una linea CSV por comas; devuelve la cantidad de campos
    static int dividir(const string& linea, string campos[], int maxCampos) {
        stringstream ss(linea);
        string campo;
        int n = 0;
        while (n < maxCampos && getline(ss, campo, ',')) {
            if (!campo.empty() && campo[campo.size() - 1] == '\r') {
                campo.erase(campo.size() - 1);
            }
            campos[n++] = campo;
        }
        return n;
    }

    int totalEnEspera() {
        int t = 0;
        for (int i = 0; i < nVuelos; i++) { t += vuelos[i].espera.tamano(); }
        return t;
    }

    // ---------- 1. carga ----------
    int cargarVuelos(const string& archivo) {
        ifstream f(archivo.c_str());
        if (!f.is_open()) { return -1; }
        string linea, c[6];
        while (getline(f, linea)) {
            if (linea.empty()) { continue; }
            if (dividir(linea, c, 6) < 6) { continue; }
            if (c[0] == "codigoVuelo") { continue; }            // encabezado
            if (nVuelos >= MAX_VUELOS) { break; }
            Vuelo& v = vuelos[nVuelos];
            v.codigo = c[0]; v.origen = c[1]; v.destino = c[2];
            v.fecha = c[3];  v.hora = c[4];
            v.capacidad = atoi(c[5].c_str());
            nVuelos++;
        }
        return nVuelos;
    }

    // Inserta la reserva en hash + arbol (+ heap si queda EnEspera).
    // Si el vuelo esta lleno, una reserva que pedia asiento pasa a EnEspera.
    // Si falla, devuelve false y r sigue siendo del que llama (debe liberarla).
    bool insertarReserva(Reserva* r, string& error) {
        int iv = buscarVuelo(r->codigoVuelo);
        if (iv < 0) { error = "el vuelo no existe"; return false; }
        if (!estadoValido(r->estado)) { error = "estado invalido"; return false; }
        if (hash.buscar(r->idReserva) != nullptr) { error = "idReserva duplicado"; return false; }

        Vuelo& v = vuelos[iv];
        if (ocupaAsiento(r->estado)) {
            if (v.ocupados >= v.capacidad) { r->estado = "EnEspera"; }
            else                           { v.ocupados++; }
        }

        hash.insertar(r);
        arbol.insertar(r);
        if (r->estado == "EnEspera") {
            r->ordenLlegada = ++contadorLlegada;
            v.espera.insertar(r);
        }
        return true;
    }

    void cargarReservas(const string& archivo, int& leidas, int& rechazadas, bool& abierto) {
        leidas = 0;
        rechazadas = 0;
        ifstream f(archivo.c_str());
        abierto = f.is_open();
        if (!abierto) { return; }

        string linea, c[12], error;
        while (getline(f, linea)) {
            if (linea.empty()) { continue; }
            if (dividir(linea, c, 12) < 12) { rechazadas++; continue; }
            if (c[0] == "idReserva") { continue; }
            leidas++;
            Reserva* r = new Reserva();
            r->idReserva = c[0];  r->dni = c[1];       r->pasajero = c[2];
            r->codigoVuelo = c[3]; r->origen = c[4];   r->destino = c[5];
            r->fecha = c[6];       r->hora = c[7];     r->clase = c[8];
            r->categoria = c[9];   r->precio = atof(c[10].c_str());
            r->estado = c[11];
            if (!insertarReserva(r, error)) {
                delete r;
                rechazadas++;
                if (rechazadas <= 5) { cout << "  [rechazada] " << c[0] << ": " << error << endl; }
            }
        }
    }

    // ---------- 2. registro manual ----------
    // estadoInicial: "Confirmada" o "Pendiente"; si no hay cupo queda EnEspera
    bool registrar(Reserva* r, string& error) {
        return insertarReserva(r, error);
    }

    // ---------- 6. actualizar estado (Pendiente->Confirmada, Confirmada->CheckIn) ----------
    int actualizarEstado(const string& id, const string& nuevo) {
        Reserva* r = hash.buscar(id);
        if (r == nullptr) { return OP_NO_EXISTE; }
        bool ok = (r->estado == "Pendiente" && nuevo == "Confirmada") ||
                  (r->estado == "Confirmada" && nuevo == "CheckIn");
        if (!ok) { return OP_INVALIDA; }
        r->estado = nuevo;          // puntero compartido: se ve en hash, arbol y heap
        return OP_OK;
    }

    // ---------- 7. cancelar ----------
    // Caso A: ocupaba asiento -> se libera y se promueve al primero de la espera
    // Caso B: estaba EnEspera -> se retira del heap desde cualquier posicion
    int cancelar(const string& id, Reserva** promovido) {
        if (promovido != nullptr) { *promovido = nullptr; }
        Reserva* r = hash.buscar(id);
        if (r == nullptr) { return OP_NO_EXISTE; }
        if (r->estado == "Cancelada") { return OP_YA_CANCELADA; }

        int iv = buscarVuelo(r->codigoVuelo);
        Vuelo& v = vuelos[iv];

        if (r->estado == "EnEspera") {                       // Caso B
            v.espera.eliminar(r);
            r->estado = "Cancelada";
            return OP_CASO_B;
        }

        r->estado = "Cancelada";                             // Caso A
        v.ocupados--;
        Reserva* p = v.espera.extraerMax();                  // mayor prioridad
        if (p != nullptr) {
            p->estado = "Confirmada";
            v.ocupados++;
            if (promovido != nullptr) { *promovido = p; }
        }
        return OP_CASO_A;
    }

    // ---------- 8. eliminacion fisica ----------
    int eliminarFisica(const string& id) {
        Reserva* r = hash.buscar(id);
        if (r == nullptr) { return OP_NO_EXISTE; }
        if (r->estado != "Cancelada") { return OP_INVALIDA; }
        arbol.eliminar(r);       // primero el arbol (necesita los campos de la clave)
        hash.eliminar(id);       // al final: quita el nodo y hace delete de la Reserva
        return OP_OK;
    }

    // ---------- 9. lista de espera ----------
    void mostrarEspera(int iv) {
        Vuelo& v = vuelos[iv];
        cout << "Vuelo " << v.codigo << " (" << v.origen << " -> " << v.destino << ") "
             << v.fecha << " " << v.hora << " | capacidad " << v.capacidad
             << " | ocupados " << v.ocupados << " | en espera " << v.espera.tamano() << endl;
        if (v.espera.vacio()) { cout << "  (lista de espera vacia)" << endl; return; }

        Reserva* sig = v.espera.verMax();
        cout << "Siguiente a atender (raiz del heap):" << endl;
        mostrarEncabezado();
        mostrarReserva(sig);

        cout << "\nLista completa en orden de prioridad (sobre una copia del heap):" << endl;
        cout << left << setw(5) << "#";
        mostrarEncabezado();
        MaxHeap copia(v.espera);
        int pos = 1;
        while (!copia.vacio()) {
            Reserva* r = copia.extraerMax();
            cout << left << setw(5) << pos++;
            mostrarReserva(r);
        }
    }

    // ---------- 10. estadisticas ----------
    void estadisticas() {
        const int MAXD = 64;
        string dest[MAXD];
        int cdest[MAXD];
        int nd = 0;
        string clases[3] = {"Economica", "Premium", "Ejecutiva"};
        int cclase[3] = {0, 0, 0};
        string estados[5] = {"Confirmada", "Pendiente", "EnEspera", "CheckIn", "Cancelada"};
        int cestado[5] = {0, 0, 0, 0, 0};

        int total = 0;
        double ingresos = 0, suma = 0;
        Reserva* mayorP = nullptr;
        Reserva* menorP = nullptr;

        for (int i = 0; i < hash.capacidad(); i++) {
            for (NodoHash* a = hash.cabeza(i); a != nullptr; a = a->sig) {
                Reserva* r = a->r;
                total++;
                suma += r->precio;
                if (r->estado == "Confirmada" || r->estado == "CheckIn") { ingresos += r->precio; }
                if (mayorP == nullptr || r->precio > mayorP->precio) { mayorP = r; }
                if (menorP == nullptr || r->precio < menorP->precio) { menorP = r; }

                int k = -1;
                for (int j = 0; j < nd; j++) { if (dest[j] == r->destino) { k = j; break; } }
                if (k < 0 && nd < MAXD) { dest[nd] = r->destino; cdest[nd] = 0; k = nd++; }
                if (k >= 0) { cdest[k]++; }

                for (int j = 0; j < 3; j++) { if (clases[j] == r->clase) { cclase[j]++; } }
                for (int j = 0; j < 5; j++) { if (estados[j] == r->estado) { cestado[j]++; } }
            }
        }

        cout << fixed << setprecision(2);
        cout << "Total de reservas      : " << total << endl;
        cout << "Ingresos (Conf.+CheckIn): " << ingresos << endl;
        cout << "Precio promedio        : " << (total > 0 ? suma / total : 0.0) << endl;
        cout << "\nPor destino:" << endl;
        for (int j = 0; j < nd; j++) { cout << "  " << left << setw(12) << dest[j] << cdest[j] << endl; }
        cout << "Por clase:" << endl;
        for (int j = 0; j < 3; j++) { cout << "  " << left << setw(12) << clases[j] << cclase[j] << endl; }
        cout << "Por estado:" << endl;
        for (int j = 0; j < 5; j++) { cout << "  " << left << setw(12) << estados[j] << cestado[j] << endl; }

        if (mayorP != nullptr) {
            cout << "\nReserva de MAYOR precio:" << endl;
            mostrarEncabezado(); mostrarReserva(mayorP);
            cout << "Reserva de MENOR precio:" << endl;
            mostrarEncabezado(); mostrarReserva(menorP);
        }

        int mejor = -1;
        for (int i = 0; i < nVuelos; i++) {
            if (mejor < 0 || vuelos[i].espera.tamano() > vuelos[mejor].espera.tamano()) { mejor = i; }
        }
        if (mejor >= 0 && vuelos[mejor].espera.tamano() > 0) {
            cout << "\nVuelo con mas pasajeros en espera: " << vuelos[mejor].codigo
                 << " (" << vuelos[mejor].espera.tamano() << ")" << endl;
        } else {
            cout << "\nNo hay pasajeros en espera." << endl;
        }
    }

    // ---------- verificacion de consistencia entre las 3 estructuras ----------
    bool verificarConsistencia(bool imprimir) {
        int ocupados[MAX_VUELOS];
        for (int i = 0; i < MAX_VUELOS; i++) { ocupados[i] = 0; }
        int enEspera = 0, enHash = 0;
        bool ok = true;

        for (int i = 0; i < hash.capacidad(); i++) {
            for (NodoHash* a = hash.cabeza(i); a != nullptr; a = a->sig) {
                Reserva* r = a->r;
                enHash++;
                int iv = buscarVuelo(r->codigoVuelo);
                if (iv < 0) { ok = false; continue; }
                if (ocupaAsiento(r->estado)) { ocupados[iv]++; }
                if (r->estado == "EnEspera") {
                    enEspera++;
                    if (vuelos[iv].espera.buscarPosicion(r) < 0) { ok = false; }   // EnEspera debe estar en su heap
                }
            }
        }

        bool cHash = (enHash == arbol.cantidad());
        bool cEsp = (enEspera == totalEnEspera());
        bool cAsientos = true, cHeaps = true;
        for (int i = 0; i < nVuelos; i++) {
            if (ocupados[i] != vuelos[i].ocupados) { cAsientos = false; }
            if (!vuelos[i].espera.esValido()) { cHeaps = false; }
        }
        bool cAVL = arbol.esValido();
        ok = ok && cHash && cEsp && cAsientos && cHeaps && cAVL;

        if (imprimir) {
            cout << "  Reservas en hash = reservas en arbol (" << enHash << " / " << arbol.cantidad() << "): " << (cHash ? "OK" : "FALLA") << endl;
            cout << "  EnEspera = elementos en heaps (" << enEspera << " / " << totalEnEspera() << "), todos en su heap: " << (cEsp && ok ? "OK" : "FALLA") << endl;
            cout << "  Asientos ocupados por vuelo coinciden con la hash: " << (cAsientos ? "OK" : "FALLA") << endl;
            cout << "  Todos los heaps cumplen la propiedad: " << (cHeaps ? "OK" : "FALLA") << endl;
            cout << "  AVL balanceado y ordenado por clave compuesta: " << (cAVL ? "OK" : "FALLA") << endl;
        }
        return ok;
    }
};

#endif
