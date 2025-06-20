# map-2025
Primera Practica Distribuidos Herramienta de Big Data en C
# MAP — Herramienta de *Big Data* en C

### README del proyecto (versión extensa)

---

## 1. Visión general

**MAP** es una mini-implementación educativa, inspirada en *MapReduce*, que permite procesar en paralelo ficheros muy grandes (decenas de MB hasta GB) sobre un clúster Linux distribuido.
A diferencia de MapReduce completo, nuestra herramienta **solo implementa la fase *map*** (transformación y filtrado). El objetivo es:

* cómo se orquesta un trabajo distribuido con un **maestro** y varios **trabajadores**;
* cómo reservar nodos a través de un **gestor** independiente;
* cómo trocear un fichero por bloques y lanzar tareas en paralelo;
* cómo usar **sockets TCP** puro y control de procesos en C para construir software de sistemas real.

La práctica está pensada para que comprendas la anatomía básica de las plataformas de *Big Data* y entrenes programación concurrente, redes y E/S de alto rendimiento.

---

## 2. Arquitectura de alto nivel

```
┌────────────┐       reserva          ┌─────────────┐
│  Cliente    │ ———————————————→ │   Gestor    │
│  (map)      │←———————————————  │ (manager)  │
└─────┬───────┘     info workers      └─────┬──────┘
      │                                  ↑  ↑
      │  tareas (bloques)                │  │ alta
      │                                  │  │
┌─────▼──────┐  resultados / ack         │  │
│Trabajador 1│ ───────────────────────────┘  │
│ (worker)   │                                 │
└────────────┘                                 │
       ⋮                                       │
┌────────────┐                                 │
│Trabajador N│ ────────────────────────────────┘
└────────────┘
```

1. **Gestor** (*manager*)

   * Arranca una única vez por clúster.
   * Registra a cada trabajador que se da de alta.
   * Atiende reservas de clientes en servicio concurrente (1 thread/conn).
   * Mantiene tabla de estado (`srv_addr_arr`) con **IP, puerto y estado** (`free | allocated`).
   * Cuando el cliente se desconecta, libera automáticamente los nodos.

2. **Trabajador** (*worker*)

   * Se lanza en cualquier nodo libre.
   * Fase inicial: se conecta al Gestor → envía su puerto de servicio → queda registrado.
   * Luego pasa a modo servidor secuencial: acepta UN cliente, procesa sus tareas una a una, y cuando el cliente cierra la conexión vuelve a quedar disponible.

3. **Cliente / Maestro** (*map*)

   * Programa que invocas para disparar un trabajo:

     ```bash
     ./map <gestor_host> <gestor_port> <N workers> <tamaño_bloque>
           <fichero_entrada> <directorio_salida> <programa> [args…]
     ```
   * Solicita **N** trabajadores al Gestor y mantiene la conexión abierta toda la vida del trabajo.
   * Divide el fichero en bloques → cada bloque es una **tarea**.
   * Envía las tareas a los trabajadores y espera acknowledgements.
   * Implementa *scheduling* simple:
     *Si hay más tareas que workers*, reutiliza los sockets con `select()` para saber qué worker acaba antes y asignarle la siguiente tarea.

---

## 3. Flujo de ejecución de un trabajo

1. **Reserva**

   * El Cliente se conecta al Gestor, indica cuántos workers necesita.
   * El Gestor marca los primeros `free` que encuentre como `allocated` y devuelve sus (IP, puerto).

2. **Distribución de tareas**

   * El Cliente abre sockets directos con cada worker reservado.
   * Envía al inicio:

     * ruta fichero entrada (visible en todos los nodos mediante FS compartido);
     * directorio salida;
     * tamaño de bloque;
     * nombre del programa + args.
   * Para cada bloque *i*: manda `i` y espera confirmación.

3. **Procesamiento en el Trabajador**

   * Recibe la descripción global una sola vez; después, en bucle:

     * lee número de bloque;
     * hace `fork()` → hijo:

       1. crea una tubería;
       2. el padre vuelca el bloque con `sendfile()` al extremo escritura;
       3. el hijo redirige su stdin a la tubería, redirige stdout a un fichero con patrón

          ```
          <directorio_salida>/<basename(fichero)>-<bloque# en 5 dígitos>
          ```
       4. ejecuta `execvp(programa, argv)`;
     * padre espera (`wait()`) y cuando el hijo termina envía ACK al Cliente.

