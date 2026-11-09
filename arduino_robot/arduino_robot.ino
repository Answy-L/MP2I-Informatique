/*
 * arduino_robot.ino
 * =================
 * Robot 2WD : reçoit les commandes de hand_robot.py par le port série USB
 * et pilote les deux moteurs DC via un driver L298N (pont en H double).
 *
 * Protocole : une lettre par ligne, 9600 bauds.
 *   'F' avancer | 'B' reculer | 'L' tourner à gauche | 'R' tourner à droite
 *   'S' arrêt
 * Les fins de ligne (\n, \r) sont ignorées, les caractères inconnus aussi.
 *
 * Sécurité indépendante de Python (heartbeat) :
 *   Python renvoie la commande en cours toutes les 200 ms même si le geste
 *   ne change pas. Si l'Arduino ne reçoit AUCUN message pendant plus de
 *   TIMEOUT_MS (500 ms) — câble débranché, programme fermé, plantage —
 *   elle arrête les moteurs d'elle-même.
 *
 * La boucle loop() n'utilise aucun delay() bloquant : la surveillance du
 * timeout et la lecture série sont donc toujours réactives.
 */

// ------------------------- BROCHAGE (cf. README) ---------------------------
// Driver L298N       Broche Arduino
//   ENA (PWM)     ->  D5      vitesse moteur GAUCHE (cavalier ENA retiré)
//   IN1           ->  D7
//   IN2           ->  D8
//   IN3           ->  D9
//   IN4           ->  D10
//   ENB (PWM)     ->  D6      vitesse moteur DROIT (cavalier ENB retiré)
//   GND           ->  GND     masse commune Arduino + alimentation moteurs
// Moteur gauche sur OUT1/OUT2, moteur droit sur OUT3/OUT4 du L298N.

const byte PIN_ENA = 5;
const byte PIN_IN1 = 7;
const byte PIN_IN2 = 8;
const byte PIN_IN3 = 9;
const byte PIN_IN4 = 10;
const byte PIN_ENB = 6;

// ------------------------- PARAMÈTRES À ADAPTER ----------------------------
const byte VITESSE = 150;                 // PWM 0..255 : commencer bas (100-150)
const unsigned long TIMEOUT_MS = 500;     // Délai sans message avant arrêt

unsigned long derniereReceptionMs = 0;    // heure du dernier message reçu
char commandeCourante = 'S';              // commande appliquée actuellement

// --------------------------- MOUVEMENTS -----------------------------------
// ATTENTION : les sens « avant » et « gauche » dépendent du montage physique.
// Si une roue tourne à l'envers, inverser les DEUX fils du moteur concerné
// sur OUT1/OUT2 ou OUT3/OUT4 (ou échanger IN1/IN2 dans le code).

void forward() {          // les deux roues en avant
  digitalWrite(PIN_IN1, HIGH); digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, HIGH); digitalWrite(PIN_IN4, LOW);
}

void backward() {         // les deux roues en arrière
  digitalWrite(PIN_IN1, LOW);  digitalWrite(PIN_IN2, HIGH);
  digitalWrite(PIN_IN3, LOW);  digitalWrite(PIN_IN4, HIGH);
}

void turnLeft() {         // pivot à gauche : roue gauche arrière, droite avant
  digitalWrite(PIN_IN1, LOW);  digitalWrite(PIN_IN2, HIGH);
  digitalWrite(PIN_IN3, HIGH); digitalWrite(PIN_IN4, LOW);
}

void turnRight() {        // pivot à droite : roue gauche avant, droite arrière
  digitalWrite(PIN_IN1, HIGH); digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, LOW);  digitalWrite(PIN_IN4, HIGH);
}

void stopRobot() {        // roues libres, moteurs coupés
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, LOW);
  digitalWrite(PIN_IN4, LOW);
}

// Applique une commande. Renvoie false si le caractère est inconnu.
bool appliquer(char c) {
  switch (c) {
    case 'F': forward();  break;
    case 'B': backward(); break;
    case 'L': turnLeft(); break;
    case 'R': turnRight(); break;
    case 'S': stopRobot(); break;
    default: return false;   // caractère inconnu : ignoré silencieusement
  }
  return true;
}

void setup() {
  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);

  analogWrite(PIN_ENA, VITESSE);   // vitesse par PWM (cavaliers ENA/ENB retirés)
  analogWrite(PIN_ENB, VITESSE);

  stopRobot();                     // état de départ : robot ARRÊTÉ
  commandeCourante = 'S';

  Serial.begin(9600);
  derniereReceptionMs = millis();
}

void loop() {
  // 1) Lecture de tous les caractères disponibles (non bloquant)
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') continue;   // fins de ligne ignorées
    if (appliquer(c)) {
      commandeCourante = c;
      derniereReceptionMs = millis();       // alimente le watchdog
    }
  }

  // 2) Sécurité : aucun message depuis plus de TIMEOUT_MS -> arrêt
  //    (la soustraction gère correctement le débordement de millis())
  if (commandeCourante != 'S'
      && millis() - derniereReceptionMs > TIMEOUT_MS) {
    stopRobot();
    commandeCourante = 'S';
  }
}
