# Tests du compresseur Huffman

## Mode d'emploi

Depuis le dossier `projet_huffman/` :

```bash
make           # compiler d'abord le binaire
make test      # exécuter toute la suite
```

Ou directement :

```bash
bash ./tests/test_huffman.sh
```

Ou avec le binaire instrumenté UBSan :

```bash
make debug
HUFFMAN_BIN=./huffman-debug bash ./tests/test_huffman.sh
```

Le script compile rien lui-même : il exécute le binaire `huffman` du
dossier (variable d'environnement `HUFFMAN_BIN` pour en changer). Il
travaille dans un répertoire temporaire supprimé à la fin : aucun
fichier de test n'est laissé derrière.

## Prérequis

- `bash`, `cmp`, `mktemp`, `head` ;
- `python3` pour les fixtures générées (tous les octets, données
  pseudo-aléatoires déterministes, gros fichier) et pour corrompre des
  métadonnées. **Sans python3**, ces tests sont signalés `[SAUTE]` et
  comptés à part (aucun échec) ; 18 tests sur 25 restent exécutables.

## Ce que la suite vérifie (25 vérifications)

| Catégorie | Tests |
|---|---|
| Aller-retour identique (compression puis décompression, comparaison `cmp`) | fichier vide, 1 octet, un symbole ×4000, texte court, texte avec accents/ponctuation, tous les octets 0–255, 100 000 octets pseudo-aléatoires (graine 42 : reproductible), 5,8 Mo répétitifs, double compression |
| Fichiers corrompus refusés | troncature du flux, en-tête tronqué, mauvaise signature, fréquence modifiée, taille originale modifiée, octets superflus en fin de fichier |
| Erreurs d'utilisation refusées | entrée inexistante, aucun argument, option inconnue, argument manquant, entrée et sortie identiques (avec vérification que le fichier d'origine reste intact) |
| Aide | `--help` affiche l'aide et sort avec le code 0 |

Chaque test affiche `[OK]`, `[ECHEC]` ou `[SAUTE]` ; le résumé final
donne les comptes et le script sort avec le code 1 si au moins un test
a échoué.

## Fixtures versionnées (`fixtures/`)

- `vide.txt` : fichier vide.
- `un_octet.txt` : un seul octet `a`.
- `symbole_unique.txt` : 4000 fois `a` (cas du symbole unique).
- `texte_court.txt` : un texte court.
- `texte_riche.txt` : accents, ponctuation, tabulations, sauts de ligne.

Les autres entrées (tous les octets, aléatoire, volumineux) sont
**générées à chaque exécution** par le script (reproductibles : même
graine), elles ne sont volontairement pas versionnées.
