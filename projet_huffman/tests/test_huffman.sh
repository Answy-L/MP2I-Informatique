#!/usr/bin/env bash
#
# test_huffman.sh — suite de tests du compresseur Huffman.
#
# Exécution (depuis le dossier projet_huffman/) :
#     make test
# ou directement :
#     ./tests/test_huffman.sh
# ou avec un binaire précis :
#     HUFFMAN_BIN=./huffman-debug ./tests/test_huffman.sh
#
# Prérequis : bash, cmp, mktemp, head. python3 est utilisé pour les
# fixtures générées (tous les octets, données pseudo-aléatoires, gros
# fichier) et pour corrompre des métadonnées ; en son absence, ces
# tests sont signalés « SAUTÉS » et comptés à part.
#
# Chaque test affiche [OK], [ECHEC] ou [SAUTE], et un résumé final
# donne le détail. Le script se termine par 0 seulement si aucun
# test n'a échoué.

set -u

# ------------------------------------------------------------------ #
# Préparation                                                         #
# ------------------------------------------------------------------ #

REP_PROJET="$(cd "$(dirname "$0")/.." && pwd)"
BINAIRE="${HUFFMAN_BIN:-$REP_PROJET/huffman}"
FIXTURES="$REP_PROJET/tests/fixtures"
TRAVAIL="$(mktemp -d)"
trap 'rm -rf "$TRAVAIL"' EXIT

NB_OK=0
NB_ECHECS=0
NB_SAUTES=0

ok()    { NB_OK=$((NB_OK + 1));     printf '  [OK]     %s\n' "$1"; }
echec() { NB_ECHECS=$((NB_ECHECS + 1)); printf '  [ECHEC]  %s\n' "$1"; }
saute() { NB_SAUTES=$((NB_SAUTES + 1)); printf '  [SAUTE]  %s (%s)\n' "$1" "$2"; }
section() { printf '%s\n' "$1"; }

# échec attendu : la commande doit échouer (code de sortie non nul)
doit_echouer() {
    local description="$1"
    shift
    if "$@" > "$TRAVAIL/stdout.txt" 2> "$TRAVAIL/stderr.txt"; then
        echec "$description (la commande aurait dû échouer)"
        return 1
    fi
    ok "$description"
    return 0
}

# Aller-retour complet : compresser, décompresser, comparer octet par
# octet. $1 = nom du test, $2 = fichier à traiter.
aller_retour() {
    local nom="$1"
    local fichier="$2"

    if ! "$BINAIRE" -c "$fichier" "$TRAVAIL/compresse.huf" \
            > "$TRAVAIL/stdout.txt" 2> "$TRAVAIL/stderr.txt"; then
        echec "$nom (la compression a échoué)"
        return 1
    fi
    if ! "$BINAIRE" -d "$TRAVAIL/compresse.huf" "$TRAVAIL/restaure" \
            > "$TRAVAIL/stdout.txt" 2> "$TRAVAIL/stderr.txt"; then
        echec "$nom (la décompression a échoué)"
        return 1
    fi
    if cmp -s "$fichier" "$TRAVAIL/restaure"; then
        ok "$nom"
        return 0
    fi
    echec "$nom (le fichier restauré diffère de l'original)"
    return 1
}

# ------------------------------------------------------------------ #
# Vérifications préalables                                            #
# ------------------------------------------------------------------ #

if [ ! -x "$BINAIRE" ]; then
    printf "test_huffman : binaire introuvable (%s).\n" "$BINAIRE"
    printf "Lancez d'abord « make » depuis le dossier projet_huffman/.\n"
    exit 1
fi

if command -v python3 > /dev/null 2>&1; then
    PYTHON_OK=1
else
    PYTHON_OK=0
fi

# ------------------------------------------------------------------ #
# Tests 1 à 5 : fichiers réels versionnés                             #
# ------------------------------------------------------------------ #

section "Tests 1 à 5 : cas de base et fichiers texte"

aller_retour "1. fichier vide" "$FIXTURES/vide.txt"
[ -f "$TRAVAIL/restaure" ] && [ ! -s "$TRAVAIL/restaure" ] \
    && ok "1. fichier vide : la sortie restaurée est bien vide" \
    || echec "1. fichier vide : la sortie restaurée devrait être vide"

