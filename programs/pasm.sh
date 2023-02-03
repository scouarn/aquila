#!/usr/bin/sh
IN="$1"
OUT="${IN%.*}.bin"

cat "$IN" | cut -d';' -f1 | xxd -p -r > "$OUT"