4. **Finalización**

   * Cuando el Cliente ha enviado y recibido todos los ACK, cierra sockets a workers y Gestor.
   * El Gestor detecta el cierre, cambia workers `allocated → free`.
   * Los workers vuelven a bloquearse en `accept()` esperando un nuevo cliente.

---

## 4. Entorno y dependencias

| Requisito                       | Motivo                                                                        |
| ------------------------------- | ----------------------------------------------------------------------------- |
| **GNU/Linux** con `gcc`, `make` | Compilación en C y uso de syscalls POSIX                                      |
| Acceso a un **FS compartido**   | Todos los nodos deben ver los mismos ficheros de datos y programas            |
| Conectividad TCP entre nodos    | Sockets de tipo *stream* (`AF_INET`, `SOCK_STREAM`)                           |
| Posible uso de **Docker**       | Opcional para pruebas aisladas; se monta `/map` para compartir código y datos |

---

## 5. Estructura del repositorio

```text
map-2025/
├─ client_node/        # Cliente (map.c)
├─ manager_node/       # Gestor (manager.c, manager.h, srv_addr_arr.*)
├─ worker_node/        # Trabajador (worker.c, worker.h)
├─ common/             # Utilidades compartidas
│  ├─ common_cln.*     # Helpers cliente: sockets, parsing, etc.
│  ├─ common_srv.*     # Helpers servidor: sockets, logging
│  └─ common/          # (si necesitases algo común a ambos)
├─ entrada/            # Ficheros de datos de ejemplo
├─ salida/             # Carpeta vacía — resultados de pruebas
├─ programas/          # Scripts/ejecutables de prueba (filtros)
└─ ejemplos_sockets/   # Referencia: server_thr.c, server_seq.c, select.c, ...
```

Los únicos ficheros **que debes modificar** se encuentran en:

* `manager_node/manager.c` (+ `manager.h` si quieres)
* `worker_node/worker.c`  (+ `worker.h` opcional)
* `client_node/map.c`
* Cualquier cosa dentro de `common/` que te resulte útil.

---

## 6. Compilación

Cada componente se compila por separado:

```bash
cd manager_node && make         # genera ./manager
cd worker_node  && make         # genera ./worker
cd client_node  && make         # genera ./map
```

`Makefile` ya incluye `-pthread` y las cabeceras necesarias, pero puedes añadir banderas (`-Wall -Wextra -O2`) para mayor calidad.

---

## 7. Desarrollo paso a paso (fases)

> **Consejo**: No saltes fases; cada una introduce un concepto nuevo y testea lo anterior.

| Fase                            | Qué implementar                      | Puntos clave                                                                 |
| ------------------------------- | ------------------------------------ | ---------------------------------------------------------------------------- |
| **1. Alta de trabajador**       | - Worker se registra en Gestor       | *create\_socket\_cln\_by\_name*, `srv_addr_arr_add`, manejo IP/puerto en red |
| **2. Reserva de workers**       | - Cliente pide *1* nodo              | Responder IP/puerto; conexión persistente para liberar al final              |
| **3. Trabajo 1 tarea (sin FS)** | - Exec simple en el worker           | `fork + execlp`, padre espera y envía ACK                                    |
| **4. Trabajo 1 tarea (con FS)** | - Redireccionar stdin/out a ficheros | Construir nombre `<outdir>/<in>-00000`                                       |
| **5. Muchas tareas, 1 worker**  | - Dividir fichero en bloques         | Tubo + `sendfile`, bucle en worker                                           |
| **6. #tareas ≤ #workers**       | - Paralelizar primera ronda          | Conectar a cada worker una sola vez                                          |
| **7. #tareas > #workers**       | - Reparto dinámico con `select()`    | Reasignar tarea libre al que termine antes                                   |
| **8. Soporte de argumentos**    | - Pasar vector `argv` completo       | Cambiar a `execvp`, parsing robusto                                          |

Avanza fase a fase, probando siempre en local antes de saltar a triqui o Docker.