aller_retour "2. fichier d'un seul octet" "$FIXTURES/un_octet.txt"
aller_retour "3. un seul symbole répété 4000 fois" "$FIXTURES/symbole_unique.txt"
aller_retour "4. texte court" "$FIXTURES/texte_court.txt"
aller_retour "5. texte avec espaces, accents et ponctuation" "$FIXTURES/texte_riche.txt"

# ------------------------------------------------------------------ #
# Tests 6 à 8 : fixtures générées (python3 requis)                     #
# ------------------------------------------------------------------ #

section "Tests 6 à 8 : données binaires et fichiers volumineux"

if [ "$PYTHON_OK" = 1 ]; then
    python3 - <<'EOF' > "$TRAVAIL/tous_octets.bin"
import sys
sys.stdout.buffer.write(bytes(range(256)) * 40)
EOF
    aller_retour "6. tous les octets de 0 à 255 (répétés 40 fois)" \
        "$TRAVAIL/tous_octets.bin"

    python3 - <<'EOF' > "$TRAVAIL/aleatoire.bin"
import sys
sys.stdout.buffer.write(__import__("random").Random(42).randbytes(100000))
EOF
    aller_retour "7. 100 000 octets pseudo-aléatoires (graine 42)" \
        "$TRAVAIL/aleatoire.bin"

    python3 - <<'EOF' > "$TRAVAIL/volumineux.bin"
import sys
sys.stdout.buffer.write(b"des donnees qui se repetent, " * 200000)
EOF
    aller_retour "8. fichier volumineux (5,8 Mo, motif répété)" \
        "$TRAVAIL/volumineux.bin"
else
    saute "6. tous les octets" "python3 absent"
    saute "7. données pseudo-aléatoires" "python3 absent"
    saute "8. fichier volumineux" "python3 absent"
fi

# ------------------------------------------------------------------ #
# Test supplémentaire : compresser un fichier déjà compressé          #
# ------------------------------------------------------------------ #

section "Test supplémentaire : fichier déjà compressé"

if [ "$PYTHON_OK" = 1 ]; then
    "$BINAIRE" -c "$TRAVAIL/aleatoire.bin" "$TRAVAIL/once.huf" > /dev/null 2>&1 \
        && "$BINAIRE" -c "$TRAVAIL/once.huf" "$TRAVAIL/twice.huf" > /dev/null 2>&1 \
        && "$BINAIRE" -d "$TRAVAIL/twice.huf" "$TRAVAIL/once2" > /dev/null 2>&1 \
        && "$BINAIRE" -d "$TRAVAIL/once2" "$TRAVAIL/aleatoire2" > /dev/null 2>&1 \
        && cmp -s "$TRAVAIL/aleatoire.bin" "$TRAVAIL/aleatoire2" \
        && ok "8b. double compression/décompression (aller-retour ×2)" \
        || echec "9b. double compression/décompression"
else
    saute "9b. double compression" "python3 absent"
fi

# ------------------------------------------------------------------ #
# Tests 9 à 14 : fichiers corrompus et erreurs                        #
# ------------------------------------------------------------------ #

section "Tests 9 à 14 : corruption et erreurs attendues"

# 9. Fichier compressé tronqué : une compression valide, puis on coupe.
"$BINAIRE" -c "$FIXTURES/texte_riche.txt" "$TRAVAIL/valide.huf" > /dev/null 2>&1
taille_valide=$(wc -c < "$TRAVAIL/valide.huf")
head -c $((taille_valide - 5)) "$TRAVAIL/valide.huf" > "$TRAVAIL/tronque.huf"
doit_echouer "9. fichier compressé tronqué : décompression refusée" \
    "$BINAIRE" -d "$TRAVAIL/tronque.huf" "$TRAVAIL/sortie9"

# 9b. En-tête seul tronqué.
head -c 10 "$TRAVAIL/valide.huf" > "$TRAVAIL/tronque10.huf"
doit_echouer "9b. en-tête tronqué : décompression refusée" \
    "$BINAIRE" -d "$TRAVAIL/tronque10.huf" "$TRAVAIL/sortie9b"

