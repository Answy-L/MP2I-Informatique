# -*- coding: utf-8 -*-
"""
Hand tracker - MP2I
Reconnaissance indépendante des 10 doigts avec MediaPipe.

MODE = "demo"     : LED virtuelles à l'écran, aucun Arduino nécessaire.
MODE = "arduino"  : envoi série vers un Arduino qui pilote 10 LED.

IDs :
  Main droite : R1 pouce, R2 index, R3 majeur, R4 annulaire, R5 auriculaire
  Main gauche : L1 pouce, L2 index, L3 majeur, L4 annulaire, L5 auriculaire

Installation (Python 3.12 conseillé) :
  py -3.12 -m pip install opencv-python mediapipe pyserial

Lancement :
  py -3.12 hand_tracker.py
"""

import time
from collections import defaultdict, deque

import cv2
import mediapipe as mp

# ---------------------------- CONFIGURATION ----------------------------

MODE = "demo"          # Mettre "arduino" pour utiliser l'Arduino
CAMERA_INDEX = 0       # Essayer 1 si la webcam n'est pas détectée
SERIAL_PORT = "COM3"   # À adapter dans le Gestionnaire de périphériques
BAUD_RATE = 9600

MAX_HANDS = 2
SMOOTHING_FRAMES = 5
SMOOTHING_THRESHOLD = 3
SERIAL_INTERVAL = 0.10  # Envoi périodique si au moins un doigt est levé

# Le traitement suppose une image miroir (comme un selfie).
# On retourne l'image AVANT MediaPipe pour obtenir les côtés corrects.
MIRROR_IMAGE = True

FINGER_NAMES = ["Pouce", "Index", "Majeur", "Annulaire", "Auriculaire"]
FINGER_IDS = {
    "Right": ["R1", "R2", "R3", "R4", "R5"],
    "Left":  ["L1", "L2", "L3", "L4", "L5"],
}

# Repères MediaPipe
WRIST = 0

# Pour chaque doigt : (MCP, PIP, DIP, TIP)
FINGER_LANDMARKS = [
    (5, 6, 7, 8),       # Index
    (9, 10, 11, 12),    # Majeur
    (13, 14, 15, 16),   # Annulaire
    (17, 18, 19, 20),   # Auriculaire
]

# ---------------------------- RECONNAISSANCE ---------------------------

def angle_degrees(a, b, c):
    """Calcule l'angle ABC en degrés avec les coordonnées 3D de MediaPipe."""
    import math

    ba = (a.x - b.x, a.y - b.y, a.z - b.z)
    bc = (c.x - b.x, c.y - b.y, c.z - b.z)
    dot = sum(x * y for x, y in zip(ba, bc))
    norm_ba = math.sqrt(sum(x * x for x in ba))
    norm_bc = math.sqrt(sum(x * x for x in bc))

    if norm_ba == 0 or norm_bc == 0:
        return 0.0

    cosine = dot / (norm_ba * norm_bc)
    cosine = max(-1.0, min(1.0, cosine))
    return math.degrees(math.acos(cosine))


def finger_is_extended(landmarks, mcp, pip, dip, tip):
    """Un doigt est tendu si ses articulations PIP et DIP sont assez ouvertes."""
    pip_angle = angle_degrees(landmarks[mcp], landmarks[pip], landmarks[dip])
    dip_angle = angle_degrees(landmarks[pip], landmarks[dip], landmarks[tip])
    return pip_angle > 155 and dip_angle > 145


def thumb_is_extended(landmarks):
    """Détection du pouce par son angle IP et son écart avec la base de l'index."""
    thumb_angle = angle_degrees(landmarks[2], landmarks[3], landmarks[4])

    tip = landmarks[4]
    index_mcp = landmarks[5]
    thumb_mcp = landmarks[2]

    tip_to_index = (
        (tip.x - index_mcp.x) ** 2
        + (tip.y - index_mcp.y) ** 2
        + (tip.z - index_mcp.z) ** 2
    ) ** 0.5

    base_to_index = (
        (thumb_mcp.x - index_mcp.x) ** 2
        + (thumb_mcp.y - index_mcp.y) ** 2
        + (thumb_mcp.z - index_mcp.z) ** 2
    ) ** 0.5

    return thumb_angle > 145 and tip_to_index > 1.15 * base_to_index


