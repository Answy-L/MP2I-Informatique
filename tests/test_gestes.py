# -*- coding: utf-8 -*-
"""
test_gestes.py
==============
Tests automatiques de la logique de reconnaissance de hand_robot.py,
SANS webcam, sans Arduino et sans installer opencv / mediapipe / pyserial :
les modules externes sont simulés et les mains sont des listes de
coordonnées synthétiques (repères MediaPipe fictifs mais réalistes).

Vérifie :
  1. la classification des cinq gestes, des gestes inconnus et des
     gestes ambigus (qui doivent donner l'arrêt par sécurité) ;
  2. l'insensibilité à l'orientation : la même main tournée de -45, +45
     et 90 degrés est reconnue de la même façon ;
  3. le filtrage temporel (validation après STABLE_FRAMES images) et la
     conversion geste inconnu / main absente en commande d'arrêt.

Lancement :
    py -3.12 tests/test_gestes.py
Le script affiche un bilan et renvoie un code de retour non nul en cas
d'échec.
"""

import math
import os
import sys
import types


def installer_simulations():
    """Crée de faux modules cv2 / mediapipe / serial afin de pouvoir
    importer hand_robot sans aucune bibliothèque installée."""
    cv2_m = types.ModuleType("cv2")
    mp_m = types.ModuleType("mediapipe")
    solutions_m = types.ModuleType("mediapipe.solutions")
    mains_m = types.ModuleType("mediapipe.solutions.hands")
    dessin_m = types.ModuleType("mediapipe.solutions.drawing_utils")
    mains_m.Hands = lambda **kwargs: None
    mains_m.HAND_CONNECTIONS = None
    mp_m.solutions = solutions_m
    solutions_m.hands = mains_m
    solutions_m.drawing_utils = dessin_m
    serial_m = types.ModuleType("serial")
    serial_m.SerialException = type("SerialException", (Exception,), {})
    serial_m.Serial = object
    for nom, module in (("cv2", cv2_m),
                        ("mediapipe", mp_m),
                        ("mediapipe.solutions", solutions_m),
                        ("mediapipe.solutions.hands", mains_m),
                        ("mediapipe.solutions.drawing_utils", dessin_m),
                        ("serial", serial_m)):
        sys.modules[nom] = module


CHEMIN_DEPOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, CHEMIN_DEPOT)
installer_simulations()
import hand_robot  # noqa: E402  (import après les simulations, volontairement)

ECHECS = []


def check(nom, condition):
    if condition:
        print("[OK    ] " + nom)
    else:
        print("[ECHEC ] " + nom)
        ECHECS.append(nom)


# --- Construction de mains synthétiques ------------------------------------
# Repères utilisés : 0 = poignet, 17 = base de l'auriculaire,
# (PIP, pointe) pour index, majeur, annulaire, auriculaire.
def main_typee(index, majeur, annulaire, auriculaire, pouce):
    """Main paume face caméra : pointes loin du poignet si le doigt est
    tendu, repliées vers la paume s'il est plié."""
    points = [None] * 21
    points[0] = (300, 400)     # poignet
    points[17] = (320, 335)    # base de l'auriculaire
    pips = {8: (290, 320), 12: (300, 315), 16: (310, 320), 20: (322, 330)}
    joints = {8: 6, 12: 10, 16: 14, 20: 18}   # pointe -> PIP correspondant
    tendus = {8: index, 12: majeur, 16: annulaire, 20: auriculaire}
    for pointe, pip in pips.items():
        points[joints[pointe]] = pip        # on place aussi l'articulation PIP
        if tendus[pointe]:
            points[pointe] = (pip[0], pip[1] - 70)
        else:
            points[pointe] = (pip[0] + 10, pip[1] + 30)
    points[3] = (265, 370)     # articulation IP du pouce
    points[4] = (225, 350) if pouce else (305, 350)   # pointe du pouce
    return points


def tourner(points, degres):
    """Fait tourner tous les repères autour du poignet (simule une main
    penchée ou tournée par rapport à la caméra)."""
    angle = math.radians(degres)
    cos_a, sin_a = math.cos(angle), math.sin(angle)
    cx, cy = points[0]
    resultat = []
    for p in points:
        if p is None:
            resultat.append(None)
            continue
        x, y = p
        resultat.append((int(cos_a * (x - cx) - sin_a * (y - cy) + cx),
                         int(sin_a * (x - cx) + cos_a * (y - cy) + cy)))
    return resultat


# --- Tests ------------------------------------------------------------------
def test_classification():
    cas = [
        ("main ouverte",            main_typee(1, 1, 1, 1, 1), ("Main ouverte", "F")),
        ("poing fermé",             main_typee(0, 0, 0, 0, 0), ("Poing fermé", "S")),
        ("signe V, pouce rentré",   main_typee(1, 1, 0, 0, 0), ("Signe V", "R")),
        ("signe V, pouce dehors",   main_typee(1, 1, 0, 0, 1), ("Signe V", "R")),
        ("index seul",              main_typee(1, 0, 0, 0, 0), ("Index seul", "L")),
        ("pouce levé",              main_typee(0, 0, 0, 0, 1), ("Pouce levé", "B")),
        ("geste inconnu (3 doigts)", main_typee(1, 1, 1, 0, 0), (None, None)),
        ("geste ambigu (index+pouce)", main_typee(1, 0, 0, 0, 1), (None, None)),
    ]
    for nom, points, attendu in cas:
        check("classification : " + nom,
              hand_robot.classify_gesture(points) == attendu)


def test_rotation():
    cas = [
        ("main ouverte", main_typee(1, 1, 1, 1, 1), ("Main ouverte", "F")),
        ("poing fermé",  main_typee(0, 0, 0, 0, 0), ("Poing fermé", "S")),
        ("signe V",      main_typee(1, 1, 0, 0, 0), ("Signe V", "R")),
        ("index seul",   main_typee(1, 0, 0, 0, 0), ("Index seul", "L")),
        ("pouce levé",   main_typee(0, 0, 0, 0, 1), ("Pouce levé", "B")),
    ]
    for nom, points, attendu in cas:
        for angle in (-45, 45, 90):
            check("rotation " + str(angle) + " degres : " + nom,
                  hand_robot.classify_gesture(tourner(points, angle)) == attendu)


def test_filtre():
    filtre = hand_robot.GestureFilter(stable_frames=5)
    changements = [filtre.update("F") for _ in range(5)]
    check("filtrage : rien avant 5 images, validation a la 5e",
          changements == [False, False, False, False, True])
    check("filtrage : pas de nouveau changement si le geste ne change pas",
          filtre.update("F") is False)
    for _ in range(5):
        filtre.update("S")
    check("filtrage : changement valide apres 5 nouvelles images",
          filtre.stable == "S")
    filtre.reset()
    check("filtrage : reinitialisation (apres reconnexion)",
          filtre.stable is None)

    filtre2 = hand_robot.GestureFilter(stable_frames=3)
    for _ in range(3):
        filtre2.update(None)
    commande = "S" if filtre2.stable is None else filtre2.stable
    check("main absente / geste inconnu -> commande S", commande == "S")


def main():
    test_classification()
    test_rotation()
    test_filtre()
    if ECHECS:
        print()
        print(str(len(ECHECS)) + " test(s) en echec : " + ", ".join(ECHECS))
        sys.exit(1)
    print()
    print("Tous les tests passent.")


if __name__ == "__main__":
    main()
