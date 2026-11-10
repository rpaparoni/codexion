# Guía de Desarrollo: Codexion (Currículo 42)

> Esta guía está pensada para que desarrolles el proyecto por ti mismo, paso a paso, comprendiendo cada línea y cada decisión de diseño para superar tanto la evaluación entre pares como las pruebas de *recode*.

---

## 1. Diagnóstico y Ajustes sobre tu Código Actual

Actualmente en tu repositorio tienes los siguientes archivos base:
- `coders/includes/codexion.h`
- `coders/src/main.c`
- `coders/src/parsing.c`
- `coders/Makefile`

### A. Ajustes en `codexion.h`
1. **Librerías autorizadas que debes incluir:**
   El subject autoriza funciones específicas. Necesitarás incluir en el header:
   ```c
   #include <pthread.h>    /* hilos, mutexes, variables de condición */
   #include <sys/time.h>   /* gettimeofday */
   #include <stdio.h>      /* printf, fprintf */
   #include <stdlib.h>     /* malloc, free, atoi */
   #include <unistd.h>     /* usleep, write */
   #include <string.h>     /* strcmp, strlen, memset */
   ```
2. **Norma 42 en el header:**
   - Todas las directivas (`#include`, `typedef`, prototipos) deben estar dentro del bloque `#ifndef CODEXION_H / # define CODEXION_H`.
   - Recuerda tabular los nombres de tipos y prototipos conforme a la norma (sin espacios múltiples).
3. **Estructuras que debes diseñar:**
   - Necesitarás representar:
     - El planificador (`enum { SCHED_FIFO, SCHED_EDF }`).
     - El nodo y la estructura de la **cola de prioridad (min-heap)**.
     - Cada **dongle** (su mutex, su variable de condición, estado libre/ocupado, timestamp de fin de *cooldown* y su propio heap de peticiones).
     - Cada **programador/coder** (ID, hilo `pthread_t`, última vez que empezó a compilar, contador de compilaciones, índices de sus dongles izquierdo y derecho).
     - La estructura global **`t_data`** que agrupa la configuración compartida, los arrays de dongles y programadores, los mutexes de control (`state_mutex`, `print_mutex`) y el flag de parada (`sim_stopped`).

---

### B. Ajustes en `main.c`
1. **Número de argumentos:**
   - En tu archivo tienes `if (argc == 7)`.
   - **Corrección obligatoria:** El subject define exactamente **8 argumentos**, más el nombre del ejecutable (`argv[0]`), por lo que la condición debe ser:
     ```c
     if (argc != 9)
         /* Mostrar error y uso del programa */
     ```
2. **Flujo de `main`:**
   El `main` debe tener una responsabilidad clara y concisa:
   - Declarar la estructura principal `t_data data;`.
   - Llamar a la función de parseo de argumentos. Si falla, retornar error (`return (1);`).
   - Inicializar las estructuras de datos, mutexes y variables de condición.
   - Lanzar los hilos de los programadores y el hilo monitor.
   - Esperar a que terminen los hilos (`pthread_join`).
   - Liberar toda la memoria y destruir todos los mutexes/condvars.

---

### C. Ajustes en `parsing.c`
1. **Validación estricta de argumentos:**
   El enunciado exige: *"Rechaza entradas no válidas como números negativos, valores no enteros o un scheduler diferente de fifo o edf."*
   - Debes verificar que los primeros 7 argumentos contengan exclusivamente caracteres numéricos (`'0'` a `'9'`, opcionalmente un signo `'+'`).
   - `number_of_coders` debe ser estrictamente `> 0`.
   - `time_to_burnout`, `time_to_compile`, `time_to_debug`, `time_to_refactor`, `number_of_compiles_required`, `dongle_cooldown` deben ser `>= 0`.
   - Prevenir desbordamiento de enteros (overflow) al convertir cadenas a números (`long long`).
2. **El argumento 8 (`scheduler`):**
   - No se procesa con `atoi`. Es una cadena de texto.
   - Debes usar `strcmp(argv[8], "fifo") == 0` o `strcmp(argv[8], "edf") == 0`. Si no es ninguno de los dos, rechazar con error.

