# Benchmarks y puesta en acción de QVM

> Todas las cifras de este documento se obtuvieron ejecutando los binarios incluidos en el repositorio sobre `HEAD = 82e596a`. No son estimaciones ni datos copiados de versiones anteriores.

## 1. Entorno de medición

| Elemento | Valor |
|:---|:---|
| CPU | x86-64 |
| Sistema | Linux 7.0.0-43-generic x86_64 |
| Compilador | cc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 |
| CMake | 4.2.3 |
| Configuración | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` |
| Pruebas | `ctest --test-dir build --output-on-failure` → 17/17 |
| Repeticiones | Una por binario; los resultados son orientativos |

Los benchmarks usan `clock_gettime(CLOCK_MONOTONIC)` e iteran **100 000 000** veces por caso. La referencia en C declara `volatile` para impedir que el compilador elimine el bucle (`examples/benchmark_native.c:25-35`). El banner de `benchmark_native` menciona `-march=native`, pero `CMakeLists.txt` no añade esa bandera: la configuración `Release` aporta `-O3` y `-DNDEBUG`, no optimizaciones específicas de CPU.

## 2. C vs. QVM: resultado completo

Ejecución de `./build/benchmark_native`:

| Modo | Tiempo | M iter/s | Ratio de tiempo sobre C |
|:---|---:|---:|---:|
| C puro, registros (`volatile`) | 0.0415 s | 2407.90 | 1.00x |
| C puro, memoria (`volatile`) | 0.0456 s | 2190.83 | 1.00x |
| QVM registros, fast path | 1.3982 s | 71.52 | 33.67x |
| QVM QMS-Zero, fast path | 2.2349 s | 44.74 | 48.96x |

El intérprete general, medido con `./build/benchmark` y `./build/benchmark_compare`:

| Modo | Tiempo | M iter/s |
|:---|---:|---:|
| QVM registros, intérprete | 0.4354 s | 22.97 |
| QVM QMS-Zero, intérprete | 0.8538 s | 11.71 |
| QVM registros, benchmark básico | 4.3759 s | 22.85 |

`./build/benchmark_optimized` (solo fast path):

| Modo | Tiempo | M iter/s |
|:---|---:|---:|
| Registros | 1.3608 s | 73.49 |
| QMS-Zero | 2.2075 s | 45.30 |

**Lectura**: el fast path es ≈3.1x más rápido que el intérprete para registros (73.49 vs 22.97 M iter/s). Los accesos QMS-Zero cuestan ≈1.6x sobre registros en fast path y ≈1.96x en intérprete, a cambio de eliminar checks en caliente. La diferencia frente a C puro (≈34x) refleja el coste de decodificación y despacho de una VM de propósito general; la referencia C no ejecuta verificador ni despacho de bytecode.

## 3. Código nativo generado: JIT y QIT

`./build/benchmark_jit` y `./build/benchmark_qit` compilan el mismo bucle de 100 M iteraciones:

| Modo | Tiempo | M iter/s | Código generado | Ratio de tiempo sobre C |
|:---|---:|---:|---:|---:|
| C puro | 0.0432 s | 2316.95 | — | 1.00x |
| JIT (`qvm_jit_compile`) | 0.1173 s | 852.53 | 4096 B reservados | 2.72x |
| C puro | 0.0424 s | 2356.16 | — | 1.00x |
| QIT (`qit_compile`) | 0.0929 s | 1076.53 | 94 B emitidos | 2.19x |

QIT queda a ≈2.2x de C puro y es ≈1.26x más rápido que el JIT básico en este microbenchmark. Ambos soportan únicamente `ADD`, `CMP_LT`, `JMP_IF` y `RETURN`; no son backends de propósito general. Véase [qit.md](qit.md).

## 4. Comparación C / C++ / JVM / QVM

No se presenta una tabla comparativa con C++ o JVM porque **el repositorio no contiene implementación, harness ni resultados para esos entornos**. Cualquier cifra sería especulativa. La comparativa real disponible es:

| Capa | Qué se mide |
|:---|:---|
| C | Bucle `volatile` equivalente al trabajo del bytecode |
| QVM intérprete | `qvm_runtime_execute` |
| QVM fast path | `qvm_runtime_execute_fast` (computed goto) |
| QVM JIT / QIT | Código x86-64 emitido en memoria |

C++ puede consumir la API porque las cabeceras tienen `extern "C"`, pero no hay fuentes `.cpp` ni benchmarks C++ en el árbol. Para ampliar la comparativa haría falta añadir un harness equivalente con la misma carga de trabajo y publicar los resultados medidos.

## 5. QVM en acción: comandos y salidas observadas

```bash
./build/test_qms_simple        # bucle QMS-Zero: resultado 5 (esperado 5)
./build/test_qit               # 10 + 20 = 30 mediante código QIT
./build/demo_strings_native    # strings nativos desde QCB
./build/benchmark_optimized    # registros vs QMS-Zero en fast path
./build/benchmark_native       # C vs QVM, comparación completa
./build/benchmark_jit          # C vs JIT
./build/benchmark_qit          # C vs QIT
```

Salida relevante de `test_qms_simple`: `Bytecode size: 53 bytes`, `Resultado: 5 (esperado: 5)`. El test QIT valida la aritmética generada; los tests del verificador cubren desbordamiento y doble liberación.

### Demos con estado conocido

Las siguientes demos terminan con `ERROR: loader: invalid bytecode` en este `HEAD`, porque serializan constantes como `sizeof(qvm_value)` (16 B) y el loader espera `type_id:u16 + payload:u64` (10 B):

- `./build/demo`
- `./build/demo_ffi`
- `./build/demo_strings`

`demo_strings_native` sí completa correctamente. Estos fallos están acotados al formato de serialización de las demos, no al loader ni al runtime.

## 6. Reproducción y estado del repositorio

```bash
# 1. Configurar y compilar
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# 2. Verificar
ctest --test-dir build --output-on-failure

# 3. Medir
for b in benchmark benchmark_compare benchmark_optimized \
         benchmark_native benchmark_jit benchmark_qit; do
    ./build/$b
done
```

<details>
<summary>Nota histórica: tests/ estuvo excluido del repositorio</summary>

En revisiones anteriores `.gitignore` contenía `tests/`, por lo que los 19 archivos del directorio (17 pruebas C y 2 SVG) no estaban versionados y un clon limpio fallaba en la configuración:

```text
CMake Error at CMakeLists.txt:35 (add_executable):
  Cannot find source file: tests/core/test_lifecycle.c
```

Esto se corrigió eliminando la entrada `tests/` de `.gitignore` y añadiendo los archivos al índice. Compruébalo con `git ls-files tests | wc -l` → `19`.

</details>

## 7. Interpretación responsable

- Son microbenchmarks de un solo bucle; no miden carga real de módulos, FFI, strings ni verifies.
- No hay múltiples repeticiones, intervalos de confianza ni control de ruido del sistema.
- Las cifras dependen de la CPU, el compilador y las banderas; vuelve a medir antes de citarlas.
- `qvm_core` se compila con `-Wall -Wextra -Wpedantic`; no se encontraron workflows de CI en el repositorio.
