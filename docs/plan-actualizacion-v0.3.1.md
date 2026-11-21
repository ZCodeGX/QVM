# Plan de actualización QVM v0.3.1-beta

> Plan de trabajo, no implementación. Estado de partida: `HEAD = 17cd281`. No existe `CLAUDE.md`; se aplican las reglas globales de `AGENTS.md`. El código declara hoy `0.1.0-dev` (`src/core/version.c`), por lo que el primer acto de la release es alinear versión, documentación y etiqueta.

## 0. Objetivo y decisiones de diseño

### 0.1 Objetivos

| ID | Objetivo | Resultado observable |
|:--|:--|:--|
| O1 | Unificar JIT y QIT en un único subsistema **QIT 2.0** | Una sola API, una sola ABI, un solo pipeline de emisión; `src/jit/` desaparece o queda como *shim* deprecated |
| O2 | Portabilidad real | El mismo `.qcb` ejecuta en Linux, macOS, Windows y Android; x86-64, AArch64, RISC-V; fallback intérprete siempre disponible |
| O3 | QCB preformateado y portable | QCB v2 con spec canonizada, writer/reader únicos, validación estricta y toolchain (`qvm-as`, `qvm-dis`, `qvm-verify`) |
| O4 | Reporte de errores centralizado | Catálogo único de códigos + diagnóstico con `pc`/función/módulo + archivo legible/machine-readable |
| O5 | Auditoría total de memoria y bugs | Cero fugas bajo ASan/LSan/Valgrind, cero UB bajo UBSan, registro de bugs con repro y test de regresión |

### 0.2 Decisiones de diseño (no negociables)

1. **Portabilidad = bytecode + intérprete en todas partes; JIT nativo donde el SO lo permita.** En iOS/tvOS, navegadores o entornos con W^X estricto se usa el intérprete sin degradar funcionalidad.
2. **Una sola semántica canónica.** Hoy `RETURN` lee `ins.src1` en el intérprete (`src/runtime/runtime.c:418`) pero `bytecode[pc+1]` en fast path y backends (`runtime_fast.c:89`, `jit.c:88`, `qit.c:107`). Se fija `src1` en los cuatro motores con tests de equivalencia.
3. **Una sola ABI.** `qvm_exec_result qvm_entry(const qvm_exec_context *ctx)` con `ctx->registers` obtenido por API, no con el literal `[rdi+24]` (`jit.c:26`, `qit.c:137`). Se eliminan los casts `void*`→función no estándar tras la capa OS.
4. **Verificación obligatoria antes de compilar.** QIT 2.0 no consume bytes crudos: recibe módulo verificado + `register_count` + tipos. Hoy los backends bypassan `verifier.c` por completo.
5. **Degradación, no `NULL`.** Opcode no soportado por el backend nativo ⇒ ejecutar por intérprete; solo error real si falla validación/memoria.
6. **W^X siempre.** RW → emisión → RX → `clear_cache`; nunca RWX (`jit.c:12`). `mprotect`/`VirtualProtect` se comprueban y se reportan.
7. **Ninguna struct del host se serializa jamás.** QCB se escribe/lee byte a byte en LE; `qvm_value`, `qcb_header` y `qvm_frame` quedan como representación interna con `_Static_assert`.

---

## 1. Fase 0 — Baseline, higiene y control

**Objetivo:** poder medir si cada cambio mejora o rompe algo.

- **A0.1** Congelar rama de trabajo `release/0.3.1-beta` y etiquetar el estado actual como `v0.2.0-baseline` (o el tag que exista). No tocar `main` hasta Fase 10.
- **A0.2** Ejecutar y guardar el baseline: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && ctest --test-dir build --output-on-failure` (17/17 esperados) y `./install.sh`. Guardar salida en `docs/baseline-v0.2.md`.
- **A0.3** Añadir `.github/workflows/ci.yml` con job `baseline`: GCC y Clang en Ubuntu, `-Wall -Wextra -Wpedantic -Werror`, `ctest`. Hoy **no hay CI**; es prerrequisito de todo lo demás.
- **A0.4** Corregir deriva documental inmediata: `docs/arquitectura.md:3` dice `HEAD = 82e596a` (real `17cd281`); `docs/api.md:47` anuncia `qit_execute`, que no existe; `jit.c:22` dice 6 bytes de prólogo (son 8); README badge `0.1.0-dev`.
- **A0.5** Registrar `examples/check_offsets.c` en CMake o eliminarlo; hoy existe un ELF precompilado `./check_offsets` sin fuente construida (`CMakeLists.txt:91-144`).
- **A0.6** Sustituir los `target_sources` dispersos (`CMakeLists.txt:127,132,137`) por una lista única de fuentes en `qvm_core` y separar librerías: `qvm_core`, `qvm_os`, `qvm_qit`, `qvm_tools`.
- **A0.7** Añadir `scripts/check_headers.sh`: toda cabecera pública debe tener `#ifdef __cplusplus extern "C"`; hoy `include/qvm/jit.h` no lo tiene y `include/qvm/qit.h` sí (`qit.h:7-9`).