---

### D. Ajustes en `Makefile`
- Asegúrate de incluir el flag `-pthread` en `CFLAGS = -Wall -Wextra -Werror -pthread`.
- Agrega `-I includes` para que el compilador encuentre `codexion.h`.
- Conforme crees nuevos archivos `.c` modulares, agrégalos a la variable `SRCS`.

---

## 2. Herramientas y Primitivas Concurrenciales

Para este proyecto debes dominar las siguientes primitivas de POSIX:

### 1. `pthread_mutex_t` (Exclusión Mutua)
- **Para qué sirve:** Evitar condiciones de carrera (*race conditions*) y asegurar que solo un hilo modifique un dato a la vez.
- **Dónde lo necesitas:**
  - **Un mutex por cada dongle:** Para consultar/cambiar si está ocupado y modificar su cola de prioridad.
  - **`print_mutex`:** Para que los `printf` no se mezclen en la terminal (requisito: mensajes serializados).
  - **`state_mutex`:** Para leer o escribir variables compartidas como `sim_stopped`, marcas de tiempo y número de compilaciones.

### 2. `pthread_cond_t` (Variables de Condición)
- **Para qué sirve:** Permite a un hilo suspenderse eficientemente mientras espera a que se cumpla una condición (por ejemplo: que un dongle se libere o pase su cooldown), sin consumir CPU en bucles infinitos (*busy-waiting*).
- **Funciones clave:**
  - `pthread_cond_wait(&cond, &mutex)`: Libera el mutex y duerme al hilo de forma atómica. Al despertar, vuelve a adquirir el mutex.
  - `pthread_cond_timedwait(&cond, &mutex, &timespec)`: Duerme al hilo hasta que sea avisado O hasta que se alcance un tiempo límite absoluto (ideal para esperar el `dongle_cooldown`).
  - `pthread_cond_broadcast(&cond)`: Despierta a todos los hilos que esperan en esa condición (ideal cuando un dongle se libera o cuando la simulación se detiene).

### 3. `gettimeofday` (Medición del Tiempo)
- Retorna segundos y microsegundos transcurridos desde 1970 (*Epoch*).
- Fórmula recomendada para obtener milisegundos absolutos:
  ```c
  long long get_time_ms(void)
  {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      return ((tv.tv_sec * 1000LL) + (tv.tv_usec / 1000LL));
  }
  ```
- Para mostrar el tiempo relativo en los logs: `tiempo_actual - data->start_time`.

---

## 3. Hoja de Ruta Paso a Paso

Te recomendamos abordar el proyecto en este orden lógico:

```
Paso 1: Parsing de Argumentos
   ↓
Paso 2: Cola de Prioridad (Min-Heap en C)
   ↓
Paso 3: Estructuras, Inicialización y Limpieza (Sin Leaks)
   ↓
Paso 4: Utilidades de Tiempo y Logs Serializados
   ↓
Paso 5: Gestión de Dongles (Adquisición, Cooldown y Prevención de Deadlock)
   ↓
Paso 6: Rutina del Programador (Compilar, Depurar, Refactorizar)
   ↓
Paso 7: Hilo Monitor (Detección de Burnout < 10 ms)
   ↓
Paso 8: Pruebas Exhaustivas y Casos Límite
```

---

### Paso 1: Parsing y Validación (`src/parsing.c`)
- **Objetivo:** Recibir los 8 argumentos, comprobar su validez sintáctica y numérica, y guardarlos en `t_data`.
- **Estrategia sugerida:**
  1. Escribe una función auxiliar `is_number(char *str)` que compruebe que no esté vacío y que solo tenga dígitos (permitiendo un `+` inicial).
  2. Escribe una función de conversión segura a `long long` comprobando que no supere los límites de `int` o `long long`.
  3. Compara `argv[8]` con `"fifo"` y `"edf"`.
  4. Si alguna validación falla, muestra un mensaje descriptivo y haz que el programa devuelva 1.

---

