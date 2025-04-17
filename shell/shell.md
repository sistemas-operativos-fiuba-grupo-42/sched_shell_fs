# shell

### Búsqueda en $PATH
-> **Responder**: ¿cuáles son las diferencias entre la syscall `execve(2)` y la familia de wrappers proporcionados por la librería estándar de C (libc) `exec(3)`?

La principal diferencia entre la syscall `execve(2)` y los wrappers de libc `exec(3)` es el nivel de abstracción. Es decir que la syscall interactúa directamente con en kernel, mientras que los wrappers son un intermediario entre el usuario y el kernel. 

Los wrappers de libc son invocados por el usuario, y según las letras en las que termine le agrega, a los parámetros enviados por el usuario, otros más para luego sí llamar a `execve(2)`. 

En cambio, cuando el usuario utiliza la syscall es él mismo quien se tiene que encargar de enviar todos los parámetros necesarios. 

-> **Responder**: ¿Puede la llamada a `exec(3)` fallar? ¿Cómo se comporta la implementación de la shell en ese caso?

Sí, puede fallar y devuelve -1, especificando el error en **errno**. Nuestra implementación, en ese caso imprime dicho error y termina el proceso con **exit_code** -1.

---

### Procesos en segundo plano

-> Explicar detalladamente el mecanismo completo utilizado.

*EN `runcmd.c`:*
- Hicimos que todos los comando excepto los BACK tengan el mismo PID y PGID para poder identificarlos
- Hicimos que el wait bloqueante se haga siempre y cuando no sea un  comando BACK
- Imprimimos la información de los procesos BACK de forma específica comparada a los demás.

*EN `sh.c`:*
- Creamos un stack alternativo para manejar las acciones señal cuando terminaban.
- Creamos un handler para la señal SIGCHLD que filtra usando el PID y el PGID para que se ejecute solo con los hijos que se ejecutaron en background.
- Hicimos el handler de tal forma que fuera `async-signal-safe` evitando el uso de funciones como `printf` de C.

*EN `exec.c`:*
- Modificamos el case PIPE para que los comandos "derechos" tengan el mismo PID y PGID.
- Agregamos la funcionalidad de ejecutar comandos en el case BACK simplemente parseando a struct backcmd y utilizando la función exec_cmd() 

-> **Responder**: ¿Por qué es necesario el uso de señales?

El uso de señales es necesario para que el kernel pueda liberar siempre los recursos del proceso que se ejecutó en background inmediatamente este termine. Sin esto, la shell podría acumular recursos innecesarios. El handler invocando waitpid permite llamar inmediatamente a wait solo para los hijos que ya terminaron, liberando su espacio de kernel.

---

### Flujo estándar
-> **Responder**: Investigar el significado de `2>&1`, explicar cómo funciona su forma general

El fragmento `2>&1` es parte de la redirección de flujos en shells como bash. Se utiliza para redirigir la salida de error estándar (stderr) al mismo lugar que la salida estándar (stdout). 
Se está redirigiendo a donde 1 apunta en ese momento (por eso el &, que indica "referencia a"). El 1 apunta a la salida estándar, mientras que el 2 a la salida de errores estándar.

-> Mostrar qué sucede con la salida de cat out.txt en el ejemplo.
Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, `2> &1 >out.txt`). ¿Cambió algo? Compararlo con el comportamiento en `bash(1)`.

En ambos casos, la salida de cat out.txt es la siguiente:

```shell
    ls: cannot access '/noexiste': No such file or directory
    /home:
    ulises
```

No cambió nada al invertir el orden de las redirecciones, ya que nuestra shell primero se ocupa de verificar las redirecciones de salida y luego la del error. 



Comparándolo con el comportamiento en bash(1), aquí sí importa el orden de las redirecciones, como se ve en la foto.

![alt text](parte2.png)

---

### Tuberías múltiples

-> **Responder**: Investigar que ocurre con el exit code reportado por la shell si se ejecuta un pipe. 
Cambia en algo?

Al ejecutar un pipe en nuestra shell, decidimos que el exit code que se reporte sea el del último comando del pipeline, es decir, el de la derecha del todo. Esto es una convención que se adoptó en shells como Bash y resulta cómodo ya que el exit code del último comando es un indicador global del éxito o fracaso de la operación. Además, en ese último comando se produce la salida "final" que interesa.

-> Que ocurre si, en un pipe, alguno de los comandos falla?

En nuestra implementación depende cual es el comando que falla, el comportamiento que se espera. En el caso de que falle el primer comando, pero el último sea exitoso, el exit code será 0, ya que se devuelve siempre el exit code del último comando ejecutado (el de la derecha). En caso de que el último falle, se devuelve un exit code de error. Esto lleva la misma lógica que Bash, con la diferencia de que dicha shell implementa una serie de codigos de error específicos (como 127 para "comando no encontrado", o 126 para "no se pudo ejecutar"), mientras que nuestros exit codes dependen de valores fijos que se determinan en cada situación. El exit code del último comando del pipe se ve ejecutando el comando $?. Adjuntamos como es en Bash, y luego como es en nuestra implementación.

