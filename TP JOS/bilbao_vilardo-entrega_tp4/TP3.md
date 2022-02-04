TP3: Multitarea con desalojo
=============================

env_return
-----------

- Cuando termina el `umain`, se vuelve a la función `libmain`, la cual llama a `exit()` y esta a su vez llama al _wrapper_ de la _syscall_ `env_destroy`. La _syscall_, luego de eliminar el proceso y liberar la memoria que este utilizaba, llama a `sched_yield`, el cual termina con una llamada a `sched_halt`, que como no hay más procesos para ejecutar, llama al monitor indefinidamente. 

- En el TP anterior, `env_destroy` destruía el único proceso que había y llamaba al monitor. Ahora, luego de destruir el proceso, se llama a `sched_yield` para que el _scheduler_ le asigne el procesador a otro proceso.

sys_yield
----------

Al modificar la función `i386_init` para que lance tres procesos de `user/yield.c`, obtenemos la siguiente salida:
```console
[00000000] new env 00001000
[00000000] new env 00001001
[00000000] new env 00001002
Hello, I am environment 00001000.
Hello, I am environment 00001001.
Hello, I am environment 00001002.
Back in environment 00001000, iteration 0.
Back in environment 00001001, iteration 0.
Back in environment 00001002, iteration 0.
Back in environment 00001000, iteration 1.
Back in environment 00001001, iteration 1.
Back in environment 00001002, iteration 1.
Back in environment 00001000, iteration 2.
Back in environment 00001001, iteration 2.
Back in environment 00001002, iteration 2.
Back in environment 00001000, iteration 3.
Back in environment 00001001, iteration 3.
Back in environment 00001002, iteration 3.
Back in environment 00001000, iteration 4.
All done in environment 00001000.
[00001000] exiting gracefully
[00001000] free env 00001000
Back in environment 00001001, iteration 4.
All done in environment 00001001.
[00001001] exiting gracefully
[00001001] free env 00001001
Back in environment 00001002, iteration 4.
All done in environment 00001002.
[00001002] exiting gracefully
[00001002] free env 00001002
No runnable environments in the system!
```

Esto tiene sentido, ya que el programa se desaloja en cada iteración, permitiendo que se ejecute el siguiente proceso. Cuando el último proceso se desaloja, se vuelve al primero.

envid2env
----------

Si se ejecuta `sys_env_destroy(0)`, dentro de la función se va a llamar a `envid2env` la cual asigna al segundo argumento el _env_ con el ID solicitado. Cuando el ID es 0, `envid2env` asigna el _current env_. Esto significa que al llamar a `sys_env_destroy(0)` se está destruyendo el _env_ en ejecución.

dumbfork
---------

1. No, ya que en la función `page_map` se verifica que no se quiera dar permiso de escritura al nuevo _env_ cuando el padre no lo tiene.
2. ```c
    envid_t dumbfork(void) {
        // ...
        for (addr = UTEXT; addr < end; addr += PGSIZE) {
            bool readonly, pagina_valida, permisos_correctos;
            pagina_valida = uvpd[PDX(addr)] & PTE_P;
            permisos_correctos = (uvpt[PGNUM(addr)] & (PTE_P | PTE_U | PTE_W)) == (PTE_P | PTE_U | PTE_W);
            readonly = !(pagina_valida && permisos_correctos); 
   
            duppage(envid, addr, readonly);
        }
        // ...
    ```
3. ```c
    void
    duppage(envid_t dstenv, void *addr, bool read_only)
    {
        int r;
        if ((r = sys_page_alloc(dstenv, addr, PTE_P|PTE_U|PTE_W)) < 0)
            panic("sys_page_alloc: %e", r);

        int write_flag = (read_only) ? 0 : PTE_W;
        if ((r = sys_page_map(dstenv, addr, 0, UTEMP, PTE_P|PTE_U|write_flag)) < 0)
            panic("sys_page_map: %e", r);
        memmove(UTEMP, addr, PGSIZE);
        if ((r = sys_page_unmap(0, UTEMP)) < 0)
            panic("sys_page_unmap: %e", r);
    }
    ```

multicore_init
---------------

1. Copia el código de arranque de los CPUs no _booteables_ en la dirección de memoria (correspondiente a la dirección física `0x7000`) donde los mismos van a empezar su ejecución.
2. Se usa para guardar la dirección de memoria del inicio _stack_ del CPU que se está inicializando. Si esta se declarara en _kern/mpentry.S_, todos los CPUs no _booteables_ compartirían el _stack_ al tener la misma dirección.
3. Sabemos que `mpentry_start` está mapeado a la dirección (física) `0x7000`. Teniendo en cuenta que solo hay algunas pocas instrucciones entre esta etiqueta y la instrucción a evaluar, podemos decir que el registro _%eip_ contendrá un valor del estilo `0x7XXX`. Si lo redondeamos a 12 bits, como pide la consigna, obtendríamos `0x7000`.

ipc_recv
---------

Si el valor devuelto es `-E_INVAL`, podemos verificar el valor de `src`, ya que en caso de error este valdría 0. Si `src` es distinto a 0, entonces `-E_INVAL` fue envíado por el _sender_ y no como error.

sys_ipc_try_send
-----------------

Si queremos que sea bloqueante, deberíamos verificar si el proceso _reciever_ está esperando algún mensaje. En caso negativo, debemos poner al _sender_ en estado `ENV_NOT_RUNNABLE` y ceder el uso del CPU. Antes de hacer eso, deberíamos agregar al _sender_ a una lista enlazada en el proceso _receiver_ (que habría que agregar al `struct Env`), donde se guardarían todos los procesos que están esperando para enviarle algo.

Cuando el _receiver_ quiera recibir un mensaje, la _syscall_ evaluaría si tiene procesos esperando para enviarle algo y les cambia el estado a `ENV_RUNNABLE` de a uno, para que puedan retornar su ejecución y ahora sí enviarle el mensaje.

No debería haber un _deathlock_, ya que el _sender_ no _lockea_ nada hasta volver a despertarse. Y a su vez, el _receiver_ pone al _sender_ como `ENV_RUNNABLE` justo antes de ponerse a esperar.