*Este proyecto ha sido creado como parte del currículo de 42 por ripaparo*

# Codexion

## Descripción
**Codexion** es una simulación concurrente multihilo inspirada en el clásico problema de los filósofos comensales (*Dining Philosophers*). En este escenario, múltiples personas que programan se sientan en un espacio de co-working circular con un Compilador Cuántico en el centro.

Para compilar su código cuántico, cada programador necesita adquirir simultáneamente dos dongles USB especializados (uno a su izquierda y otro a su derecha). Los participantes alternan entre tres fases principales:
1. **Compilar (`compiling`):** Sostienen ambos dongles durante `time_to_compile` milisegundos.
2. **Depurar (`debugging`):** Devuelven ambos dongles a la mesa (donde entran en periodo de enfriamiento) y depuran durante `time_to_debug` milisegundos.
3. **Refactorizar (`refactoring`):** Refactorizan código durante `time_to_refactor` milisegundos y, al terminar, intentan adquirir de nuevo los dongles necesarios.

La simulación modela la contienda por recursos escasos, evitando el agotamiento (*burnout*) de los programadores mediante políticas de planificación justas (`fifo` y `edf`) implementadas sobre colas de prioridad (min-heap).

---

## Instrucciones

### Requisitos y Compilación
El proyecto está desarrollado en C estándar y cumple con la **Norma de 42**. Requiere las librerías de hilos POSIX (`pthread`).

Para compilar el proyecto:
```bash
cd coders
make
```
Esto generará el ejecutable `codexion`.

Para limpiar los archivos objeto y binarios generados:
```bash
make clean    # Elimina los archivos objeto (.o)
make fclean   # Elimina los archivos objeto y el ejecutable codexion
make re       # Recompila todo desde cero
```

