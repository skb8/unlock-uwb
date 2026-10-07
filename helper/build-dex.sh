#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="$HERE/../native/generated"
mkdir -p "$OUT"
CLASSES="$HERE/classes"
rm -rf "$CLASSES"
mkdir -p "$CLASSES"

echo "Compiling Java helper classes..."
javac -source 8 -target 8 -d "$CLASSES" "$HERE"/src/io/github/skb8/unlockuwb/helper/*.java

echo "Dexing with dx..."
/data/data/com.termux/files/usr/bin/dx --dex --output="$OUT/helper.dex" "$CLASSES"

echo "Generating helper_dex.h..."
python3 - "$OUT/helper.dex" "$HERE/../native/jni/helper_dex.h" <<'PY'
import sys
data = open(sys.argv[1], 'rb').read()
with open(sys.argv[2], 'w') as f:
    f.write("#pragma once\n#include <cstddef>\n")
    f.write("static const unsigned char kHelperDex[] = {")
    f.write(",".join(str(b) for b in data))
    f.write("};\n")
    f.write(f"static const size_t kHelperDexLen = {len(data)};\n")
PY

echo "Done! helper.dex size: $(wc -c < "$OUT/helper.dex") bytes"
