/*
 * bmp.c — jalon 1 : lecture d'un BMP 24 bits.
 * Auteurs : Romain CHEN, Mohammed MOUSSAOUI
 *
 * lire_bmp ouvre le fichier, vérifie la signature "BM", lit l'en-tête
 * (54 octets), récupère l'offset des pixels et les dimensions, puis lit
 * les pixels ligne par ligne en niveaux de gris. Les deux pièges du
 * format BMP sont gérés :
 *   - chaque ligne est arrondie à un multiple de 4 octets (padding) ;
 *   - les lignes sont stockées de bas en haut.
 *
 * Limites connues : seuls les BMP 24 bits non compressés sont acceptés ;
 * une hauteur négative (BMP stocké de haut en bas) est refusée, comme
 * toute dimension invalide.
 */

#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"

Image *lire_bmp(const char *nom) {
    /* 1. Ouverture du fichier en lecture binaire. */
    FILE *f = fopen(nom, "rb");
    if (f == NULL) {
        fprintf(stderr, "lire_bmp : impossible d'ouvrir \"%s\"\n", nom);
        return NULL;
    }

    /* 2. Lecture de l'en-tête complet (54 octets) d'un seul coup. */
    unsigned char entete[54];
    if (fread(entete, 1, 54, f) != 54) {
        fprintf(stderr, "lire_bmp : \"%s\" est trop court (en-tête incomplet)\n", nom);
        fclose(f);
        return NULL;
    }

    /* 3. Signature : les deux premiers octets doivent être 'B' puis 'M'. */
    if (entete[0] != 'B' || entete[1] != 'M') {
        fprintf(stderr, "lire_bmp : \"%s\" n'est pas un fichier BMP\n", nom);
        fclose(f);
        return NULL;
    }

    /* 4. Reconstruction des entiers, stockés en petit-boutiste
     *    (octet de poids faible en premier). On décale en unsigned :
     *    décaler un int signé dans son bit de signe est un comportement
     *    indéfini en C, et l'octet de poids fort peut dépasser 127. */
    int offset = (int)((unsigned)entete[10] | ((unsigned)entete[11] << 8)
                     | ((unsigned)entete[12] << 16) | ((unsigned)entete[13] << 24));
    int largeur = (int)((unsigned)entete[18] | ((unsigned)entete[19] << 8)
                      | ((unsigned)entete[20] << 16) | ((unsigned)entete[21] << 24));
    int hauteur = (int)((unsigned)entete[22] | ((unsigned)entete[23] << 8)
                      | ((unsigned)entete[24] << 16) | ((unsigned)entete[25] << 24));

    /* 5. Dimensions : une hauteur négative (BMP top-down) est refusée. */
    if (largeur <= 0 || hauteur <= 0) {
        fprintf(stderr, "lire_bmp : dimensions invalides (%d x %d)\n",
                largeur, hauteur);
        fclose(f);
        return NULL;
    }

    /* 6. Allocation de l'image. */
    Image *img = malloc(sizeof *img);
    if (img == NULL) {
        fprintf(stderr, "lire_bmp : allocation impossible\n");
        fclose(f);
        return NULL;
    }
    img->largeur = largeur;
    img->hauteur = hauteur;
    img->pixels = malloc((size_t)largeur * (size_t)hauteur);
    if (img->pixels == NULL) {
        fprintf(stderr, "lire_bmp : allocation impossible\n");
        liberer_image(img);
        fclose(f);
        return NULL;
    }

    /* 7. Positionnement sur les pixels (l'offset vient de l'en-tête). */
    if (fseek(f, offset, SEEK_SET) != 0) {
        fprintf(stderr, "lire_bmp : positionnement impossible\n");
        liberer_image(img);
        fclose(f);
        return NULL;
    }

    /* 8. Lecture des pixels, ligne par ligne.
     *    Une ligne BMP est arrondie à un multiple de 4 octets : on saute
     *    le padding après chaque ligne. Les lignes sont stockées de bas
     *    en haut : la ligne y du fichier correspond à la ligne
     *    (hauteur - 1 - y) de l'image. Chaque pixel est stocké dans
     *    l'ordre (B, V, R), converti en niveaux de gris. */
    int padding = (4 - (largeur * 3) % 4) % 4;
    for (int y = 0; y < hauteur; y++) {
        int ligne = hauteur - 1 - y;
        for (int x = 0; x < largeur; x++) {
            unsigned char bgr[3];
            if (fread(bgr, 1, 3, f) != 3) {
                fprintf(stderr, "lire_bmp : \"%s\" est tronqué\n", nom);
                liberer_image(img);
                fclose(f);
                return NULL;
            }
            img->pixels[ligne * largeur + x] =
                (unsigned char)((bgr[0] + bgr[1] + bgr[2]) / 3);
        }
        if (padding > 0 && fseek(f, padding, SEEK_CUR) != 0) {
            fprintf(stderr, "lire_bmp : \"%s\" est tronqué\n", nom);
            liberer_image(img);
            fclose(f);
            return NULL;
        }
    }

    /* 9. Fermeture du fichier et retour de l'image. */
    fclose(f);
    return img;
}

void liberer_image(Image *img) {
    if (img == NULL) {
        return;
    }
    free(img->pixels);
    free(img);
}
