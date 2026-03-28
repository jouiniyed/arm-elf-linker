#!/bin/bash
# script de test pour etape 7 (fusion symboles)

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
echo "        TESTS ETAPE 7 — FUSION DES SYMBOLES ELF"
echo "============================================================"
echo ""

parse_symtab() {
    awk '
        $1 ~ /^[0-9]+:$/ {
            idx = $1;
            sub(":", "", idx);
            name = (NF >= 8 ? $8 : "");
            printf "%s|%s|%s|%s|%s|%s|%s\n",
                   idx, name, $5, $4, $7, $2, $3;
        }
    '
}

hex_to_dec() {
    local v="$1"
    v="${v#0x}"
    [ -z "$v" ] && { echo 0; return; }
    echo $((16#$v))
}

is_numeric() {
    [[ "$1" =~ ^[0-9]+$ ]]
}

inc_count() {
    local -n arr=$1
    local key=$2
    arr["$key"]=$(( ${arr["$key"]:-0} + 1 ))
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

    declare -A map2 delta2
    while IFS= read -r line; do
        if [[ "$line" =~ Section\ \[([0-9]+)\]\ \-\>\ \[([0-9]+)\]\ \ \(delta\ =\ ([0-9]+)\ octets\) ]]; then
            map2["${BASH_REMATCH[1]}"]="${BASH_REMATCH[2]}"
            delta2["${BASH_REMATCH[1]}"]="${BASH_REMATCH[3]}"
        fi
    done <<< "$step6_out"

    local sym_a="$TMPDIR/${name}_a.sym"
    local sym_b="$TMPDIR/${name}_b.sym"

    "$READELF" -s "$file_a" | parse_symtab > "$sym_a" || {
        echo -e "[RESULTAT] $FAIL_C readelf a"
        TESTS_FAIL=$((TESTS_FAIL+1))
        return
    }

    "$READELF" -s "$file_b" | parse_symtab > "$sym_b" || {
        echo -e "[RESULTAT] $FAIL_C readelf b"
        TESTS_FAIL=$((TESTS_FAIL+1))
        return
    }

    declare -A def_a def_b all_globals
    while IFS='|' read -r idx nm bind type ndx value size; do
        [ -z "$nm" ] && continue
        [ "$type" = "FILE" ] || [ "$type" = "SECTION" ] && continue
        if [ "$bind" = "GLOBAL" ] || [ "$bind" = "WEAK" ]; then
            [ "$ndx" != "UND" ] && def_a["$nm"]=1 || def_a["$nm"]=${def_a["$nm"]:-0}
            all_globals["$nm"]=1
        fi
    done < "$sym_a"

    while IFS='|' read -r idx nm bind type ndx value size; do
        [ -z "$nm" ] && continue
        [ "$type" = "FILE" ] || [ "$type" = "SECTION" ] && continue
        if [ "$bind" = "GLOBAL" ] || [ "$bind" = "WEAK" ]; then
            [ "$ndx" != "UND" ] && def_b["$nm"]=1 || def_b["$nm"]=${def_b["$nm"]:-0}
            all_globals["$nm"]=1
        fi
    done < "$sym_b"

    local expected_fail=0
    for nm in "${!all_globals[@]}"; do
        if [ "${def_a["$nm"]:-0}" -eq 1 ] && [ "${def_b["$nm"]:-0}" -eq 1 ]; then
            expected_fail=1
            break
        fi
    done

    local step7_out
    step7_out="$($FUSION -7 "$file_a" "$file_b" 2>&1)"
    local step7_ret=$?

    if [ "$expected_fail" -eq 1 ]; then
        if [ "$step7_ret" -ne 0 ]; then
            echo -e "[RESULTAT] $OK_C (conflit attendu)"
            TESTS_OK=$((TESTS_OK + 1))
            return
        fi
        echo -e "[RESULTAT] $FAIL_C conflit non detecte"
        TESTS_FAIL=$((TESTS_FAIL + 1))
        return
    fi

    if [ "$step7_ret" -ne 0 ]; then
        echo -e "[RESULTAT] $FAIL_C etape7"
        TESTS_FAIL=$((TESTS_FAIL + 1))
        return
    fi

    echo -e "[RESULTAT] $OK_C"
    TESTS_OK=$((TESTS_OK + 1))

    echo ""
    echo -e "\e[34m================ SYMTAB PRODUITE ================\e[0m"
    echo "$step7_out" | sed '/^$/N;/^\n$/D'

    if [ -f "${file_a%_a.o}_ref.o" ]; then
        echo ""
        echo -e "\e[32m=========== SYMTAB REFERENCE (ld -r) ============\e[0m"
        readelf -s "${file_a%_a.o}_ref.o"
    fi
}




for f in fixtures/symbol_merge/*.s; do
    [ -f "$f" ] || continue
    base="${f%.s}"
    arm-none-eabi-as -EB "$f" -o "${base}.o" 2>/dev/null
done




if command -v arm-none-eabi-ld >/dev/null 2>&1; then
    for a in fixtures/symbol_merge/test*_a.o; do
        [ -f "$a" ] || continue
        b="${a%_a.o}_b.o"
        [ -f "$b" ] || continue
        arm-none-eabi-ld -r -EB -o "${a%_a.o}_ref.o" "$a" "$b" 2>/dev/null
    done
else
    echo "arm-none-eabi-ld introuvable, refs non generes"
fi




for a in fixtures/symbol_merge/test*_a.o; do
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
