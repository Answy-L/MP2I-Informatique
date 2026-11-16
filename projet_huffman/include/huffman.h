#ifndef HUFFMAN_H
#define HUFFMAN_H

/*
 * huffman.h — interface publique du compresseur Huffman.
 *
 * Projet 2 (Compresseur de Huffman) — MP2I, Romain CHEN et Mohammed
 * MOUSSAOUI, enseignante E. SABAHI.
 *
 * Le programme compresse et décompresse n'importe quel fichier binaire
 * (pas seulement du texte) sans perte, à l'aide de l'algorithme de
 * Huffman. Les fichiers sont lus par blocs : aucun fichier n'est
 * chargé entièrement en mémoire.
 */

#include <stdint.h>
#include <stddef.h>

/* Nombre de symboles possibles : un octet peut prendre 256 valeurs. */
#define HUF_NB_SYMB 256

/*
 * Taille de l'en-tête du fichier compressé, en octets :
 *   - signature "HUF1"            :  4 octets
 *   - taille originale du fichier :  8 octets (petit-boutiste)
 *   - 256 fréquences              : 256 * 8 octets (petit-boutiste)
 * Soit 4 + 8 + 2048 = 2060 octets.
 */
#define HUF_TAILLE_ENTETE ((size_t)(4 + 8 + HUF_NB_SYMB * 8))

/*
 * Un code Huffman : une suite de bits (0 = "aller à gauche", 1 = "aller
 * à droite"). Le i-ème bit du code est stocké au bit (i % 8) de
 * l'octet bits[i / 8].
 *
 * Avec 256 symboles au maximum, la profondeur de l'arbre ne peut pas
 * dépasser 255, donc 32 octets (256 bits) suffisent toujours.
 */
typedef struct {
    uint8_t longueur;   /* nombre de bits utiles du code (0 = pas de code) */
    uint8_t bits[32];   /* bits du code, le premier bit est le bit 0 de bits[0] */
} CodeHuffman;

/*
 * Nœud de l'arbre de Huffman. Une feuille porte un symbole (0 à 255) ;
 * un nœud interne n'en porte pas (symbole = -1) et a toujours deux fils.
 */
typedef struct Noeud {
    int symbole;                 /* 0 à 255 pour une feuille, -1 sinon */
    uint64_t frequence;          /* nombre d'occurrences (feuille) ou somme */
    struct Noeud *gauche;        /* fils pour le bit 0 (NULL sur une feuille) */
    struct Noeud *droite;        /* fils pour le bit 1 (NULL sur une feuille) */
} Noeud;

/*
 * Compresse le fichier `entree` dans le fichier `sortie`.
 *
 * Retour : 0 en cas de succès, -1 en cas d'erreur (un message détaillé
 * est alors écrit sur la sortie d'erreur standard). En cas d'échec, le
 * fichier de sortie peut rester présent mais incomplet : c'est le code
 * de sortie du programme qui fait foi.
 *
 * Refuse d'écrire dans le fichier d'entrée lui-même (mêmes données
 * `struct stat` : même périphérique et même numéro d'inode).
 */
int huf_compresser(const char *entree, const char *sortie);

/*
 * Décompresse un fichier `entree` au format HUF1 vers `sortie`.
 *
 * Vérifie la signature, la cohérence des métadonnées (somme des
 * fréquences égale à la taille annoncée, sans débordement), la longueur
 * exacte du fichier et s'arrête exactement au nombre d'octets
 * originaux : les bits de bourrage du dernier octet sont ignorés.
 *
 * Retour : 0 en cas de succès, -1 en cas d'erreur.
 */
int huf_decompresser(const char *entree, const char *sortie);

#endif /* HUFFMAN_H */
