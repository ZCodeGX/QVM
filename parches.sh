#!/bin/bash
# QVM - Fix Fase 5: Añadir include de qit.h en runtime.h
set -e

echo "== 1. Añadiendo #include \"qvm/qit.h\" a runtime.h =="
python3 << 'PYEOF'
with open('include/qvm/runtime.h', 'r') as f:
    content = f.read()

if '#include "qvm/qit.h"' not in content:
    # Añadir después de los otros includes
    content = content.replace(
        '#include "qvm/memory.h"',
        '#include "qvm/memory.h"\n#include "qvm/qit.h"'
    )
    with open('include/qvm/runtime.h', 'w') as f:
        f.write(content)
    print("✅ Include de qit.h añadido a runtime.h")
else:
    print("ℹ️ runtime.h ya incluye qit.h")
PYEOF

echo "== 2. Recompilando =="
cmake --build build -j$(nproc)

echo "== 3. Ejecutando tests =="
ctest --test-dir build --output-on-failure

echo "== 4. Ejecutando test_tiering =="
./build/test_tiering

echo "== 5. Commit =="
git add include/qvm/runtime.h
git commit -m "fix(fase-5): add missing include of qit.h in runtime.h

- runtime.h uses qvm_exec_result which is defined in qit.h
- Added #include \"qvm/qit.h\" to resolve compilation error"

echo ""
echo "✅ Fase 5 completada y corregida. Lista para push."