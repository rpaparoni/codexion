# Codexion - Registro de Progreso y Contexto del Proyecto

> **Propósito de este archivo:** Registrar el estado actual, la arquitectura técnica, las decisiones de diseño y las tareas pendientes para poder retomar el proyecto en cualquier momento o sesión futura sin perder el contexto.

---

## 1. Resumen Ejecutivo del Proyecto
- **Proyecto:** `codexion` (Currículo 42)
- **Tema:** Concurrencia avanzada, hilos POSIX (`pthread`), mutexes, variables de condición (`pthread_cond_t`), colas de prioridad (min-heap) y algoritmos de planificación (`fifo` y `edf`).
- **Problema de fondo:** Simulación concurrente inspirada en el problema de los filósofos comensales (Dining Philosophers), pero adaptado a programadores que compiten por dongles USB compartidos en una mesa circular para compilar código cuántico.
- **Restricciones clave de 42:**
  - Código escrito en C conforme a la **Norma 42 v3/v4**.
  - Prohibidas las variables globales.
  - Prohibido el uso de `libft`.
  - Sin fugas de memoria (`valgrind / fsanitize`).
  - Detección de agotamiento (*burnout*) dentro de los **10 ms** posteriores al evento real.
  - Compilación con flags: `-Wall -Wextra -Werror -pthread`.

---

## 2. Argumentos del Programa
El binario `./codexion` recibe exactamente 8 argumentos obligatorios:
```bash
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

| Argumento | Tipo | Descripción |
| :--- | :--- | :--- |
| `number_of_coders` | `int > 0` | Número de personas y de dongles en la mesa circular. |
| `time_to_burnout` | `ms >= 0` | Tiempo máximo sin iniciar una compilación antes de quemarse. |
| `time_to_compile` | `ms >= 0` | Duración de la fase de compilación (requiere 2 dongles). |
| `time_to_debug` | `ms >= 0` | Duración de la fase de depuración (dongles ya liberados). |
| `time_to_refactor` | `ms >= 0` | Duración de la fase de refactorización (antes de volver a pedir dongles). |
| `number_of_compiles_required` | `int >= 0` | Fin de la simulación si todos compilan esta cantidad de veces. |
| `dongle_cooldown` | `ms >= 0` | Tiempo de enfriamiento en el que un dongle liberado no puede ser tomado. |
| `scheduler` | `cadena` | Política de arbitraje para dongles: estrictamente `"fifo"` o `"edf"`. |

---

## 3. Decisiones de Arquitectura y Sincronización

### A. Prevención de Interbloqueos (Deadlock / Condiciones de Coffman)
Para romper la condición de **Espera Circular (Circular Wait)**:
- Cada programador $i$ se sienta entre los dongles `left` y `right`.
- Se establece un orden jerárquico global estricto: **siempre se solicita primero `min(left, right)` y luego `max(left, right)`**.
- Al ser el grafo de solicitud de recursos estrictamente acíclico, el interbloqueo es matemáticamente imposible.

### B. Planificación (`FIFO` vs `EDF`) y Cola de Prioridad (Min-Heap)
- Cada dongle dispone de un mutex propio, una variable de condición y una **cola de prioridad binaria (min-heap)**.
- **FIFO:** Clave de ordenación = secuencia incremental de solicitud (`request_seq`).
- **EDF (Earliest Deadline First):** Clave de ordenación = `deadline = last_compile_start + time_to_burnout`.
  - En caso de empate en deadline, se utiliza FIFO (`request_seq`) como desempate.
- Un programador solo toma el dongle cuando está en la raíz del heap (`heap_peek`), el dongle está libre y su periodo de `dongle_cooldown` ha expirado.

### C. Enfriamiento de Dongles (*Cooldown*)
- Cuando un dongle es devuelto, se anota `cooldown_until = timestamp_actual + dongle_cooldown`.
- El hilo en espera utiliza `pthread_cond_timedwait` para esperar de manera eficiente sin bucles activos (*busy-waiting*).

### D. Monitor de Agotamiento (*Burnout*) y Serialización de Logs
- Un hilo independiente (monitor) vigila periódicamente con precisión de sub-milisegundo si algún programador supera `time_to_burnout`.
- Un `print_mutex` serializa todas las salidas por terminal para evitar mezcla de líneas (*data races* en salida estándar).
- En cuanto ocurre un agotamiento o se completan las compilaciones, el monitor activa `sim_stopped = 1` y despierta a todos los hilos con `pthread_cond_broadcast`.

---

## 4. Estado de los Archivos del Proyecto

```
codexion/
├── codexion_enunciado_del_proyecto.md  # Enunciado oficial del proyecto
├── README.md                           # README obligatorio (Capítulo VII)
├── PROGRESS.md                         # Este documento de contexto y seguimiento
└── coders/
    ├── Makefile                        # Reglas: all, clean, fclean, re, codexion
    ├── includes/
    │   └── codexion.h                  # Header con structs, enums y prototipos
    └── src/
        ├── main.c                      # Punto de entrada y orquestador
        ├── parsing.c                   # Validación y parseo de argumentos
        ├── init.c                      # Inicialización de memoria, mutexes y condvars
        ├── cleanup.c                   # Limpieza y liberación de recursos
        ├── heap.c                      # Implementación de cola de prioridad (min-heap)
        ├── dongle.c                    # Gestión de adquisición y enfriamiento de dongles
        ├── routine.c                   # Ciclo de vida del programador (compilar, depurar...)
        ├── monitor.c                   # Hilo monitor de agotamiento y parada
        ├── time_utils.c                # Medición de tiempo preciso y esperas
        └── print_utils.c               # Salida serializada segura
