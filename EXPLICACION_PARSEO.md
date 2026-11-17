# Explicación Detallada del Parseo y Decisiones de Diseño (`parsing.c`)

> **Proyecto:** `codexion` (Currículo 42)  
> **Archivo implementado:** `coders/src/parsing.c`  
> **Header asociado:** `coders/includes/codexion.h`

---

## 1. El Problema: ¿Por qué el parseo generaba confusión?

Al iniciar el parseo en un proyecto como *Codexion*, es común encontrarse con varias dudas y errores típicos:

1. **Número erróneo de argumentos (`argc`):**
   - El programa espera 8 parámetros obligatorios.
   - Sin embargo, en C, `argv[0]` es siempre el nombre del propio binario (`./codexion`).
   - Por tanto, la comprobación correcta **no es `argc == 7` ni `argc == 8`, sino estrictamente `argc == 9`**.

2. **Uso ingenuo de `atoi`:**
   - La función estándar `atoi()` tiene limitaciones graves:
     - Si recibe `"abc"`, devuelve `0` sin indicar ningún error. No puedes distinguir si el usuario introdujo un cero real (`"0"`) o una letra.
     - Si el usuario pasa un número gigantesco (ej. `999999999999999999999999`), `atoi()` sufre un **desbordamiento de enteros (*integer overflow*)**, provocando comportamiento indefinido (*Undefined Behavior*).
     - Si se le pasa un string vacío `""` o con espacios, su comportamiento puede causar fallos.

3. **El argumento del *scheduler* no es un número:**
   - El 8.º parámetro (`argv[8]`) es una cadena de texto: `"fifo"` o `"edf"`.
   - Si se intenta pasar por un bucle numérico con `atoi()`, fallará irremediablemente. Debe evaluarse con `strcmp()`.

4. **Colisión de nombres con librerías del sistema:**
   - En Linux, `<pthread.h>` incluye internamente `<sched.h>`, donde ya existe una macro del sistema llamada `#define SCHED_FIFO 1`.
   - Si definimos un enum con `SCHED_FIFO`, el compilador lanza un error confuso (`expected identifier before numeric constant`). Por ello, renombramos nuestra política a `POLICY_FIFO` y `POLICY_EDF`.

5. **Restricciones estrictas de la Norma 42:**
   - Máximo **25 líneas por función**.
   - Máximo **5 funciones por archivo `.c`**.
   - Máximo **4 declaraciones de variables** al inicio de la función.
   - Sin variables globales.
   - Valores de retorno obligatoriamente entre paréntesis: `return (1);`.

---

## 2. Arquitectura de `parsing.c`: Las 5 Funciones Exactas

Para cumplir la Norma 42 y mantener el código modular, limpio y legible, el archivo se estructuró exactamente en **5 funciones**:

```
parse_args (Punto de entrada principal)
   ├── parse_numeric_args (Valida y convierte los 7 valores numéricos)
   │      ├── is_valid_numeric (Comprueba sintaxis: dígitos y '+')
   │      └── parse_safe_long (Convierte a número evitando overflow)
   └── parse_scheduler (Valida "fifo" o "edf" y asigna la política)
```

Veamos qué hace cada una y por qué se diseñó así:

---

### Función 1: `is_valid_numeric`
```c
static int	is_valid_numeric(const char *str)
{
	int	i;

	if (!str || !str[0])
		return (0);
	i = 0;
	if (str[i] == '+')
		i++;
	if (!str[i])
		return (0);
	while (str[i])
	{
		if (str[i] < '0' || str[i] > '9')
			return (0);
		i++;
	}
	return (1);
}
```

* **Propósito:** Validar que una cadena sea pura y exclusivamente numérica antes de intentar convertirla.
* **Por qué así:**
  - Si el puntero es `NULL` o la cadena está vacía (`""`), se rechaza de inmediato.
  - El enunciado prohíbe explícitamente números negativos: *"Rechaza entradas no válidas como números negativos"*. Por eso, **no se permite el signo `'-'`**; solo se permite un signo opcional `'+'`.
  - Si la cadena es solo `"+"` sin dígitos detrás, se rechaza (`if (!str[i]) return (0);`).
  - Si contiene espacios, letras o caracteres extraños (ej. `"800ms"` o `" 123"`), se rechaza.

