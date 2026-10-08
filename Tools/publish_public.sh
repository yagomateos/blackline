#!/usr/bin/env bash
# Publica una instantánea del proyecto en el repositorio público de GitHub SIN los assets de terceros que no
# se pueden redistribuir en abierto (packs de Epic de las plantillas de UE) ni las voces TTS provisionales.
# El repositorio de trabajo (este, con todo el historial) no se toca: la copia pública vive en ../BlacklinePublic
# con su propio historial (un commit por publicación).
#
# Uso: bash Tools/publish_public.sh "mensaje"      (primera vez: crea ../BlacklinePublic y el remoto)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PUB="$ROOT/../BlacklinePublic"
MSG="${1:-Actualización}"

# Rutas excluidas (ver README.md "Assets de terceros")
EXCLUDE=(
  "Content/Characters/Mannequins"
  "Content/Weapons/GrenadeLauncher"
  "Content/Weapons/Pistol"
  "Content/Weapons/Rifle"
  "Content/Weapons/Shared"
  "Content/LevelPrototyping"
  "Content/Input/Touch"
  "Content/Input/IMC_Default.uasset"
  "Content/Input/IMC_MouseLook.uasset"
  "Content/Input/Actions/IA_Jump.uasset"
  "Content/Input/Actions/IA_Look.uasset"
  "Content/Input/Actions/IA_MouseLook.uasset"
  "Content/Input/Actions/IA_Move.uasset"
  "Content/Audio/Voice"
  "ArtSource/Audio/SFX/Voice"
)

if [[ ! -d "$PUB/.git" ]]; then
  mkdir -p "$PUB"
  git -C "$PUB" init -q -b main
  git -C "$PUB" lfs install --local >/dev/null
fi

# Copia limpia de HEAD (los ficheros LFS se exportan con su contenido real)
TMP="$(mktemp -d)"
git -C "$ROOT" archive --format=tar HEAD | tar -x -C "$TMP"
( cd "$ROOT" && git lfs ls-files -n ) | while read -r f; do
  mkdir -p "$TMP/$(dirname "$f")"; cp "$ROOT/$f" "$TMP/$f"
done
for p in "${EXCLUDE[@]}"; do rm -rf "$TMP/$p"; done

# Sustituir el contenido de la copia pública (sin tocar su .git)
find "$PUB" -mindepth 1 -maxdepth 1 ! -name .git -exec rm -rf {} +
cp -a "$TMP"/. "$PUB"/
rm -rf "$TMP"
git -C "$PUB" add -A
if git -C "$PUB" diff --cached --quiet; then
  echo "== Sin cambios que publicar"; exit 0
fi
git -C "$PUB" commit -q -m "$MSG"
echo "== Commit público: $(git -C "$PUB" log --oneline -1)"
if git -C "$PUB" remote get-url origin >/dev/null 2>&1; then
  git -C "$PUB" push -u origin main
else
  echo "== Sin remoto: crea el repo y ejecuta  git -C \"$PUB\" remote add origin <url> && git -C \"$PUB\" push -u origin main"
fi
