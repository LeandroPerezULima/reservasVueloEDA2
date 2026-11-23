# Proyecto EDA II - Sistema de Reservas Aereas con Lista de Espera

Estructuras implementadas desde cero (sin STL): **Tabla Hash con encadenamiento**, **Arbol AVL** y **Max-Heap**.

## Compilar y ejecutar

```
g++ -std=c++11 -O2 -o generador generador.cpp     # genera vuelos.csv, reservas_pequeno.csv, reservas_masivo.csv
./generador
g++ -std=c++11 -O2 -o reservas main.cpp
./reservas
```

En el menu, la opcion 1 carga los datos (Enter = `vuelos.csv` + `reservas_masivo.csv`; escribir `2` para usar `reservas_pequeno.csv`).

## Modulos

| Archivo | Contenido |
|---|---|
| `reserva.h` | Registro `Reserva` (unico en memoria) y clave compuesta `fecha + hora + idReserva` |
| `tabla_hash.h` | Hash con listas enlazadas, clave `idReserva`, tamano 10007. Es duena de las `Reserva` |
| `arbol_avl.h` | AVL por clave compuesta, listado en orden y consulta por rango con poda |
| `heap.h` | Max-heap en arreglo: `insertar`, `extraerMax`, `eliminarEnPosicion` |
| `sistema.h` | Logica: carga, registro, actualizar, cancelar (casos A y B), eliminar, espera, estadisticas, consistencia |
| `pruebas.h` | Benchmark con `<chrono>`, metricas, colisiones y orden desfavorable |
| `main.cpp` | Menu por terminal |
| `generador.cpp` | Generador de los 3 archivos de prueba |

## Como se mantiene la consistencia

Cada reserva se crea una sola vez con `new`. La hash, el arbol y el heap guardan punteros (`Reserva*`) al mismo objeto, por lo que un cambio de estado se ve en todas.

- Alta: hash + arbol (+ heap si queda `EnEspera`).
- Cancelacion caso A (ocupaba asiento): estado `Cancelada`, `extraerMax` del heap del vuelo y el promovido pasa a `Confirmada`.
- Cancelacion caso B (`EnEspera`): `eliminarEnPosicion` en el heap.
- Eliminacion fisica: primero se saca del arbol, despues de la hash, que hace el `delete`.
- La opcion 14 del menu verifica hash = arbol, `EnEspera` = heaps, asientos por vuelo, propiedad de cada heap y balance/orden del AVL.