# 10. Mauvaise signature.
{ printf 'XXXX'; tail -c +5 "$TRAVAIL/valide.huf"; } > "$TRAVAIL/signature.huf"
doit_echouer "10. mauvaise signature : décompression refusée" \
    "$BINAIRE" -d "$TRAVAIL/signature.huf" "$TRAVAIL/sortie10"

# 11. Fichier d'entrée inexistant.
doit_echouer "11. fichier d'entrée inexistant : compression refusée" \
    "$BINAIRE" -c "$TRAVAIL/nimporte.quoi" "$TRAVAIL/sortie11"
doit_echouer "11b. fichier d'entrée inexistant : décompression refusée" \
    "$BINAIRE" -d "$TRAVAIL/nimporte.quoi" "$TRAVAIL/sortie11b"

# 12. Arguments incorrects.
doit_echouer "12. aucun argument : aide sur stderr et échec" "$BINAIRE"
doit_echouer "12b. option inconnue" "$BINAIRE" -x "$FIXTURES/vide.txt" "$TRAVAIL/o"
doit_echouer "12c. argument manquant à -c" "$BINAIRE" -c "$FIXTURES/vide.txt"
"$BINAIRE" --help > "$TRAVAIL/aide.txt" 2>&1 \
    && grep -qi "usage" "$TRAVAIL/aide.txt" \
    && ok "12d. --help affiche l'aide et réussit" \
    || echec "12d. --help devrait afficher l'aide et réussir"

# 13. Entrée et sortie identiques : refus, fichier d'origine intact.
cp "$FIXTURES/texte_riche.txt" "$TRAVAIL/meme.bin"
doit_echouer "13. entrée et sortie identiques (-c) : opération refusée" \
    "$BINAIRE" -c "$TRAVAIL/meme.bin" "$TRAVAIL/meme.bin"
cmp -s "$FIXTURES/texte_riche.txt" "$TRAVAIL/meme.bin" \
    && ok "13b. le fichier d'origine est resté intact" \
    || echec "13b. le fichier d'origine a été modifié"
doit_echouer "13c. entrée et sortie identiques (-d) : opération refusée" \
    "$BINAIRE" -d "$TRAVAIL/valide.huf" "$TRAVAIL/valide.huf"

# 14. Métadonnées incohérentes.
if [ "$PYTHON_OK" = 1 ]; then
    # a. une fréquence modifiée : la somme ne redonne plus la taille.
    python3 - "$TRAVAIL/valide.huf" "$TRAVAIL/freq.huf" <<'EOF'
import sys
data = bytearray(open(sys.argv[1], 'rb').read())
data[100] ^= 1          # zone des fréquences (octets 12 à 2067)
open(sys.argv[2], 'wb').write(data)
EOF
    doit_echouer "14. fréquence modifiée : métadonnées incohérentes refusées" \
        "$BINAIRE" -d "$TRAVAIL/freq.huf" "$TRAVAIL/sortie14"

    # b. la taille originale annoncée ne correspond plus.
    python3 - "$TRAVAIL/valide.huf" "$TRAVAIL/taille.huf" <<'EOF'
import sys
data = bytearray(open(sys.argv[1], 'rb').read())
data[4] += 1            # champ taille originale (octets 4 à 11)
open(sys.argv[2], 'wb').write(data)
EOF
    doit_echouer "14b. taille originale modifiée : refusée" \
        "$BINAIRE" -d "$TRAVAIL/taille.huf" "$TRAVAIL/sortie14b"

    # c. octets superflus ajoutés à la fin.
    python3 - "$TRAVAIL/valide.huf" "$TRAVAIL/superflu.huf" <<'EOF'
import sys
data = open(sys.argv[1], 'rb').read()
open(sys.argv[2], 'wb').write(data + b'\x00\x00\x00')
EOF
    doit_echouer "14c. octets superflus : fichier incohérent refusé" \
        "$BINAIRE" -d "$TRAVAIL/superflu.huf" "$TRAVAIL/sortie14c"
else
    saute "14. métadonnées incohérentes" "python3 absent"
fi

# ------------------------------------------------------------------ #
# Résumé                                                              #
# ------------------------------------------------------------------ #

printf '\nRésumé : %d OK, %d échec(s), %d sauté(s)\n' \
    "$NB_OK" "$NB_ECHECS" "$NB_SAUTES"

if [ "$NB_ECHECS" -ne 0 ]; then
    exit 1
fi
exit 0