``` shell
    true | false | echo hola
    echo $?
```

Al hacer esto en Bash, devuelve
        
``` shell
    hola
    0
```

Por más que haya fallado en false (el exit code debería ser 1), tanto Bash como nuestra implementación devuelven 0, por ser el exit code del comando echo hola.

---

### Variables de entorno temporarias

-> **Responder**: ¿Por qué es necesario hacerlo luego de la llamada a `fork(2)`?

Es necesario hacerlo luego de la llamada a `fork(2)` para que solo cambie el entorno de la ejecución de ese proceso. Si lo hiciéramos antes, el hijo heredaría las variables de entorno temporarias del padre, lo cual no queremos que suceda.

-> **Responder**: En algunos de los wrappers de la familia de funciones de `exec(3)` (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar `setenv(3)` por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de `exec(3)`.

-> ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.
Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.

No, el comportamiento no es exactamente el mismo si se usan funciones exec de la familia `execle()` o `execve()` pasando un arreglo de variables de entorno en lugar de usar `setenv()` previamente.
- Uso de `setenv()`:
    Las variables de entorno se modifican en el proceso actual.
    Cuando luego se llama a una función `exec()` (como `execlp()` o `execvp()`), el nuevo proceso hereda automáticamente el entorno del proceso original (ya modificado).
    Es más simple si se quiere agregar o modificar unas pocas variables sin tocar el entorno completo.
- Uso de `execle()` o `execve()` con arreglo de entorno:
    Las funciones `execle()` y  `execve()` permiten pasar un entorno completamente nuevo al proceso reemplazado.
    Si no se incluye una copia completa del entorno original más las nuevas variables, el nuevo proceso puede arrancar con un entorno diferente o incompleto (por ejemplo, sin **PATH**, **HOME**, etc.).
    El entorno del proceso padre no se modifica.
- Mismo comportamiento:
    No, salvo que se copie explícitamente el entorno original, o se modifique con las nuevas variables, y luego se pase ese entorno modificado como tercer argumento de `execle()` o `execve()`. De lo contrario, se estaría eliminando implícitamente todo el entorno original.
    Una posible forma de replicar el comportamiento de `setenv()` sería:
    Obtener el entorno actual del proceso, por ejemplo, desde la variable global environ.
    Copiar ese entorno a un nuevo arreglo de strings.
    Agregar o modificar las variables necesarias dentro de ese arreglo.
    Pasar ese nuevo arreglo como el tercer argumento a `execve()` o `execle()`.
---

### Pseudo-variables

-> **Responder**: Investigar al menos otras tres variables mágicas estándar, y describir su propósito.
    Incluir un ejemplo de su uso en bash (u otra terminal similar).

Algunas otras mágicas estándar son:

- `\$$` Esto representa el PID del proceso actual del script.

    ```shell
        $ echo $$
        5716
    ```

- `\$#` Indica la cantidad de argumentos pasados a un script o función.
    ```shell
        $ contar() { echo "Cantidad de argumentos: $#"; }
        $ contar uno dos tres
        Cantidad de argumentos: 3
    ```

- `\$@` | `$*` Por un lado, `$@` representa todos los argumentos pasados al script como una lista separada, respetando los espacios. En cambio `$*` también representa todos los argumentos, pero los junta en un solo string y no respeta las comillas como la otra variable mágica.
    ```shell
        echo "Usando \"\$@\":"
        for arg in "$@"; do
            echo "[$arg]"
        done

        echo ""
        echo "Usando \"\$*\":"
        for arg in "$*"; do
            echo "[$arg]"
        done

        $ ./argumentos.sh uno "dos con espacios" tres

        Usando "$@":
        [uno] 
        [dos con espacios] 
        [tres]

        Usando "$*":
        [uno dos con espacios tres]
    ```

- `\$0` Representa el nombre del script o comando que se está ejecutando
    ```shell
        echo $0
        /bin/bash
    ```

- `\$!` Devuelve el pid del último proceso ejecutado en el back.

    ```shell
        $ sleep 3 &
        [1] 5894
        $ echo $!
        5894
        [1]+  Done                    sleep 3
    ```


---

### Comandos built-in

-> **Responder**: ¿Entre `cd` y `pwd`, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)

`cd` no se podría implementar sin ser built-in porque tiene que cambiarse el directorio directamente en el proceso padre y si no fuera built-in solo sería cambiado en el proceso hijo, dejando al proceso padre de la terminal sin modificar.
pwd se podría implementar sin necesidad de ser built-in, ya que el directorio actual se podría obtener con `getcwd()` o leyendo la variable de entorno **PWD**. Si no fuera un built-in, sin embargo, cambiaría el proceso del hijo sin que el padre se entere. Entonces, el motivo de hacerlo como built-in radica en evitar crear un proceso nuevo, principalmente.

---

### Historial

---
