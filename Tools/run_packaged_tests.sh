#!/usr/bin/env bash
# Pasa pruebas automáticas en la build empaquetada (../BlacklineBuild/Windows) y resume los resultados.
# Uso: Tools/run_packaged_tests.sh "Prueba:Mapa[:args extra]" ...   (por defecto: las misiones y el menú)
# Resultados: ../BlacklineBuild/Windows/Blackline/Saved/BLTest/<Prueba>_results.txt
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/../BlacklineBuild/Windows"
OUT="$BUILD/Blackline/Saved/BLTest"
if [[ $# -eq 0 ]]; then
  set -- "Menu:/Game/Maps/Menu/L_MainMenu" \
         "Mission:/Game/Maps/M01/L_M01_AmanecerRoto" \
         "Mission2:/Game/Maps/M02/L_M02_Manifiesto" \
         "Mission3:/Game/Maps/M03/L_M03_Ria" \
         "Mission4:/Game/Maps/M04/L_M04_FuegoCruzado" \
         "Mission5:/Game/Maps/M05/L_M05_LineaNegra" \
         "Mission5Fallo:/Game/Maps/M05/L_M05_LineaNegra:-BLStart=Azotea"
fi
mkdir -p "$OUT"
for spec in "$@"; do
  IFS=':' read -r TEST MAP EXTRA <<< "$spec"
  rm -f "$OUT/${TEST}_results.txt"
  echo "== $TEST ($MAP)"
  ( cd "$BUILD" && MSYS_NO_PATHCONV=1 timeout "${BL_TIMEOUT:-700}" ./Blackline.exe "$MAP" -windowed -ResX=1280 -ResY=720 \
      -NoSound -unattended -BLTest="$TEST" $EXTRA ) || true
  if [[ -f "$OUT/${TEST}_results.txt" ]]; then
    grep -E "FAIL|RESUMEN|Rendimiento" "$OUT/${TEST}_results.txt" | tail -4
  else
    echo "   sin resultados (¿crash o tiempo agotado?)"
  fi
done
