#include <iostream>
#include <string>
#include <cstdlib>
#include "sistema.h"
#include "pruebas.h"
using namespace std;

// ---------- entrada por terminal ----------
string leer(const string& mensaje) {
    cout << mensaje;
    string s;
    getline(cin, s);
    while (!s.empty() && (s[s.size() - 1] == '\r' || s[s.size() - 1] == ' ')) { s.erase(s.size() - 1); }
    return s;
}

void pausa() {
    leer("\n[Enter para continuar]");
}

void titulo(const string& t) {
    cout << "\n==================== " << t << " ====================" << endl;
}

void menu() {
    cout << "\n+==========================================================+" << endl;
    cout << "|   SISTEMA DE RESERVAS AEREAS  (Hash + AVL + Max-Heap)    |" << endl;
    cout << "+==========================================================+" << endl;
    cout << "|  1. Carga masiva (vuelos.csv + reservas)                 |" << endl;
    cout << "|  2. Registrar nueva reserva                              |" << endl;
    cout << "|  3. Buscar reserva por ID                      (hash)    |" << endl;
    cout << "|  4. Listado cronologico                        (AVL)     |" << endl;
    cout << "|  5. Consulta por rango de fechas               (AVL)     |" << endl;
    cout << "|  6. Actualizar estado                          (hash)    |" << endl;
    cout << "|  7. Cancelar reserva (con promocion de la espera)        |" << endl;
    cout << "|  8. Eliminacion fisica de una reserva Cancelada          |" << endl;
    cout << "|  9. Lista de espera de un vuelo                (heap)    |" << endl;
    cout << "| 10. Estadisticas generales                               |" << endl;
    cout << "|----------------------------------------------------------|" << endl;
    cout << "| 11. Metricas de las estructuras                          |" << endl;
    cout << "| 12. Casos extremos (colisiones / orden desfavorable)     |" << endl;
    cout << "| 13. Benchmark de tiempos (<chrono>)                      |" << endl;
    cout << "| 14. Verificar consistencia hash-arbol-heap               |" << endl;
    cout << "|  0. Salir                                                |" << endl;
    cout << "+==========================================================+" << endl;
}

bool hayDatos(Sistema* s) {
    if (s->hash.cantidad() == 0) {
        cout << "No hay datos cargados. Use primero la opcion 1." << endl;
        return false;
    }
    return true;
}

void opCarga(Sistema*& s) {
    titulo("1. CARGA MASIVA");
    string archV = leer("Archivo de vuelos [Enter = vuelos.csv]: ");
    if (archV.empty()) { archV = "vuelos.csv"; }
    string archR = leer("Archivo de reservas [Enter = reservas_masivo.csv, 2 = reservas_pequeno.csv]: ");
    if (archR.empty()) { archR = "reservas_masivo.csv"; }
    else if (archR == "2") { archR = "reservas_pequeno.csv"; }

    delete s;                      // reinicia: libera hash, arbol y heaps anteriores
    s = new Sistema();

    Reloj::time_point t0 = Reloj::now();
    int nv = s->cargarVuelos(archV);
    if (nv < 0) { cout << "No se pudo abrir " << archV << endl; return; }
    int leidas, rechazadas;
    bool abierto;
    s->cargarReservas(archR, leidas, rechazadas, abierto);
    if (!abierto) { cout << "No se pudo abrir " << archR << endl; return; }
    double ms = microsegundos(t0, Reloj::now()) / 1000.0;

    cout << "\nVuelos cargados            : " << nv << endl;
    cout << "Lineas de reservas leidas  : " << leidas << endl;
    cout << "Rechazadas                 : " << rechazadas << endl;
    cout << "Insertadas en la HASH      : " << s->hash.cantidad() << endl;
    cout << "Insertadas en el ARBOL AVL : " << s->arbol.cantidad() << endl;
    cout << "En HEAPS de espera (total) : " << s->totalEnEspera() << endl;
    for (int i = 0; i < s->nVuelos; i++) {
        if (s->vuelos[i].espera.tamano() > 0) {
            cout << "    heap " << s->vuelos[i].codigo << ": " << s->vuelos[i].espera.tamano() << " pasajeros" << endl;
        }
    }
    cout << "Tiempo de carga            : " << fixed << setprecision(2) << ms << " ms" << endl;
}

