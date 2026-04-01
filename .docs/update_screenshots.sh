#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/build/bin/hxed"
OUT_DIR="$ROOT_DIR/.docs"
RENDER="$OUT_DIR/render_terminal_svg.py"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

if [[ ! -x "$BIN" ]]; then
    echo "Binary not found: $BIN"
    echo "Build first with: cmake -S . -B build && cmake --build build"
    exit 1
fi

if [[ ! -x "$RENDER" ]]; then
    echo "Renderer not executable: $RENDER"
    exit 1
fi

# Build small demo fixtures ...
printf '\x89PNG\r\n\x1a\nIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x02\x00\x00\x00\x90wS\xdeIDATx\x9cc``\x00\x00\x00\x02\x00\x01\xe2!\xbc3IEND\xaeB`\x82HELLO_WORLD\x00\x00ABCDEFsecret_key=demo\n' > "$TMP_DIR/sample.bin"
printf 'MZ\x90\x00This is a demo executable header\x00config=true\n' > "$TMP_DIR/sample.exe"
dd if=/dev/urandom of="$TMP_DIR/random.bin" bs=256 count=1 status=none
python3 - "$TMP_DIR/heatmap_gradient.bin" <<'PY'
from pathlib import Path
import sys

Path(sys.argv[1]).write_bytes(bytes(range(256)))
PY

python3 - "$TMP_DIR/heatmap_hxed.bin" <<'PY'
from pathlib import Path
import sys

out = Path(sys.argv[1])
width = 44
rows = 12
canvas = [[0 for _ in range(width)] for _ in range(rows)]

patterns = {
    "H": ["X   X", "X   X", "X   X", "XXXXX", "X   X", "X   X", "X   X", "X   X", "X   X"],
    "X": ["X   X", " X X ", " X X ", "  X  ", " X X ", " X X ", "X   X", "X   X", "X   X"],
    "E": ["XXXXX", "X    ", "X    ", "XXXX ", "X    ", "X    ", "X   ", "X    ", "XXXXX"],
    "D": ["XXXX ", "X   X", "X   X", "X   X", "X   X", "X   X", "X   X", "X   X", "XXXX "],
}
starts = [("H", 1), ("X", 12), ("E", 23), ("D", 34)]

# Angepasste Base-Werte für harmonischere Verteilung in deiner 16-Farben-Palette
# H startet höher → deutlich besser sichtbar im Cyan-Bereich
base = {"H": 0x40, "X": 0x80, "E": 0xC0, "D": 0xE0}

for letter, start_x in starts:
    for y, row in enumerate(patterns[letter], start=1):
        for x, cell in enumerate(row):
            if cell != " ":
                # Stärkerer, gleichmäßiger Gradient pro Buchstabe
                value = base[letter] + (y - 1) * 4
                canvas[y][start_x + x] = min(value, 0xFF)

with out.open("wb") as handle:
    for row in canvas:
        handle.write(bytes(row))
PY

capture() {
    local output="$1"
    local title="$2"
    shift 2
    local ansi_file="$TMP_DIR/${output%.svg}.ansi"

    "$@" > "$ansi_file"
    "$RENDER" "$ansi_file" "$OUT_DIR/$output" --title "$title"
}

capture "magic.svg"  "hxed -th sample.bin" "$BIN" -th "$TMP_DIR/sample.bin"
capture "normal.svg" "hxed sample.bin" "$BIN" "$TMP_DIR/sample.bin"
capture "grouping.svg" "hxed -g 4 sample.bin" "$BIN" -g 4 "$TMP_DIR/sample.bin"
capture "raw.svg" "hxed -ro sample.bin" "$BIN" -ro "$TMP_DIR/sample.bin"
capture "heat.svg"   "hxed -hm fixed heatmap_gradient.bin" "$BIN" -hm fixed "$TMP_DIR/heatmap_gradient.bin"
capture "heatmap_hxed.svg" "hxed -th -hm fixed -a -g 1 -w 44 heatmap_hxed.bin" "$BIN" -th -hm fixed -a -g 1 -w 44 "$TMP_DIR/heatmap_hxed.bin"
capture "search.svg" "hxed -se 'a:demo' sample.bin" "$BIN" -se "a:demo" "$TMP_DIR/sample.bin"
capture "string.svg" "hxed -s sample.bin" "$BIN" -s "$TMP_DIR/sample.bin"
capture "entrop.svg" "hxed -e -hm fixed random.bin" "$BIN" -e -hm fixed "$TMP_DIR/random.bin"
bash -lc "cat '$TMP_DIR/sample.bin' | '$BIN' -th" > "$TMP_DIR/pipe.ansi"
"$RENDER" "$TMP_DIR/pipe.ansi" "$OUT_DIR/pipe.svg" --title "cat sample.bin | hxed -th"

echo "Updated screenshots in $OUT_DIR"