**Aceptación:** CI verde en GCC+Clang, baseline documentado, cero referencias a HEAD/versiones viejas, build reproducible desde clon limpio.

---

## 2. Fase 1 — Semántica canónica y ABI única

**Objetivo:** que intérprete, fast path, JIT y QIT den el mismo resultado para el mismo bytecode.

- **A1.1** Crear `include/qvm/execution.h` con:
  - `qvm_exec_context { const qvm_value *registers; uint32_t register_count; const uint8_t *bytecode; uint32_t bytecode_size; const qvm_type_id_t *types; void *os_env; }`.
  - `qvm_exec_result { qvm_error error; int64_t payload; qvm_type_id_t type_id; uint8_t flags; }`.
  - `typedef qvm_exec_result (*qvm_entry_fn)(const qvm_exec_context *);`.
- **A1.2** Fijar en `docs/semantica.md` la tabla de verdad: ADD/CMP sobre payload con signo definido por `type_id`, `RETURN` lee `ins.src1`, stores escriben `payload + type_id + flags` completos (hoy los backends escriben solo 8 bytes y arrastran tags obsoletos, `qit.c:53-61`).
- **A1.3** Decidir y documentar la signedness de `CMP_LT`: hoy `setb` es unsigned en los tres motores (`jit.c:59`, `qit.c:84`, `runtime.c:179`) y `docs/qit.md:129` lo advierte. Se propone `QVM_OP_CMP_LT` con signo según `type_id` y un opcode `CMP_LT_U` explícito; migar benchmarks y tests.
- **A1.4** Crear `tests/equivalence/test_equivalence.c`: corpus de ~40 programas (todos los opcodes soportados, saltos forward/backward, registros 0..N-1, signed/unsigned) ejecutados en los 4 motores; fail si difiere `payload`, `type_id`, `flags` o error.
- **A1.5** Unificar el decoder: `runtime_fast.c` y QIT deben llamar a `qvm_instruction_decode`/`qvm_insn_size` (`src/instruction/decode.c:22-100`) en vez de leer `bytecode[pc+N]` a mano (`jit.c:33-35`, `qit.c:64-66`).
- **A1.6** Añadir `_Static_assert` en `src/frames/frames.c` y `include/qvm/value.h` para `offsetof(registers)==24`, `offsetof(payload)==0`, `sizeof(qvm_value)==16`; si cambia la ABI, el build falla en vez de corromper memoria.
- **A1.7** Añadir validación central `qvm_validate_operand(ctx, reg)` antes de emisión/ejecución; hoy `reg>=16` aliasa por truncamiento a `uint8_t` y `reg>=register_count` escribe fuera del `calloc` de `frames.c:29`.

**Aceptación:** `test_equivalence` verde; canon de semántica firmado; ningún motor lee el bytecode con offsets literales propios.

---

## 3. Fase 2 — QIT 2.0: unificación de JIT y QIT

**Objetivo:** un solo emisor, un solo lifecycle, cero globals.

- **A2.1** Nueva API pública `include/qvm/qit.h` (reemplaza `jit.h` y `qit.h`):
  - `qvm_qit *qvm_qit_create(const qvm_context *, qvm_error *)`.
  - `const qvm_compiled_code *qvm_qit_compile(qvm_qit *, const qvm_module *, uint32_t function_index, qvm_error *)`.
  - `qvm_entry_fn qvm_qit_entry(const qvm_compiled_code *)`; `size_t qvm_qit_size(...)`; `const char *qvm_qit_backend(...)`; `void qvm_qit_release(...)`; `void qvm_qit_destroy(...)`.
  - `qvm_qit_set_debug(qvm_qit *, int)` → debug por compilador, no global (`qit.c:8`).
  - Mantener `qit_compile`/`qit_destroy` como *wrappers* deprecated durante una release, con aviso en docs.
