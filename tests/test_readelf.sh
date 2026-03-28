
#!/bin/bash
# script de test automatique pour phase 1

cd "$(dirname "$0")"

TESTS_DIR="fixtures/readelf"
PROG="../mon_readelf"
READELF="arm-none-eabi-readelf"

VERT="\033[0;32m"
ROUGE="\033[0;31m"
RESET="\033[0m"

total=0
reussis=0
echoues=0

# nettoyer les espaces et lignes vides pour comparaison
normaliser() {
    sed -i 's/[[:space:]]*$//' "$1" 2>/dev/null
    sed -i 's/[[:space:]][[:space:]]*/ /g' "$1" 2>/dev/null
    sed -i '/^$/d' "$1" 2>/dev/null
}
# teste un fichier avec une option donnee
tester() {
    local fichier="$1"
    local option="$2"
    local section="$3"
    
    local nom=$(basename "$fichier" .o)
    local out_prog="/tmp/prog_${nom}_$$.txt"
    local out_ref="/tmp/ref_${nom}_$$.txt"
    
    printf "Test : %-45s " "$nom.o"
    
    total=$((total + 1))
    
    
    if [[ "$nom" == *"invalide"* ]]; then
        timeout 5 $PROG $option $section "$fichier" >/dev/null 2>&1
        local code=$?
        
        if [[ $code -eq 0 ]]; then
            echo -e "${ROUGE}[ÉCHEC]${RESET}"
            echoues=$((echoues + 1))
        elif [[ $code -eq 124 ]]; then
            echo -e "${ROUGE}[ÉCHEC]${RESET}"
            echoues=$((echoues + 1))
        else
            echo -e "${VERT}[CORRECT]${RESET}"
            reussis=$((reussis + 1))
        fi
        rm -f "$out_prog" "$out_ref" 2>/dev/null
        return
    fi
    
    
    timeout 5 $PROG $option $section "$fichier" > "$out_prog" 2>&1
    local code_prog=$?
    
    timeout 5 $READELF $option $section "$fichier" > "$out_ref" 2>&1
    local code_ref=$?
    
    if [[ $code_prog -ne 0 ]] || [[ $code_ref -ne 0 ]]; then
        echo -e "${ROUGE}[ÉCHEC]${RESET}"
        echoues=$((echoues + 1))
    else
        normaliser "$out_prog"
        normaliser "$out_ref"
        
        if diff -u "$out_ref" "$out_prog" >/dev/null 2>&1; then
            echo -e "${VERT}[CORRECT]${RESET}"
            reussis=$((reussis + 1))
        else
            echo -e "${ROUGE}[ÉCHEC]${RESET}"
            echoues=$((echoues + 1))
        fi
    fi
    
    rm -f "$out_prog" "$out_ref" 2>/dev/null
}


etape1() {
    echo ""
    echo "=== Etape 1 : En-tete ELF (-h) ==="
    echo ""
    
    for f in "$TESTS_DIR"/*.o; do
        [[ -f "$f" ]] || continue
        tester "$f" "-h" ""
    done
}


etape2() {
    echo ""
    echo "=== Etape 2 : Table des sections (-S) ==="
    echo ""
    
    for f in "$TESTS_DIR"/*.o; do
        [[ -f "$f" ]] || continue
        tester "$f" "-S" ""
    done
}


etape3() {
    echo ""
    echo "=== Etape 3 : Contenu section (-x) ==="
    echo ""
    
    for f in "$TESTS_DIR"/*.o; do
        [[ -f "$f" ]] || continue
        
        if [[ "$(basename "$f")" == *"invalide"* ]]; then
            tester "$f" "-x" ".text"
        else
            for sec in .text .data .rodata; do
                if $READELF -S "$f" 2>/dev/null | grep -q " $sec " 2>/dev/null; then
                    tester "$f" "-x" "$sec"
                    break
                fi
            done
        fi
    done
}


etape4() {
    echo ""
    echo "=== Etape 4 : Table des symboles (-s) ==="
    echo ""
    
    for f in "$TESTS_DIR"/*.o; do
        [[ -f "$f" ]] || continue
        tester "$f" "-s" ""
    done
}


etape5() {
    echo ""
    echo "=== Etape 5 : Relocations (-r) ==="
    echo ""
    
    for f in "$TESTS_DIR"/*.o; do
        [[ -f "$f" ]] || continue
        tester "$f" "-r" ""
    done
}


tous() {
    total=0
    reussis=0
    echoues=0
    
    etape1
    etape2
    etape3
    etape4
    etape5
    
    echo ""
    echo "========================================"
    echo "Total : $total"
    echo -e "Réussis : ${VERT}$reussis${RESET}"
    echo -e "Échoués : ${ROUGE}$echoues${RESET}"
    echo "========================================"
    echo ""
}


if [[ ! -d "$TESTS_DIR" ]]; then
    echo "Erreur: repertoire $TESTS_DIR introuvable"
    exit 1
fi

if [[ ! -x "$PROG" ]]; then
    echo "Erreur: programme $PROG introuvable"
    exit 1
fi

if ! command -v $READELF &>/dev/null; then
    echo "Erreur: $READELF introuvable"
    exit 1
fi


if [[ $# -eq 0 ]]; then
    tous
    exit $((echoues > 0 ? 1 : 0))
fi

case "$1" in
    1) etape1 ;;
    2) etape2 ;;
    3) etape3 ;;
    4) etape4 ;;
    5) etape5 ;;
    a|all) tous ;;
    *) echo "Usage: $0 [1|2|3|4|5|all]"; exit 1 ;;
esac

exit $((echoues > 0 ? 1 : 0))
