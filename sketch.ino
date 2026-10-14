#include <LiquidCrystal.h>   // bibliothèque pour piloter l'écran LCD (intégrée à Arduino)
#include <Keypad.h>          // bibliothèque pour lire le clavier matriciel (à ajouter via Library Manager)

// ======================= ÉCRAN LCD ====================
// Crée l'objet "lcd". Ordre des pins : RS, E, D4, D5, D6, D7
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

// ======================= CLAVIER =======================
const byte ROWS = 4;         // nombre de lignes du clavier (const = ne change jamais)
const byte COLS = 4;         // nombre de colonnes du clavier

// Carte des touches : reproduit le clavier physique case par case
char keys[ROWS][COLS] = {
  {'1', '2', '3', '+'},      // ligne 1 : 1 2 3 A  (A fait '+')
  {'4', '5', '6', '-'},      // ligne 2 : 4 5 6 B  (B fait '-')
  {'7', '8', '9', '*'},      // ligne 3 : 7 8 9 C  (C fait '*')
  {'.', '0', '=', '/'}       // ligne 4 : * 0 # D  (* fait '.', # fait '=', D fait '/')
};

byte rowPins[ROWS] = {7, 6, 5, 4};       // pins Arduino reliés aux 4 fils des lignes
byte colPins[COLS] = {A0, A1, A2, A3};   // pins Arduino reliés aux 4 fils des colonnes

// Crée l'objet "keypad" avec : la carte des touches, les pins lignes/colonnes, la taille
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ======================= VARIABLES DE LA CALCULATRICE =======================
const byte MAX_LEN  = 32;    // nombre maximum de caractères dans l'expression
const byte MAX_NUMS = 17;    // nombre maximum de nombres dans une expression

char expr[MAX_LEN + 1] = ""; // stocke l'expression tapée, ex : "12+3*4" (+1 pour le '\0')
byte len = 0;                // nombre de caractères actuellement dans expr
char resultStr[24] = "";     // le résultat sous forme de texte, ex : "14"
bool resultOk = false;       // true si le dernier résultat est valide (pas une erreur)
bool showResult = false;     // true si on doit afficher le résultat sur la ligne 2
bool justCalculated = false; // true si on vient d'appuyer sur '='
bool holdHandled = false;    // true si un appui long a déjà été traité (évite de le traiter 2 fois)

// ======================= OUTILS =======================
// Dit si le caractère c est un opérateur
bool isOp(char c) {
  return c == '+' || c == '-' || c == '*' || c == '/';   // vrai si c'est l'un des 4 opérateurs
}

// ======================= AFFICHAGE =======================
// Écrit un texte sur une ligne de 16 colonnes, alignée à gauche ou à droite
// row = numéro de ligne (0 ou 1), text = le texte, right = true pour aligner à droite
void printRow(byte row, const char *text, bool right) {
  char line[17];                       // ligne complète : 16 caractères + '\0'
  byte l = strlen(text);               // longueur du texte à afficher
  const char *t = text;                // pointeur sur le début du texte à copier
  if (l > 16) {                        // si le texte dépasse l'écran...
    t = text + (l - 16);               // ...on saute le début pour garder les 16 derniers caractères
    l = 16;                            // la longueur affichée est donc 16
  }
  memset(line, ' ', 16);               // remplit la ligne de 16 espaces (efface l'ancien contenu)
  line[16] = '\0';                     // termine la chaîne
  byte start = right ? (16 - l) : 0;   // position de départ : à droite = 16 - longueur, à gauche = 0
  memcpy(line + start, t, l);          // copie le texte dans la ligne à la bonne position
  lcd.setCursor(0, row);               // place le curseur au début de la ligne voulue
  lcd.print(line);                     // écrit les 16 caractères d'un coup
}

// Met à jour tout l'écran
void refresh() {
  printRow(0, len == 0 ? "0" : expr, false);       // ligne 1 : l'expression (ou "0" si vide), à gauche
  printRow(1, showResult ? resultStr : "", true);  // ligne 2 : le résultat (ou vide), à droite
}