void opRegistro(Sistema* s) {
    titulo("2. REGISTRO DE NUEVA RESERVA");
    if (s->nVuelos == 0) { cout << "Cargue primero los vuelos (opcion 1)." << endl; return; }
    Reserva* r = new Reserva();
    r->idReserva = leer("idReserva (ej. RES-999999): ");
    if (s->hash.buscar(r->idReserva) != nullptr) {
        cout << "ERROR: el idReserva ya existe." << endl; delete r; return;
    }
    r->codigoVuelo = leer("Codigo de vuelo: ");
    int iv = s->buscarVuelo(r->codigoVuelo);
    if (iv < 0) { cout << "ERROR: el vuelo no existe." << endl; delete r; return; }
    Vuelo& v = s->vuelos[iv];
    r->origen = v.origen; r->destino = v.destino; r->fecha = v.fecha; r->hora = v.hora;
    cout << "Vuelo " << v.codigo << ": " << v.origen << " -> " << v.destino << " " << v.fecha << " " << v.hora
         << " | libres: " << (v.capacidad - v.ocupados) << endl;

    r->dni = leer("DNI: ");
    r->pasajero = leer("Pasajero: ");
    string cl = leer("Clase (1=Economica, 2=Premium, 3=Ejecutiva): ");
    r->clase = (cl == "3") ? "Ejecutiva" : (cl == "2") ? "Premium" : "Economica";
    string ca = leer("Categoria (1=Premium, 2=Frecuente, 3=Regular): ");
    r->categoria = (ca == "1") ? "Premium" : (ca == "2") ? "Frecuente" : "Regular";
    r->precio = atof(leer("Precio: ").c_str());
    string pago = leer("Pago realizado? (s = Confirmada / n = Pendiente): ");
    r->estado = (pago == "n" || pago == "N") ? "Pendiente" : "Confirmada";

    string error;
    if (!s->registrar(r, error)) { cout << "ERROR: " << error << endl; delete r; return; }
    cout << "\nReserva registrada con estado: " << r->estado << endl;
    if (r->estado == "EnEspera") {
        cout << "El vuelo esta lleno: entro al heap de espera (posicion de llegada " << r->ordenLlegada
             << ", en espera ahora: " << v.espera.tamano() << ")." << endl;
    }
    mostrarDetalle(r);
}

void opBuscar(Sistema* s) {
    titulo("3. BUSQUEDA POR ID");
    if (!hayDatos(s)) { return; }
    string id = leer("idReserva: ");
    Reloj::time_point t0 = Reloj::now();
    Reserva* r = s->hash.buscar(id);
    double us = microsegundos(t0, Reloj::now());
    if (r == nullptr) { cout << "No existe ninguna reserva con ID " << id << endl; }
    else { mostrarDetalle(r); }
    cout << "(busqueda en hash: " << fixed << setprecision(2) << us << " us)" << endl;
}

void opListado(Sistema* s) {
    titulo("4. LISTADO CRONOLOGICO");
    if (!hayDatos(s)) { return; }
    string k = leer("Cuantas mostrar? [Enter = 20]: ");
    int cant = k.empty() ? 20 : atoi(k.c_str());
    if (cant <= 0) { cant = 20; }
    mostrarEncabezado();
    int m = s->arbol.listarPrimeros(cant, true);
    cout << "(" << m << " reservas, recorrido en orden del AVL por fecha + hora + id)" << endl;
}

void opRango(Sistema* s) {
    titulo("5. CONSULTA POR RANGO DE FECHAS");
    if (!hayDatos(s)) { return; }
    string ini = leer("Fecha inicial (AAAA-MM-DD): ");
    string fin = leer("Fecha final   (AAAA-MM-DD): ");
    if (ini > fin) { cout << "La fecha inicial es mayor que la final." << endl; return; }

    cout << "\nMuestra (primeras 10):" << endl;
    mostrarEncabezado();
    int total = s->arbol.consultarRango(ini, fin, 10, true);
    cout << "\nReservas encontradas en [" << ini << ", " << fin << "]: " << total << endl;
    cout << "Nodos visitados: " << s->arbol.nodosVisitados() << " de " << s->arbol.cantidad()
         << " (las ramas fuera del rango se descartan)" << endl;
}

void opActualizar(Sistema* s) {
    titulo("6. ACTUALIZACION DE ESTADO");
    if (!hayDatos(s)) { return; }
    string id = leer("idReserva: ");
    Reserva* r = s->hash.buscar(id);
    if (r == nullptr) { cout << "No existe esa reserva." << endl; return; }
    cout << "Estado actual: " << r->estado << endl;
    cout << "Transiciones permitidas: Pendiente -> Confirmada, Confirmada -> CheckIn" << endl;
    string nuevo = leer("Nuevo estado: ");
    int res = s->actualizarEstado(id, nuevo);
    if (res == OP_OK) { cout << "OK: " << id << " ahora esta " << r->estado << " (visible en hash y arbol)." << endl; }
    else { cout << "ERROR: transicion no permitida (" << r->estado << " -> " << nuevo << ")." << endl; }
}

