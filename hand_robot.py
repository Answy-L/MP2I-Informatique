# -*- coding: utf-8 -*-
"""
hand_robot.py
=============
Robot 2WD commandé par gestes de la main devant la webcam.

Chaîne de traitement :
    webcam -> OpenCV -> MediaPipe Hands (21 repères) -> reconnaissance du geste
    -> filtrage temporel -> liaison série USB -> Arduino -> driver L298N -> moteurs.

Commandes série (une lettre + saut de ligne, 9600 bauds) :
    'F' avancer | 'B' reculer | 'L' tourner à gauche | 'R' tourner à droite | 'S' arrêt

Gestes reconnus (une seule main, paume face à la caméra) :
    Main ouverte (5 doigts)  -> F   (avancer)
    Poing fermé              -> S   (arrêter)
    Signe V (index + majeur) -> R   (tourner à droite)
    Index seul               -> L   (tourner à gauche)
    Pouce levé               -> B   (reculer)
    Geste inconnu / pas de main -> S (arrêt par sécurité)

Lancement :
    py -3.12 hand_robot.py            (le port COM est défini plus bas)
    py -3.12 hand_robot.py COM4       (port passé en argument)

Quitter avec la touche 'q' ou Échap : arrêt du robot et fermeture propre.

Prérequis :
    py -3.12 -m pip install opencv-python mediapipe pyserial
"""

import sys
import time

import cv2
import mediapipe as mp
import serial

# =========================== PARAMÈTRES À ADAPTER ===========================

SERIAL_PORT = "COM3"          # EXEMPLE : voir README « Trouver le port COM »
BAUD_RATE = 9600              # Doit être identique côté Arduino
CAM_INDEX = 0                 # 0 = webcam intégrée, 1 = première webcam USB
AUTO_RECONNECT_S = 2.0        # Nouvelle tentative d'ouverture du port toutes les 2 s

HEARTBEAT_INTERVAL_S = 0.20   # Renvoi de la commande en cours toutes les 200 ms
                               # même si le geste n'a pas changé (heartbeat)
NO_HAND_DELAY_S = 0.50        # Main absente depuis plus de 0,5 s -> arrêt
STABLE_FRAMES = 5             # Images consécutives identiques pour valider un geste

MIN_DETECTION_CONFIDENCE = 0.6   # Confiance minimale MediaPipe (0 à 1)
MIN_TRACKING_CONFIDENCE = 0.5

# Seuil du pouce : le pouce est « étendu » si sa pointe est nettement plus loin
# de la base de l'auriculaire que l'articulation IP du pouce.
# 1.2 = marge de sécurité pour distinguer « pouce levé » d'un poing fermé.
THUMB_EXTEND_FACTOR = 1.2

# ======================= INDICES DES REPÈRES MEDIAPIPE ======================
# Main = 21 repères. 0 = poignet.
# Pouce : 1 CMC, 2 MCP, 3 IP, 4 TIP
# Index : 5 MCP, 6 PIP, 7 DIP, 8 TIP
# Majeur : 9 MCP, 10 PIP, 11 DIP, 12 TIP
# Annulaire : 13 MCP, 14 PIP, 15 DIP, 16 TIP
# Auriculaire : 17 MCP, 18 PIP, 19 DIP, 20 TIP
WRIST = 0
THUMB_IP = 3
THUMB_TIP = 4
PINKY_MCP = 17
# (pip, tip) pour index, majeur, annulaire, auriculaire
FINGER_JOINTS = [(6, 8), (10, 12), (14, 16), (18, 20)]


