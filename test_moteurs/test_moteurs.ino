/*
 * test_moteurs.ino
 * ================
 * Mode de test : vérifier les moteurs SANS webcam ni MediaPipe.
 * À téléverser à la place d'arduino_robot.ino, moteurs débranchés au premier
 * passage, puis roues levées du sol.
 *
 * Enchaîne toutes les 2 secondes :
 *   Arrêt -> Avancer -> Arrêt -> Reculer -> Arrêt -> Gauche -> Arrêt -> Droite
 * puis recommence. Le moniteur série (9600 bauds) annonce chaque étape.
 *
 * Vérifications attendues :
 *   - avancer : les deux roues tournent vers l'avant du robot ;
 *   - reculer : les deux roues tournent dans l'autre sens ;
 *   - gauche/droite : les roues tournent en sens opposés (pivot).
 * Si une roue tourne à l'envers : inverser les deux fils de CE moteur
 * sur le L298N (OUT1/OUT2 pour la gauche, OUT3/OUT4 pour la droite),
 * ou échanger IN1/IN2 (resp. IN3/IN4) dans le code.
 */

// Brochage identique à arduino_robot.ino
const byte PIN_ENA = 5;
const byte PIN_IN1 = 7;
const byte PIN_IN2 = 8;
const byte PIN_IN3 = 9;
const byte PIN_IN4 = 10;
const byte PIN_ENB = 6;

const byte VITESSE = 120;               // PWM bas pour les essais
const unsigned long DUREE_MS = 2000;    // durée de chaque étape

const char SEQUENCE[] = {'S', 'F', 'S', 'B', 'S', 'L', 'S', 'R'};
const char* NOMS[] = {"Arret", "Avancer", "Arret", "Reculer",
                      "Arret", "Gauche", "Arret", "Droite"};
const int NB_ETAPES = 8;

unsigned int etape = 0;
unsigned long derniereEtapeMs = 0;

void appliquer(char c) {
  switch (c) {
    case 'F':  // avant
      digitalWrite(PIN_IN1, HIGH); digitalWrite(PIN_IN2, LOW);
      digitalWrite(PIN_IN3, HIGH); digitalWrite(PIN_IN4, LOW);
      break;
    case 'B':  // arrière
      digitalWrite(PIN_IN1, LOW);  digitalWrite(PIN_IN2, HIGH);
      digitalWrite(PIN_IN3, LOW);  digitalWrite(PIN_IN4, HIGH);
      break;
    case 'L':  // pivot gauche : roue gauche arrière, roue droite avant
      digitalWrite(PIN_IN1, LOW);  digitalWrite(PIN_IN2, HIGH);
      digitalWrite(PIN_IN3, HIGH); digitalWrite(PIN_IN4, LOW);
      break;
    case 'R':  // pivot droite : roue gauche avant, roue droite arrière
      digitalWrite(PIN_IN1, HIGH); digitalWrite(PIN_IN2, LOW);
      digitalWrite(PIN_IN3, LOW);  digitalWrite(PIN_IN4, HIGH);
      break;
    default:   // 'S' et tout le reste : arrêt
      digitalWrite(PIN_IN1, LOW);
      digitalWrite(PIN_IN2, LOW);
      digitalWrite(PIN_IN3, LOW);
      digitalWrite(PIN_IN4, LOW);
      break;
  }
}

void setup() {
  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);

  analogWrite(PIN_ENA, VITESSE);
  analogWrite(PIN_ENB, VITESSE);

  appliquer('S');                 // état de départ : arrêté
  Serial.begin(9600);
  Serial.println("Test moteurs : sequence Arret/F/B/L/R toutes les 2 s.");
  Serial.println("Roues LEVEES du sol pendant le test !");
  derniereEtapeMs = millis();
}

void loop() {
  // Attente non bloquante entre deux étapes
  if (millis() - derniereEtapeMs >= DUREE_MS) {
    derniereEtapeMs = millis();
    etape = (etape + 1) % NB_ETAPES;
    appliquer(SEQUENCE[etape]);
    Serial.print("Etape ");
    Serial.print(etape + 1);
    Serial.print(" / ");
    Serial.print(NB_ETAPES);
    Serial.print(" : ");
    Serial.println(NOMS[etape]);
  }
}