```

---

## 5. Hoja de Ruta (Checklist)

- [x] **Fase 1: Configuración, Contexto y Esqueleto**
  - [x] Análisis del enunciado y restricciones de la Norma 42.
  - [x] Creación de `PROGRESS.md` para persistencia del contexto.
  - [x] Redacción del `README.md` según los requisitos del Capítulo VII.
  - [x] Definición del header `codexion.h` y modularización de archivos en `coders/src/`.
  - [x] Configuración del `Makefile` con flags `-Wall -Wextra -Werror -pthread`.

- [ ] **Fase 2: Parseo y Validación de Argumentos (`parsing.c`)**
  - [ ] Comprobar exactamente 8 argumentos.
  - [ ] Validación de enteros positivos/no negativos y límites de desbordamiento.
  - [ ] Validación exacta de `"fifo"` o `"edf"`.

- [ ] **Fase 3: Estructuras de Datos y Min-Heap (`heap.c`)**
  - [ ] Funciones `heap_init`, `heap_push`, `heap_pop`, `heap_peek`, `heap_destroy`.
  - [ ] Sift-up y sift-down respetando el comparador de prioridades (FIFO y EDF).

- [ ] **Fase 4: Inicialización y Destrucción Limpia (`init.c`, `cleanup.c`)**
  - [ ] Inicialización de mutexes, variables de condición y memoria dinámica.
  - [ ] Limpieza total sin memory leaks ante fallos intermedios y al finalizar.

- [ ] **Fase 5: Temporización y Logs (`time_utils.c`, `print_utils.c`)**
  - [ ] `get_time_ms` con `gettimeofday`.
  - [ ] `precise_sleep` responsivo a la señal de fin.
  - [ ] `print_status` protegido por mutex.

- [ ] **Fase 6: Adquisición de Dongles y Cooldown (`dongle.c`)**
  - [ ] Entrada en cola de prioridad y espera con `pthread_cond_timedwait`.
  - [ ] Liberación e inicio de enfriamiento (*cooldown*).

- [ ] **Fase 7: Ciclo del Programador (`routine.c`)**
  - [ ] Caso especial: 1 solo programador (toma 1 dongle y espera hasta agotarse).
  - [ ] Casos generales: adquirir dongles en orden ascendente, compilar, depurar, refactorizar.

- [ ] **Fase 8: Hilo Monitor (`monitor.c`)**
  - [ ] Verificación de agotamiento con tolerancia estricta (< 10 ms).
  - [ ] Comprobación de número de compilaciones requeridas.
  - [ ] Difusión de parada segura para todos los hilos.

- [ ] **Fase 9: Batería de Pruebas y Validación**
  - [ ] Comprobación con `norminette`.
  - [ ] Pruebas de casos borde: $N=1$, cooldown largo, tiempos extremos.
  - [ ] Detección de fugas con Valgrind / AddressSanitizer.
  - [ ] Verificación de ausencia de agotamiento en EDF con parámetros viables.

---

## 6. Instrucciones para Retomar en Próximas Sesiones
Al abrir una nueva sesión, bastará con pedir al asistente:
> *"Revisa `PROGRESS.md` y continuemos con la siguiente fase pendiente de la hoja de ruta."*
Esto garantizará que el asistente tenga todo el contexto inmediatamente alineado.
