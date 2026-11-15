/*
  LED des 10 doigts - MP2I
  Réception des trames envoyées par hand_tracker_angle.py sur le port série :
    R1=1,R2=0,...,L5=0
  Chaque doigt levé allume la LED correspondante, un poing fermé les éteint.

  Câblage (voir diagram.json, schéma Wokwi) :
    R1 -> broche 2   R2 -> broche 3   R3 -> broche 4   R4 -> broche 5   R5 -> broche 6
    L1 -> broche 7   L2 -> broche 8   L3 -> broche 9   L4 -> broche 10  L5 -> broche 11
  Chaque broche commande une LED à travers une résistance de 220 ohms,
  les cathodes des LED rejoignent la masse (GND).
*/

// Nombre de LED : une par doigt.
const int NOMBRE_LED = 10;

// Broche de chaque LED, dans l'ordre du protocole série.
const int BROCHES_LED[NOMBRE_LED] = {
  2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

// Identifiants des doigts, dans le même ordre que les broches.
const char *IDS_DOIGTS[NOMBRE_LED] = {
  "R1", "R2", "R3", "R4", "R5",
  "L1", "L2", "L3", "L4", "L5"
};

// Sécurité : sans nouvelle trame pendant ce délai (en millisecondes),
// toutes les LED s'éteignent. Python envoie une trame toutes les 100 ms
// tant qu'au moins un doigt est levé, donc 1 s sans trame signifie que
// la main est fermée ou que la liaison est perdue.
const unsigned long DELAI_EXTINCTION = 1000;

// Trame en cours de lecture (une trame complète fait 59 caractères).
char tampon[80];
int longueurTampon = 0;

// Date de la dernière trame reçue.
unsigned long derniereTrame = 0;

void eteindreTout() {
  for (int i = 0; i < NOMBRE_LED; i++) {
    digitalWrite(BROCHES_LED[i], LOW);
  }
}

// Renvoie la position d'un identifiant de doigt dans les tableaux,
// ou -1 si l'identifiant n'est pas reconnu.
int chercherDoigt(const char *id) {
  for (int i = 0; i < NOMBRE_LED; i++) {
    if (strncmp(id, IDS_DOIGTS[i], 2) == 0) {
      return i;
    }
  }
  return -1;
}

// Découpe la trame en blocs séparés par des virgules ("R1=1", "R2=0", ...)
// et allume ou éteint la LED correspondante.
void appliquerTrame(const char *trame) {
  int i = 0;

  while (trame[i] != '\0') {
    // Début du bloc courant.
    int debut = i;
    while (trame[i] != '\0' && trame[i] != ',') {
      i++;
    }

    // Un bloc valide a la forme "R1=1" : identifiant de 2 caractères,
    // signe égal, valeur 0 ou 1.
    if (i - debut == 4 && trame[debut + 2] == '=') {
      char id[3] = { trame[debut], trame[debut + 1], '\0' };
      int numero = chercherDoigt(id);
      if (numero >= 0) {
        int etat = (trame[debut + 3] == '1') ? HIGH : LOW;
        digitalWrite(BROCHES_LED[numero], etat);
      }
    }

    // On saute la virgule pour passer au bloc suivant.
    if (trame[i] == ',') {
      i++;
    }
  }
}

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < NOMBRE_LED; i++) {
    pinMode(BROCHES_LED[i], OUTPUT);
  }
  eteindreTout();

  // Test au démarrage : chaque LED s'allume 100 ms, une par une.
  // Cela vérifie le câblage sans ordinateur ni programme Python.
  for (int i = 0; i < NOMBRE_LED; i++) {
    digitalWrite(BROCHES_LED[i], HIGH);
    delay(100);
    digitalWrite(BROCHES_LED[i], LOW);
  }
}

void loop() {
  // Lecture des caractères reçus jusqu'au saut de ligne.
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n') {
      tampon[longueurTampon] = '\0';
      appliquerTrame(tampon);
      longueurTampon = 0;
      derniereTrame = millis();
    } else if (c == '\r') {
      // Ignoré : certains logiciels envoient \r avant \n.
    } else if (longueurTampon < (int)(sizeof(tampon) - 1)) {
      tampon[longueurTampon] = c;
      longueurTampon++;
    } else {
      // Trame trop longue : on la jette pour rester synchronisé.
      longueurTampon = 0;
    }
  }

  // Sécurité : si Python ne parle plus (programme fermé, câble débranché,
  // ou simplement plus aucun doigt levé), les LED s'éteignent.
  if (millis() - derniereTrame > DELAI_EXTINCTION) {
    eteindreTout();
  }
}