- **A2.2** Extraer `src/qit/codebuf.c`: `cb_reserve`, `cb_emit8/16/32/64`, `cb_align`, `cb_patch32`, `cb_seal`. Overflow = error `QVM_ERR_LIMIT_EXCEEDED`; hoy `emit_byte` imprime y descarta (`qit.c:31-37`) y JIT desborda sin control (`jit.c:16`).
- **A2.3** Extraer `src/qit/reloc.c`: etiquetas por compilación, `label_pc[pc]`, `patch[]` con `{kind, offset, target_pc}`, validación de target `< bytecode_size` y alineado a inicio de instrucción. Resuelve: saltos forward rotos (`jit.c:78`, `qit.c:173` usan entradas en 0), arrays globales `offset_map[1024]`/`patches[256]` (`qit.c:16-25`), `rel8` truncado (`jit.c:80-83`) y OOB de target hostil.
- **A2.4** Reestructurar `src/qit/qit.c` como orquestador: decode → valida → emite prólogo → emite instrucciones → resuelve fixups → epilogue → `os_commit`. El epilogue es obligatorio aunque falte `RETURN` (hoy la ejecución cae en bytes cero, `qit.c:160-166`).
- **A2.5** Estado por compilación + concurrencia: mover `patches`, `patch_count`, `offset_map` y debug a `struct qvm_qit`; `qvm_compiled_code` con refcount atómico para destruir mientras otro hilo ejecuta.
- **A2.6** Registro de backends `src/qit/backend.c`: `{name, arch, os, supports(opcode), emit(ctx, buf, ins), priority}`; selección por detección en build + override de entorno `QVM_JIT=off|interp|native`.
- **A2.7** Helpers compartidos para todo lo que no se inlinea: límites de registro, manejo de `type_id`/flags, `ALLOC/FREE/LOAD_PTR/STORE_PTR`, FFI y GC. Respetar alineación de pila de cada ABI antes de `call`.
- **A2.8** Migrar `examples/benchmark_jit.c`, `examples/benchmark_qit.c` y `tests/qit/test_qit.c` a la API nueva; eliminar los typedef duplicados (`benchmark_jit.c:12`, `benchmark_qit.c:12`) y las ABIs incompatibles de 1 vs 2 argumentos.
- **A2.9** Borrar `src/jit/jit.c`, `include/qvm/jit.h` y `targets benchmark_jit` tras A2.8, o dejar `src/jit/jit_compat.c` de ~30 líneas que delegue a QIT y avise de deprecación.

**Aceptación:** un solo archivo de emisión por arquitectura; cero `static` globale en QIT; `test_qit` + `test_equivalence` verdes; compilaciones concurrentes bajo TSan sin race.

---

## 4. Fase 3 — Capa OS y portabilidad

**Objetivo:** separar "qué se emite" de "dónde se ejecuta".

- **A3.1** Crear `include/qvm/os.h` con `qvm_os_memory`:
  - `reserve(size)`, `protect_rw`, `protect_rx`, `flush_icache`, `release`, `page_size()`, `is_jit_allowed()`.
- **A3.2** Implementar backends:
  - `src/os/posix.c`: `mmap(RW)` → `mprotect(RX)` → `munmap`; `sysconf(_SC_PAGESIZE)`; MAP_JIT en macOS con `pthread_jit_write_protect_np`; comprobar **todos** los retornos.
  - `src/os/windows.c`: `VirtualAlloc(MEM_COMMIT, PAGE_READWRITE)` → `VirtualProtect(PAGE_EXECUTE_READ)` → `FlushInstructionCache` → `VirtualFree`; shadow space y ABI RCX.
  - `src/os/android.c`: hereda POSIX, añade detección de page size 16K/64K y política W^X de Android; API level mínimo y NDK rxx en CI.
  - `src/os/generic.c`: solo intérprete, `is_jit_allowed() == false`.
- **A3.3** Coherencia de caché: `__builtin___clear_cache()` en AArch64/RISC-V tras parcheo; hoy no existe y el código emitido puede ejecutar instrucciones stale.
- **A3.4** Macro de conversión función `QVM_FUNC_FROM_PTR` centralizada (POSIX lo permite, C17 estricto no); un único punto con `#if`.
- **A3.5** Eliminar dependencias de glibc no portables: `aligned_alloc` → shim `_aligned_malloc` en MSVC; `clock_gettime` → `qvm_os_now()`; `strncpy` → `snprintf`/`memcpy` acotado.
- **A3.6** CMake con generator expressions por `CMAKE_SYSTEM_NAME`/`CMAKE_SYSTEM_PROCESSOR`, toolchains `cmake/toolchains/{android-aarch64,linux-riscv64,windows-mingw}.cmake`, y `install(EXPORT qvm-targets)` para que otros proyectos enlacen la librería.
- **A3.7** `install.sh` debe detectar Windows/Android y no asumir `nproc`/`./build` POSIX, o delegar siempre a CMake Presets (`CMakePresets.json`).