void opCancelar(Sistema* s) {
    titulo("7. CANCELACION DE RESERVA");
    if (!hayDatos(s)) { return; }
    string id = leer("idReserva a cancelar: ");
    Reserva* r = s->hash.buscar(id);
    if (r != nullptr) { cout << "Estado actual: " << r->estado << " | vuelo " << r->codigoVuelo << endl; }
    Reserva* p = nullptr;
    int res = s->cancelar(id, &p);
    if (res == OP_NO_EXISTE) { cout << "No existe esa reserva." << endl; }
    else if (res == OP_YA_CANCELADA) { cout << "La reserva ya estaba cancelada." << endl; }
    else if (res == OP_CASO_B) {
        cout << "CASO B: estaba EnEspera. Se retiro del heap desde una posicion cualquiera y quedo Cancelada." << endl;
    } else {
        cout << "CASO A: ocupaba asiento. Se libero y quedo Cancelada." << endl;
        if (p != nullptr) {
            cout << "Se promovio al primero de la lista de espera (extraerMax):" << endl;
            mostrarEncabezado();
            mostrarReserva(p);
        } else {
            cout << "No habia nadie en espera: el asiento queda libre." << endl;
        }
    }
}

void opEliminar(Sistema* s) {
    titulo("8. ELIMINACION FISICA");
    if (!hayDatos(s)) { return; }
    string id = leer("idReserva (debe estar Cancelada): ");
    int res = s->eliminarFisica(id);
    if (res == OP_NO_EXISTE) { cout << "No existe esa reserva." << endl; return; }
    if (res == OP_INVALIDA) { cout << "ERROR: solo se pueden eliminar reservas Canceladas." << endl; return; }
    cout << "Reserva " << id << " eliminada de la hash y del arbol." << endl;
    cout << "Verificacion -> busqueda por ID: " << (s->hash.buscar(id) == nullptr ? "ya no aparece" : "SIGUE (error)") << endl;
    cout << "Reservas en hash: " << s->hash.cantidad() << " | en arbol: " << s->arbol.cantidad() << endl;
}

void opEspera(Sistema* s) {
    titulo("9. LISTA DE ESPERA DE UN VUELO");
    if (s->nVuelos == 0) { cout << "Cargue primero los datos (opcion 1)." << endl; return; }
    string cod = leer("Codigo de vuelo (Enter = lista de vuelos con espera): ");
    if (cod.empty()) {
        for (int i = 0; i < s->nVuelos; i++) {
            if (s->vuelos[i].espera.tamano() > 0) {
                cout << "  " << s->vuelos[i].codigo << " -> " << s->vuelos[i].espera.tamano() << " en espera" << endl;
            }
        }
        cod = leer("Codigo de vuelo: ");
    }
    int iv = s->buscarVuelo(cod);
    if (iv < 0) { cout << "El vuelo no existe." << endl; return; }
    s->mostrarEspera(iv);
}

void opCasosExtremos() {
    titulo("12. CASOS EXTREMOS");
    cout << "--- Colisiones en la hash ---" << endl;
    pruebaColisiones();
    cout << "\n--- Orden desfavorable en el arbol (1000 fechas crecientes) ---" << endl;
    pruebaOrdenDesfavorable();
}

int main() {
    Sistema* sis = new Sistema();
    cout << "Proyecto EDA II - Reservas aereas con lista de espera" << endl;
    cout << "Estructuras: Tabla Hash (encadenamiento) + Arbol AVL + Max-Heap" << endl;

    bool salir = false;
    while (!salir) {
        menu();
        string op = leer("Opcion: ");
        if (op == "1") { opCarga(sis); }
        else if (op == "2") { opRegistro(sis); }
        else if (op == "3") { opBuscar(sis); }
        else if (op == "4") { opListado(sis); }
        else if (op == "5") { opRango(sis); }
        else if (op == "6") { opActualizar(sis); }
        else if (op == "7") { opCancelar(sis); }
        else if (op == "8") { opEliminar(sis); }
        else if (op == "9") { opEspera(sis); }
        else if (op == "10") { titulo("10. ESTADISTICAS GENERALES"); if (hayDatos(sis)) { sis->estadisticas(); } }
        else if (op == "11") { titulo("11. METRICAS DE LAS ESTRUCTURAS"); mostrarMetricas(*sis); }
        else if (op == "12") { opCasosExtremos(); }
        else if (op == "13") { titulo("13. BENCHMARK"); benchmark(*sis); }
        else if (op == "14") {
            titulo("14. CONSISTENCIA");
            bool ok = sis->verificarConsistencia(true);
            cout << (ok ? "RESULTADO: CONSISTENTE" : "RESULTADO: INCONSISTENTE") << endl;
        }
        else if (op == "0") { salir = true; }
        else { cout << "Opcion invalida." << endl; }
        if (!salir && !cin.eof()) { pausa(); }
        if (cin.eof()) { salir = true; }
    }

    delete sis;      // los destructores liberan hash (y sus reservas), arbol y heaps
    cout << "Memoria liberada. Hasta luego." << endl;
    return 0;
}