### Paso 2: Cola de Prioridad / Min-Heap (`src/heap.c`)
- **Por qué es obligatoria:** El enunciado dice: *"Se debe implementar una cola de prioridad (heap) para la planificación FIFO/EDF (C89 no tiene una biblioteca estándar para esto)."*
- **Cómo modelar un Min-Heap en C:**
  Un heap binario se almacena en un array de tamaño fijo o dinámico (por ejemplo, con capacidad para el número de programadores):
  - Hijo izquierdo del nodo $i$: `2 * i + 1`
  - Hijo derecho del nodo $i$: `2 * i + 2`
  - Padre del nodo $i$: `(i - 1) / 2`
- **Estructura del nodo del heap:**
  ```c
  typedef struct s_heap_node
  {
      int       coder_id;  /* ID del programador */
      long long key;       /* En FIFO: turno de llegada. En EDF: deadline */
      long long seq;       /* Desempate (número de secuencia incremental) */
  } t_heap_node;
  ```
- **Comparador:**
  El nodo $A$ tiene mayor prioridad (va más arriba en el min-heap) que el nodo $B$ si:
  - `A.key < B.key`
  - O en caso de empate (`A.key == B.key`), si `A.seq < B.seq` (para desempatar por orden de llegada).
- **Operaciones necesarias:**
  - `heap_init(t_heap *heap, int capacity)`: Aloja memoria para los nodos.
  - `heap_push(t_heap *heap, int coder_id, long long key, long long seq)`: Inserta al final y hace *sift-up* (subir mientras sea menor que su padre).
  - `heap_peek(t_heap *heap)`: Devuelve el ID del programador con máxima prioridad (la raíz, índice 0).
  - `heap_pop(t_heap *heap)`: Extrae la raíz, mueve el último elemento arriba y hace *sift-down* (bajar intercambiando con el hijo menor).
  - `heap_destroy(t_heap *heap)`: Libera la memoria alojada.

---

### Paso 3: Estructuras, Inicialización y Limpieza (`src/init.c` y `src/cleanup.c`)
- **Estructuras en `codexion.h`:**
  - `t_dongle`: ID, mutex, condvar, `is_held`, `holder_id`, `cooldown_until`, y su `t_heap`.
  - `t_coder`: ID (1 a N), `last_compile_start`, `compiles_count`, dongle izquierdo y derecho, puntero a `t_data`.
  - `t_data`: Variables del enunciado, array de dongles, array de coders, mutexes globales.
- **Topología de la mesa circular:**
  Si hay $N$ programadores (índices 0 a $N-1$):
  - El programador $i$ tiene a su izquierda el dongle $i$ y a su derecha el dongle $(i + 1) \% N$.
  - Cada dongle $d$ es compartido únicamente entre dos programadores adyacentes.
- **Regla de oro de 42 para la memoria:**
  - Si un `malloc`, `pthread_mutex_init` o `pthread_cond_init` falla a mitad de la inicialización, debes destruir lo ya creado y liberar la memoria previa antes de salir.
  - Escribe una función centralizada de `cleanup_simulation` que se encargue de todo.

---

### Paso 4: Utilidades de Tiempo y Logs Serializados (`src/time_utils.c`, `src/print_utils.c`)
- **Logs atómicos:**
  - Cada vez que un programador cambie de estado, debes llamar a una función `print_status(coder, status)`.
  - Esta función debe:
    1. Bloquear `print_mutex`.
    2. Verificar si la simulación ya se detuvo (`sim_stopped`). Si ya se detuvo, desbloquear y salir inmediatamente.
    3. Imprimir con el formato exacto del subject:
       - `timestamp X has taken a dongle`
       - `timestamp X is compiling`
       - `timestamp X is debugging`
       - `timestamp X is refactoring`
    4. Desbloquear `print_mutex`.
- **Dormir con precisión (`precise_sleep`):**
  - No uses un simple `usleep(ms * 1000)` para periodos largos porque no es preciso y además el hilo no se enteraría si la simulación se detiene en medio de su espera.
  - Duerme en intervalos cortos (ej. `usleep(250)`) comprobando en un bucle si ha transcurrido el tiempo necesario o si `sim_stopped` se ha activado.

---