**Aceptación:** `cmake --preset linux-*` / `macos-*` / `windows-*` / `android-*` configuran y compilan; en plataforma sin JIT `qvm_qit_compile` devuelve un código de tipo `interp` y la ejecución funciona.

---

## 5. Fase 4 — Backends por arquitectura y fallback universal

**Objetivo:** "Java-like": el bytecode viaja, el motor se adapta.

- **A4.1** Backend `interp` (primero, referencia): adaptar `qvm_runtime_execute` a `qvm_entry_fn`, con `qvm_exec_result`. Es el fallback de todas las plataformas.
- **A4.2** Backend `x86_64_sysv`: rescatar secuencias de `qit.c`, con disp32 completo (no truncado a 8 bits), `rel32` para todos los saltos, prólogo/epilogue del framework, `endbr64` opcional si CET está activo.
- **A4.3** Backend `x86_64_win64`: RCX como contexto, 32 B shadow space, callee-saved distintos; no reutilizar el prólogo SysV.
- **A4.4** Backend `aarch64`: `x0` contexto/retorno, `stp x29,x30,[sp,#-16]!` / `ldp` / `ret`, words LE de 32 bits, `movz/movk` para inmediatos y `b.cond`/`auipc` equivalente para saltos largos, `clear_cache` obligatorio.
- **A4.5** Backend `riscv64`: `a0` contexto/retorno, prólogo con `sd ra/s0`, branches ±4 KB con `auipc+jalr`, `fence.i`.
- **A4.6** Backend `wasm` (opcional, fase posterior): emitir `.wasm` o traducir a intrinsics para navegador; cuando no haya JIT, el fallback ya cubre el caso.
- **A4.7** Tests golden por backend: `tests/backends/golden/*.qcb` + expected result; compilación cruzada y ejecución bajo QEMU para AArch64/RISC-V en CI.

**Aceptación:** mismo corpus `test_equivalence` pasa en los 4 backends nativos + interp; CI matrix verde.

---

## 6. Fase 5 — Runtime integrado, caché y tiering

**Objetivo:** que JIT deje de ser un experimento separado de los benchmarks.

- **A6.1** Añadir `qvm_runtime_execute_function(rt, function_index, args, result)`.
- **A6.2** Caché de código claveada por `(module_id, function_index, qcb_version, backend_id, flags)`, con invalidación al destruir módulo y refcount.
- **A6.3** Contador de calls por función; compilar a nativo al superar umbral configurable; antes de eso, intérprete. `QVM_JIT=off` desactiva la promoción.
- **A6.4** `rt->return_value`/`rt->has_return` (`runtime.h:27-28`) se llenan desde `qvm_exec_result`, incluido `error`. Hoy la ABI `int64_t` no puede reportar errores.
- **A6.5** Soporte para `CALL`, `CALL_NATIVE`, frames anidados y QMS-Zero desde código compilado vía helpers (A2.7); verificar `arg_count <= 4` (hoy `verifier.c:101` solo compara con `param_count` y `decode.c:81-84` acepta más, leyendo fuera de `args[4]`).
- **A6.6** `qvm_verify_qms_module` se compila dentro de `qvm_core` y se invoca desde el loader cuando el módulo use memoria; hoy solo se enlaza en `test_qms_zero` (`CMakeLists.txt:107`).

**Aceptación:** un `.qcb` con `CALL`, FFI y memoria se ejecuta igual en interp y nativo; benchmarks usan el runtime real, no bytecode armado a mano (`benchmark_jit.c:54-69`).

---

## 7. Fase 6 — QCB v2 "preformateado"

**Objetivo:** dejar de escribir QCB a mano en cada demo.

