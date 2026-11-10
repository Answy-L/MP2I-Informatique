# Explication pour l'oral — Compresseur Huffman

Ce document aide le binôme à comprendre et défendre le code du dossier
`projet_huffman/`. Il ne suppose pas que tout est déjà maîtrisé : chaque
notion est expliquée à partir du code.

## 1. Pourquoi la compression de Huffman est-elle « sans perte » ?

Contrairement à la compression d'image JPEG ou de musique MP3, qui
jettent des informations que l'œil ou l'oreille perçoit mal, Huffman ne
fait que **remplacer** chaque octet par un autre motif binaire plus
court. Le motif de remplacement et la table de correspondance sont
connus des deux côtés (compresseur et décompresseur), donc le message
d'origine se retrouve exactement. Les tests du projet le vérifient
octet par octet avec `cmp` : pour chacun des fichiers d'essai, le
fichier restauré est **identique** à l'original.

## 2. Pourquoi les codes sont-ils « préfixes » et pourquoi est-ce indispensable ?

Un ensemble de codes est **préfixe** si aucun code n'est le début d'un
autre. Exemple : si `A = 0` et `B = 01`, impossible : `01` commence par
`0`. Avec des codes préfixes, la suite `010010` se découpe sans
ambiguïté, comme des mots français séparés par des espaces : on sait
toujours quand un code se termine.

C'est exactement ce que garantit l'arbre de Huffman : les codes sont
les **chemins de la racine vers les feuilles**. Un chemin vers une
feuille ne peut pas être le début d'un autre chemin vers une autre
feuille, parce que les feuilles n'ont pas de fils : le parcours
s'arrête dessus. Si deux codes se chevauchaient, deux feuilles seraient
l'une sur le chemin de l'autre, ce qui est impossible dans un arbre.

Réponse courte à l'oral : *« Chaque code est un chemin racine-feuille ;
une feuille n'a pas de descendants, donc aucun code ne peut en prolonger
un autre : les codes sont préfixes, et c'est pour ça qu'on peut décoder
sans séparateur. »*

## 3. Comment la construction « gloutonne » de l'arbre fonctionne-t-elle ?

L'algorithme part d'une forêt de feuilles isolées (une par symbole
présent, avec sa fréquence) et répète jusqu'à n'avoir plus qu'un arbre :

1. prendre les **deux nœuds de plus petite fréquence** ;
2. en faire les fils gauche et droit d'un nouveau nœud interne dont la
   fréquence est la somme ;
3. remettre ce nouveau nœud dans la file.

C'est « glouton » car chaque fusion fait le choix local optimal (les
petites fréquences descendent en bas de l'arbre, donc reçoivent des
codes longs). Au total, chaque fusion fait perdre un nœud : on part de
n feuilles, on fait n−1 fusions, il reste une racine. Dans le code :
`construire_arbre` dans `src/huffman.c`, boucle `while (tas.nb >= 2)`.

La file de priorité utilisée est un **tas min** (`TasMin`) : un tableau
où le plus petit élément est toujours accessible en O(1) et
extractible en O(log n) (`tas_extraire_min` + `tas_descendre`).

## 4. Quel est le rôle des fréquences ?

Elles mesurent ce qui est **prévisible** dans le fichier. Un octet
fréquent (par exemple `e` dans un texte français) reçoit un code court ;
un octet rare en reçoit un long. Le nombre total de bits du fichier
compressé vaut Σ fréquence(s) × longueur(s), que Huffman rend **minimal
parmi les codes de longueur fixe par symbole** : c'est le sens de
« optimal » pour cet algorithme.

Remarque utile : si les 256 octets sont équiprobables (fichier
aléatoire), tous les codes font 8 bits : aucun gain, et le fichier
compressé est plus grand que l'original à cause de l'en-tête. C'est le
test 7 de la suite, et c'est normal : Huffman ne peut compresser que ce
qui a des fréquences inégales.

## 5. Comment passe-t-on des codes binaires aux octets du fichier ?

L'ordinateur n'écrit que des octets : il faut donc **entasser** les
codes bout à bout. `src/bit_io.c` contient :

- `BitEcrivain` : accumule les bits un par un dans un octet (premier
  bit = bit de poids fort), écrit l'octet dès qu'il est plein ;
- `BitLecteur` : lit chaque octet du fichier et le décompose en 8 bits
  dans le même ordre.

Le dernier octet est rarement plein : on le complète avec des zéros
(bourrage, `bit_flush`). Le décodeur ne sera pas trompé grâce au point
suivant.

## 6. Comment le décodeur sait-il quand s'arrêter ?

Deux informations l'y aident :

1. **La taille originale**, stockée dans l'en-tête sur 8 octets : le
   décodeur produit exactement ce nombre d'octets, puis s'arrête. Les
   zéros de bourrage du dernier octet ne sont donc jamais décodés en
   trop.
2. **La forme même de l'arbre** : en partant de la racine et en
   descendant bit après bit (0 à gauche, 1 à droite), on s'arrête
   forcément sur une feuille ; un nœud interne a toujours deux fils, donc
   le parcours ne peut jamais « sortir » de l'arbre.

## 7. Les cas du fichier vide et du symbole unique

