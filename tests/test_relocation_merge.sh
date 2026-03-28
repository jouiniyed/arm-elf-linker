#!/bin/bash

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

READELF="$SCRIPT_DIR/../mon_readelf"
FUSION="$SCRIPT_DIR/../elf_fusion"

TESTS_OK=0
TESTS_FAIL=0

TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT


OK_C="\e[32mOK\e[0m"
FAIL_C="\e[31mFAIL\e[0m"


echo ""
echo "============================================================"
echo "     TESTS ETAPE 8 — FUSION DES TABLES DE RELOCATION"
echo "============================================================"
echo ""

parse_reltab() {
    awk '
        /Relocation section/ { section = $4; gsub(/'\''/, "", section); next }
        $1 ~ /^[0-9a-f]+$/ && NF >= 5 {
            offset = $1;
            type = $3;
            value = $4;
            name = (NF >= 6 ? $6 : $5);
            printf "%s|%s|%s|%s|%s\n", section, offset, type, value, name;
        }
    '
}

hex_to_dec() {
    local v="$1"
    v="${v#0x}"
    [ -z "$v" ] && { echo 0; return; }
    echo $((16#$v))
}

run_test() {
    local name=$1
    local file_a=$2
    local file_b=$3

    
    local step6_out
    step6_out="$($FUSION -6 "$file_a" "$file_b" 2>/dev/null)"
    if [ $? -ne 0 ]; then
        echo -e "[RESULTAT] $FAIL_C etape6"
        TESTS_FAIL=$((TESTS_FAIL + 1))
        return
    fi

    declare -A delta_sec
    while IFS= read -r line; do
        if [[ "$line" =~ Section\ .*\ \[([0-9]+)\].*Offset\ fichier2\ :\ ([0-9]+) ]]; then
            delta_sec["${BASH_REMATCH[1]}"]="${BASH_REMATCH[2]}"
        fi
    done <<< "$step6_out"


    local step7_out
    step7_out="$($FUSION -7 "$file_a" "$file_b" 2>&1)"
    local step7_ret=$?

    if [ "$step7_ret" -ne 0 ]; then
        echo -e "[RESULTAT] $FAIL_C etape7"
        TESTS_FAIL=$((TESTS_FAIL + 1))
        return
    fi

    local step8_out
    step8_out="$($FUSION -8 "$file_a" "$file_b" 2>&1)"
    local step8_ret=$?

    if [ "$step8_ret" -ne 0 ]; then
        echo -e "[RESULTAT] $FAIL_C etape8"
        TESTS_FAIL=$((TESTS_FAIL + 1))
        return
    fi

    echo -e "[RESULTAT] $OK_C"
    TESTS_OK=$((TESTS_OK + 1))

    echo ""
    echo -e "\e[34m================ RELOCATIONS PRODUITES ================\e[0m"
    echo "$step8_out" | grep -v "^===" | grep -v "ETAPE 8 : FUSION" | sed '/^$/N;/^\n$/D'

    if [ -f "${file_a%_a.o}_ref.o" ]; then
        echo ""
        echo -e "\e[32m=========== RELOCATIONS REFERENCE (ld -r) ============\e[0m"
        readelf -r "${file_a%_a.o}_ref.o"
    fi
  
    local rel_count_a rel_count_b rel_count_out
    rel_count_a=$("$READELF" -r "$file_a" 2>/dev/null | grep -c "^[0-9a-f]\{8\}" || echo 0)
    rel_count_b=$("$READELF" -r "$file_b" 2>/dev/null | grep -c "^[0-9a-f]\{8\}" || echo 0)
    rel_count_out=$(echo "$step8_out" | grep -c "^[0-9a-f]\{8\}" || echo 0)
    
    local expected=$((rel_count_a + rel_count_b))
    
    echo ""
    echo -e "\e[33m[INFO] Nombre de relocations:\e[0m"
    echo "  - Fichier A      : $rel_count_a"
    echo "  - Fichier B      : $rel_count_b"
    echo "  - Attendu (A+B)  : $expected"
    echo "  - Produit        : $rel_count_out"
    
    if [ "$rel_count_out" -eq "$expected" ]; then
        echo -e "  \e[32m Nombre correct\e[0m"
    else
        echo -e "  \e[33m Nombre différent (peut être normal selon fusion)\e[0m"
    fi
    
    echo ""
}


echo "Compilation des fichiers tests..."
for f in fixtures/relocation_merge/*.s; do
    [ -f "$f" ] || continue
    base="${f%.s}"
    arm-none-eabi-as -EB "$f" -o "${base}.o" 2>/dev/null
done
echo "✓ Compilation terminée"
echo ""


echo "Génération des références avec ld -r..."
if command -v arm-none-eabi-ld >/dev/null 2>&1; then
    for a in fixtures/relocation_merge/test*_a.o; do
        [ -f "$a" ] || continue
        b="${a%_a.o}_b.o"
        [ -f "$b" ] || continue
        arm-none-eabi-ld -r -EB -o "${a%_a.o}_ref.o" "$a" "$b" 2>/dev/null
    done
    echo " Références générées"
else
    echo " arm-none-eabi-ld introuvable, références non générées"
fi
echo ""


for a in fixtures/relocation_merge/test*_a.o; do
    [ -f "$a" ] || continue
    b="${a%_a.o}_b.o"
    [ -f "$b" ] || continue
    name="$(basename "${a%_a.o}")"

    echo ""
    echo -e "\e[31m------------------------------------------------------------"
    echo -e "TEST : $name"
    echo -e "------------------------------------------------------------\e[0m"

    run_test "$name" "$a" "$b"
done


echo ""
echo "============================================================"
echo "RESULTAT FINAL"
echo -e "OK   : \e[32m$TESTS_OK\e[0m"
echo -e "FAIL : \e[31m$TESTS_FAIL\e[0m"
echo "============================================================"

if [ "$TESTS_FAIL" -eq 0 ]; then
    exit 0
fi
exit 1