- **A7.1** Escribir `docs/qcb-v2.md` como spec normativa: magic `QARM`, `format_version=2`, `isa_version`, flags con máscara de obligatorios, `total_size`, checksum, tabla de secciones `{type, offset, size, alignment}`; LE canónico; secciones requeridas y únicas; sin padding implícito.
- **A7.2** Implementar `src/qcb/writer.c` y `src/qcb/reader.c` con `rd16/32/64_le` y `wr16/32/64_le`. Sustituir los `memcpy` de payload (`loader.c:73`, `test_call_return.c:29`, `demo_strings_native.c:49-57`).
- **A7.3** Validación estricta en loader: aritmética en `uint64_t` con `section_size <= size - offset` (hoy `loader.c:183` desborda `uint32_t`), `section_count` acotado, secciones duplicadas rechazadas (fugas en `loader.c:78-80,126-127,199-200`), `len` de string dentro de sección y ≤ `max_string_length`, `count ≤ max_constants`, `bytecode_offset` dentro de BYTECODE y en límite de instrucción, tipos resueltos, `arg_count <= 4`, `local_count <= max_registers_per_frame`.
- **A7.4** Aplicar los límites de `qvm_limits` (`limits.c:4-16`) en loader; hoy `max_constants`, `max_functions`, `max_string_length`, `max_memory_bytes` y `max_modules` están definidos pero no se consultan.
- **A7.5** Propagar `qvm_verifier_result` con `pc`/offset al_error del loader (hoy `loader.c:215` descarta el detalle).
- **A7.6** Crear `tools/qvm-as`: ensamblador textual con etiquetas, relocaciones, directivas `.i32/.f64/.string/.i128`, genera FUNCTIONS automáticamente.
- **A7.7** Crear `tools/qvm-dis` (round-trip golden) y `tools/qvm-verify` (usa exactamente el mismo validador del loader) y `tools/qvm-objdump`.
- **A7.8** Corregir las tres demos rotas: `examples/demo.c:29-34`, `examples/demo_ffi.c:62-67`, `examples/demo_strings.c:142-145` escriben `sizeof(qvm_value)=16` por constante cuando el formato exige **10 bytes** (`type_id:u16 + payload:u64`). Migrarlas al writer de A7.2.
- **A7.9** Loader v1 compatible solo con `verify_on_load=1` y warnings; herramienta `qvm-qcb-upgrade` de v1→v2; fixture corpus en `tests/fixtures/qcb/`.
- **A7.10** Fuzzing del parser con libFuzzer/oss-fuzz empezando por los casos `section_size=0xFFFFFFF0`, `len=0xFFFFFFFC`, section_count gigante.

**Aceptación:** `qvm-as → qvm-verify → qvm-dis → qvm-as` produce bytes idénticos; las 3 demos ejecutan; parser aguanta 1M entradas fuzz sin crash/leak.

---

## 8. Fase 7 — Sistema de errores y reportes

**Objetivo:** sustituir `return NULL` y mensajes genéricos por diagnóstico accionable.

- **A8.1** Limpiar `include/qvm/error.h:39-64`: hay macros QMS duplicados **fuera** del `#endif` y con colisiones (`QVM_ERR_UNINITIALIZED_READ=32` vs `QVM_ERR_USE_AFTER_FREE=32`). Un único enum, rangos reservados por subsistema (core 1-99, QCB 100-199, QIT 200-299, QMS 300-399, OS 400-499).
- **A8.2** Crear `include/qvm/diagnostic.h`:
  - `qvm_diagnostic { uint32_t code; qvm_severity severity; const char *subsystem; uint32_t pc; uint32_t function_index; const char *module_name; const char *file; uint32_t line; char message[256]; qvm_diagnostic *cause; }`.
  - API: `qvm_diag_push`, `qvm_diag_last` (thread-local), `qvm_diag_format(char*,size_t)`, `qvm_diag_report(FILE*)`, `qvm_diag_clear`.
- **A8.3** Catálogo único `errors/catalog.yaml` con `{code, name, severity, subsystem, message, cause, fix_hint, since}`. Genera `include/qvm/error_codes.h`, `src/core/error_table.c`, `docs/errores.md` y `bugs/errors.json` mediante `scripts/gen_errors.py`. Prohibido escribir mensajes a mano en los `switch` de `error.c:3-47`.
- **A8.4** Todo punto de fallo inicializa `err`/diagnóstico con contexto: loader (`pc`, offset de sección), verifier (`pc`, opcode), QIT (`pc`, backend, `size`), OS (`errno`/`GetLastError`), memoria (`tag`, `bytes`).
- **A8.5** Niveles: `fatal` aborta compilación/ejecución; `error` propaga; `warning` continúa (ej. JIT no soporta opcode ⇒ fallback); `info` para tiering.
- **A8.6** `qvm_set_diagnostic_hook(void(*)(const qvm_diagnostic*,void*), void*)` para integrar con logging del host; sin `printf` directo desde la librería (hoy `qit.c:31,188-195`, `verifier_qms.c:34,54,77,84`).
- **A8.7** Tests: cada código del catálogo tiene un test que lo dispara y comprueba `code + pc + subsystem + mensaje`.

**Aceptación:** `ctest` incluye `test_diagnostics`; `docs/errores.md` y el header generado están sincronizados; cero `printf` en `src/`.

---

## 9. Fase 8 — Auditoría de memoria y bugs

