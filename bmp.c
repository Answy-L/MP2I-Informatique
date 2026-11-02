/*
 * bmp.c — jalon 1 : lecture d'un BMP 24 bits.
 * Auteurs : Romain CHEN, Mohammed MOUSSAOUI
 *
 * ⚠️ C'est ICI que vous codez. La fonction liberer_image est donnée ;
 * lire_bmp est à écrire en suivant le guide ci-dessous, pas à pas.
 */

#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"

Image *lire_bmp(const char *nom) {
    /*
     * GUIDE DU JALON 1 — déroulez les étapes dans cet ordre :
     *
     * 1. Ouvrir le fichier : FILE *f = fopen(nom, "rb");
     *    Si f == NULL : afficher une erreur et renvoyer NULL.
     *
     * 2. Vérifier la signature : lire 2 octets (fread ou fgetc),
     *    ils doivent être 'B' puis 'M'. Sinon : fermer f, renvoyer NULL.
     *
     * 3. Lire l'en-tête. Les BMP stockent leurs nombres en "petit-boutiste"
     *    (octet de poids faible en premier). Trois lectures possibles :
     *      - fread dans des unsigned char puis reconstruire les entiers :
     *        largeur  = b[0] | b[1]<<8 | b[2]<<16 | b[3]<<24;
     *      - ou fread(&v, 4, 1, f) directement (x86/Windows est petit-boutiste)
     *    Nombres à récupérer :
     *      - octets 10-13 : position des pixels dans le fichier ("offset")
     *      - octets 18-21 : largeur
     *      - octets 22-25 : hauteur
     *    Conseil : lisez TOUT l'en-tête d'un coup (fread de 54 octets dans un
     *    tableau unsigned char entete[54]), puis piochez dedans. C'est plus
     *    simple que 10 petits fread, et facile à afficher pour déboguer.
     *
     * 4. Vérifications : largeur > 0, hauteur > 0 (sinon fermer, renvoyer NULL).
     *
     * 5. Allouer l'image :
     *      Image *img = malloc(sizeof *img);
     *      img->largeur = largeur; img->hauteur = hauteur;
     *      img->pixels = malloc((size_t)largeur * hauteur);
     *    Puis vérifier les deux malloc != NULL.
     *
     * 6. Se positionner sur les pixels : fseek(f, offset, SEEK_SET);
     *
     * 7. Lire les pixels ligne par ligne. ATTENTION aux deux pièges :
     *      - une ligne BMP est arrondie à un multiple de 4 octets :
     *        padding = (4 - (largeur * 3) % 4) % 4;
     *        après avoir lu largeur*3 octets, sauter padding octets ;
     *      - les lignes sont stockées de BAS en haut : la première ligne du
     *        fichier est celle du bas de l'image.
     *    Pour chaque pixel : 3 octets dans l'ordre (B, V, R).
     *    gris = (B + V + R) / 3;  puis stockez-le dans img->pixels.
     *
     * 8. fermer le fichier (fclose) et renvoyer img.
     *    En cas d'erreur après l'allocation : libérer avant de renvoyer NULL.
     */
    (void)nom; /* à supprimer une fois le code écrit */
    return NULL; /* à remplacer */
}

void liberer_image(Image *img) {
    if (img == NULL) {
        return;
    }
    free(img->pixels);
    free(img);
}