// ======================= CALCUL =======================
// Calcule l'expression s et met le résultat dans out
// Retourne 0 = OK, 1 = erreur de syntaxe, 2 = division par zéro
// "double &out" = référence : modifier out modifie la variable de l'appelant
byte evaluate(const char *s, double &out) {
  double nums[MAX_NUMS];     // liste des nombres trouvés
  char ops[MAX_NUMS];        // liste des opérateurs trouvés
  byte n = 0;                // nombre de nombres trouvés jusqu'ici
  byte i = 0;                // position de lecture dans la chaîne s

  // ---- Étape 1 : découper l'expression en nombres et opérateurs ----
  while (true) {                                   // boucle jusqu'à la fin de l'expression
    bool neg = false;                              // ce nombre est-il négatif ?
    if (s[i] == '-') { neg = true; i++; }          // un '-' avant un nombre = signe moins

    char buf[MAX_LEN + 1];                         // zone temporaire pour écrire le nombre en texte
    byte b = 0;                                    // nombre de caractères dans buf
    while ((s[i] >= '0' && s[i] <= '9') || s[i] == '.') {  // tant qu'on lit un chiffre ou un point
      buf[b++] = s[i++];                           // copie le caractère dans buf et avance
    }
    if (b == 0 || n >= MAX_NUMS) return 1;         // aucun chiffre trouvé ou trop de nombres : erreur
    buf[b] = '\0';                                 // termine la chaîne du nombre

    double v = atof(buf);                          // convertit le texte en nombre décimal
    nums[n++] = neg ? -v : v;                      // stocke le nombre (négatif si besoin)

    if (s[i] == '\0') break;                       // fin de l'expression : on sort de la boucle
    if (!isOp(s[i])) return 1;                     // caractère inattendu : erreur de syntaxe
    ops[n - 1] = s[i++];                           // stocke l'opérateur qui suit ce nombre et avance
  }

  // ---- Étape 2 : première passe, on traite * et / (priorité) ----
  double vals[MAX_NUMS];     // nombres restants après les * et /
  char vops[MAX_NUMS];       // opérateurs + et - restants
  byte m = 0;                // indice du dernier nombre dans vals
  vals[0] = nums[0];         // on démarre avec le premier nombre

  for (byte k = 0; k < n - 1; k++) {               // parcourt chaque opérateur
    if (ops[k] == '*') {                           // multiplication
      vals[m] *= nums[k + 1];                      // multiplie directement le nombre en cours
    } else if (ops[k] == '/') {                    // division
      if (nums[k + 1] == 0) return 2;              // diviseur nul : erreur division par zéro
      vals[m] /= nums[k + 1];                      // divise directement le nombre en cours
    } else {                                       // + ou - : on les garde pour la 2e passe
      vops[m] = ops[k];                            // mémorise l'opérateur
      m++;                                         // passe à la case suivante
      vals[m] = nums[k + 1];                       // mémorise le nombre suivant
    }
  }

  // ---- Étape 3 : deuxième passe, on traite + et - de gauche à droite ----
  double r = vals[0];                              // le résultat démarre au premier nombre
  for (byte k = 0; k < m; k++) {                   // parcourt les + et - restants
    if (vops[k] == '+') r += vals[k + 1];          // addition
    else                r -= vals[k + 1];          // soustraction
  }

  out = r;                                         // renvoie le résultat via la référence
  return 0;                                        // 0 = tout s'est bien passé
}

// Convertit un nombre en texte (max 16 caractères, sans zéros inutiles)
// Retourne true si OK, false si le nombre est invalide ou trop grand
bool formatResult(double v, char *out) {
  if (isnan(v) || isinf(v) || fabs(v) >= 1e9) {    // pas un nombre, infini, ou >= 1 milliard
    strcpy(out, "Erreur");                         // on écrit "Erreur"
    return false;                                  // résultat non valide
  }
  for (int d = 6; d >= 0; d--) {                   // essaie 6 décimales, puis 5, 4... jusqu'à 0
    dtostrf(v, 1, d, out);                         // convertit le float en texte avec d décimales
    if (strlen(out) <= 16) break;                  // si ça tient sur l'écran, on arrête
  }
  if (strchr(out, '.')) {                          // s'il y a un point décimal...
    int l = strlen(out);                           // longueur actuelle du texte
    while (l > 0 && out[l - 1] == '0') out[--l] = '\0';   // enlève les '0' à la fin : 2.500 -> 2.5
    if (l > 0 && out[l - 1] == '.') out[--l] = '\0';      // enlève un '.' final : 3. -> 3
  }
  if (strcmp(out, "-0") == 0) strcpy(out, "0");    // évite d'afficher "-0"
  return true;                                     // résultat valide
}

// ======================= ÉDITION DE L'EXPRESSION =======================
// Ajoute un caractère à la fin de l'expression
void appendChar(char c) {
  if (len < MAX_LEN) {                             // s'il reste de la place
    expr[len++] = c;                               // écrit le caractère puis augmente len
    expr[len] = '\0';                              // referme la chaîne
  }
}

// Efface tout et remet la calculatrice à zéro
void clearAll() {
  expr[0] = '\0';                                  // expression vide
  len = 0;                                         // longueur à 0
  showResult = false;                              // on cache le résultat
  justCalculated = false;                          // plus de calcul "fini"
  resultOk = false;                                // plus de résultat valide
  refresh();                                       // met l'écran à jour
}

// Efface le dernier caractère
void backspace() {
  if (justCalculated) { clearAll(); return; }      // après un '=', on efface tout
  if (len > 0) {                                   // s'il y a quelque chose à effacer
    len--;                                         // on recule d'un caractère
    expr[len] = '\0';                              // et on referme la chaîne là
  }
  refresh();                                       // met l'écran à jour
}

