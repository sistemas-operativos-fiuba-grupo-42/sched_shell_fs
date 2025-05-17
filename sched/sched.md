# sched

## Seguimiento cambio de contexto con GDB

Contenido de la pila al correr la funcion `context_switch`. Se puede apreciar como el contenido de la pila va cambiando dependiendo del offset

Luego del `popa`, el cual hace pop de todos los registros de propósito general, se puede ver que la pila se movio 8 posiciones, una por cada registro (edi, esi, ebp, esp, ebx, edx, ecx, eax).

![](img/context_switch1.jpg)

Luego de `pop $es` y `pop $ds`, se mueve una posición, ya que se esta sacando un registro de la pila. 

![](img/context_switch2.jpg)

Se puede apreciar como el contenido de los registers cambia luego de la instruccion `iret`, el cual modifica los siguientes registros: eip, cs, eflags, esp y ss.

![](img/context_switch3.jpg)



## Política de scheduling con prioridades

Primero definimos las prioridades con tres tipos: 0; 1; 2, siendo 0 la prioridad más alta.

Todos los procesos comienzan con prioridad 0, la cual puede ser modificada por el usuario mediante el uso de la syscall `sys_set_priority()`. Asimismo, creamos la syscall `sys_get_priority()` para obtener la prioridad de un proceso en particular. 

El scheduler con prioridades que implementamos, consiste en recorrer los procesos y guardar en el `struct Env *next[]`, para cada prioridad, el que se encuentre con la posibilidad de enviarlo a correr y tenga la menor cantidad de `env_runs`. Entonces, para cada proceso no NULL seleccionado, se corre el de mayor prioridad (el menor número entero) siempre y cuando las variables globales priority_$N_runs sea menor que las constantes globales `MAX_RUNS_PRIORITY_$N` (siendo $N el número de prioridad). Si ningun proceso seleccionado se puede correr, se corre el proceso actual, de estar en estado ENV_RUNNING. 

Una vez que todos los `priority_$N_runs` alcanzan las `MAX_RUNS_PRIORITY_$N` ó cuando se quiere ejecutar un proceso de menor prioridad, y los de mayor prioridad están disponibles pero han alcanzado su cuota máxima, se reinician los `priority_$N_runs` a cero, dando la posibilidad de ejecutar procesos de prioridades altas que estaban estáticos y evitando el `sched_halt` cuando hay algún proceso disponible para correr.

### Test de prioridades

Para comprobar el funcionamiento esperado para nuestro scheduler con prioridades, creamos dos procesos de usuario, los cuales cada uno se setea su prioridad en 1 y 2 respectivamente, para verificar que luego el scheduler seleccionaba primero el de prioridad más alta (menor número) y luego el otro.  

![](img/prueba_procesos_distinta_prioridad.jpg)
