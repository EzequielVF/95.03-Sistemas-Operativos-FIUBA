TP2: Procesos de usuario
========================

env_alloc
---------

1. Los primeros cinco identificadores se asignan en `0x1000`, `0x1001`, `0x1002`, `0x1003`, `0x1004`. Esto es porque al inicializar el vector de _environments_ se establecen todos los ids en 0 lo cual hace que la variable _generation_ valga `0x1000` y luego a eso se le aplica un OR con el número de índice del _env_.
2. La primera vez que se lanza el proceso asociado al _env_ del índice 630 (`0x276`), se le asigna un id de `0x1276`. Luego de matarlo, como es el único elemento del vector que está disponible, el próximo proceso se le va a asignar a él. Ahora, para calcular el nuevo id, hay que tener en cuenta que ese elemento ya tenía uno del proceso anterior que no fue eliminado. Por lo que, al hacer nuevamente las cuentas, nos da que el id del nuevo proceso es `0x2276`. Si realizamos esto mismo cuatro veces más vamos a obtener los ids `0x3276`, `0x4276`, `0x5276`, `0x6276`.

env_init_percpu
---------------

La función `lgdt` escribe 48 bits (6 bytes) en el registro _GDTR_, los cuales son 16 para indicar el tamaño de la _Global Descriptor Table_ y 32 para la dirección en memoria de la misma.

env_pop_tf
----------

1. 
   1. El tope de la pila contiene la dirección de memoria del TrapFrame del proceso que se quiere ejecutar.

   2. Contiene la dirección de memoria de `tf_eip` del TrapFrame del proceso, es decir, la dirección de la próxima instrucción a ejecutar por el proceso.

   3. Contiene la dirección de memoria del campo `tf_eflags` (del TrapFrame del proceso).

2. x86 guarda el nivel de privilegio en los últimos 2 bits del registro CS. `00` implica modo kernel mientras que `11` significa modo usuario. La CPU determina si hay un cambio de privilegio leyendo esos bits. Si están seteados en `11`, al llamar a `iret` se pasa al ring 3.

gdb_hello
---------

2.
```
(qemu) info registers
EAX=003bc000 EBX=00010094 ECX=f03bc000 EDX=0000020f
ESI=00010094 EDI=00000000 EBP=f0118fd8 ESP=f0118fbc
EIP=f0102f24 EFL=00000092 [--S-A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
```

3.
```
(gdb) p tf
$1 = (struct Trapframe *) 0xf01c0000
```

4.
```
(gdb) x/17x tf
0xf01c0000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c0030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c0040:	0x00000023
```

5.
```
(gdb) si 4
(gdb) disas
Dump of assembler code for function env_pop_tf:
   0xf0102f24 <+0>:	    push   %ebp
   0xf0102f25 <+1>:  	mov    %esp,%ebp
   0xf0102f27 <+3>:	    sub    $0xc,%esp
   0xf0102f2a <+6>:	    mov    0x8(%ebp),%esp
=> 0xf0102f2d <+9>:	    popa   
   0xf0102f2e <+10>:	pop    %es
   0xf0102f2f <+11>:	pop    %ds
   0xf0102f30 <+12>:	add    $0x8,%esp
   0xf0102f33 <+15>:	iret   
   0xf0102f34 <+16>:	push   $0xf01053e4
   0xf0102f39 <+21>:	push   $0x1eb
   0xf0102f3e <+26>:	push   $0xf010535a
   0xf0102f43 <+31>:	call   0xf01000a9 <_panic>
End of assembler dump.
```

6. Comprobamos que los valores coinciden.
```
(gdb) x/17x tf
0xf01c0000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c0030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c0040:	0x00000023
```

7. Los primeros 8 enteros (todos en 0) corresponden a los campos del _struct_ PushRegs. El siguiente entero corresponde con el campo `tf_es` y su _padding_. Luego, con el mismo valor, tenemos el campo `tf_ds` y su _padding_. A continuación, vienen dos enteros nulos, correspondientes a `tf_trapno` y `tf_err`. Después, viene el campo `tf_eip` con valor `0x00800020`, el `tf_cs` con su _padding_ y el `tf_eflags` en nulo. Por último, tenemos los campos `tf_esp`, seteado en `0xeebfe000`, y `tf_ss` con su _padding_.

