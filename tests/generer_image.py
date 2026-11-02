"""
Génère l'image de test tests/formes01.bmp (BMP 24 bits) :
fond blanc, carré noir 20x20 et rectangle noir 12x8.

Aucune bibliothèque externe : uniquement la bibliothèque standard Python.
Usage :  python3 tests/generer_image.py
Résultat attendu quand le programme C fonctionnera :
  Image chargee : 64 x 48 px
  Pixels noirs (<128) : 496
"""

import struct
from pathlib import Path

LARGEUR, HAUTEUR = 64, 48


def pixel(x, y):
    """Noir (0,0,0) à l'intérieur des formes, blanc (255,255,255) sinon."""
    if 10 <= x < 30 and 10 <= y < 30:      # carré 20x20
        return (0, 0, 0)
    if 40 <= x < 52 and 30 <= y < 38:      # rectangle 12x8
        return (0, 0, 0)
    return (255, 255, 255)


def main():
    taille_ligne = LARGEUR * 3            # 192 : déjà multiple de 4, pas de padding ici
    donnees = b""
    for y in range(HAUTEUR - 1, -1, -1):  # lignes BMP stockées de bas en haut
        ligne = b""
        for x in range(LARGEUR):
            ligne += bytes(reversed(pixel(x, y)))  # BMP : ordre B, V, R
        donnees += ligne

    taille_fichier = 54 + len(donnees)
    entete_fichier = b"BM" + struct.pack("<IHHI", taille_fichier, 0, 0, 54)
    entete_info = struct.pack(
        "<IiiHHIIiiII",
        40, LARGEUR, HAUTEUR, 1, 24, 0, len(donnees),
        2835, 2835, 0, 0,
    )

    sortie = Path(__file__).parent / "formes01.bmp"
    sortie.write_bytes(entete_fichier + entete_info + donnees)
    print(f"{sortie} : {LARGEUR} x {HAUTEUR} px, "
          f"carré 20x20 + rectangle 12x8 (496 px noirs)")


if __name__ == "__main__":
    main()
