#!/usr/bin/env bash
# Compila el módulo y ejecuta una prueba automática del juego.
# Uso: Tools/run_test.sh [Prueba=Movement] [Mapa=/Game/Maps/Dev/L_Dev_Movement] [--nobuild]
# Comandos de consola extra (diagnóstico): BL_EXEC="r.X 0, r.Y 1" Tools/run_test.sh ...
# Resolución (por defecto 1280x720): BL_RESX=1920 BL_RESY=1080 Tools/run_test.sh ...
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TEST="${1:-Movement}"
MAP="${2:-/Game/Maps/Dev/L_Dev_Movement}"
UE="/c/Program Files/Epic Games/UE_5.8/Engine"
WINROOT="$(cygpath -m "$ROOT")"

if [[ " $* " != *" --nobuild "* ]]; then
  echo "== Compilando..."
  if ! "$UE/Build/BatchFiles/Build.bat" BlacklineEditor Win64 Development -Project="$WINROOT/Blackline.uproject" -WaitMutex > "$ROOT/Saved/build_last.log" 2>&1; then
    grep -E "error|Error" "$ROOT/Saved/build_last.log" | head -30
    echo "== COMPILACIÓN FALLIDA (log completo: Saved/build_last.log)"
    exit 1
  fi
  grep -E "warning C|: warning" "$ROOT/Saved/build_last.log" | head -10 || true
  echo "== Compilación OK"
fi

rm -f "$ROOT/Saved/BLTest/${TEST}_"*.png "$ROOT/Saved/BLTest/${TEST}_results.txt"
EXTRA=()
if [[ -n "$BL_EXEC" ]]; then EXTRA=(-ExecCmds="$BL_EXEC"); fi
echo "== Ejecutando prueba $TEST en $MAP"
MSYS_NO_PATHCONV=1 timeout "${BL_TIMEOUT:-300}" "$UE/Binaries/Win64/UnrealEditor.exe" "$WINROOT/Blackline.uproject" "$MAP" \
  -game -windowed -ResX="${BL_RESX:-1280}" -ResY="${BL_RESY:-720}" -nosplash -NoSound -unattended -BLTest="$TEST" \
  -abslog="$WINROOT/Saved/BLTest/${TEST}_game.log" "${EXTRA[@]}" $BL_ARGS || true

if [[ -f "$ROOT/Saved/BLTest/${TEST}_results.txt" ]]; then
  cat "$ROOT/Saved/BLTest/${TEST}_results.txt"
else
  echo "== Sin resultados. Errores del log:"
  grep -E "Error|Fatal|Assert|Exception" "$ROOT/Saved/BLTest/${TEST}_game.log" | grep -v "AutomationTest\|UnifiedError\|FError\|Error test\|Error with" | head -20
fi