// ======================= TRAITEMENT D'UNE TOUCHE =======================
void handleKey(char k) {

  // ---- Cas 1 : chiffre ou point ----
  if ((k >= '0' && k <= '9') || k == '.') {
    if (justCalculated) clearAll();                // après un '=', un chiffre démarre un nouveau calcul

    if (k == '.') {                                // cas particulier du point
      int s = len;                                 // on cherche le début du nombre en cours
      while (s > 0 && !isOp(expr[s - 1])) s--;     // on recule jusqu'au dernier opérateur
      for (int j = s; j < len; j++) {              // on regarde chaque caractère de ce nombre
        if (expr[j] == '.') return;                // déjà un point : on ignore cette touche
      }
      if (len + 2 > MAX_LEN) return;               // pas assez de place pour "0." : on ignore
      if (len == s) appendChar('0');               // le nombre commence par '.' : on ajoute '0' avant
    }
    appendChar(k);                                 // ajoute le chiffre ou le point à l'expression
  }

  // ---- Cas 2 : opérateur ----
  else if (isOp(k)) {
    if (justCalculated) {                          // on vient de faire '=' : on continue avec le résultat
      if (resultOk) {                              // si le résultat est valide...
        strcpy(expr, resultStr);                   // ...il devient le début de la nouvelle expression
        len = strlen(expr);                        // on met à jour la longueur
      } else {
        clearAll();                                // résultat en erreur : on repart de zéro
      }
      justCalculated = false;                      // on n'est plus "juste après un calcul"
      showResult = false;                          // on cache l'ancien résultat
    }

    if (len == 0) {                                // expression vide
      if (k != '-') return;                        // seul '-' est permis au début (nombre négatif)
      appendChar(k);                               // ajoute le '-'
    } else if (isOp(expr[len - 1])) {              // le dernier caractère est déjà un opérateur
      if (len == 1) return;                        // c'est juste un '-' de début : on ne le remplace pas
      expr[len - 1] = k;                           // sinon on remplace l'ancien opérateur par le nouveau
    } else {
      appendChar(k);                               // cas normal : on ajoute l'opérateur
    }
  }

  // ---- Cas 3 : égal ----
  else if (k == '=') {
    if (len == 0 || justCalculated) return;        // rien à calculer, ou déjà calculé : on ignore

    if (isOp(expr[len - 1])) {                     // l'expression finit par un opérateur, ex : "5+"
      len--;                                       // on retire cet opérateur
      expr[len] = '\0';                            // et on referme la chaîne
    }
    if (len > 0) {                                 // s'il reste quelque chose à calculer
      double r;                                    // variable qui recevra le résultat
      byte status = evaluate(expr, r);             // calcule et récupère le code de retour

      if (status == 0)      resultOk = formatResult(r, resultStr);                // OK : convertit en texte
      else if (status == 2) { strcpy(resultStr, "Div par 0"); resultOk = false; } // division par zéro
      else                  { strcpy(resultStr, "Erreur");    resultOk = false; } // erreur de syntaxe

      showResult = true;                           // on affichera le résultat sur la ligne 2
      justCalculated = true;                       // on note qu'un calcul vient d'être fait
    }
  }

  refresh();                                       // met l'écran à jour après chaque touche
}

// ======================= ÉVÉNEMENTS DU CLAVIER =======================
// Appelée automatiquement par la bibliothèque à chaque changement d'état d'une touche
// Appui court = action normale (au relâchement)
// Appui long sur '=' = tout effacer, sur '.' = effacer le dernier caractère
void keypadEvent(KeypadEvent key) {
  switch (keypad.getState()) {                     // lit l'état de la touche concernée
    case PRESSED:                                  // la touche vient d'être enfoncée
      holdHandled = false;                         // on repart sans appui long traité
      break;                                       // fin de ce cas
    case HOLD:                                     // la touche est maintenue assez longtemps
      if (key == '=') { clearAll();  holdHandled = true; }        // appui long sur '=' : tout effacer
      else if (key == '.') { backspace(); holdHandled = true; }   // appui long sur '.' : effacer 1 caractère
      break;                                       // fin de ce cas
    case RELEASED:                                 // la touche vient d'être relâchée
      if (!holdHandled) handleKey(key);            // si ce n'était pas un appui long, on traite la touche
      break;                                       // fin de ce cas
    default:                                       // tous les autres états (IDLE...)
      break;                                       // on ne fait rien
  }
}

// ======================= PROGRAMME PRINCIPAL =======================
void setup() {                                     // s'exécute une seule fois au démarrage
  lcd.begin(16, 2);                                // l'écran a 16 colonnes et 2 lignes
  lcd.print("Calculatrice");                       // message de bienvenue, ligne 1
  lcd.setCursor(0, 1);                             // curseur au début de la ligne 2
  lcd.print("=:C   .:<-");                         // rappel : '=' long = tout effacer, '.' long = effacer 1
  delay(1800);                                     // attend 1,8 seconde pour laisser lire

  keypad.setHoldTime(800);                         // un appui compte comme "long" après 800 ms
  keypad.addEventListener(keypadEvent);            // dit à la bibliothèque d'appeler keypadEvent()

  clearAll();                                      // efface tout et affiche l'écran initial
}

void loop() {                                      // s'exécute en boucle, pour toujours
  keypad.getKey();                                 // scanne le clavier ; les événements passent par keypadEvent()
}