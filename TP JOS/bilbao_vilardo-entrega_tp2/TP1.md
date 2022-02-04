TP1: Memoria virtual en JOS
===========================

boot_alloc_pos
--------------

a. Leyendo el archivo `obj/kern/kernel` con la herramienta `nm`, obtenemos que el símbolo `end` apunta a la dirección `0xF0117950`. La primera vez que se llama a `boot_alloc()`, va a devolver la primera direccion de memoria disponible que esté alineada a _PGSIZE_ partiendo desde `end`. Para que esté alineada, los ultimos tres digitos hexadecimales deben ser 0, por lo tanto, en nuestro caso, la siguiente dirección alineada es `0xF0118000`.

b. Se corrió _GDB_, poniendo un _breakpoint_ en la funcion `boot_alloc()` y luego se utilizó el comando _finish_. Debido a un _bug_ con la versión de QEMU, este no imprime el valor del registro, así que lo imprimimos manualmente.

```
(gdb) break boot_alloc
Punto de interrupción 1 at 0xf0100b30: file kern/pmap.c, line 91.
(gdb) continue
Continuando.
Se asume que la arquitectura objetivo es i386
=> 0xf0100b30 <boot_alloc>:	push   %ebp

Breakpoint 1, boot_alloc (n=4096) at kern/pmap.c:91
91	{
(gdb) finish
Correr hasta la salida desde #0  boot_alloc (n=4096) at kern/pmap.c:91
Could not fetch register "orig_eax"; remote failure reply 'E14'
(gdb) p/x $eax
$1 = 0xf0118000
(gdb) 
```

page_alloc
----------

`page2pa()` devuelve la direccion física de una página a la cual se representa con un `struct PageInfo`. Por su parte, `page2kva()` realiza la misma acción, pero devolviendo la dirección virtual de la página. Para esto primero llama a `page2pa()` para obtener la dirección física, y luego la traduce a virtual.

map_region_large
----------------

Se ahorran 4KiB por _large page_ utilizada. Esto se debe a que al usar _large pages_ se evita crear una _page table_, que tiene 1024 entradas de 4B cada una, es decir, que ocupa 4KiB.

Es independiente de la cantidad de memoria física, ya que en x86 las _page tables_ siempre van a ocupar 4KiB independientemente de lo que tengamos disponible para mapear.