- **Fichier vide** : aucune fréquence non nulle, aucun arbre ; le
  fichier compressé se réduit à l'en-tête (2060 octets) ; la
  décompression produit un fichier vide. Testé (test 1).
- **Un seul symbole** (par exemple 4000 fois `a`) : la forêt ne
  contient qu'une feuille, aucune fusion n'a lieu, la racine **est** la
  feuille. Sans précaution, son code serait vide (aucun bit jamais
  écrit) et la décompression tournerait sans rien produire. Le code
  attribue donc conventionnellement le code **« 0 » sur un bit** à ce
  symbole (`generer_codes_recursif`, cas `noeud == racine`). Testé
  (tests 2 et 3).

## 8. Complexités temporelles et spatiales (et leurs hypothèses)

Notons S la taille du fichier d'entrée et σ ≤ 256 le nombre de
symboles distincts.

- **Compression** : deux lectures complètes du fichier (fréquences puis
  encodage), donc O(S) en temps ; construction de l'arbre O(σ log σ) ;
  au total **O(S)** avec σ ≤ 256 constant.
- **Décompression** : O(S) également.
- **Mémoire** : constant par rapport à S : table de 256 fréquences,
  arbre de 2σ−1 nœuds ≤ 511, table de codes 256×33 octets, et des
  tampons de lecture/écriture de 64 Kio. Le fichier n'est **jamais**
  chargé en entier.
- Hypothèses : lecture/écriture sur disque comptées à part (ce sont
  elles qui dominent en pratique) ; l'entrée est un fichier ordinaire
  (il faut pouvoir la relire depuis le début).

## 9. Limites du format choisi

- **En-tête fixe de 2060 octets** (4 + 8 + 256×8) : un fichier plus
  petit ou incompressible ressort plus gros. Amélioration possible :
  écrire l'arbre en préordre avec 2 bits par nœud (environ 160 octets
  maximum).
- Les fréquences sont stockées sur 8 octets chacune : format portable
  mais verbeux ; une taille par octets significatifs (1/2/4/8 selon la
  taille du fichier) économiserait de la place.
- La décompression refuse les fichiers plus longs que la taille
  prévue : c'est un choix de stricte vérification, mais un fichier
  avec des octets de bourrage ajoutés par un autre outil serait refusé.

## 10. Questions probables de l'enseignante, avec réponses courtes

**« Pourquoi un tas min et pas un simple tri ? »**
Un tri répété donne O(n²) ; le tas extrait le minimum en O(log n),
donc la construction est en O(n log n) avec n ≤ 256. Et le cahier des
charges demande explicitement une file de priorité.

**« Votre fichier compressé d'un petit fichier est plus gros que
l'original, pourquoi ? »**
À cause de l'en-tête de 2060 octets. Huffman ne compresse que le flux
de bits ; pour 78 octets de texte, le flux fait 39 octets mais le
fichier final fait 2099. Le programme affiche les deux tailles, sans
jamais prétendre à un gain inexistant.

**« Que se passe-t-il si je coupe la fin du fichier compressé ? »**
La décompression calcule la taille exacte attendue
(en-tête + flux) et compare à la taille réelle ; en cas d'écart, ou si
les bits s'épuisent avant la taille originale, elle refuse le fichier
avec un message clair et sort avec le code 1. Testé (tests 9 et 14).

**« Et si la somme des fréquences ne correspond pas à la taille ? »**
C'est refusé : la somme est recalculée à la lecture, avec contrôle de
débordement, et doit égaler exactement la taille annoncée. Testé.

**« Comment être sûr qu'il n'y a pas de fuite mémoire ? »**
Chaque structure n'a qu'une seule allocation (pool de nœuds, tas,
table de codes, tampons) et un seul propriétaire, libéré sur tous les
chemins, y compris d'erreur (les libérations sont regroupées à l'étiquette
`fin:`). La suite de tests passe entièrement avec UBSan ; ASan et
valgrind n'étaient pas disponibles dans l'environnement de
développement, ce qui est documenté dans le README.

**« Pourquoi relire le fichier deux fois en compression ? »**
Première passe : compter les fréquences (il faut toutes les connaître
avant de construire l'arbre). Deuxième passe : encoder avec les codes.
L'alternative serait de garder tout le fichier en mémoire : interdit
par la contrainte mémoire du cahier des charges.

**« Comment gérez-vous `0x00` ou les octets > 0x7F ? »**
Le programme ne fait aucune différence entre texte et binaire : il lit
des `unsigned char` et une table de 256 entrées. Le test 6 comprime
tous les octets 0 à 255 avec un aller-retour identique.

**« Pourquoi ne pas faire un `fwrite` de la structure des fréquences ? »**
Une structure C peut contenir des octets de remplissage (padding) et
son ordre des octets dépend de la machine : un `fwrite` direct donnerait
un fichier illisible ailleurs. L'en-tête est donc écrit **octet par
octet**, en petit-boutiste documenté.

**« Que renvoie le programme en cas d'erreur ? »**
Un message en français sur la sortie d'erreur standard, jamais de
données binaires affichées, et le code de sortie 1 (`EXIT_FAILURE`).
Le succès renvoie 0 (`EXIT_SUCCESS`).
