# shell

### Búsqueda en $PATH
-> Responder: ¿cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?

La principal diferencia entre la syscall execve(2) y los wrappers de libc exec(3) es el nivel de abstracción. Es decir que la syscall interactua directamente con en kernel, mientras que los wrappers son un intermediario entre el usuario y el kernel. 

Los wrappers de libc son invocados por el usuario, y según las letras en las que termine le agrega, a los parámetros enviados por el usuario, otros más para luego sí llamar a execve(2). 

En cambio, cuando el usuario utiliza la syscall es él mismo quien se tiene que encargar de enviar todos los parámetros necesarios. 

## letras e, p, l

-> Responder: ¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?

Sí, puede fallar y devuelve -1, especificando el error en **errno**. Nuestra implementación, en ese caso imprime dicho error y termina el proceso con **exit_code** -1.

---

### Procesos en segundo plano
-> Responder: Investigar el significado de 2>&1, explicar cómo funciona su forma general

El fragmento 2>&1 es parte de la redirección de flujos en shells como bash. Se utiliza para redirigir la salida de error estándar (stderr) al mismo lugar que la salida estándar (stdout). 
Se está redirigiendo a donde 1 apunta en ese momento (por eso el &, que indica "referencia a"). El 1 apunta a la salida estándar, mientras que el 2 a la salida de erorres estándar.

- Mostrar qué sucede con la salida de cat out.txt en el ejemplo.
Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el comportamiento en bash(1).

    En ambos casos, la salida de cat out.txt es la siguiente:

    ```shell
    ls: cannot access '/noexiste': No such file or directory
    /home:
    ulises
    ```
    
    No cambió nada al invertir el orden de las redirecciones, ya que nuestra shell primero se ocupa de verificar las redirecciones de salida y luego la del error. 

    

    Comparandolo con el comportamiento en bash(1), aquí sí importa el orden de las redirecciones, como se ve en la foto.

    ![alt text](parte2.png)

---

### Flujo estándar

---

### Tuberías múltiples

-> Responder: Investigar que ocurre con el exit code reportado por la shell si se ejecuta un pipe. 
Cambia en algo?

    Al ejecutar un pipe en nuestra shell, decidimos que el exit code que se reporte sea el del ultimo comando del pipeline, es decir, el de la derecha del todo. Esto es una convencion que se adopto en shells como Bash y resulta comodo ya que el exit code del ultimo comando es un indicador global del exito o fracaso de la operacion. Ademas, en ese ultimo comando se produce la salida "final" que interesa.

Que ocurre si, en un pipe, alguno de los comandos falla?

    En nuestra implementacion depende cual es el comando que falla, el comportamiento que se espera. En el caso de que falle el primer comando, pero el ultimo sea exitoso, el exit code sera 0, ya que se devuelve siempre el exit code del ultimo comando ejecutado (el de la derecha). En caso de que el ultimo falle, se devuelve un exit code de error. Esto lleva la misma logica que Bash, con la diferencia de que dicha shell implementa una serie de codigos de error especificos (como 127 para "comando no encontrado", o 126 para "no se pudo ejecutar"), mientras que nuestros exit codes dependen de valores fijos que se determinan en cada situacion. El exit code del ultimo comando del pipe se ve ejecutando el comando $?. Adjuntamos imagen de como es en Bash, y luego otra de como es en nuestra implementacion.

---

### Variables de entorno temporarias

---

### Pseudo-variables

---

### Comandos built-in

---

### Historial

---
