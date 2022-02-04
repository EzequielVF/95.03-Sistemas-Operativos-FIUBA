# Segmentación

## Simulación de traducciones

### Padrón 102.732

```console
$ ./segmentation.py -a 64 -p 256 -s 102732 -n 2
ARG seed 102732
ARG address space size 64
ARG phys mem size 256

Segment register information:

  Segment 0 base  (grows positive) : 0x0000008c (decimal 140)
  Segment 0 limit                  : 22

  Segment 1 base  (grows negative) : 0x0000002c (decimal 44)
  Segment 1 limit                  : 29

Virtual Address Trace
  VA  0: 0x0000003a (decimal:   58) --> PA or segmentation violation?
  VA  1: 0x0000002f (decimal:   47) --> PA or segmentation violation?
```

VA 0: `0x0000003a = 11 1010` (Segmento 1)

El segmento 1 crece negativamente, por lo tanto, para conseguir la dirección física, le restamos el tamaño máximo a la dirección virtual obtenida y sumamos la base del segmento. Es decir `0x3A - 0x40 + 0x2C = 0x26 (dec 38)`. `0x0F < 0x26 <= 0x2C`, entonces, es una dirección válida.

VA 1: `0x0000002f = 10 1111` (Segmento 1)

Al igual que el caso anterior, tenemos que `0x2F - 0x40 + 0x2C = 0x1B`. Por lo tanto, como `0x0F < 0x1B <= 0x2C`, es una direccion valida.

```console
$ ./segmentation.py -a 64 -p 256 -s 102732 -n 2 -c
ARG seed 102732
ARG address space size 64
ARG phys mem size 256

Segment register information:

  Segment 0 base  (grows positive) : 0x0000008c (decimal 140)
  Segment 0 limit                  : 22

  Segment 1 base  (grows negative) : 0x0000002c (decimal 44)
  Segment 1 limit                  : 29

Virtual Address Trace
  VA  0: 0x0000003a (decimal:   58) --> VALID in SEG1: 0x00000026 (decimal:   38)
  VA  1: 0x0000002f (decimal:   47) --> VALID in SEG1: 0x0000001b (decimal:   27)
```

### Padrón 104.980

```console
$ ./segmentation.py -a 64 -p 256 -s 104980 -n 2
ARG seed 104980
ARG address space size 64
ARG phys mem size 256

Segment register information:

  Segment 0 base  (grows positive) : 0x0000005b (decimal 91)
  Segment 0 limit                  : 25

  Segment 1 base  (grows negative) : 0x00000046 (decimal 70)
  Segment 1 limit                  : 16

Virtual Address Trace
  VA  0: 0x0000001d (decimal:   29) --> PA or segmentation violation?
  VA  1: 0x00000017 (decimal:   23) --> PA or segmentation violation?
```

VA 0: `0x0000001d = 01 1101`

El segmento 0 crece positivamente, con lo cual, para conseguir la dirección física simplemente hay que sumarle la base del segmento a la direccion virtual obtenida. En este caso, `0x1D + 0x5B = 0x78`. Como `0x78 > 0x74`, esto arroja un _segmentation fault_.

VA 1: `0x00000017 = 01 0111`

Nuevamente, tenemos una dirección virtual del primer segmento. Si hacemos la cuenta `0x17 + 0x5B = 0x72`, que al ser menor que `0x74`, es una dirección válida.

```console
$ ./segmentation.py -a 64 -p 256 -s 104980 -n 2 -c
ARG seed 104980
ARG address space size 64
ARG phys mem size 256

Segment register information:

  Segment 0 base  (grows positive) : 0x0000005b (decimal 91)
  Segment 0 limit                  : 25

  Segment 1 base  (grows negative) : 0x00000046 (decimal 70)
  Segment 1 limit                  : 16

Virtual Address Trace
  VA  0: 0x0000001d (decimal:   29) --> SEGMENTATION VIOLATION (SEG0)
  VA  1: 0x00000017 (decimal:   23) --> VALID in SEG0: 0x00000072 (decimal:  114)
```