### Paso 5: Gestión de Dongles y Prevención de Deadlock (`src/dongle.c`)

#### A. ¿Cómo evitar el Interbloqueo (*Deadlock*) matemáticamente?
- Las cuatro condiciones de Coffman para que exista un deadlock son:
  1. *Exclusión mutua* (los dongles no se pueden compartir).
  2. *Retención y espera* (sostener un recurso mientras se espera otro).
  3. *No apropiación* (no se puede quitar un dongle a la fuerza).
  4. *Espera circular* (el programador 1 espera al 2, el 2 al 3... y el $N$ al 1).
- **Solución (Ordenación Jerárquica de Recursos):**
  - Cada programador $i$ necesita dos dongles: `left` y `right`.
  - Define `first = min(left, right)` y `second = max(left, right)`.
  - **Todos los programadores adquieren obligatoriamente su dongle de menor índice primero, y el de mayor índice después.**
  - Como nadie puede pedir un dongle inferior después de tener uno superior, el grafo de dependencias es un Grafo Acíclico (DAG). **La espera circular es imposible.**

#### B. Algoritmo de toma de un Dongle:
Cuando el programador necesita un dongle:
1. Bloquea el mutex del dongle.
2. Calcula su clave de prioridad:
   - En **FIFO**: `key = ++global_request_seq` (o timestamp de llegada).
   - En **EDF**: `key = last_compile_start + time_to_burnout`.
3. Inserta su petición en el `heap` del dongle.
4. Entra en un bucle:
   - Si no soy la raíz del heap (`heap_peek != mi_id`) o el dongle está ocupado:
     - Espera con `pthread_cond_wait(&dongle->cond, &dongle->mutex)`.
   - Si soy la raíz del heap y el dongle está libre:
     - Comprueba el *cooldown*: ¿`tiempo_actual >= dongle->cooldown_until`?
     - Si el cooldown ya pasó:
       - Saca la petición del heap (`heap_pop`).
       - Marca el dongle como ocupado (`is_held = 1`, `holder = mi_id`).
       - Desbloquea el mutex.
       - Imprime `timestamp X has taken a dongle`.
       - Termina con éxito.
     - Si el cooldown no ha pasado:
       - Espera el tiempo restante con `pthread_cond_timedwait`.
5. Al salir del bucle, recuerda desbloquear el mutex.

#### C. Liberación y Cooldown de un Dongle:
1. Bloquea el mutex del dongle.
2. Marca `is_held = 0`.
3. Calcula `cooldown_until = tiempo_actual + dongle_cooldown`.
4. Avisa a los hilos en espera con `pthread_cond_broadcast(&dongle->cond)`.
5. Desbloquea el mutex.

---

### Paso 6: Rutina del Programador (`src/routine.c`)

Cada hilo de programador ejecuta un bucle continuo:
1. **Caso especial $N = 1$:**
   - Si solo hay 1 programador, en la mesa solo hay 1 dongle.
   - Toma el único dongle (`has taken a dongle`).
   - Como necesita 2 dongles para compilar y no existe un segundo dongle, simplemente espera pasivamente hasta que el monitor detecte su agotamiento (*burnout*).
2. **Caso general ($N > 1$):**
   - Adquiere el primer dongle (`first = min(left, right)`).
   - Adquiere el segundo dongle (`second = max(left, right)`).
   - **Fase de Compilación:**
     - Actualiza `last_compile_start = tiempo_actual`.
     - Incrementa `compiles_count`.
     - Imprime `timestamp X is compiling`.
     - Duerme durante `time_to_compile` con `precise_sleep`.
   - **Liberación de recursos:**
     - Libera ambos dongles (los dos entran en *cooldown*).
   - **Fase de Depuración:**
     - Imprime `timestamp X is debugging`.
     - Duerme durante `time_to_debug`.
   - **Fase de Refactorización:**
     - Imprime `timestamp X is refactoring`.
     - Duerme durante `time_to_refactor`.
   - Comprueba si se ha alcanzado la condición de parada o si el hilo debe volver a solicitar dongles.

---

### Paso 7: Hilo Monitor (`src/monitor.c`)