### Ejecución
El programa requiere obligatoriamente 8 argumentos:
```bash
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

#### Parámetros:
1. `number_of_coders`: Número de programadores y dongles en la mesa circular (entero positivo > 0).
2. `time_to_burnout`: Tiempo máximo (en ms) que un programador puede pasar sin comenzar a compilar antes de quemarse.
3. `time_to_compile`: Tiempo (en ms) que dura la fase de compilación.
4. `time_to_debug`: Tiempo (en ms) que dura la fase de depuración.
5. `time_to_refactor`: Tiempo (en ms) que dura la fase de refactorización.
6. `number_of_compiles_required`: Cantidad de veces que cada programador debe compilar para que la simulación finalice con éxito.
7. `dongle_cooldown`: Tiempo (en ms) durante el cual un dongle permanece inaccesible tras ser liberado.
8. `scheduler`: Política de planificación de dongles (`fifo` o `edf`).

#### Ejemplo de uso:
```bash
./codexion 4 800 200 200 200 5 10 edf
```

---

## Recursos

### Referencias Bibliográficas y Documentación
- **POSIX Threads Programming:** Documentación IEEE POSIX 1003.1c y tutoriales de LLNL sobre `pthread_mutex`, `pthread_cond_wait` y `pthread_cond_timedwait`.
- **Dijkstra, E. W. (1968):** *Hierarchical ordering of sequential processes*. Solución clásica para la prevención de interbloqueos en asignación de recursos.
- **Coffman, E. G., Elphick, M. J., & Shoshani, A. (1971):** *System Deadlocks*. Caracterización de las cuatro condiciones necesarias para que ocurra un interbloqueo.
- **Liu, C. L., & Layland, J. W. (1973):** *Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment*. Fundamentos teóricos del planificador *Earliest Deadline First* (EDF).
- **Cormen, T. H., et al.:** *Introduction to Algorithms* (Capítulo sobre Colas de Prioridad y Heaps Binarios).

### Uso de Inteligencia Artificial (IA)
En conformidad con el Capítulo II y Capítulo VII del enunciado:
- **Tareas asistidas por IA:**
  - Diseño conceptual de la arquitectura de colas de prioridad y análisis teórico de interbloqueos mediante ordenación jerárquica de dongles.
  - Generación de la estructura base del `README.md` y organización del seguimiento de fases en `PROGRESS.md`.
  - Discusión interactiva y validación cruzada de las condiciones de carrera y cálculos de tiempos relativos con `gettimeofday`.
- **Compromiso ético y autoría:** Todo el código y lógica implementada ha sido analizado, validado y comprendido íntegramente por el autor, garantizando la capacidad de explicar y defender cualquier línea durante las evaluaciones entre pares (*peer-evaluations*) y pruebas de modificación de código (*recodes*).

---

## Blocking cases handled

La implementación contempla rigurosamente los problemas clásicos de concurrencia y contienda de recursos:

1. **Prevención de Interbloqueos (Condiciones de Coffman):**
   - Para que ocurra un interbloqueo (*deadlock*), deben cumplirse simultáneamente las cuatro condiciones de Coffman: exclusión mutua, retención y espera, no apropiación y espera circular.
   - Rompemos la condición de **Espera Circular (*Circular Wait*)** aplicando una **ordenación jerárquica total de dongles**:
     - Cada programador $i$ que compite por los dongles `left` y `right` adquiere siempre primero el dongle con el índice menor `min(left, right)` y posteriormente el de índice mayor `max(left, right)`.
     - Esto convierte el grafo de asignación de recursos en un Grafo Acíclico Dirigido (DAG), garantizando formalmente la imposibilidad de un ciclo de bloqueo mutuo.

2. **Prevención de Inanición (*Starvation*) y Equidad:**
   - En el planificador `fifo`, los programadores son atendidos estrictamente por orden de solicitud en una cola de prioridad, evitando que programadores rápidos monopolicen los dongles.
   - En el planificador `edf`, las solicitudes se priorizan por la cercanía a su momento de agotamiento (`deadline = last_compile_start + time_to_burnout`), asegurando que quien esté en situación más crítica obtenga el recurso prioritariamente.

3. **Gestión del Cooldown de Dongles:**
   - Un dongle liberado registra la marca de tiempo `cooldown_until = now + dongle_cooldown`.
   - Si un programador es seleccionado para tomar el dongle pero el cooldown aún no ha vencido, espera la diferencia de tiempo mediante `pthread_cond_timedwait`, evitando bucles de consumo inútil de CPU (*busy waiting*) y permitiendo ser despertado si la simulación se interrumpe.

4. **Detección Precisa del Agotamiento (Precisión <= 10 ms):**
   - Un hilo monitor dedicado supervisa a intervalos inferiores a 1 ms (`usleep(500)`) si algún programador ha excedido su `time_to_burnout`.
   - Esto asegura que el mensaje de `burned out` se imprima con un margen de tolerancia estricto muy inferior a los 10 ms requeridos.

5. **Serialización del Registro (Logs Thread-Safe):**
   - Toda salida por terminal se realiza a través de una función atómica protegida por un mutex de salida (`print_mutex`), impidiendo la intercalación o corrupción de líneas de estado en la consola.
   - En cuanto ocurre un agotamiento o se alcanzan las compilaciones requeridas, se impide la impresión de mensajes posteriores.

---

## Thread synchronization mechanisms

El proyecto implementa primitivas de la librería POSIX Threads para coordinar los diferentes hilos de ejecución de manera segura y eficiente:

1. **`pthread_mutex_t` (Exclusión Mutua):**
   - **`dongle->mutex`:** Protege el estado interno de cada dongle (estado de ocupación, titular actual, timestamp de enfriamiento y su cola de prioridad).
   - **`data->state_mutex`:** Protege la lectura y modificación de variables globales de simulación (como el flag `sim_stopped`, contador secuencial global para desempates FIFO, y marcas temporales de compilación de cada programador).
   - **`data->print_mutex`:** Serializa los accesos a `printf` / `write`, garantizando que cada línea del log se emita de manera atómica sin interferencias.

2. **`pthread_cond_t` (Variables de Condición):**
   - **`dongle->cond`:** Asociada al mutex de cada dongle. Permite a los programadores esperar pasivamente cuando el dongle está ocupado o en enfriamiento, siendo despertados con `pthread_cond_broadcast` o `pthread_cond_timedwait` cuando el dongle es liberado o su cooldown expira.
   - Permite además una parada limpia y coordinada: cuando el monitor detecta que la simulación debe terminar, emite un broadcast a todas las condiciones para desbloquear a los hilos en espera sin dejar bloqueos residuales.

3. **Cola de Prioridad Min-Heap Personalizada:**
   - Cada dongle aloja un montículo binario mínimo (*binary min-heap*) que gestiona las peticiones pendientes según el planificador activo (`fifo` o `edf`).
   - El acceso al heap se realiza de forma totalmente protegida dentro de la sección crítica gobernada por el mutex del dongle correspondiente.
