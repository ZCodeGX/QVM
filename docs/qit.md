# QIT — QVM Inline Translation

> Traducción de bytecode QVM a código máquina x86-64 en memoria. Documentación técnica basada en `include/qvm/qit.h` y `src/qit/qit.c` (`HEAD = 82e596a`).

## 1. Propósito

QIT compila un fragmento de bytecode QVM a una función nativa y la ejecuta con una ABI mínima: `int64_t (*)(void *frame)`. Su objetivo es eliminar el coste del bucle de decodificación sin cambiar el formato del bytecode. No sustituye al verificador: **no valida tipos, límites de registros ni saltos**.

```mermaid
flowchart LR
    BC[Bytecode QVM] --> C[qit_compile]
    C --> M[mmap RW 4 KiB]
    M --> E[Emisión x86-64 + tabla de offsets]
    E --> P[Parches JMP_IF rel32]
    P --> X[mprotect R+X]
    X --> F[qit_get_entry]
    F --> R[func(frame)]
    R --> D[qit_destroy]
```

## 2. API pública

Declarada en `include/qvm/qit.h` (bloque `extern "C"` en las líneas 7-9):

| Símbolo | Firma | Descripción |
|:---|:---|:---|
| `qit_code` | tipo opaco | Código compilado |
| `qit_compile` | `qit_code *qit_compile(const uint8_t *bytecode, uint32_t size)` | Compila; `NULL` si falla |
| `qit_destroy` | `void qit_destroy(qit_code *code)` | `munmap` + `free` |
| `qit_get_entry` | `void *qit_get_entry(qit_code *code)` | Puntero al primer byte ejecutable |
| `qit_get_size` | `size_t qit_get_size(qit_code *code)` | Bytes emitidos |
| `qit_func_t` | `typedef int64_t (*qit_func_t)(void *frame)` | ABI de la función generada |
| `qit_set_debug` | `void qit_set_debug(int enabled)` | Volcado hex y trazas de parcheo |

## 3. Proceso de compilación

1. **Reserva**: `mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)` (`src/qit/qit.c:116-119`).
2. **Prólogo** (`qit.c:134-137`):

   ```text
   55                push rbp
   48 89 e5          mov  rbp, rsp
   48 8b 7f 18       mov  rdi, [rdi+24]   ; base del array qvm_value del frame
   ```

3. **Recorrido lineal** (`qit.c:139-166`): antes de emitir cada instrucción se guarda `offset_map[pc]` con la posición de destino; el `pc` avanza 6 bytes (`ADD`, `CMP_LT`), 9 (`JMP_IF`) o 5 (`RETURN`). `RETURN` termina el bucle con `goto done`.
4. **Parcheo de saltos** (`qit.c:168-186`): para cada `JMP_IF` se calcula

   ```c
   int32_t offset = target_gen - (patch_pos + 4);
   ```

   y se escriben 4 bytes little-endian en el hueco reservado tras `0f 85` (`jnz rel32`).
5. **Endurecimiento**: `mprotect(code, capacity, PROT_READ|PROT_EXEC)` (`qit.c:197`). El buffer pasa de RW a RX: política *write XOR execute*.

### 3.1 Mapeo de registros

`emit_load_reg` y `emit_store_reg` (`qit.c:43-61`) usan un *stride* de 16 bytes por `qvm_value`:

```text
48 8b 87 disp32     mov  rax, [rdi + reg*16]   ; load
48 89 87 disp32     mov  [rdi + reg*16], rax   ; store
```

Los campos de tipo (`insn[2..3]`) se ignoran; los operandos se toman de `insn[1]` (dst), `insn[4]` (src1) y `insn[5]` (src2).

### 3.2 Secuencias emitidas

| Opcode QVM | Secuencia | Notas |
|:---|:---|:---|
| `ARITH_ADD` | `load src1` → `mov rcx,rax` → `load src2` → `add rax,rcx` → `store dst` | Suma de 64 bits (`qit.c:63-73`) |
| `CMP_LT` | `load src1` → `rcx` → `load src2` → `cmp rcx,rax` → `setb al` → `movzx rax,al` → `store dst` | `setb` es **sin signo** (`qit.c:75-87`) |
| `JMP_IF` | `load cond` → `test rax,rax` → `jnz rel32` | Parche diferido (`qit.c:89-104`) |
| `RETURN` | `mov rax, insn[1]` → `pop rbp` → `ret` | El registro devuelto se toma del **byte** del opcode (`qit.c:106-111`) |

Cualquier otro opcode hace que `qit_compile` libere los recursos y devuelva `NULL` (`qit.c:161-165`). No hay incidencia de `ALLOC`, `FREE`, `LOAD_PTR`, `STORE_PTR`, `CALL` ni strings en este backend.

