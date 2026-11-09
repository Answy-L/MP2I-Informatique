# Cahier des charges : Robot contrôlé par reconnaissance des gestes de la main

## 1. Objectif du projet

Projet d'informatique en Python : contrôler une petite voiture robotisée à deux roues motrices en faisant des gestes avec la main devant la webcam de l'ordinateur.

La solution doit être complète, cohérente, testable et expliquée : tous les fichiers nécessaires, leur contenu intégral, les branchements, les étapes d'installation et une procédure de test.

**Contexte :**
- Ordinateur : Windows.
- Python : Python 3.12.10 est déjà installé.
- Caméra : webcam intégrée ou USB connectée à l'ordinateur.
- Carte : Arduino Uno R3 ou carte Uno compatible.
- Budget maximal visé : environ 50 €.
- Langage principal : Python.
- L'Arduino ne fait pas la reconnaissance d'image : Python analyse la webcam et envoie une commande série à l'Arduino.
- Niveau attendu : code complet, avec des explications suffisamment détaillées pour comprendre et présenter le projet.

## 2. Fonctionnement général attendu

Le système doit fonctionner ainsi :

1. La webcam capture la main en temps réel.
2. Python récupère les images avec OpenCV.
3. MediaPipe détecte les repères de la main (21 landmarks).
4. Python détermine quel geste est effectué.
5. Un filtrage temporel évite les changements de commande dus à une détection instable.
6. Python transmet une commande à l'Arduino via USB et le port série.
7. L'Arduino commande les deux moteurs via un driver moteur.
8. Le robot s'arrête automatiquement si la main n'est plus détectée, si la communication est perdue ou si le programme se ferme.

Schéma logique :

Webcam → OpenCV → MediaPipe → reconnaissance du geste → filtrage → liaison série USB → Arduino → driver moteur → moteurs.

## 3. Gestes et commandes

Utiliser une seule main et prévoir les gestes suivants :

| Geste | Commande série | Action |
|---|---|---|
| Main ouverte, quatre doigts et pouce étendus | `F` | Avancer |
| Poing fermé | `S` | Arrêter |
| Signe V (index et majeur levés) | `R` | Tourner à droite |
| Index seul levé | `L` | Tourner à gauche |
| Pouce levé | `B` | Reculer |
| Geste non reconnu | `S` | Arrêter par sécurité |
| Main absente | `S` après un court délai | Arrêter |

Important : la détection simple basée uniquement sur la hauteur des doigts peut échouer selon l'orientation de la main. Il faut donc :
- expliquer les limites de cette première méthode ;
- tenir compte de l'orientation de la paume autant que possible ;
- éviter de confondre un geste inconnu avec un geste de déplacement ;
- proposer une méthode de calibration ou une amélioration géométrique si nécessaire ;
- ne jamais commander un déplacement sur la base d'une détection incertaine.

Le signe « pouce levé » doit être distingué d'un poing fermé. Si la méthode géométrique est trop fragile, indiquer clairement cette limite et proposer une méthode plus robuste plutôt que de prétendre qu'elle fonctionne dans toutes les orientations.

## 4. Matériel visé

Liste d'achat réaliste pour rester autour de 50 €, en indiquant les éléments indispensables, les éléments optionnels et les éléments qui peuvent déjà être présents.

- Arduino Uno R3 ou clone compatible.
- Châssis de voiture robot 2WD (deux moteurs DC avec réducteurs, deux roues, une roue folle, visserie).
- Driver moteur double pont en H L298N, ou alternative plus efficace et compatible comme TB6612FNG si le budget et le câblage le permettent.
- Support de piles adapté aux moteurs et piles compatibles.
- Fils Dupont.
- Câble USB adapté à la carte.
- Webcam de l'ordinateur (pas d'achat nécessaire si elle existe déjà).
- Interrupteur général conseillé.
- Éventuellement un capteur ultrasonique HC-SR04 pour éviter les obstacles, en option et si le budget le permet.

Ne pas inventer de prix exacts ni garantir la disponibilité d'un produit : donner des fourchettes réalistes et signaler les frais de livraison. Vérifier que le moteur, le driver et l'alimentation sont électriquement compatibles.

## 5. Architecture électronique et câblage proposé

Brochage avec le driver L298N :

| Arduino Uno | L298N |
|---|---|
| D5 (PWM) | ENA |
| D7 | IN1 |
| D8 | IN2 |
| D9 | IN3 |
| D10 | IN4 |
| D6 (PWM) | ENB |
| GND | GND |

- Moteur gauche : OUT1 et OUT2.
- Moteur droit : OUT3 et OUT4.
- Retirer les cavaliers ENA et ENB si l'on veut régler la vitesse par PWM.
- Relier les masses de l'Arduino et de l'alimentation moteur.
- Alimenter les moteurs avec une source adaptée à leurs caractéristiques.
- Ne pas alimenter les moteurs depuis la sortie 5 V de l'Arduino.
- Ne pas connecter une tension moteur inconnue au 5 V de l'Arduino.
- Vérifier la documentation du module L298N concernant le régulateur 5 V et son cavalier : ne pas supposer que le câblage d'alimentation est identique sur tous les modules.
- Prévoir un interrupteur facilement accessible pour couper l'alimentation des moteurs.

Si les roues tournent dans le mauvais sens, il faut inverser les fils du moteur concerné ou adapter la logique de commande. Le sens « gauche » et « droite » dépendra du montage physique.

## 6. Protocole de communication Python ↔ Arduino