def read_fingers(landmarks):
    """Renvoie 5 booléens : pouce, index, majeur, annulaire, auriculaire."""
    values = [thumb_is_extended(landmarks)]
    values.extend(
        finger_is_extended(landmarks, mcp, pip, dip, tip)
        for mcp, pip, dip, tip in FINGER_LANDMARKS
    )
    return values


# ------------------------- STABILISATION -------------------------------

class FingerSmoother:
    """Vote majoritaire sur les dernières images pour limiter le clignotement."""

    def __init__(self):
        self.history = defaultdict(lambda: deque(maxlen=SMOOTHING_FRAMES))

    def update(self, hand_side, values):
        result = []
        for finger_id, value in zip(FINGER_IDS[hand_side], values):
            history = self.history[finger_id]
            history.append(bool(value))
            result.append(
                sum(history) >= SMOOTHING_THRESHOLD
            )
        return result

    def clear_side(self, hand_side):
        for finger_id in FINGER_IDS[hand_side]:
            self.history[finger_id].clear()


# ------------------------------ AFFICHAGE ------------------------------

def draw_led_panel(frame, states, statuses):
    """Dessine les dix LED virtuelles et leurs états dans la fenêtre caméra."""
    height, width = frame.shape[:2]
    panel_width = 300
    x0 = max(10, width - panel_width - 10)

    overlay = frame.copy()
    cv2.rectangle(overlay, (x0, 10), (width - 10, min(height - 10, 330)),
                  (25, 25, 25), -1)
    cv2.addWeighted(overlay, 0.78, frame, 0.22, 0, frame)

    cv2.putText(frame, "ETAT DES DOIGTS", (x0 + 15, 38),
                cv2.FONT_HERSHEY_SIMPLEX, 0.65, (255, 255, 255), 2)

    y = 68
    for side, side_name in (("Right", "DROITE (R)"), ("Left", "GAUCHE (L)")):
        cv2.putText(frame, side_name, (x0 + 15, y),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.52, (220, 220, 220), 1)
        y += 24

        for index, finger_name in enumerate(FINGER_NAMES):
            finger_id = FINGER_IDS[side][index]
            is_on = states.get(finger_id, False)
            color = (40, 220, 80) if is_on else (70, 70, 70)
            cv2.circle(frame, (x0 + 25, y - 5), 8, color, -1)
            label = f"{finger_id}  {finger_name:<12}  {'TRUE' if is_on else 'FALSE'}"
            cv2.putText(frame, label, (x0 + 43, y),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.43,
                        (255, 255, 255), 1)
            y += 22
        y += 10

    status_text = " | ".join(statuses) if statuses else "AUCUNE MAIN"
    cv2.putText(frame, status_text[:65], (10, height - 18),
                cv2.FONT_HERSHEY_SIMPLEX, 0.48, (0, 255, 255), 1)


def print_states(states):
    """Affiche les états uniquement lorsqu'ils changent."""
    active = [key for key, value in states.items() if value]
    print("Doigts levés :", ", ".join(active) if active else "aucun")


# -------------------------- COMMUNICATION ------------------------------

def encode_serial(states):
    """
    Format envoyé à l'Arduino :
    R1=1,R2=0,...,L5=0\\n
    Les 10 IDs sont toujours présents dans une trame active.
    """
    ordered_ids = (
        FINGER_IDS["Right"] + FINGER_IDS["Left"]
    )
    return ",".join(
        f"{finger_id}={1 if states.get(finger_id, False) else 0}"
        for finger_id in ordered_ids
    ) + "\n"


