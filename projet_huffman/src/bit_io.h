#ifndef BIT_IO_H
#define BIT_IO_H

/*
 * bit_io.h — écriture et lecture bit à bit dans des flux binaires.
 *
 * Les bits sont rangés dans les octets de gauche à droite : le premier
 * bit écrit occupe le bit de poids fort (bit 7) du premier octet, le
 * huitième bit écrit complète l'octet, etc. Le lecteur relit les bits
 * dans le même ordre.
 */

#include <stdio.h>

/*
 * Écrivain bit à bit. Les bits s'accumulent dans `octet` en partant du
 * bit de poids fort ; l'octet est écrit dans le flux dès qu'il est
 * plein. À la fin, `bit_flush` écrit l'octet incomplet s'il en reste
 * un, complété par des zéros (bourrage). Le décodeur ignore ce bourrage
 * grâce à la taille originale stockée dans l'en-tête.
 */
typedef struct {
    FILE *flux;             /* flux binaire de destination (non possédé) */
    unsigned char octet;    /* octet en cours de remplissage */
    unsigned nb_bits;       /* bits déjà remplis dans `octet` (0 à 8) */
} BitEcrivain;

/*
 * Lecteur bit à bit. Lit les octets du flux un par un et les décompose
 * en 8 bits, du bit de poids fort au bit de poids faible.
 */
typedef struct {
    FILE *flux;             /* flux binaire source (non possédé) */
    unsigned char octet;    /* octet en cours de décomposition */
    unsigned nb_lus;        /* bits déjà consommés de `octet` (0 à 8) */
} BitLecteur;

/* Initialise l'écrivain pour écrire dans `flux`. */
void bit_ecrivain_init(BitEcrivain *ecrivain, FILE *flux);

/*
 * Écrit un bit (0 ou 1). L'octet plein est écrit dès que possible.
 * Retour : 0 en cas de succès, -1 en cas d'erreur d'écriture.
 */
int bit_ecrire(BitEcrivain *ecrivain, unsigned bit);

/*
 * Écrit l'octet partiel restant, complété par des zéros, puis
 * s'assure que tout est parti vers le flux (ne ferme pas le flux).
 * Ne fait rien si l'octet courant est vide.
 * Retour : 0 en cas de succès, -1 en cas d'erreur d'écriture.
 */
int bit_flush(BitEcrivain *ecrivain);

/* Initialise le lecteur pour lire dans `flux`. */
void bit_lecteur_init(BitLecteur *lecteur, FILE *flux);

/*
 * Lit le bit suivant.
 * Retour : 0 ou 1 pour un bit, -1 si le flux est épuisé ou en erreur.
 */
int bit_lire(BitLecteur *lecteur);

#endif /* BIT_IO_H */