---

### Función 2: `parse_safe_long`
```c
static int	parse_safe_long(const char *str, long long *out)
{
	long long	val;
	int			i;
	int			digit;

	val = 0;
	i = 0;
	if (str[i] == '+')
		i++;
	while (str[i])
	{
		digit = str[i] - '0';
		if (val > (9223372036854775807LL - digit) / 10)
			return (0);
		val = val * 10 + digit;
		i++;
	}
	*out = val;
	return (1);
}
```

* **Propósito:** Convertir la cadena validada a un entero de 64 bits (`long long`) garantizando inmunidad frente a desbordamientos (*overflow*).
* **Por qué así:**
  - En lugar de multiplicar a ciegas `val = val * 10 + digit`, primero se verifica matemáticamente si la operación superará `LLONG_MAX` (`9223372036854775807LL`):
    $$\text{val} > \frac{\text{LLONG\_MAX} - \text{digit}}{10}$$
  - Si superase el límite, la función aborta y retorna `0` (error).
  - Usar `long long` es fundamental en este proyecto porque los tiempos se miden en milisegundos desde 1970 (*Epoch*), alcanzando valores superiores a $1.7 \times 10^{12}$ ms, los cuales desbordan un `int` estándar de 32 bits ($2.14 \times 10^9$).

---

### Función 3: `parse_scheduler`
```c
static int	parse_scheduler(t_config *cfg, const char *str)
{
	if (!str)
		return (0);
	if (strcmp(str, "fifo") == 0)
	{
		cfg->scheduler = POLICY_FIFO;
		return (1);
	}
	if (strcmp(str, "edf") == 0)
	{
		cfg->scheduler = POLICY_EDF;
		return (1);
	}
	return (0);
}
```

* **Propósito:** Validar el 8.º argumento y traducir la cadena a un valor tipado (`enum`).
* **Por qué así:**
  - El enunciado establece: *"El valor debe ser exactamente uno de: fifo o edf"*.
  - Utilizamos `strcmp()` (autorizada en el subject) para comprobar la coincidencia exacta.
  - Si el usuario introduce cualquier otra cosa (ej. `"round-robin"`, `"FIFO"`, `"EDF"`, `"random"`), la función devuelve `0` y la simulación se detiene de forma segura.

---

### Función 4: `parse_numeric_args`
```c
static int	parse_numeric_args(t_config *cfg, char **argv)
{
	long long	vals[7];
	int			i;

	i = 0;
	while (i < 7)
	{
		if (!is_valid_numeric(argv[i + 1]))
			return (0);
		if (!parse_safe_long(argv[i + 1], &vals[i]))
			return (0);
		i++;
	}
	if (vals[0] <= 0 || vals[0] > 2147483647)
		return (0);
	if (vals[5] < 0 || vals[5] > 2147483647)
		return (0);
	cfg->num_coders = (int)vals[0];
	cfg->time_to_burnout = vals[1];
	cfg->time_to_compile = vals[2];
	cfg->time_to_debug = vals[3];
	cfg->time_to_refactor = vals[4];
	cfg->num_compiles_required = (int)vals[5];
	cfg->dongle_cooldown = vals[6];
	return (1);
}
```

* **Propósito:** Iterar los 7 argumentos numéricos (`argv[1]` a `argv[7]`), comprobar sus rangos específicos y volcar los datos en la estructura de configuración.
* **Por qué así:**
  - **Evita duplicación de código:** Un solo bucle de 0 a 6 valida la sintaxis y convierte los 7 números en un array auxiliar `vals[7]`.
  - **Validación de reglas del negocio:**
    - `vals[0]` (`number_of_coders`): Debe ser estrictamente mayor que cero (`> 0`) y caber en un `int` positivo. Una mesa con 0 programadores carece de sentido físico en la simulación.
    - `vals[5]` (`number_of_compiles_required`): Debe ser no negativo (`>= 0`) y caber en un `int`.
    - Los tiempos de burnout, compile, debug, refactor y cooldown son `>= 0` (garantizado al rechazar signos negativos en `is_valid_numeric`).

---