def _dist(p, q):
    """Distance euclidienne entre deux points (x, y) en pixels."""
    return ((p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2) ** 0.5


def finger_extended(points, pip, tip):
    """Vrai si le doigt est étendu.

    Méthode géométrique insensible à l'orientation de la main dans le plan de
    l'image : un doigt étendu a sa pointe PLUS LOIN du poignet que son
    articulation PIP ; un doigt plié ramène sa pointe vers la paume, donc
    plus près du poignet. Un simple test « pointe au-dessus de l'articulation »
    (comparaison des y) échoue dès que la main est penchée ou retournée.
    """
    return _dist(points[tip], points[WRIST]) > _dist(points[pip], points[WRIST])


def thumb_extended(points):
    """Vrai si le pouce est étendu.

    Le pouce d'un poing fermé reste souvent droit au niveau de son
    articulation IP (c'est l'articulation MCP qui plie), donc un test d'angle
    ne suffit pas. On compare plutôt les distances à la base de l'auriculaire :
    pouce levé -> pointe nettement plus loin ; poing fermé -> pointe proche
    de la paume. C'est le point le plus fragile du système, voir README
    « Limites connues ».
    """
    return (_dist(points[THUMB_TIP], points[PINKY_MCP])
            > THUMB_EXTEND_FACTOR * _dist(points[THUMB_IP], points[PINKY_MCP]))


def classify_gesture(points):
    """Classe un geste à partir des 21 repères en pixels.

    Renvoie (nom, commande) ou (None, None) si le geste est inconnu.
    En cas de doute on renvoie None : le programme enverra alors 'S'
    (arrêt par sécurité) et ne JAMAIS un ordre de déplacement.
    """
    index, majeur, annulaire, auriculaire = (
        finger_extended(points, pip, tip) for (pip, tip) in FINGER_JOINTS
    )
    pouce = thumb_extended(points)

    if index and majeur and annulaire and auriculaire and pouce:
        return ("Main ouverte", "F")
    if not index and not majeur and not annulaire and not auriculaire and not pouce:
        return ("Poing fermé", "S")
    if index and majeur and not annulaire and not auriculaire:
        return ("Signe V", "R")
    if index and not majeur and not annulaire and not auriculaire and not pouce:
        return ("Index seul", "L")
    if pouce and not index and not majeur and not annulaire and not auriculaire:
        return ("Pouce levé", "B")
    return (None, None)


class GestureFilter:
    """Filtrage temporel : un geste n'est validé qu'après STABLE_FRAMES
    images consécutives identiques. Évite les ordres fugitifs dus à une
    détection instable (une image de transition, un repère mal placé...)."""

    def __init__(self, stable_frames):
        self.stable_frames = stable_frames
        self.candidate = None   # geste vu sur l'image courante (en cours de confirmation)
        self.count = 0          # images consécutives identiques au candidat
        self.stable = None     # dernier geste validé (None = arrêt)

    def update(self, gesture):
        """Enregistre le geste de l'image courante.
        Renvoie True si le geste validé vient de changer."""
        if gesture == self.candidate:
            self.count += 1
        else:
            self.candidate = gesture
            self.count = 1
        if self.count >= self.stable_frames and self.stable != self.candidate:
            self.stable = self.candidate
            return True
        return False

    def reset(self):
        """Après une reconnexion série : on repart à l'arrêt."""
        self.candidate = None
        self.count = 0
        self.stable = None


class SerialLink:
    """Liaison série vers l'Arduino : envoi sur changement + heartbeat +
    tentative de reconnexion si le câble est débranché."""

    def __init__(self, port, baud):
        self.port = port
        self.baud = baud
        self.ser = None
        self.last_send_time = 0.0

    def connect(self):
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=0, write_timeout=1)
            print(f"[Série] Port {self.port} ouvert à {self.baud} bauds.")
            return True
        except serial.SerialException as err:
            print(f"[Série] Impossible d'ouvrir le port {self.port} : {err}")
            print("        Vérifiez le port dans le Gestionnaire de périphériques")
            print("        > Ports (COM et LPT), et fermez le moniteur série de")
            print("        l'IDE Arduino s'il est ouvert.")
            print("        Le programme continue en MODE DÉMO (sans robot).")
            self.ser = None
            return False

    def connected(self):
        return self.ser is not None

    def send(self, cmd):
        """Envoie une commande ('F','B','L','R','S') suivie d'un saut de ligne.
        Renvoie True si l'envoi a réussi."""
        if self.ser is None:
            return False
        try:
            self.ser.write((cmd + "\n").encode("ascii"))
            self.last_send_time = time.time()
            return True
        except serial.SerialException:
            print(f"[Série] Liaison perdue sur {self.port} : mode démo.")
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None
            return False

    def close(self):
        """Fermeture propre : on tente d'envoyer un dernier arrêt 'S'."""
        if self.ser is not None:
            try:
                self.ser.write(b"S\n")
                print("[Série] Commande d'arrêt 'S' envoyée avant fermeture.")
            except Exception:
                pass
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT

    link = SerialLink(port, BAUD_RATE)
    link.connect()
    link.send("S")  # état de départ : robot arrêté

    # CAP_DSHOW accélère l'ouverture de la webcam sous Windows.
    cap = cv2.VideoCapture(CAM_INDEX, cv2.CAP_DSHOW)
    if not cap.isOpened():
        cap.release()
        print("[Caméra] Impossible d'ouvrir la webcam.")
        print("         Vérifiez qu'aucune autre application ne l'utilise,")
        print("         et essayez CAM_INDEX = 1 en haut du fichier.")
        link.close()
        return

    mp_hands = mp.solutions.hands
    mp_draw = mp.solutions.drawing_utils
    hands = mp_hands.Hands(
        max_num_hands=1,
        min_detection_confidence=MIN_DETECTION_CONFIDENCE,
        min_tracking_confidence=MIN_TRACKING_CONFIDENCE,
    )

    filtre = GestureFilter(STABLE_FRAMES)
    last_hand_seen = time.time()     # dernier instant où une main a été vue
    next_reconnect = 0.0             # instant de la prochaine tentative série
    nom_geste = "..."                # nom affiché à l'écran

    try:
        while True:
            ok, frame = cap.read()
            if not ok:
                print("[Caméra] Échec de lecture de l'image, arrêt du programme.")
                break

            frame = cv2.flip(frame, 1)  # effet miroir : plus naturel pour l'utilisateur
            hauteur, largeur = frame.shape[:2]
            rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            resultats = hands.process(rgb)

            main_detectee = resultats.multi_hand_landmarks is not None
            geste_image = None  # commande vue sur CETTE image (avant filtrage)

            if main_detectee:
                last_hand_seen = time.time()
                landmarks = resultats.multi_hand_landmarks[0].landmark
                points = [(int(p.x * largeur), int(p.y * hauteur)) for p in landmarks]

                # Dessin des 21 repères et de leurs connexions
                mp_draw.draw_landmarks(
                    frame, resultats.multi_hand_landmarks[0],
                    mp_hands.HAND_CONNECTIONS)

                nom_geste, geste_image = classify_gesture(points)

            # Alimentation du filtre temporel
            if main_detectee:
                change = filtre.update(geste_image)
            elif time.time() - last_hand_seen > NO_HAND_DELAY_S:
                # Main absente depuis trop longtemps : candidat « aucun geste »
                nom_geste = "aucune main"
                change = filtre.update(None)
            else:
                # Main vient de disparaître : on garde l'état pendant le délai
                change = False

            # Commande effective : geste validé, 'S' si inconnu ou pas de main
            commande = "S" if filtre.stable is None else filtre.stable

            if change:
                print(f"Geste validé : {nom_geste} -> commande '{commande}'")
                link.send(commande)
            elif link.connected() and time.time() - link.last_send_time >= HEARTBEAT_INTERVAL_S:
                # Heartbeat : même commande renvoyée pour tenir l'Arduino éveillé
                link.send(commande)

            # Reconnexion automatique si la liaison a été perdue
            if not link.connected() and time.time() >= next_reconnect:
                next_reconnect = time.time() + AUTO_RECONNECT_S
                if link.connect():
                    link.send("S")   # après reconnexion : robot arrêté
                    filtre.reset()   # jusqu'au prochain geste validé

            # Affichage sur l'image
            etat_serie = f"Serie: {port} OK" if link.connected() else "Serie: mode demo"
            cv2.putText(frame, f"Geste: {nom_geste}", (10, 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
            cv2.putText(frame, f"Commande: {commande}", (10, 65),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 255), 2)
            cv2.putText(frame, etat_serie, (10, 100),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 1)
            cv2.putText(frame, "'q' ou Echap pour quitter", (10, hauteur - 15),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 1)
            cv2.imshow("Robot a gestes - hand_robot", frame)

            cle = cv2.waitKey(1) & 0xFF
            if cle == ord("q") or cle == 27:  # 'q' ou Échap
                break

    finally:
        # Fermeture propre, même en cas d'erreur ou d'interruption
        link.close()               # envoie 'S' puis ferme le port
        cap.release()
        hands.close()
        cv2.destroyAllWindows()
        print("Fermeture propre effectuée. Le robot doit être à l'arrêt.")


if __name__ == "__main__":
    main()
