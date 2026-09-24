#!/bin/sh
set -e
LCC="${GBDK_HOME:-$HOME/gbdk}/bin/lcc"
rm -rf build && mkdir -p build/obj
OBJS=""
for f in $(find src assets -name '*.c'); do
  o="build/obj/$(basename "${f%.c}").o"
  "$LCC" -Isrc -Iassets -c -o "$o" "$f"
  OBJS="$OBJS $o"
done
"$LCC" -Wm-yC -Wm-ynGAME -Wl-yt0x1B -Wl-ya1 -Wl-yoA -autobank -o build/game.gb $OBJS
echo "BUILD OK: build/game.gb"