### Función 5: `parse_args`
```c
int	parse_args(t_config *config, int argc, char **argv)
{
	if (!config || argc != 9)
	{
		fprintf(stderr, "Error: invalid number of arguments.\n");
		fprintf(stderr, "Usage: %s <coders> <burnout> <compile> <debug> "
			"<refactor> <compiles_req> <cooldown> <fifo|edf>\n", argv[0]);
		return (1);
	}
	if (!parse_numeric_args(config, argv))
	{
		fprintf(stderr, "Error: invalid numeric arguments or out of bounds.\n");
		return (1);
	}
	if (!parse_scheduler(config, argv[8]))
	{
		fprintf(stderr, "Error: scheduler must be 'fifo' or 'edf'.\n");
		return (1);
	}
	return (0);
}
```

* **Propósito:** Actuar como la única interfaz pública de parseo llamada desde `main()`.
* **Por qué así:**
  - Verifica en primer lugar `argc == 9`. Si no es correcto, imprime por la salida de error (`stderr`) el mensaje de uso y retorna `1` sin realizar cálculos innecesarios.
  - Orquesta la validación de números y la validación del planificador de forma secuencial.
  - Retorna `0` en caso de éxito y `1` ante cualquier fallo.

---

## 3. Resumen de Ventajas de esta Implementación

| Aspecto | Implementación Anterior (Borrador) | Implementación Actual |
| :--- | :--- | :--- |
| **Comprobación de `argc`** | `argc == 7` (incorrecto) | `argc == 9` (exacto para 8 parámetros + binario) |
| **Control de tipos** | Bucle llamando a `atoi` en todos los args | Separación estricta entre números y cadena de scheduler |
| **Seguridad de enteros** | Vulnerable a desbordamiento | `parse_safe_long` con detección matemática de overflow |
| **Norma 42** | Múltiples errores de indentación y sintaxis | **100% OK en Norminette** (0 errores, 0 warnings) |
| **Compilación** | Fallos por colisión con `SCHED_FIFO` de Linux | Resuelto con `POLICY_FIFO` y flags `-Wall -Wextra -Werror -pthread` |

---

## 4. Pruebas Realizadas

Se ejecutó una batería de tests con los siguientes resultados:

1. **Sin argumentos (`./codexion`):**
   - Salida: `Error: invalid number of arguments.` / Código de salida: `1` ✅
2. **Menos argumentos de los debidos (`./codexion 4 800 200 200 200 5 fifo`):**
   - Salida: `Error: invalid number of arguments.` / Código de salida: `1` ✅
3. **Número negativo (`./codexion -4 800 200 200 200 5 10 fifo`):**
   - Salida: `Error: invalid numeric arguments or out of bounds.` / Código de salida: `1` ✅
4. **Cero programadores (`./codexion 0 800 200 200 200 5 10 fifo`):**
   - Salida: `Error: invalid numeric arguments or out of bounds.` / Código de salida: `1` ✅
5. **Letras mezcladas (`./codexion 4 800abc 200 200 200 5 10 fifo`):**
   - Salida: `Error: invalid numeric arguments or out of bounds.` / Código de salida: `1` ✅
6. **Cadena vacía (`./codexion 4 "" 200 200 200 5 10 fifo`):**
   - Salida: `Error: invalid numeric arguments or out of bounds.` / Código de salida: `1` ✅
7. **Scheduler no permitido (`./codexion 4 800 200 200 200 5 10 round_robin`):**
   - Salida: `Error: scheduler must be 'fifo' or 'edf'.` / Código de salida: `1` ✅
8. **Desbordamiento (`./codexion 4 999999999999999999999999999 200 200 200 5 10 fifo`):**
   - Salida: `Error: invalid numeric arguments or out of bounds.` / Código de salida: `1` ✅
9. **Argumentos válidos con FIFO (`./codexion 4 800 200 200 200 5 10 fifo`):**
   - Código de salida: `0` ✅
10. **Argumentos válidos con EDF (`./codexion 4 800 200 200 200 5 10 edf`):**
    - Código de salida: `0` ✅
11. **Argumentos con signo más explícito (`./codexion +4 +800 +200 +200 +200 +5 +10 edf`):**
    - Código de salida: `0` ✅