Exceptuado el campo `tf_eip`, todos los que tienen un valor no nulo, fueron inicializados en la función `env_alloc`. Tanto `tf_ds` como `tf_es` y `tf_ss` tienen un valor de `0x23` que equivale al _User Data Segment_ del GDT junto con el RPL en 3 para indicar modo usuario. `tf_cs` es similar, pero en lugar de usar el _User Data Segment_, utiliza el _User Text Segment_. A `tf_esp` se le asigna el valor de inicio del _stack_ del usuario. `tf_eip`, por su parte, fue establecido en la función `load_icode` con el _entry point_ del _ELF_.

8.
```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=f01c0030
EIP=f0102f33 EFL=00000096 [--S-AP-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
```

9.
```
(gdb) p $pc
$2 = (void (*)()) 0x800020
(gdb) add-symbol-file obj/user/hello 0x800020
add symbol table from file "obj/user/hello" at
	.text_addr = 0x800020
(y or n) y
Leyendo símbolos desde obj/user/hello...hecho.
(gdb) p $pc
$3 = (void (*)()) 0x800020 <_start>
```

```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=eebfe000
EIP=00800020 EFL=00000002 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
```

10. Viendo los registros en el monitor de QEMU justo antes y después de ejecutar `iret`, se puede observar como cambiaron algunos valores. Entre ellos, de los más destacados es el CPL que paso de 3 (modo usuario) a 0 (modo kernel). Otros registros que cambiaron fueron el _stack pointer_, el _instruction pointer_ y el _code segment_ (donde se almacena el CPL). A continuación, se pueden ver todas las diferencias de los registros más importantes:
```
# Antes:
EAX=00000000 EBX=00000000 ECX=0000000d EDX=eebfde88
ESI=00000000 EDI=00000000 EBP=eebfde40 ESP=eebfde18
EIP=008009f9 EFL=00000096 [--S-AP-] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
```

```
# Despues:
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000663
ESI=00000000 EDI=00000000 EBP=00000000 ESP=00000000
EIP=0000e05b EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0000 00000000 0000ffff 00009300
CS =f000 000f0000 0000ffff 00009b00
```

kern_idt
---------

- Usamos `TRAPHANDLER` cuando queremos que se guarde en el stack el código de error de la excepción. Por el contrario, usamos `TRAPHANDLER_NOEC` que se encarga de _pushear_ un 0 en lugar del código de error. Si solo se utilizara la primera, el TrapFrame no tendría un formato constante entre las excepciones con y sin código de error.
- El parámetro `istrap` indica si es posible el anidamiento de interrupciones. Es conveniente permitir esto, ya que en un caso real necesitamos manejar distintas interrupciones tales como las del disco, el controlador de red, etc. Para este trabajo práctico, se decide no soportarlo en JOS para lograr un sistema más simple.
- Intenta hacer un _page fault_, pero al no tener permisos suficientes, se lanza una excepción de _general protection_. Para evitar esto se podría cambiar los permisos de esa excepción para habilitar a los usuarios.

user_evil_hello
----------------

Al correr el programa de la consigna:
```c
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
    char *entry = (char *) 0xf010000c;
    char first = *entry;
    sys_cputs(&first, 1);
}
```
Obtenemos un _Page Fault_, ya que el usuario no tiene permisos para acceder a la posición de memoria `0xf010000c`.

En la versión del enunciado, se intenta acceder al contenido de la dirección para almacenarla en la variable `first`. Esto genera un _Page Fault_, ya que el programa, que está corriendo en modo usuario, no tiene los permisos para leer esa dirección. En cambio, en el código original, se intenta acceder al contenido de la dirección dentro de la _syscall_. Como todavía no se tienen en cuenta los permisos de memoria en las _syscalls_, el programa accede sin problemas al contenido.

En el código original se accede a la dirección `0xf010000c` dentro de la _syscall_, por lo tanto desde el _ring_ 0. Por su parte, en el código del enunciado se crean 2 variables `entry` y `first` las cuales se encuentran en el espacio de direcciones del usuario. Y en este caso, como mencionamos anteriormente, a la dirección `0xf010000c` se intenta acceder desde el _ring_ 3.