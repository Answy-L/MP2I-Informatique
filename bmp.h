#ifndef BMP_H
#define BMP_H

/*
 * bmp.h — module de lecture/écriture d'images BMP 24 bits (projet MP2I).
 * Auteurs : Romain CHEN, Mohammed MOUSSAOUI
 */

/* Image en niveaux de gris : 0 = noir, 255 = blanc.
 * pixels est un tableau de largeur * hauteur octets, alloué dynamiquement.
 * pixel (x, y)  ==  pixels[y * largeur + x]
 */
typedef struct {
    int largeur, hauteur;
    unsigned char *pixels;
} Image;

/*
 * Lit un fichier BMP 24 bits et renvoie l'image convertie en niveaux de gris.
 * Renvoie NULL si le fichier n'existe pas, n'est pas un BMP, ou est tronqué.
 */
Image *lire_bmp(const char *nom);

/* Libère une image allouée par lire_bmp (NULL est accepté). */
void liberer_image(Image *img);

#endif