**Objetivo:** encontrar y cerrar fugas, UB y corrupciones en todo el QVM.

- **A9.1** Habilitar sanitizers en CI: `-fsanitize=address,undefined` (Linux/macOS), `/fsanitize=address` (MSVC), LSAN/Valgrind en job separado; `ASAN_OPTIONS=detect_leaks=1`.
- **A9.2** Allocation tracker en debug: `qvm_alloc(tag, size, align)` / `qvm_free`, con contador por tag (core, loader, string, frame, jit, os) y `qvm_mem_report(FILE*)`; el tracker alimenta los tests de leak.
- **A9.3** Corregir GCQ (`src/memory/gcq/gcq.c`): `gcq_free` no descuenta `bytes_allocated` (`:75-78`), ruta de `realloc` fallido deja contabilidad hinchada (`:113-119`), `aligned_alloc` no existe en MSVC (`:69`), `bytes_allocated + size` puede desbordar (`:63`). Definir propiedad: o el allocator rastrea bloques y los libera, o la región los rastrea; hoy `gcq_allocator_destroy` es `free(alloc)` (`:59`) y deja regiones colgando.
- **A9.4** Corregir runtime: `malloc(1 MiB)` de `frame_region.base` sin chequear y `err` queda `QVM_OK` (`runtime.c:49-52`); `qvm_ptr` de 16 bytes copiado en `payload` de 8 en ALLOC/FREE (`runtime.c:234,252-253`) y leído en LOAD_PTR/STORE_PTR (`:272,302`) sin bounds.
- **A9.5** Corregir loader: `count*sizeof(qvm_value)` sin control (`loader.c:37`), `malloc` sin inicializar flags/padding (`:37,45,65-73`), `malloc(len+1)` con `len=UINT32_MAX` envuelve (`string.c:20-27`), doble copia del archivo en memoria (`:250,164-170`), `ftell` recortado a `uint32_t` (`:241,266`).
- **A9.6** Corregir QIT/JIT: buffer overflow silencioso, parcheo sin check de capacidad (`qit.c:176-180`), `malloc` sin chequear en JIT (`jit.c:107-110`), `mprotect` sin comprobar (`jit.c:105`, `qit.c:197`), saltos a offsets no emitidos (`qit.c:131` inicializa `offset_map` a 0 ⇒ salto al prólogo ⇒ desbordamiento de pila).
- **A9.7** Unificar codificación de opcode QMS-Zero: `verifier.c:6-36` asume 4 bytes por defecto mientras `decode.c:87-96` y `runtime.c:98-105` usan 9 para LOAD_PTR/STORE_PTR; `verifier_qms.c:24-96` decodifica big-endian y avanza 8. Una sola tabla de tamaños generada desde la spec de ISA.
- **A9.8** Revisión de propiedad: documento `docs/propiedad.md` con quién libera `qvm_module`, `qvm_string` (guardado como puntero dentro de `payload`, `loader.c:52,277-279`), regiones GCQ y código JIT; añadir tests de doble free y use-after-free.
- **A9.9** `tests/leaks/` con escenarios: cargar/destruir módulo 1000 veces, compilar/destruir código 1000 veces, frames con FFI/strings, errores parciales de loader. Cero bytes perdidos.
- **A9.10** Revisión de hilos con TSan: QIT concurrente, caché de código, hook de diagnóstico y contexts compartidos.

**Aceptación:** ASan/LSan/UBSan/TSan/Valgrind limpios; `qvm_mem_report` en cero al final de cada test; catálogo de bugs con cada finding cerrado y su test.

---

## 10. Fase 9 — Repositorio central de bugs

**Objetivo:** que "todos los errores, bugs y fugas" queden registrados con evidencia y estado.

- **A10.1** Crear `bugs/README.md`, `bugs/catalog.json` y `bugs/TEMPLATE.md` con esquema:
  `{id, title, severity, area, status, discovered_in, fixed_in, repro, expected, actual, evidence(file:line), root_cause, fix_commit, regression_test, notes}`.
- **A10.2** Sembrar el catálogo con los findings de este plan (sección 11). Cada uno lleva evidencia `archivo:línea` y fase de resolución.
- **A10.3** `scripts/bug_report.sh`: recoge versión, SO/arquitectura, compiler, `qvm_version()`, `qvm_diag_report()` y backtrace; genera un JSON listo para pegar en un issue.
- **A10.4** Comando `qvm-report` (tool) que imprime: errores activos, bugs abiertos por severidad, fugas detectadas por el tracker y cobertura de tests por código de error.
- **A10.5** Regla de proceso: **ningún bug se cierra sin test de regresión y entrada en `CHANGELOG.md`**. El CI falla si un `bugs/catalog.json` con `status:fixed` no referencia un test existente.
- **A10.6** Plantilla de issue de GitHub que enlace el esquema y pida `.qcb` mínimo reproducible.

