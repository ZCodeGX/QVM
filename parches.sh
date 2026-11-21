#!/bin/bash
# QVM - Commit final de Fase 1
set -e

echo "== 1. Verificando cambios pendientes =="
git status

echo "== 2. Añadiendo ejemplos corregidos =="
git add examples/benchmark_jit.c examples/benchmark_qit.c

echo "== 3. Commit de corrección de casts =="
git commit -m "fix(examples): use memcpy for void* to function pointer casts (ISO C compliant)

- benchmark_jit.c:88 and benchmark_qit.c:88 now use memcpy instead of direct cast
- Eliminates -Wpedantic warnings without relaxing compiler flags
- Maintains type safety and portability"

echo "== 4. Verificación final =="
echo "Tests:"
ctest --test-dir build --output-on-failure | tail -5

echo "Headers:"
./scripts/check_headers.sh | tail -3

echo "Build status:"
git log --oneline -3

echo ""
echo "✅ Fase 1 completada. Listo para push con:"
echo "   git push origin main"