## 4. Comparación con el JIT básico

| Característica | QIT (`src/qit/qit.c`) | JIT (`src/jit/jit.c`) |
|:---|:---|:---|
| Subset soportado | `ADD`, `CMP_LT`, `JMP_IF`, `RETURN` | Los mismos 4 opcodes |
| Saltos | `jnz rel32` con parches y `offset_map[1024]` global | `jnz rel8`, tabla local `bc_to_gen[256]`, solo saltos cortos hacia atrás |
| Reentrancia | No: tablas globales, no *thread-safe* | Sí: tabla local por compilación |
| Protección de memoria | `mmap RW` → `mprotect RX` (W^X) | `mmap RWX` y `mprotect` redundante |
| Buffer | 4 KiB con control de desbordamiento | 4 KiB sin control; `size` informa 4096 aunque emita menos |
| Diagnósticos | `qit_set_debug` con volcado hex | Ninguno |
| ABI medida | `func(frame)` | `func(frame, NULL)` (segundo parámetro ignorado) |

El JIT no maneja opcode desconocido con detalle: hace `munmap` y devuelve `NULL` (`src/jit/jit.c:99-102`). QIT, a diferencia de él, valida el resultado de `malloc` (`qit.c:121-125`).

## 5. Uso mínimo

El siguiente fragmento refleja el patrón de `examples/benchmark_qit.c:47-108`:

```c
#include "qvm/qvm.h"
#include "qvm/qit.h"

/* bytecode: ADD r2,r0,r1 ; CMP_LT r3,r2,r0 ; JMP_IF r3,0 ; RETURN r2 */

qit_code *code = qit_compile(bytecode, bytecode_size);
if (!code) return -1;                       /* opcode no soportado o error */

qvm_error err;
qvm_context *ctx = qvm_context_create(NULL, &err);
qvm_type_id_t types[4] = {
    QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_I32, QVM_TYPE_BOOL
};
qvm_frame *frame = qvm_frame_create(0, 4, types, NULL, &err);
qvm_register_write(frame, 0, qvm_value_make(QVM_TYPE_I32, 10));
qvm_register_write(frame, 1, qvm_value_make(QVM_TYPE_I32, 20));

qit_func_t fn = (qit_func_t)qit_get_entry(code);
int64_t result = fn(frame);                 /* 30 */

qit_destroy(code);
qvm_frame_destroy(frame);
qvm_context_destroy(ctx);
```

El prólogo asume que `frame->registers` está en el offset 24 de `qvm_frame`, por lo que la ABI está acoplada a la representación interna del frame (`qit.c:137`).

## 6. Limitaciones y plataforma

- x86-64 y POSIX/Linux: `sys/mman.h`, `MAP_ANONYMOUS`, `mprotect`, `push rbp`, `movzx`. Sin soporte ARM o Windows.
- `offset_map[1024]` y `patches[256]` son globales (`qit.c:16`, `qit.c:25`): las compilaciones concurrentes no son seguras y los bytecode de más de 1024 posiciones pueden corromper el mapeo.
- No hay validación de `size` frente a los 9 bytes que lee `JMP_IF`, ni de índices de registro, ni de tipos.
- `setb` implementa comparación no firmada; `CMP_LT` de QVM es de **bajo nivel** y debe usarse con cautela en valores con signo.
- `emit_byte` solo imprime `QIT: Buffer overflow` y descarta el byte (`qit.c:31-37`), sin propagar error.
- QIT no participa en la verificación QMS-Zero: los accesos a memoria del bytecode compilado no pasan por el verificador de memoria.

## 7. Pruebas y rendimiento

`tests/qit/test_qit.c` contiene dos pruebas: compilación de un `RETURN` con debug activo (`:20-37`) y ejecución de `r2 = r0 + r1` con `10 + 20 = 30` (`:39-82`). No hay tests de `CMP_LT`, `JMP_IF`, parcheo ni del JIT básico.

Resultados medidos en este entorno (100 M iteraciones, `Release`, una sola ejecución; ver [benchmarks.md](benchmarks.md)):

| Modo | M iter/s | Tiempo | Ratio sobre C |
|:---|---:|---:|---:|
| C puro (`volatile`) | 2356.16 | 0.0424 s | 1.00x |
| QIT | 1076.53 | 0.0929 s | 2.19x |
| JIT | 852.53 | 0.1173 s | 2.72x |

QIT genera 94 bytes de código para el bucle de prueba; el JIT informa 4096 bytes reservados. Estas cifras son orientativas y no sustituyen a una medición estadística con múltiples repeticiones.
