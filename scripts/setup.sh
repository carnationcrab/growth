#!/usr/bin/env bash
# Growth onboarding (Linux / macOS) — Python deps, godot-cpp, native build.
# Run from repo root:  ./scripts/setup.sh
# Links only:          ./scripts/setup.sh --quick

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GDE_ROOT="$REPO_ROOT/gde"
GODOT_CPP_DIR="$GDE_ROOT/godot-cpp"

quick=0
skip_build=0

for arg in "$@"; do
  case "$arg" in
    --quick) quick=1 ;;
    --skip-build) skip_build=1 ;;
    -h|--help)
      echo "Usage: $0 [--quick] [--skip-build]"
      echo "  --quick       Print checklist and links; no build"
      echo "  --skip-build  Install deps / clone only"
      exit 0
      ;;
  esac
done

if [[ ! -f "$REPO_ROOT/project.godot" ]]; then
  echo "Run from the Growth repository root (project.godot missing)." >&2
  exit 1
fi

platform="$(uname -s)"
case "$platform" in
  Linux)  scons_platform="linux" ;;
  Darwin) scons_platform="macos" ;;
  *) echo "Unsupported OS: $platform" >&2; exit 1 ;;
esac

echo ""
echo "=== Growth setup ($platform) ==="
echo "Repo: $REPO_ROOT"
echo ""

open_link() {
  if command -v xdg-open >/dev/null 2>&1; then xdg-open "$1" >/dev/null 2>&1 || true
  elif command -v open >/dev/null 2>&1; then open "$1" || true
  fi
}

echo "=== Links ==="
echo "  Install guide:  file://$REPO_ROOT/docs/install.md"
echo "  Godot 4.6:      https://godotengine.org/download/archive/4.6-stable/"
echo "  godot-cpp:      https://github.com/godotengine/godot-cpp"
echo "  .NET SDK 8:     https://dotnet.microsoft.com/download/dotnet/8.0"
echo ""

if [[ "$quick" -eq 1 ]]; then
  read -r -p "Open Godot download page in browser? [y/N] " ans
  if [[ "$ans" =~ ^[yY] ]]; then open_link "https://godotengine.org/download/archive/4.6-stable/"; fi
  exit 0
fi

need() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing: $1 — see docs/install.md" >&2
    exit 1
  fi
}

need git
need python3

echo "=== Python packages ==="
python3 -m pip install --upgrade pip
python3 -m pip install scons
python3 -m pip install -r "$REPO_ROOT/tools/requirements-lint.txt"

echo "=== godot-cpp ==="
if [[ ! -d "$GODOT_CPP_DIR" ]]; then
  git clone -b 4.x https://github.com/godotengine/godot-cpp "$GODOT_CPP_DIR"
  (cd "$GODOT_CPP_DIR" && git submodule update --init)
else
  echo "Already present: $GODOT_CPP_DIR"
fi

if [[ "$skip_build" -eq 1 ]]; then
  echo "Skip build requested."
  exit 0
fi

echo "=== Building godot-cpp (template_debug) ==="
(cd "$GODOT_CPP_DIR" && scons "platform=$scons_platform" target=template_debug)

echo "=== Building growth_sim ==="
(cd "$GDE_ROOT" && scons "platform=$scons_platform" target=template_debug)

ext_disabled="$GDE_ROOT/growth_sim.gdextension.disabled"
ext_enabled="$GDE_ROOT/growth_sim.gdextension"
if [[ -f "$ext_disabled" && ! -f "$ext_enabled" ]]; then
  mv "$ext_disabled" "$ext_enabled"
  echo "Enabled growth_sim.gdextension"
fi

echo ""
echo "=== Done ==="
echo "Install Godot 4.6 .NET for your OS, then:"
echo "  godot4 --path \"$REPO_ROOT\""
echo "See docs/install.md for distro packages and troubleshooting."