El enunciado requiere:
*"Debe existir un hilo monitor separado que detecte el agotamiento con precisión y detenga la simulación. El log de agotamiento debe imprimirse dentro de los 10 ms posteriores al momento real en que se produce."*

- **Lógica del monitor:**
  Ejecuta un bucle rápido (con pausas de `usleep(500)` para comprobar dos veces por milisegundo):
  1. Bloquea `state_mutex`.
  2. Para cada programador $i$:
     - Si `tiempo_actual - coder[i].last_compile_start >= time_to_burnout`:
       - Ha ocurrido un agotamiento.
       - Marca `sim_stopped = 1`.
       - Llama a `print_burnout(data, coder[i].id)`.
       - Despierta a todos los hilos que puedan estar esperando dongles (`pthread_cond_broadcast` en todos los dongles).
       - Desbloquea `state_mutex` y termina su función.
  3. Comprueba si se ha cumplido el número de compilaciones requeridas:
     - Si `number_of_compiles_required > 0`, y todos los programadores tienen `compiles_count >= number_of_compiles_required`:
       - Marca `sim_stopped = 1`.
       - Despierta a todos los hilos para que terminen limpiamente.
       - Desbloquea `state_mutex` y termina.
  4. Desbloquea `state_mutex` y duerme `500` microsegundos antes de la siguiente iteración.

---

## 4. Batería de Pruebas y Casos Límite a Probar

Antes de dar el proyecto por finalizado, realiza estas comprobaciones:

1. **Norminette:**
   ```bash
   norminette coders/
   ```
   No debe arrojar ningún error de formato, líneas de funciones (> 25), número de funciones por archivo (> 5) ni variables sin declarar arriba.

2. **Caso $N = 1$:**
   ```bash
   ./codexion 1 800 200 200 200 5 10 fifo
   ```
   Debe tomar 1 dongle y morir a los 800 ms aproximadamente.

3. **Caso de Parada por Compilaciones:**
   ```bash
   ./codexion 4 800 200 200 200 3 0 edf
   ```
   Debe completar 3 compilaciones por cada uno de los 4 programadores y finalizar limpiamente sin que nadie se agote.

4. **Comparativa FIFO vs EDF:**
   - Prueba con parámetros ajustados donde el orden FIFO cause inanición a un programador con poco tiempo, mientras que EDF le otorgue prioridad y evite su agotamiento.

5. **Detección de Fugas de Memoria (Valgrind):**
   ```bash
   valgrind --leak-check=full --show-leak-kinds=all ./codexion 4 800 200 200 200 2 10 edf
   ```
   Debe indicar: `All heap blocks were freed -- no leaks are possible`.

6. **Detección de Data Races (ThreadSanitizer):**
   Compila temporalmente agregando `-fsanitize=thread` en el `Makefile` y ejecuta varias simulaciones. No debe haber ningún aviso de colisión entre hilos.

---

## 5. Preguntas Clave para tu Defensa (Peer-Evaluation)

Ten claras estas respuestas para cuando tus compañeros te evalúen:
1. **¿Cómo demuestras que tu programa no puede entrar en Deadlock?**
   - *Respuesta:* Por el orden jerárquico de recursos. Al obligar a cada programador a pedir siempre `min(left, right)` antes que `max(left, right)`, es imposible formar un ciclo de retención y espera entre los hilos (rompemos la 4ª condición de Coffman).
2. **¿Por qué usas `pthread_cond_timedwait` para el cooldown en lugar de un `usleep`?**
   - *Respuesta:* Porque `pthread_cond_timedwait` libera el mutex mientras espera, permitiendo que otros hilos utilicen el dongle o que el hilo sea despertado de inmediato si la simulación se interrumpe, evitando además el consumo innecesario de CPU (*busy waiting*).
3. **¿Cómo funciona tu cola de prioridad?**
   - *Respuesta:* Explica la estructura de tu min-heap, cómo calculas el índice del padre `(i - 1) / 2` y los hijos `2 * i + 1`, y cómo el comparador prioriza la clave menor (en EDF el deadline más próximo, y en FIFO el turno más antiguo).