## Traducciones inversas

Se intentaron calcular los parametros necesarios para cumplir las condiciones pedidas pero en ningún caso se obtuvieron resultados posibles.

Para la primer corrida, se busca que la dirección virtual `0x3A` arroje un _segmentation fault_ y la `0x2F` sea traducida a la `0x72`. Para que se cumpla la segunda condición, se calcula la base con la ecuacion `0x2F - 0x40 + B = 0x72`, por lo que la base deberia ser `0x83`. Ahora traducimos la primer dirección con la base obtenida y resulta en `0x7D`, la cual está más cercana a la base que la dirección anterior, por lo que no se puede establecer un límite que la excluya del segmento e incluya a la otra.

Para la segunda corrida, primero buscamos la base para que la direccion `0x1D` se traduzca a `0x26`. Haciendo la ecuación `0x1D + B = 0x26` obtenemos que la base debe ser `0x09`. Si tratamos de traducir ahora la segunda dirección con la base obtenida, resulta en `0x17 + 0x09 = 0x20`, cuando necesitamos que sea `0x1B`.

## Límites de segmentación

### 1. ¿Cuál es el tamaño (en número de direcciones) de cada espacio (físico y virtual)?

El tamaño es de 2^5 = 32 direcciones virtuales y 2^7 = 128 direcciones físicas.

### 2. ¿Es posible configurar la simulación de segmentación para que dos direcciones virtuales se traduzcan en la misma dirección física? Explicar, y de ser posible brindar un ejemplo de corrida de la simulación.

Para que esto suceda, hay que buscar una configuración donde los segmentos se superpongan. Para lograr esto, ideamos un segmento 0 que tenga base en `0x08 (dec 8)` y tamaño `0x10 (dec 16)` y un segmento 1 con base `0x14 (dec 20)` y tamanio `0x10 (dec 16)`. Luego, buscamos las direcciones virtuales para cada uno que se tradujeran a la dirección física `0x0C (dec 12)`:

```
S0: VA = PA - B
    VA = 12 - 8 = 4
S1: VA = PA + VA_SIZE - B
    VA = 12 + 32 - 20 = 24
```

Para corroborar, corrimos el programa.
```console
$ ./segmentation.py -a 32 -p 128 -b 8 -l 16 -B 20 -L 16 -A 4,24 -c
ARG seed 0
ARG address space size 32
ARG phys mem size 128

Segment register information:

  Segment 0 base  (grows positive) : 0x00000008 (decimal 8)
  Segment 0 limit                  : 16

  Segment 1 base  (grows negative) : 0x00000014 (decimal 20)
  Segment 1 limit                  : 16

Virtual Address Trace
  VA  0: 0x00000004 (decimal:    4) --> VALID in SEG0: 0x0000000c (decimal:   12)
  VA  1: 0x00000018 (decimal:   24) --> VALID in SEG1: 0x0000000c (decimal:   12)
```

### 3. ¿Es posible que (aproximadamente) el 90% del espacio de direcciones virtuales esté mapeado de forma válida? Explicar, y de ser posible, dar una configuración de segmentos que de tal resultado.

Es posible. Por ejemplo, se podría configurar el segmento 0 con base en `0x00` y tamanio `0x10`, y el segmento 1 con base en `0x80` y tamanio `0x10` (es decir de `0x80` a `0x70`). En este caso, el 100% de las direcciones virtuales serían válidas sin solaparse.

Para que estén direccionadas aproximadamente el 90% de las direcciones virtuales (29), podemos reducir el tamaño del segmento 0 a `0x0F` y el del segmento 1 a `0x0E`.

### 4. ¿Es posible que (aproximadamente) el 90% del espacio de direcciones físicas esté mapeado de forma válida? Explicar, y de ser posible, dar una configuración de segmentos que de tal resultado.

No es posible, ya que solo contamos con 32 direcciones virtuales posibles como máximo, y para lograr _mapear_ el 90% de las direcciones físicas se requerirían al menos 115.