def main():
    if MODE not in ("demo", "arduino"):
        raise ValueError('MODE doit être "demo" ou "arduino".')

    serial_connection = None
    if MODE == "arduino":
        try:
            import serial
            serial_connection = serial.Serial(
                SERIAL_PORT, BAUD_RATE, timeout=0.1
            )
            time.sleep(2)  # L'Arduino redémarre souvent à l'ouverture du port.
            print(f"[Arduino] Connecté sur {SERIAL_PORT} à {BAUD_RATE} bauds.")
        except Exception as error:
            print(f"[Arduino] Connexion impossible : {error}")
            print("Vérifie le port COM et ferme le Moniteur série Arduino.")
            return

    camera = cv2.VideoCapture(CAMERA_INDEX, cv2.CAP_DSHOW)
    if not camera.isOpened():
        camera.release()
        camera = cv2.VideoCapture(CAMERA_INDEX)

    if not camera.isOpened():
        print("[Caméra] Impossible d'ouvrir la webcam.")
        if serial_connection:
            serial_connection.close()
        return

    mp_hands = mp.solutions.hands
    drawing = mp.solutions.drawing_utils
    smoother = FingerSmoother()

    previous_states = None
    last_serial_send = 0.0

    try:
        with mp_hands.Hands(
            static_image_mode=False,
            max_num_hands=MAX_HANDS,
            model_complexity=1,
            min_detection_confidence=0.6,
            min_tracking_confidence=0.6,
        ) as hands:

            while True:
                success, frame = camera.read()
                if not success:
                    print("[Caméra] Impossible de lire une image.")
                    break

                if MIRROR_IMAGE:
                    frame = cv2.flip(frame, 1)

                rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                results = hands.process(rgb)

                # État par défaut : toutes les LED sont éteintes.
                states = {
                    finger_id: False
                    for side in ("Right", "Left")
                    for finger_id in FINGER_IDS[side]
                }
                statuses = []
                detected_sides = set()

                if results.multi_hand_landmarks and results.multi_handedness:
                    for hand_landmarks, handedness in zip(
                        results.multi_hand_landmarks,
                        results.multi_handedness
                    ):
                        side = handedness.classification[0].label
                        if side not in FINGER_IDS:
                            continue

                        detected_sides.add(side)
                        drawing.draw_landmarks(
                            frame, hand_landmarks, mp_hands.HAND_CONNECTIONS
                        )

                        raw_values = read_fingers(hand_landmarks.landmark)
                        values = smoother.update(side, raw_values)

                        # Un poing fermé est un état neutre : pas de commande
                        # de doigt. Les indicateurs restent éteints.
                        if not any(values):
                            statuses.append(
                                f"{side}: poing ferme - aucun signal"
                            )
                            continue

                        statuses.append(f"{side}: main active")
                        for finger_id, value in zip(
                            FINGER_IDS[side], values
                        ):
                            states[finger_id] = bool(value)

                        # Affiche les IDs près de la main.
                        wrist = hand_landmarks.landmark[WRIST]
                        h, w = frame.shape[:2]
                        px, py = int(wrist.x * w), int(wrist.y * h)
                        cv2.putText(
                            frame, side, (px - 20, py + 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7,
                            (0, 255, 255), 2
                        )

                # Efface l'historique d'une main qui n'est plus visible.
                for side in ("Right", "Left"):
                    if side not in detected_sides:
                        smoother.clear_side(side)

                if states != previous_states:
                    print_states(states)
                    previous_states = states.copy()

                # Démo : le tableau des LED virtuelles suit les booléens.
                # Arduino : aucune trame de doigt si les deux mains sont
                # absentes ou si tous les doigts sont baissés.
                any_finger_active = any(states.values())
                if MODE == "arduino" and any_finger_active:
                    now = time.time()
                    if now - last_serial_send >= SERIAL_INTERVAL:
                        serial_connection.write(
                            encode_serial(states).encode("ascii")
                        )
                        last_serial_send = now

                draw_led_panel(frame, states, statuses)

                mode_label = (
                    "MODE DEMO - LED VIRTUELLES"
                    if MODE == "demo"
                    else f"MODE ARDUINO - {SERIAL_PORT}"
                )
                cv2.putText(frame, mode_label, (10, 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.65,
                            (0, 255, 0), 2)
                cv2.putText(frame, "Q ou ECHAP : quitter", (10, 58),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.48,
                            (255, 255, 255), 1)

                cv2.imshow("Reconnaissance des 10 doigts", frame)
                key = cv2.waitKey(1) & 0xFF
                if key == ord("q") or key == 27:
                    break

    finally:
        camera.release()
        cv2.destroyAllWindows()

        if serial_connection:
            # On envoie une trame de remise à zéro à la fermeture.
            serial_connection.write(
                encode_serial({}).encode("ascii")
            )
            serial_connection.close()

        print("Programme terminé.")


if __name__ == "__main__":
    main()
