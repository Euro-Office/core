#!/bin/bash
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
# Converts the corpus to PDF inside a DocumentServer image. Called by compare.sh.
#   run-in-image.sh image|build
#   /corpus  input documents (ro)
#   /pkg     core build output (package/), used for "build" (ro)
#   /out     results: /out/<mode>/{pdf,logs,results.tsv}
# "image" uses the binaries shipped in the image, "build" first replaces x2t
# and the core libraries with the ones from /pkg.
set -u
MODE="$1"
DS=/var/www/euro-office/documentserver
BIN=$DS/server/FileConverter/bin
OUT=/out/$MODE
rm -rf "$OUT"; mkdir -p "$OUT/pdf" "$OUT/logs" "$OUT/themes"
RES=$OUT/results.tsv
echo -e "step\tfile\texit\tseconds" > "$RES"

now() { date +%s.%N; }
record() { echo -e "$1\t$2\t$3\t$(awk "BEGIN{printf \"%.2f\", $5 - $4}")" >> "$RES"; }

THEMESGEN=$DS/server/tools/allthemesgen
if [ "$MODE" = build ]; then
    # x2t and all core libraries of the build, keep the image's configs
    find /pkg/bin -maxdepth 1 -type f ! -name '*.config' -exec cp -f {} "$BIN/" \;
    find /pkg/bin -maxdepth 1 -type l -exec cp -Pf {} "$BIN/" \;
    [ -x /pkg/tools/allthemesgen ] && THEMESGEN=/pkg/tools/allthemesgen
fi
export LD_LIBRARY_PATH=$BIN
if ldd "$BIN/libdoctrenderer.so" | grep 'not found'; then
    echo "libdoctrenderer.so: missing libraries"; exit 1
fi
# V8 versions look like 8.9.45 or 14.6.202.34
strings "$BIN/libdoctrenderer.so" | grep -m1 -oE '^([89]|[1-9][0-9])\.[0-9]+\.[0-9]+(\.[0-9]+)?$' > "$OUT/v8-version.txt"

# Fonts as documentserver-generate-allfonts.sh makes them (no JS engine involved)
$DS/server/tools/allfontsgen --input="$DS/core-fonts" --allfonts-web="$DS/sdkjs/common/AllFonts.js" \
    --allfonts="$BIN/AllFonts.js" --images="$DS/sdkjs/common/Images" --selection="$BIN/font_selection.bin" \
    --output-web="$DS/fonts" --use-system="false" > "$OUT/logs/allfontsgen.log" 2>&1

t0=$(now); $THEMESGEN --converter-dir="$BIN" --src="$DS/sdkjs/slide/themes" --output="$OUT/themes" > "$OUT/logs/allthemesgen.log" 2>&1; rc=$?; t1=$(now)
record allthemesgen - $rc $t0 $t1

t0=$(now); "$BIN/x2t" -create-js-cache > "$OUT/logs/js-cache.log" 2>&1; rc=$?; t1=$(now)
record create-js-cache - $rc $t0 $t1

convert() {  # step file output-dir
    local f="$2" name; name=$(basename "$f")
    local tmp; tmp=$(mktemp -d)
    cat > "$tmp/params.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<TaskQueueDataConvert xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">
<m_sFileFrom>$f</m_sFileFrom>
<m_sFileTo>$3/$name.pdf</m_sFileTo>
<m_nFormatTo>513</m_nFormatTo>
<m_sFontDir>$DS/core-fonts</m_sFontDir>
<m_sThemeDir>$DS/sdkjs/slide/themes</m_sThemeDir>
<m_sAllFontsPath>$BIN/AllFonts.js</m_sAllFontsPath>
<m_bIsNoBase64>true</m_bIsNoBase64>
<m_sTempDir>$tmp</m_sTempDir>
</TaskQueueDataConvert>
EOF
    t0=$(now); "$BIN/x2t" "$tmp/params.xml" > "$OUT/logs/$1-$name.log" 2>&1; rc=$?; t1=$(now)
    record "$1" "$name" $rc $t0 $t1
    rm -rf "$tmp"
}

# twice: the second run has warm OS caches and the JS code cache, use it for timing
for f in /corpus/*; do convert pdf "$f" "$OUT/pdf"; done
for f in /corpus/*; do convert pdf-warm "$f" "$OUT/pdf"; done

if [ "$MODE" = image ]; then
    # startup snapshots of the image's JS engine, for the check below
    "$BIN/x2t" -create-js-snapshots > "$OUT/logs/js-snapshots.log" 2>&1
    mkdir -p "$OUT/snapshots"
    for editor in word cell slide visio pdf; do
        [ -f "$DS/sdkjs/$editor/sdk-all.bin" ] && cp "$DS/sdkjs/$editor/sdk-all.bin" "$OUT/snapshots/$editor.bin"
    done
elif [ -d /out/image/snapshots ]; then
    # snapshots left over from the previous version must not break or change anything
    for f in /out/image/snapshots/*.bin; do
        editor=$(basename "$f" .bin)
        cp "$f" "$DS/sdkjs/$editor/sdk-all.bin"
    done
    mkdir -p "$OUT/pdf-old-snapshots"
    for f in /corpus/*; do convert pdf-old-snapshots "$f" "$OUT/pdf-old-snapshots"; done
fi
echo done
