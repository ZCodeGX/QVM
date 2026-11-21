#!/bin/bash
ERRORS=0
echo "Verificando protección extern \"C\" en cabeceras públicas..."
for header in include/qvm/*.h; do
    filename=$(basename "$header")
    if ! grep -q "#ifdef __cplusplus" "$header" || ! grep -q "extern \"C\"" "$header"; then
        echo "❌ FALLO: $filename no tiene protección C++"
        ERRORS=$((ERRORS + 1))
    else
        echo "✅ OK: $filename"
    fi
done
if [ $ERRORS -gt 0 ]; then
    echo "Se encontraron $ERRORS cabeceras sin protección."
    exit 1
fi
echo "Todas las cabeceras están correctamente protegidas."
exit 0
