/*
 * main.c — programme de test du jalon 1.
 * Auteurs : Romain CHEN, Mohammed MOUSSAOUI
 *
 * Usage :   ./formes tests/formes01.bmp
 * Sans argument, il essaie tests/formes01.bmp.
 *
 * Résultat attendu avec l'image générée par generer_image.py :
 *   Image chargee : 64 x 48 px
 *   Pixels noirs (<128) : 496
 */

#include <stdio.h>
#include "bmp.h"

int main(int argc, char **argv) {
    const char *nom = (argc > 1) ? argv[1] : "tests/formes01.bmp";

    Image *img = lire_bmp(nom);
    if (img == NULL) {
        fprintf(stderr, "Echec du chargement de \"%s\"\n", nom);
        return 1;
    }

    printf("Image chargee : %d x %d px\n", img->largeur, img->hauteur);

    /* petite vérification : compter les pixels proches du noir */
    long noirs = 0;
    for (int i = 0; i < img->largeur * img->hauteur; i++) {
        if (img->pixels[i] < 128) {
            noirs++;
        }
    }
    printf("Pixels noirs (<128) : %ld\n", noirs);

    liberer_image(img);
    return 0;
}
