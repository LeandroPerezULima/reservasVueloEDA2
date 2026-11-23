// Generador de datos de prueba: vuelos.csv, reservas_pequeno.csv, reservas_masivo.csv
// Compilar: g++ -std=c++11 -O2 -o generador generador.cpp
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <string>
using namespace std;

const int NV = 50;               // cantidad de vuelos
const int HOT = 4;               // vuelos llenos con lista de espera
const int N_MASIVO = 10500;

string dos(int x) { return (x < 10 ? "0" : "") + to_string(x); }

string idNum(int k) {
    string s = to_string(k);
    while (s.size() < 6) { s = "0" + s; }
    return "RES-" + s;
}

string nombres[16]   = {"Ana", "Luis", "Maria", "Carlos", "Rosa", "Jorge", "Lucia", "Pedro",
                        "Carmen", "Diego", "Elena", "Andres", "Sofia", "Miguel", "Paula", "Raul"};
string apellidos[16] = {"Torres", "Ramos", "Soto", "Quispe", "Flores", "Vargas", "Rojas", "Mendoza",
                        "Castro", "Reyes", "Salazar", "Paredes", "Huaman", "Chavez", "Diaz", "Lopez"};
string ciudades[10]  = {"Lima", "Cusco", "Arequipa", "Trujillo", "Piura", "Iquitos",
                        "Tacna", "Chiclayo", "Pucallpa", "Cajamarca"};
string horas[10]     = {"05:30", "06:45", "08:10", "09:20", "11:00", "13:15", "15:40", "18:05", "20:30", "22:15"};
string prefijos[3]   = {"LA", "AV", "H2"};

string codigo[NV], origen[NV], destino[NV], fecha[NV], hora[NV];
int capacidad[NV];

int azar(int n) { return rand() % n; }

string categoriaAzar() {
    int x = azar(100);
    if (x < 15) { return "Premium"; }
    if (x < 45) { return "Frecuente"; }
    return "Regular";
}

void escribirReserva(ofstream& f, int id, int v, const string& categoria, const string& estado) {
    int x = azar(100);
    string clase = x < 70 ? "Economica" : (x < 90 ? "Premium" : "Ejecutiva");
    double precio;
    if (clase == "Economica")      { precio = 90 + azar(21000) / 100.0; }
    else if (clase == "Premium")   { precio = 300 + azar(30000) / 100.0; }
    else                           { precio = 600 + azar(60000) / 100.0; }
    string dni = to_string(40000000 + azar(50000000));
    f << idNum(id) << "," << dni << "," << nombres[azar(16)] << " " << apellidos[azar(16)] << ","
      << codigo[v] << "," << origen[v] << "," << destino[v] << "," << fecha[v] << "," << hora[v] << ","
      << clase << "," << categoria << "," << fixed << setprecision(2) << precio << "," << estado << "\n";
}

string estadoConAsiento() {
    int x = azar(100);
    if (x < 60) { return "Confirmada"; }
    if (x < 85) { return "Pendiente"; }
    return "CheckIn";
}

int main() {
    srand(2026);

    // ---------- vuelos.csv ----------
    ofstream fv("vuelos.csv");
    for (int i = 0; i < NV; i++) {
        codigo[i] = prefijos[i % 3] + to_string(2001 + i);
        int o = azar(10), d = azar(10);
        while (d == o) { d = azar(10); }
        origen[i] = ciudades[o];
        destino[i] = ciudades[d];
        int dia = azar(62);                                   // 2026-12-01 .. 2027-01-31
        fecha[i] = dia < 31 ? "2026-12-" + dos(dia + 1) : "2027-01-" + dos(dia - 30);
        hora[i] = horas[azar(10)];
        capacidad[i] = (i < HOT) ? 150 : 200 + azar(101);     // 4 vuelos pequenos (se llenan) + 46 de 200-300
        fv << codigo[i] << "," << origen[i] << "," << destino[i] << "," << fecha[i] << ","
           << hora[i] << "," << capacidad[i] << "\n";
    }
    fv.close();

    // ---------- reservas_masivo.csv ----------
    // plan: cada posicion indica a que vuelo pertenece la reserva
    int* plan = new int[N_MASIVO];
    int* contPlan = new int[NV];
    for (int i = 0; i < NV; i++) { contPlan[i] = 0; }
    int k = 0;
    for (int v = 0; v < HOT; v++) {                           // 150 + 10 por vuelo lleno
        for (int j = 0; j < capacidad[v] + 10; j++) { plan[k++] = v; contPlan[v]++; }
    }
    while (k < N_MASIVO) {
        int v = HOT + azar(NV - HOT);
        if (contPlan[v] < capacidad[v]) { plan[k++] = v; contPlan[v]++; }
    }
    for (int i = N_MASIVO - 1; i > 0; i--) {                  // mezclar el orden
        int j = azar(i + 1);
        int t = plan[i]; plan[i] = plan[j]; plan[j] = t;
    }

    int ocupados[NV];
    for (int i = 0; i < NV; i++) { ocupados[i] = 0; }
    int espera[NV];
    for (int i = 0; i < NV; i++) { espera[i] = 0; }

    ofstream fm("reservas_masivo.csv");
    for (int i = 0; i < N_MASIVO; i++) {
        int v = plan[i];
        string estado;
        if (ocupados[v] < capacidad[v]) {
            if (v >= HOT && azar(100) < 3) { estado = "Cancelada"; }
            else { estado = estadoConAsiento(); ocupados[v]++; }
        } else {
            estado = "EnEspera";
            espera[v]++;
        }
        escribirReserva(fm, i + 1, v, categoriaAzar(), estado);
    }
    fm.close();

    // ---------- reservas_pequeno.csv (40 reservas, verificacion manual) ----------
    ofstream fp("reservas_pequeno.csv");
    int id = 1;
    // 32 reservas normales en 5 vuelos (fechas y horas repetidas dentro del mismo vuelo)
    for (int j = 0; j < 32; j++) {
        int v = HOT + (j % 5);
        string estado = (j % 8 == 7) ? "Cancelada" : estadoConAsiento();
        escribirReserva(fp, id++, v, categoriaAzar(), estado);
    }
    // 8 en espera del primer vuelo, categorias mezcladas (orden de llegada = orden del archivo)
    string catEspera[8] = {"Regular", "Premium", "Frecuente", "Regular", "Premium", "Regular", "Frecuente", "Premium"};
    for (int j = 0; j < 8; j++) { escribirReserva(fp, id++, 0, catEspera[j], "EnEspera"); }
    fp.close();

    cout << "Archivos generados: vuelos.csv, reservas_pequeno.csv (40), reservas_masivo.csv (" << N_MASIVO << ")" << endl;
    cout << "Vuelos llenos con espera:" << endl;
    for (int v = 0; v < NV; v++) {
        if (espera[v] > 0) { cout << "  " << codigo[v] << " capacidad " << capacidad[v] << " -> " << espera[v] << " en espera" << endl; }
    }
    delete[] plan;
    delete[] contPlan;
    return 0;
}