Communication série USB à 9600 bauds (ou une autre vitesse identique des deux côtés).

Commandes à transmettre : `F` avancer, `B` reculer, `L` tourner à gauche, `R` tourner à droite, `S` arrêt.

Exigences :
- Python doit envoyer une commande lorsqu'elle change.
- Python doit aussi envoyer un signal de présence périodique (heartbeat), même si le geste reste identique.
- L'Arduino doit enregistrer l'heure de la dernière commande ou du dernier heartbeat.
- Si aucun message n'est reçu pendant environ 500 ms, l'Arduino doit arrêter les moteurs.
- À la fermeture de Python, envoyer `S` si la connexion est encore disponible.
- Au démarrage et après une reconnexion, le robot doit rester arrêté.
- Ne pas utiliser `delay()` de façon bloquante dans la boucle principale Arduino si cela empêche la surveillance du heartbeat.

Format du protocole : envoyer le caractère de commande régulièrement, par exemple `F\n`, `S\n`, etc. L'Arduino doit lire proprement les caractères et ignorer les fins de ligne.

## 7. Logiciels et bibliothèques

Environnement Windows et Python 3.12.

Installer :

```bash
py -3.12 -m pip install opencv-python mediapipe pyserial
```

Bibliothèques :
- `cv2` / OpenCV : capture et affichage vidéo.
- `mediapipe` : détection des mains et des 21 points de repère.
- `serial` / PySerial : communication série.
- `time` : bibliothèque standard Python pour le délai et le filtrage.

Vérifier la compatibilité réelle des versions des bibliothèques avec Python 3.12. Si MediaPipe ne s'installe pas, expliquer comment créer un environnement virtuel et choisir une version compatible, sans installer au hasard des versions incompatibles.

## 8. Fichiers fournis

### A. `hand_robot.py`

Programme Python complet : ouvre la webcam ; initialise MediaPipe ; détecte une main ; dessine les 21 landmarks et les connexions ; reconnaît les cinq gestes ; affiche le geste détecté et la commande envoyée ; filtre les détections sur plusieurs images ; envoie les commandes série et le heartbeat ; arrête le robot en cas de perte de main, geste inconnu, erreur caméra ou fermeture ; permet de quitter avec la touche `q` ; ferme proprement la caméra, MediaPipe et le port série dans un bloc `try/finally` ; gère les erreurs de port COM et de webcam avec des messages compréhensibles.

Le port série est configurable en haut du fichier. Le bon port se trouve dans le Gestionnaire de périphériques Windows.

### B. `arduino_robot.ino`

Programme Arduino complet : configure les broches du driver ; implémente les fonctions `forward`, `backward`, `turnLeft`, `turnRight`, `stopRobot` ; lit les commandes série ; applique une vitesse PWM réglable ; démarre à l'arrêt ; arrête les moteurs si le heartbeat expire ; ignore proprement les caractères inconnus ; ne bloque pas la surveillance du timeout.

### C. `README.md`

Guide complet comprenant : liste des composants ; schéma de câblage sous forme de tableau ; installation Python ; téléversement du programme Arduino ; choix du port COM ; lancement du programme ; procédure de test ; dépannage ; limites connues ; consignes de sécurité.

### D. `test_moteurs.ino`

Programme de test qui permet de vérifier les moteurs sans webcam ni MediaPipe.

## 9. Tests à prévoir

Procédure progressive, en commençant sans faire rouler le robot :

1. Vérifier que la webcam s'ouvre.
2. Vérifier que MediaPipe détecte les 21 points de la main.
3. Vérifier que les gestes sont affichés correctement sans Arduino.
4. Tester la communication série avec l'Arduino, moteurs déconnectés.
5. Tester chaque moteur séparément, roues soulevées du sol.
6. Tester les commandes de déplacement à vitesse réduite dans un espace dégagé.
7. Tester la perte de main et la déconnexion USB.
8. Tester la fermeture du programme et vérifier l'arrêt.

Prévoir un tableau de diagnostic : symptôme, cause probable, vérification et correction.

## 10. Sécurité et fiabilité

- Commencer avec une vitesse PWM faible.
- Tester les moteurs roues soulevées du sol.
- Utiliser un interrupteur pour couper rapidement l'alimentation des moteurs.
- Ne pas laisser le robot fonctionner sans surveillance.
- Arrêter le robot si la main disparaît ou si le geste est ambigu.
- Prévoir un timeout côté Arduino : la sécurité ne doit pas dépendre uniquement de Python.
- Vérifier la tension et le courant requis par les moteurs avant de choisir l'alimentation.
- Ne pas faire passer le courant des moteurs par les broches de l'Arduino.

## 11. Qualité du résultat attendu

Avant de considérer la solution comme terminée :

1. Vérifier la cohérence des broches Arduino et du driver.
2. Vérifier que Python et Arduino utilisent exactement le même protocole.
3. Vérifier que le heartbeat fonctionne même si le geste ne change pas.
4. Vérifier que le robot s'arrête en cas de perte de communication.
5. Signaler honnêtement les limites de reconnaissance des gestes.
6. Éviter les dépendances inutiles.
7. Expliquer les paramètres modifiables (vitesse, seuils, port COM, délai de sécurité).
8. Ne pas supposer que le robot tourne dans le bon sens avant un test physique.

Si une information manque, l'hypothèse retenue doit être indiquée explicitement : proposer une configuration par défaut raisonnable et signaler ce qui devra être adapté au matériel réellement acheté.
