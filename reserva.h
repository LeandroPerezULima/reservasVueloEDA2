#ifndef RESERVA_H
#define RESERVA_H

#include <iostream>
#include <iomanip>
#include <string>
using namespace std;

// Registro unico de una reserva. Se crea UNA vez con new y la hash, el arbol y
// el heap guardan punteros (Reserva*) a este mismo objeto.
struct Reserva {
    string idReserva, dni, pasajero, codigoVuelo, origen, destino;
    string fecha, hora, clase, categoria;
    double precio;
    string estado;
    int ordenLlegada;        // orden de llegada a la lista de espera (desempate del heap)

    Reserva() {
        precio = 0;
        ordenLlegada = 0;
    }
};

// Premium > Frecuente > Regular
inline int rangoCategoria(const string& c) {
    if (c == "Premium")   { return 3; }
    if (c == "Frecuente") { return 2; }
    return 1;
}

// Confirmada, Pendiente y CheckIn ocupan asiento
inline bool ocupaAsiento(const string& e) {
    return e == "Confirmada" || e == "Pendiente" || e == "CheckIn";
}

inline bool estadoValido(const string& e) {
    return ocupaAsiento(e) || e == "EnEspera" || e == "Cancelada";
}

// Clave compuesta del arbol: fecha + hora + idReserva
// devuelve -1 si a < b, 0 si son iguales, 1 si a > b
inline int compararClave(const Reserva* a, const Reserva* b) {
    int c = a->fecha.compare(b->fecha);
    if (c != 0) { return c < 0 ? -1 : 1; }
    c = a->hora.compare(b->hora);
    if (c != 0) { return c < 0 ? -1 : 1; }
    c = a->idReserva.compare(b->idReserva);
    if (c != 0) { return c < 0 ? -1 : 1; }
    return 0;
}

inline void mostrarEncabezado() {
    cout << left << setw(11) << "ID" << setw(17) << "Fecha/Hora" << setw(8) << "Vuelo"
         << setw(20) << "Pasajero" << setw(10) << "Clase" << setw(10) << "Categ."
         << setw(11) << "Estado" << "Precio" << endl;
}

inline void mostrarReserva(const Reserva* r) {
    cout << left << setw(11) << r->idReserva
         << setw(17) << (r->fecha + " " + r->hora)
         << setw(8) << r->codigoVuelo
         << setw(20) << r->pasajero
         << setw(10) << r->clase
         << setw(10) << r->categoria
         << setw(11) << r->estado
         << fixed << setprecision(2) << r->precio << endl;
}

inline void mostrarDetalle(const Reserva* r) {
    cout << "  idReserva : " << r->idReserva << endl;
    cout << "  DNI       : " << r->dni << endl;
    cout << "  Pasajero  : " << r->pasajero << endl;
    cout << "  Vuelo     : " << r->codigoVuelo << " (" << r->origen << " -> " << r->destino << ")" << endl;
    cout << "  Salida    : " << r->fecha << " " << r->hora << endl;
    cout << "  Clase     : " << r->clase << endl;
    cout << "  Categoria : " << r->categoria << endl;
    cout << "  Precio    : " << fixed << setprecision(2) << r->precio << endl;
    cout << "  Estado    : " << r->estado << endl;
}

#endif
