#ifndef PRUEBAS_H
#define PRUEBAS_H

#include <chrono>
#include "sistema.h"

typedef chrono::high_resolution_clock Reloj;

inline double microsegundos(Reloj::time_point t0, Reloj::time_point t1) {
    return chrono::duration<double, micro>(t1 - t0).count();
}

inline string dos(int x) {
    return (x < 10 ? "0" : "") + to_string(x);
}

inline string idNumerado(int k) {
    string s = to_string(k);
    while (s.size() < 6) { s = "0" + s; }
    return "RES-" + s;
}

// fecha = 2027-01-01 + d dias
inline string fechaDesdeDia(int d) {
    int y = 2027, m = 1, dia = 1;
    int dm[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    for (int i = 0; i < d; i++) {
        int dimes = dm[m - 1];
        bool bisiesto = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
        if (m == 2 && bisiesto) { dimes = 29; }
        dia++;
        if (dia > dimes) {
            dia = 1;
            m++;
            if (m > 12) { m = 1; y++; }
        }
    }
    return to_string(y) + "-" + dos(m) + "-" + dos(dia);
}

// ---------- Caso extremo 1: colisiones en la hash ----------
inline void pruebaColisiones() {
    const int OBJETIVO = 50;
    TablaHash th(10007);
    string ids[OBJETIVO];
    int hallados = 0;
    for (int i = 0; hallados < OBJETIVO; i++) {
        string id = "COL-" + to_string(i);
        if (th.indice(id) == 0) { ids[hallados++] = id; }      // todos caen en la posicion 0
    }
    for (int i = 0; i < OBJETIVO; i++) {
        Reserva* r = new Reserva();
        r->idReserva = ids[i];
        if (!th.insertar(r)) { delete r; }
    }
    int encontrados = 0;
    for (int i = 0; i < OBJETIVO; i++) {
        if (th.buscar(ids[i]) != nullptr) { encontrados++; }
    }
    cout << "IDs generados que caen en la posicion 0 de la hash: " << OBJETIVO << endl;
    cout << "IDs encontrados con buscar(): " << encontrados << " de " << OBJETIVO << endl;
    cout << "Cadena mas larga: " << th.cadenaMasLarga() << endl;
    cout << (encontrados == OBJETIVO ? "RESULTADO: OK, todos se encuentran." : "RESULTADO: FALLA") << endl;
}

// ---------- Caso extremo 2: orden desfavorable en el arbol ----------
inline void pruebaOrdenDesfavorable() {
    const int N = 1000;
    ArbolAVL a;
    Reserva* rs[N];
    for (int i = 0; i < N; i++) {
        rs[i] = new Reserva();
        rs[i]->idReserva = idNumerado(i + 1);
        rs[i]->fecha = fechaDesdeDia(i);       // fechas estrictamente crecientes
        rs[i]->hora = "08:00";
        a.insertar(rs[i]);
    }
    cout << "Reservas insertadas con fechas crecientes: " << a.cantidad() << endl;
    cout << "Primera fecha: " << rs[0]->fecha << " | ultima fecha: " << rs[N - 1]->fecha << endl;
    cout << "Altura del arbol AVL: " << a.altura() << "  (minimo posible = 10; un arbol sin balancear tendria " << N << ")" << endl;
    cout << "AVL valido (balanceado y ordenado): " << (a.esValido() ? "SI" : "NO") << endl;
    for (int i = 0; i < N; i++) { delete rs[i]; }      // el arbol no es duenio de las reservas
}

// ---------- Metricas de las estructuras ----------
inline void mostrarMetricas(Sistema& s) {
    int n = s.arbol.cantidad();
    cout << fixed << setprecision(3);
    cout << "Reservas en hash / arbol     : " << s.hash.cantidad() << " / " << n << endl;
    cout << "Tamano de la tabla hash      : " << s.hash.capacidad() << endl;
    cout << "Factor de carga de la hash   : " << s.hash.factorCarga() << "  (esperado: cercano a 1 o menor)" << endl;
    cout << "Cadena mas larga de la hash  : " << s.hash.cadenaMasLarga() << "  (esperado: pocos nodos)" << endl;
    cout << "Altura del arbol AVL         : " << s.arbol.altura() << "  (esperado: cerca de 14 con 10,000; maximo teorico AVL ~ 19)" << endl;
    cout << "Reservas en listas de espera : " << s.totalEnEspera() << endl;
    for (int i = 0; i < s.nVuelos; i++) {
        if (s.vuelos[i].espera.tamano() > 0) {
            cout << "  " << s.vuelos[i].codigo << ": " << s.vuelos[i].espera.tamano()
                 << " en espera (capacidad " << s.vuelos[i].capacidad << ", ocupados " << s.vuelos[i].ocupados << ")" << endl;
        }
    }
}

// ---------- Benchmark con <chrono> ----------
// NOTA: modifica los datos cargados (crea, promueve y borra reservas de prueba).
inline void benchmark(Sistema& s) {
    int n = s.hash.cantidad();
    if (n == 0) { cout << "Primero cargue datos (opcion 1)." << endl; return; }
    cout << fixed << setprecision(3);

    // ID del ULTIMO nodo del recorrido secuencial: peor caso para la busqueda secuencial
    string idObjetivo;
    for (int i = 0; i < s.hash.capacidad(); i++) {
        for (NodoHash* a = s.hash.cabeza(i); a != nullptr; a = a->sig) { idObjetivo = a->r->idReserva; }
    }

    // Busqueda por ID (hash) x1000
    // dos copias del mismo ID alternadas + resultado en variable volatile: evita que
    // el compilador saque la busqueda repetida fuera del bucle y la mida en 0
    string copias[2] = {idObjetivo, idObjetivo};
    volatile int hallados = 0;
    Reloj::time_point t0 = Reloj::now();
    for (int i = 0; i < 1000; i++) {
        if (s.hash.buscar(copias[i & 1]) != nullptr) { hallados = hallados + 1; }
    }
    double tHash = microsegundos(t0, Reloj::now()) / 1000.0;

    // Busqueda secuencial del mismo ID x1000 (recorre todos los nodos)
    t0 = Reloj::now();
    for (int rep = 0; rep < 1000; rep++) {
        bool hallado = false;
        for (int i = 0; i < s.hash.capacidad() && !hallado; i++) {
            for (NodoHash* a = s.hash.cabeza(i); a != nullptr; a = a->sig) {
                if (a->r->idReserva == idObjetivo) { hallado = true; break; }
            }
        }
        if (hallado) { hallados++; }
    }
    double tSec = microsegundos(t0, Reloj::now()) / 1000.0;

    // Inserciones individuales x100 (en el vuelo con mas capacidad libre)
    int iv = 0;
    for (int i = 1; i < s.nVuelos; i++) {
        if (s.vuelos[i].capacidad - s.vuelos[i].ocupados > s.vuelos[iv].capacidad - s.vuelos[iv].ocupados) { iv = i; }
    }
    Vuelo& vi = s.vuelos[iv];
    string ids[100];
    t0 = Reloj::now();
    for (int i = 0; i < 100; i++) {
        Reserva* r = new Reserva();
        ids[i] = "BEN-" + to_string(i);
        r->idReserva = ids[i]; r->dni = "00000000"; r->pasajero = "Prueba Bench";
        r->codigoVuelo = vi.codigo; r->origen = vi.origen; r->destino = vi.destino;
        r->fecha = vi.fecha; r->hora = vi.hora; r->clase = "Economica";
        r->categoria = "Regular"; r->precio = 100; r->estado = "Pendiente";
        string error;
        if (!s.insertarReserva(r, error)) { delete r; }
    }
    double tIns = microsegundos(t0, Reloj::now()) / 100.0;

    // Consulta ordenada (primeras 50) x100
    t0 = Reloj::now();
    for (int i = 0; i < 100; i++) { s.arbol.listarPrimeros(50, false); }
    double tOrd = microsegundos(t0, Reloj::now()) / 100.0;

    // Consulta por rango (un mes) x100
    int enRango = 0;
    t0 = Reloj::now();
    for (int i = 0; i < 100; i++) { enRango = s.arbol.consultarRango("2026-12-01", "2026-12-31", 0, false); }
    double tRango = microsegundos(t0, Reloj::now()) / 100.0;
    int visitados = s.arbol.nodosVisitados();

    // Actualizacion de estado: 10 reservas Pendiente -> Confirmada (10 operaciones)
    t0 = Reloj::now();
    for (int i = 0; i < 10; i++) { s.actualizarEstado(ids[i], "Confirmada"); }
    double tAct = microsegundos(t0, Reloj::now()) / 10.0;

    // Cancelacion con promocion: 10 reservas con asiento del vuelo con mas espera
    int ivEsp = 0;
    for (int i = 1; i < s.nVuelos; i++) {
        if (s.vuelos[i].espera.tamano() > s.vuelos[ivEsp].espera.tamano()) { ivEsp = i; }
    }
    string aCancelar[10];
    int nc = 0;
    for (int i = 0; i < s.hash.capacidad() && nc < 10; i++) {
        for (NodoHash* a = s.hash.cabeza(i); a != nullptr && nc < 10; a = a->sig) {
            if (a->r->codigoVuelo == s.vuelos[ivEsp].codigo && ocupaAsiento(a->r->estado)) {
                aCancelar[nc++] = a->r->idReserva;
            }
        }
    }
    int promovidos = 0;
    t0 = Reloj::now();
    for (int i = 0; i < nc; i++) {
        Reserva* p = nullptr;
        s.cancelar(aCancelar[i], &p);
        if (p != nullptr) { promovidos++; }
    }
    double tCan = nc > 0 ? microsegundos(t0, Reloj::now()) / nc : 0;

    // Eliminacion fisica de esas canceladas
    t0 = Reloj::now();
    for (int i = 0; i < nc; i++) { s.eliminarFisica(aCancelar[i]); }
    double tEli = nc > 0 ? microsegundos(t0, Reloj::now()) / nc : 0;

    // limpieza de las reservas de prueba BEN-
    for (int i = 0; i < 100; i++) {
        s.cancelar(ids[i], nullptr);
        s.eliminarFisica(ids[i]);
    }

    cout << "\n+----------------------------------------------+---------+-------------+" << endl;
    cout << "| Operacion                                    | Reps.   | Promedio    |" << endl;
    cout << "+----------------------------------------------+---------+-------------+" << endl;
    cout << "| Busqueda por ID (hash)                       | 1000    | " << setw(8) << tHash << " us |" << endl;
    cout << "| Busqueda secuencial del mismo ID             | 1000    | " << setw(8) << tSec << " us |" << endl;
    cout << "| Insercion individual                         | 100     | " << setw(8) << tIns << " us |" << endl;
    cout << "| Consulta ordenada (primeras 50)              | 100     | " << setw(8) << tOrd << " us |" << endl;
    cout << "| Consulta por rango (un mes)                  | 100     | " << setw(8) << tRango << " us |" << endl;
    cout << "| Actualizacion de estado                      | 10      | " << setw(8) << tAct << " us |" << endl;
    cout << "| Cancelacion con promocion desde el heap      | " << setw(2) << nc << "      | " << setw(8) << tCan << " us |" << endl;
    cout << "| Eliminacion fisica                           | " << setw(2) << nc << "      | " << setw(8) << tEli << " us |" << endl;
    cout << "+----------------------------------------------+---------+-------------+" << endl;
    cout << "Reservas halladas en el rango de un mes: " << enRango << " (nodos visitados: " << visitados
         << " de " << s.arbol.cantidad() << ")" << endl;
    cout << "Promociones realizadas desde el heap: " << promovidos << " (vuelo " << s.vuelos[ivEsp].codigo << ")" << endl;
    cout << "(la hash fue " << (tSec / (tHash > 0 ? tHash : 1e-9)) << " veces mas rapida que la busqueda secuencial)" << endl;
}

#endif
