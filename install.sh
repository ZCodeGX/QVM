#!/bin/bash
set -e

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  QVM - Instalador y Actualizador Automático             ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Detectar sistema
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "✓ Sistema Linux detectado"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "✓ macOS detectado"
else
    echo "✗ Sistema no soportado"
    exit 1
fi

# Verificar dependencias
echo "Verificando dependencias..."
if ! command -v gcc &> /dev/null; then
    echo "✗ GCC no encontrado. Instalando..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get update && sudo apt-get install -y gcc cmake make
    elif command -v pacman &> /dev/null; then
        sudo pacman -Sy gcc cmake make
    fi
else
    echo "✓ GCC encontrado"
fi

if ! command -v cmake &> /dev/null; then
    echo "✗ CMake no encontrado. Instalando..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get install -y cmake
    elif command -v pacman &> /dev/null; then
        sudo pacman -Sy cmake
    fi
else
    echo "✓ CMake encontrado"
fi

# Compilar
echo ""
echo "Compilando QVM..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Ejecutar tests
echo ""
echo "Ejecutando tests..."
ctest --test-dir build --output-on-failure

# Ejecutar benchmark optimizado
echo ""
echo "Ejecutando benchmark optimizado..."
./build/benchmark_optimized

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║  ✓ Instalación completada exitosamente                  ║"
echo "╚══════════════════════════════════════════════════════════╝"
