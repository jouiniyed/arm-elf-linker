
#!/bin/bash
# script de test pour etape 6 (fusion sections)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

READELF="$SCRIPT_DIR/../mon_readelf"
FUSION="$SCRIPT_DIR/../elf_fusion"

# teste la fusion de 2 fichiers
test_pair() {
    local name=$1
    local file_a=$2
    local file_b=$3

    
    printf "\n\e[31m┌──────────────────────────────────────────────┐\n│ %-44s │\n└──────────────────────────────────────────────┘\e[0m\n" "Test: $name"

    
    sections_a=$($READELF -S "$file_a" | grep "PROGBITS" | grep -v "\.debug" | grep -v "\.comment" | awk '{print $3}')
    sections_b=$($READELF -S "$file_b" | grep "PROGBITS" | grep -v "\.debug" | grep -v "\.comment" | awk '{print $3}')

    all_sections=$(echo -e "$sections_a\n$sections_b" | sort -u | grep -v "^$")

    
    fusion_output=$($FUSION -6 "$file_a" "$file_b" 2>&1)

    
    
    
    echo ""
    echo -e "\e[34m[MON PROGRAMME] (elf_fusion -6)\e[0m"
    echo "$fusion_output"
    echo ""

    
    
    
    
    tmpdir="$(mktemp -d)"
    ref_o="$tmpdir/ref.o"

    
    if arm-none-eabi-ld -r -EB -o "$ref_o" "$file_a" "$file_b" 2>/dev/null; then
        :
    else
        arm-none-eabi-ld -r -o "$ref_o" "$file_a" "$file_b" 2>/dev/null
    fi

    echo -e "\e[32m[REFERENCE ld -r] (readelf -x)\e[0m"

    if [ ! -f "$ref_o" ]; then
        echo -e "\e[31mFAIL: ld -r n'a pas pu generer ref.o\e[0m"
    else
        for sec in $all_sections; do
            echo ""
            echo "---- Section $sec ----"
            readelf -x "$sec" "$ref_o" 2>&1
        done
    fi

    rm -rf "$tmpdir"
    echo ""
    
    
    

    for sec in $all_sections; do
        
        in_a=$(echo "$sections_a" | grep -w "$sec" || true)
        in_b=$(echo "$sections_b" | grep -w "$sec" || true)

        
        size_a=0
        size_b=0

        if [ -n "$in_a" ]; then
            size_a=$($READELF -S "$file_a" | grep "PROGBITS" | awk -v sec="$sec" '$3 == sec {print "0x"$7; exit}')
            size_a=$((size_a))
        fi

        if [ -n "$in_b" ]; then
            size_b=$($READELF -S "$file_b" | grep "PROGBITS" | awk -v sec="$sec" '$3 == sec {print "0x"$7; exit}')
            size_b=$((size_b))
        fi

        total=$((size_a + size_b))

        
        status="OK"

        if [ -n "$in_a" ] && [ -n "$in_b" ]; then
            
            if [ "$total" -eq 0 ]; then
                
                echo -e "  $sec: FUSION ($size_a + $size_b = $total, delta=$size_a) \e[32mOK\e[0m (vide)"
            else
                delta_check=$(echo "$fusion_output" | grep -A 3 "Section $sec" | grep "Offset fichier2" | awk '{print $3}')
                taille_totale=$(echo "$fusion_output" | grep -A 2 "Section $sec" | grep "Taille totale" | awk '{print $3}')

                if [ -z "$delta_check" ]; then
                    status="FAIL (delta manquant)"
                elif [ "$delta_check" != "$size_a" ]; then
                    status="FAIL (delta=$delta_check au lieu de $size_a)"
                elif [ -z "$taille_totale" ]; then
                    status="FAIL (taille manquante)"
                elif [ "$taille_totale" != "$total" ]; then
                    status="FAIL (taille=$taille_totale au lieu de $total)"
                fi

                
                if [[ "$status" == OK* ]]; then
                    status="\e[32m$status\e[0m"
                else
                    status="\e[31m$status\e[0m"
                fi

                echo -e "  $sec: FUSION ($size_a + $size_b = $total, delta=$size_a) $status"
            fi
        elif [ -n "$in_a" ]; then
            echo -e "  $sec: FILE1 ($size_a octets) \e[32mOK\e[0m"
        else
            echo -e "  $sec: FILE2 ($size_b octets) \e[32mOK\e[0m"
        fi
    done

    echo ""
}



echo "=== Tests Etape 6 ==="
echo ""

for f in ../Examples_fusion/*.s fixtures/section_merge/*.s; do
    if [ -f "$f" ]; then
        base="${f%.s}"
        arm-none-eabi-as -mbig-endian "$f" -o "${base}.o" 2>/dev/null
    fi
done

for a in fixtures/section_merge/test*_a.o; do
    [ -f "$a" ] || continue
    b="${a%_a.o}_b.o"
    if [ -f "$b" ]; then
        name="$(basename "${a%_a.o}")"
        test_pair "$name" "$a" "$b"
    fi
done



echo "=== Fin tests ==="