**Aceptación:** `qvm-report` muestra el inventario completo; cada bug listado en la sección 11 tiene `status` y `regression_test`.

---

## 11. Catálogo semilla de defectos conocidos

Severidad: **C** crítico (corrupción/ejecución arbitraria), **A** alto, **M** medio, **B** bajo.

| ID | Área | Sev | Evidencia | Defecto | Fase |
|:--|:--|:--:|:--|:--|:--:|
| QVM-001 | JIT | C | `src/jit/jit.c:16,37-44` | Buffer de 4 KiB sin control; ~195 ADD desbordan | 2 |
| QVM-002 | QIT | C | `src/qit/qit.c:31-37` | `emit_byte` descarta bytes tras overflow y devuelve código válido | 2 |
| QVM-003 | QIT | C | `src/qit/qit.c:176-180` | Parcheo rel32 sin check de capacidad | 2 |
| QVM-004 | Ambos | C | `jit.c:33-35`, `qit.c:64-66` | Lecturas OOB de bytecode (`pc+8` sin check de `size`) | 2 |
| QVM-005 | Ambos | C | `jit.c:78`, `qit.c:173` | Saltos forward rotos (tabla solo rellenada al visitar) | 2 |
| QVM-006 | Ambos | C | `jit.c:69-70`, `qit.c:91` | Target de salto sin validar ⇒ OOB read / ejecución arbitraria | 2 |
| QVM-007 | QIT | C | `qit.c:16-25,98-100,131-132` | `patches[256]`, `offset_map[1024]` globales: overflow y race | 2 |
| QVM-008 | Ambos | C | `jit.c:38,41`, `qit.c:47,57` | `reg*16` truncado a `uint8_t`; reg≥16 aliasa, reg≥count desborda frame | 1/2 |
| QVM-009 | Ambos | A | `jit.c:88`, `qit.c:107` vs `runtime.c:418` | `RETURN` lee campo distinto según motor | 1 |
| QVM-010 | Ambos | A | `jit.c:43-44`, `qit.c:53-61` | Stores de solo payload; `type_id`/flags obsoletos | 1 |
| QVM-011 | JIT | A | `jit.c:12-13,105` | Ventana RWX + `mprotect` sin comprobar | 3 |
| QVM-012 | QIT | A | `qit.c:197` | `mprotect` sin comprobar ⇒ objeto ejecutable inválido | 3 |
| QVM-013 | JIT | A | `jit.c:107-110` | `malloc` sin chequear | 2 |
| QVM-014 | Ambos | A | `jit.c:28-103`, `qit.c:140-166` | Sin prólogo/epilogue seguro si no hay RETURN | 2 |
| QVM-015 | JIT | A | `jit.c:80-83` | `rel8` truncado: saltos fuera de ±128 fallan | 2 |
| QVM-016 | QIT | M | `qit.c:8,27-29` | `qit_debug` global, no thread-safe | 2 |
| QVM-017 | Loader | C | `loader.c:183,210` | Overflow de `uint32_t` en `offset + section_size` | 6 |
| QVM-018 | Loader | C | `loader.c:26-28,48-52`, `string.c:20-27` | `len` de string sin validar; `malloc(len+1)` envuelve; OOB read | 6/9 |
| QVM-019 | Loader | A | `loader.c:37,78-80,126-127,199-200` | Secciones duplicadas y `count` corrupto: leaks/heap overflow | 6/9 |
| QVM-020 | Loader | M | `loader.c:37,45,65-73` | Constantes con `malloc` no inicializado; flag CONST nunca se marca | 6/9 |
| QVM-021 | Loader | M | `loader.c:149-156` | `flags` del QCB ignorados | 6 |
| QVM-022 | Loader | A | `loader.c:241,266` | `ftell` recortado a `uint32_t`; doble copia del archivo | 6/9 |
| QVM-023 | Loader | A | `loader.c:89-95`, `runtime.c:366` | `bytecode_offset` sin validar ⇒ salto a mitad de instrucción | 6 |
| QVM-024 | Verifier | A | `verifier.c:6-36` vs `decode.c:87-96` | Tamaños de instrucción QMS-Zero divergentes | 9 |
| QVM-025 | Verifier | A | `error.h:39-64` | Códigos QMS duplicados y colisionantes (32) | 7 |
| QVM-026 | Verifier | A | `decode.c:81-84`, `verifier.c:101`, `runtime.c:348,388` | `arg_count > 4` lee fuera de `args[4]` | 5 |
| QVM-027 | Memoria | A | `gcq.c:61-78,109-125` | Contabilidad de bytes rota; orden de destrucción allocator/región | 9 |
| QVM-028 | Memoria | A | `runtime.c:49-52` | `malloc` de frame region sin chequear; error no propagado | 9 |
| QVM-029 | Memoria | A | `runtime.c:234,252-253,272,302` | `qvm_ptr` de 16 B dentro de payload de 8 B | 9 |
| QVM-030 | Ejemplos | M | `demo.c:29-34`, `demo_ffi.c:62-67`, `demo_strings.c:142-145` | Serialización de constantes: 16 B escritos, 10 B esperados | 6 |
| QVM-031 | Docs | B | `docs/arquitectura.md:3`, `docs/api.md:47` | HEAD y API documentados desactualizados | 0 |
| QVM-032 | Build | M | `CMakeLists.txt:107,127,132,137` | Fuentes añadidas de forma dispersa; `verifier_qms.c` solo en un test | 0/5 |

