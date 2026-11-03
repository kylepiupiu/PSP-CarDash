#!/bin/zsh
set -e
cd "$(dirname "$0")"

export PSPDEV="${PSPDEV:-/opt/pspdev}"
export PATH="$PSPDEV/bin:$PATH"

make clean || true
make

echo
echo "Built: $(pwd)/EBOOT.PBP"
echo "Copy EBOOT.PBP to /PSP/GAME/PSPCARDASH/"
