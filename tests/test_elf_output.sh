#!/bin/bash

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

FUSION="$SCRIPT_DIR/../elf_fusion"

TESTS_OK=0
TESTS_FAIL=0

OK_C="\e[32mOK\e[0m"
FAIL_C="\e[31mFAIL\e[0m"

echo ""
echo "=== tests etape 9 : sortie ELF (.o) ==="
echo ""

# avant: juste GLOBAL/WEAK
# maintenant: LOCAL + GLOBAL + WEAK (on ignore FILE/SECTION pour eviter du bruit)
lire_syms() {
    arm-none-eabi-readelf --wide -s "$1" 2>/dev/null \
    | grep -E " LOCAL | GLOBAL | WEAK " \
    | grep -v " FILE " \
    | grep -v " SECTION " \
    | awk '{print $8, $2, $4, $5, $7}' \
    | sort
}

lire_relocs() {
    arm-none-eabi-readelf -r "$1" 2>/dev/null | grep "^[0-9a-f]" | awk '{if (NF >= 5) print $3, $4, $1}' | sort
}

check_contenu_sec() {
    local file=$1 section=$2 tmp
    tmp="$(mktemp)"
    arm-none-eabi-objcopy --dump-section "$section=$tmp" "$file" 2>/dev/null
    if [ -s "$tmp" ]; then
        sha256sum "$tmp" | awk '{print $1}'
        rm -f "$tmp"
        return 0
    fi
    rm -f "$tmp"
    echo "NOSECTION"
    return 1
}

taille_sec() {
    arm-none-eabi-readelf -S "$1" 2>/dev/null | grep " $2 " | awk '{print "0x"$6}' | head -1
}

ok_plus()   { TESTS_OK=$((TESTS_OK + 1)); }
fail_plus() { TESTS_FAIL=$((TESTS_FAIL + 1)); }

run_test() {
    local name=$1 file_a=$2 file_b=$3
    local tmpdir out_o ref_o prog_out

    echo ""
    echo -e "\e[31m--- $name ---\e[0m"

    tmpdir="$(mktemp -d)"
    out_o="$tmpdir/out.o"
    ref_o="$tmpdir/ref.o"
    prog_out="$tmpdir/prog_out"

    if [[ "$name" == "test6" ]]; then
        echo "-> double definition: on attend un echec"
        "$FUSION" -9 "$file_a" "$file_b" "$out_o" >/dev/null 2>>"$tmpdir/fusion.err"
        if [ $? -ne 0 ]; then
            echo -e "[res] $OK_C (bien refuse)"
            ok_plus
        else
            echo -e "[res] $FAIL_C (ca aurait du planter)"
            fail_plus
        fi
        rm -rf "$tmpdir"
        return
    fi

    arm-none-eabi-ld -r -EB -o "$ref_o" "$file_a" "$file_b" >/dev/null 2>>"$tmpdir/ld.err" || {
        echo -e "[res] $FAIL_C (ld -r rate)"
        fail_plus
        rm -rf "$tmpdir"
        return
    }

    "$FUSION" -9 "$file_a" "$file_b" "$out_o" >/dev/null 2>>"$tmpdir/fusion.err" || {
        echo -e "[res] $FAIL_C (elf_fusion rate)"
        cat "$tmpdir/fusion.err"
        fail_plus
        rm -rf "$tmpdir"
        return
    }

    echo "-> check: structure ELF"
    arm-none-eabi-readelf -h "$out_o" >/dev/null 2>&1 || {
        echo -e "[res] $FAIL_C (header ELF invalide)"
        fail_plus
        rm -rf "$tmpdir"
        return
    }
    arm-none-eabi-readelf -S "$out_o" >/dev/null 2>&1 || {
        echo -e "[res] $FAIL_C (table sections invalide)"
        fail_plus
        rm -rf "$tmpdir"
        return
    }
    echo "   ok: readelf lit le fichier"

    echo "-> check: sections (taille + contenu)"
    local sections_ok=1
    for sec in .text .data .rodata .bss; do
        local ref_size out_size ref_hash out_hash
        ref_size="$(taille_sec "$ref_o" "$sec")"
        out_size="$(taille_sec "$out_o" "$sec")"

        [ -z "$ref_size" ] && [ -z "$out_size" ] && continue

        if [ "$ref_size" != "$out_size" ]; then
            echo "   $sec: taille diff (ref=$ref_size out=$out_size)"
            sections_ok=0
            continue
        fi

        if [ "$sec" = ".bss" ]; then
            echo "   $sec: ok (size=$out_size)"
            continue
        fi

        ref_hash="$(check_contenu_sec "$ref_o" "$sec")"
        out_hash="$(check_contenu_sec "$out_o" "$sec")"

        [ "$ref_hash" = "NOSECTION" ] && [ "$out_hash" = "NOSECTION" ] && continue

        if [ "$ref_hash" != "$out_hash" ]; then
            echo "   $sec: contenu diff"
            sections_ok=0
        else
            echo "   $sec: ok (size=$out_size)"
        fi
    done

    echo "-> check: symboles (LOCAL/GLOBAL)"
    lire_syms "$ref_o" > "$tmpdir/ref_syms.txt"
    lire_syms "$out_o" > "$tmpdir/out_syms.txt"
    local symbols_ok=1
    if diff -q "$tmpdir/ref_syms.txt" "$tmpdir/out_syms.txt" >/dev/null 2>&1; then
        echo "   symboles: ok"
    else
        echo "   symboles: pas pareil"
        symbols_ok=0
    fi

    echo "-> check: relocations"
    lire_relocs "$ref_o" > "$tmpdir/ref_rels.txt"
    lire_relocs "$out_o" > "$tmpdir/out_rels.txt"
    local relocs_ok=1
    if diff -q "$tmpdir/ref_rels.txt" "$tmpdir/out_rels.txt" >/dev/null 2>&1; then
        echo "   relocs: ok"
    else
        echo "   relocs: pas pareil"
        relocs_ok=0
    fi

    echo "-> check: link final"
    local link_ok=1
    arm-none-eabi-ld -EB "$out_o" -o "$prog_out" >/dev/null 2>>"$tmpdir/link.err" || link_ok=0
    if [ $link_ok -eq 1 ]; then
        echo "   link: ok"
    else
        echo "   link: rate"
    fi

    if [ $sections_ok -eq 1 ] && [ $symbols_ok -eq 1 ] && [ $relocs_ok -eq 1 ] && [ $link_ok -eq 1 ]; then
        echo -e "[res] $OK_C"
        ok_plus
    else
        echo -e "[res] $FAIL_C"
        fail_plus
    fi

    rm -rf "$tmpdir"
}

echo "compile des tests (*.s) ..."
for f in fixtures/elf_output/*.s; do
    [ -f "$f" ] || continue
    base="${f%.s}"
    arm-none-eabi-as -EB "$f" -o "${base}.o" 2>/dev/null
done
echo "ok, fini"
echo ""

for a in fixtures/elf_output/test*_a.o; do
    [ -f "$a" ] || continue
    b="${a%_a.o}_b.o"
    [ -f "$b" ] || continue
    name="$(basename "${a%_a.o}")"
    run_test "$name" "$a" "$b"
done

echo ""
echo "=== bilan ==="
echo -e "OK   : \e[32m$TESTS_OK\e[0m"
echo -e "FAIL : \e[31m$TESTS_FAIL\e[0m"
echo ""

[ "$TESTS_FAIL" -eq 0 ] && exit 0
exit 1