---

## 12. Matriz de plataformas objetivo

| Sistema | Arquitecturas | Intérprete | JIT nativo | Notas |
|:--|:--|:--:|:--:|:--|
| Linux | x86-64, AArch64, RISC-V64 | sí | sí | `mmap`/`mprotect`; QEMU en CI |
| macOS | x86-64, Apple Silicon | sí | sí | `MAP_JIT` + `pthread_jit_write_protect_np` |
| Windows | x86-64, ARM64 | sí | sí | `VirtualAlloc`/`VirtualProtect`; MSVC + MinGW |
| Android | ARM64, x86-64 | sí | sí (según policy) | NDK, page sizes 4K/16K/64K, W^X |
| iOS/tvOS/watchOS | ARM64 | sí | no (por defecto) | W^X estricto; fallback sin pérdida de función |
| WebAssembly/Browser | wasm32/wasm64 | sí (vía build) | no | QIT 2.0 puede añadir backend WASM después |

---

## 13. Riesgos y mitigaciones

| Riesgo | Mitigación |
|:--|:--|
| Cambiar semántica de `RETURN`/`CMP_LT` rompe bytecode existente | Corpus `test_equivalence` primero; opcode nuevo para unsigned; opcode de ISA en la cabecera QCB v2 |
| QCB v2 rompe compatibilidad | Reader v1 con `verify_on_load=1`, herramienta de upgrade, fixtures de ambos formatos |
| JIT en macOS/Android falla por políticas W^X | `is_jit_allowed()` y fallback automático a interp; nunca abortar la ejecución |
| Alineación ABI (offset 24 / stride 16) cambie | `_Static_assert` + acceso vía `qvm_exec_context`, no literales |
| Grandeza del cambio (≈10 fases) | Entregables independientes: Fase 0-1 ya aporta valor; cada fase mergeable con CI verde |
| Fuzzing encuentre más bugs de los previstos | Catálogo `bugs/catalog.json` con estado `open`; no bloquear release beta si son `M/B` y tienen repro |

---

## 14. Orden de ejecución recomendado

1. **Fase 0** (higiene + CI) → **Fase 1** (semántica/ABI) → **Fase 2** (QIT 2.0) → **Fase 5** (integración runtime).
2. En paralelo a 2: **Fase 3** (OS) → **Fase 4** (arquitecturas).
3. **Fase 6** (QCB v2) puede arrancar después de Fase 1; alimenta a Fase 5 con módulos reales.
4. **Fase 7** (errores) y **Fase 8** (memoria) se ejecutan de forma continua desde Fase 0 y se cierran antes del release.
5. **Fase 9** (catálogo) se actualiza en cada commit; **Fase 10** (release) al final.

## 15. Definición de Done de la v0.3.1-beta

- `qvm_version() == "0.3.1-beta"` y tag `v0.3.1-beta`.
- Un solo QIT; `src/jit/` eliminado o *shim*; cero globals de compilación.
- CI matrix verde en Linux/macOS/Windows/Android + QEMU AArch64/RISC-V; sanitizers limpios.
- QCB v2 con `qvm-as`/`qvm-dis`/`qvm-verify`; las 3 demos rotas funcionan.
- `docs/errores.md` generado desde el catálogo; `qvm-report` muestra el inventario de bugs con cada finding cerrado o con repro y severidad.
- Benchmarks re-medidos con el runtime real y metodología estadística (múltiples repeticiones), sustituyendo las cifras de `README.md:110-120` y `docs/benchmarks.md`.
