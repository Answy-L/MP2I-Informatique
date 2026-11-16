/*
 * main.c — interface en ligne de commande du compresseur Huffman.
 *
 * Usage :
 *   huffman -c entree sortie.huf   compresse
 *   huffman -d entree.huf sortie   décompresse
 *   huffman --help                 affiche l'aide
 *
 * Codes de sortie : EXIT_SUCCESS (0) en cas de réussite, EXIT_FAILURE
 * (1) en cas d'échec (arguments invalides, fichier introuvable,
 * fichier corrompu...).
 */

#define _POSIX_C_SOURCE 200809L /* stat */

#include "huffman.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Affiche l'aide complète sur le flux demandé (stdout ou stderr). */
static void afficher_aide(FILE *flux)
{
    fprintf(flux,
        "Usage : huffman -c <entree> <sortie.huf>   compresser un fichier\n"
        "       huffman -d <entree.huf> <sortie>   decompresser un fichier\n"
        "\n"
        "Options :\n"
        "  -c        compression : lit <entree> et ecrit <sortie.huf>\n"
        "  -d        decompression : lit <entree.huf> et recree <sortie>\n"
        "  --help    afficher cette aide\n"
        "\n"
        "Le fichier compressé contient tout ce qu'il faut pour une\n"
        "décompression autonome (fréquences des 256 octets). L'entrée et\n"
        "la sortie ne peuvent pas désigner le même fichier.\n"
        "\n"
        "Codes de sortie : 0 si l'opération réussit, 1 en cas d'erreur.\n");
}

/*
 * Renvoie la taille en octets d'un fichier, ou 0 si l'information est
 * indisponible (la taille ne sert qu'à l'affichage).
 */
static uint64_t taille_du_fichier(const char *chemin)
{
    struct stat etat;

    if (stat(chemin, &etat) != 0) {
        return 0;
    }
    return (uint64_t)etat.st_size;
}

/* Affiche les statistiques après une compression réussie. */
static void afficher_stats_compression(const char *entree,
                                        const char *sortie)
{
    uint64_t taille_entree = taille_du_fichier(entree);
    uint64_t taille_sortie = taille_du_fichier(sortie);
    uint64_t taille_flux = 0;

    if (taille_sortie >= (uint64_t)HUF_TAILLE_ENTETE) {
        taille_flux = taille_sortie - (uint64_t)HUF_TAILLE_ENTETE;
    }
    printf("Compression de « %s » réussie.\n", entree);
    printf("  Taille originale : %" PRIu64 " octets\n", taille_entree);
    printf("  Taille compressée totale : %" PRIu64 " octets "
           "(en-tête : %u octets, flux codé : %" PRIu64 " octets)\n",
           taille_sortie, (unsigned)HUF_TAILLE_ENTETE, taille_flux);
    if (taille_entree == 0) {
        printf("  Taux de compression : sans objet (fichier vide)\n");
    } else if (taille_entree > taille_sortie) {
        printf("  Taux de compression : %.1f %%\n",
               100.0 * (1.0 - (double)taille_sortie / (double)taille_entree));
    } else {
        /* L'en-tête rend le résultat plus gros que l'original : c'est
         * normal pour les petits fichiers ou les fichiers incompressibles. */
        printf("  Taux de compression : aucun gain "
               "(l'en-tête pèse %u octets)\n",
               (unsigned)HUF_TAILLE_ENTETE);
    }
}

/* Affiche les statistiques après une décompression réussie. */
static void afficher_stats_decompression(const char *sortie,
                                          uint64_t taille_originale)
{
    printf("Décompression réussie : « %s » recréé "
           "(%" PRIu64 " octets, identique à l'original).\n",
           sortie, taille_originale);
}

int main(int argc, char *argv[])
{
    if (argc == 2
            && (strcmp(argv[1], "--help") == 0
                || strcmp(argv[1], "-h") == 0)) {
        afficher_aide(stdout);
        return EXIT_SUCCESS;
    }

    if (argc != 4 || argv[1][0] != '-' || argv[1][1] == '\0'
            || argv[1][2] != '\0') {
        fprintf(stderr, "huffman : arguments invalides.\n\n");
        afficher_aide(stderr);
        return EXIT_FAILURE;
    }

    switch (argv[1][1]) {
    case 'c':
        if (huf_compresser(argv[2], argv[3]) != 0) {
            return EXIT_FAILURE;
        }
        afficher_stats_compression(argv[2], argv[3]);
        return EXIT_SUCCESS;

    case 'd':
        if (huf_decompresser(argv[2], argv[3]) != 0) {
            return EXIT_FAILURE;
        }
        afficher_stats_decompression(argv[3], taille_du_fichier(argv[3]));
        return EXIT_SUCCESS;

    default:
        fprintf(stderr, "huffman : option inconnue « %s ».\n\n", argv[1]);
        afficher_aide(stderr);
        return EXIT_FAILURE;
    }
}
