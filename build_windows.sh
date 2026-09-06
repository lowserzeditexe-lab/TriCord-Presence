#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# devkitPro Windows installs typically expose these under /opt/devkitpro
# or through /etc/profile.d/devkit-env.sh. Prefer the already configured env.
if [[ -f /etc/profile.d/devkit-env.sh ]]; then
  # shellcheck disable=SC1091
  source /etc/profile.d/devkit-env.sh || true
fi

if [[ -z "${DEVKITPRO:-}" && -d /opt/devkitpro ]]; then
  export DEVKITPRO=/opt/devkitpro
fi
if [[ -z "${DEVKITARM:-}" && -n "${DEVKITPRO:-}" ]]; then
  export DEVKITARM="$DEVKITPRO/devkitARM"
fi

if [[ -z "${DEVKITARM:-}" || ! -x "$DEVKITARM/bin/arm-none-eabi-gcc.exe" && ! -x "$DEVKITARM/bin/arm-none-eabi-gcc" ]]; then
  echo "ERREUR: devkitARM introuvable. Installe '3DS Development' avec devkitPro." >&2
  exit 1
fi

# Windows/MSYS2 tools used by this project.
for tool in make python3 git; do
  command -v "$tool" >/dev/null 2>&1 || {
    echo "ERREUR: outil '$tool' introuvable dans MSYS2." >&2
    exit 1
  }
done

# The project build script installs CTRPluginFramework when needed.
# Host-side tools are built here when absent.
if [[ ! -x "$ROOT/tools/bin/makerom.exe" && ! -x "$ROOT/tools/bin/makerom" ]]; then
  echo "== [0/3] Outils hote =="
  bash "$ROOT/tools/build_host_tools.sh"
fi

# On Windows/MSYS2, add the project's host tools and common devkitPro paths.
export PATH="$ROOT/tools/bin:$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH"

exec bash "$ROOT/build.sh"
