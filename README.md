# 🧮 Calculatrice Arduino (Wokwi)

Calculatrice à clavier matriciel 4×4 et écran LCD 16×2, programmée sur **Arduino Uno** et simulée avec [Wokwi](https://wokwi.com/projects/477868187497439233).

👉 **[Ouvrir la simulation en ligne](https://wokwi.com/projects/477868187497439233)**

## ✨ Fonctionnalités

- Les 4 opérations : `+`, `-`, `*`, `/`
- **Priorité des opérateurs** (`*` et `/` avant `+` et `-`) : `2+3*4 = 14`
- Nombres décimaux (un seul point par nombre, `0.` ajouté automatiquement)
- Nombres négatifs (`-` au début de l'expression)
- Enchaînement de calculs : après `=`, taper un opérateur réutilise le résultat
- Gestion des erreurs : `Erreur` (syntaxe, résultat trop grand) et `Div par 0`
- **Appui long** (800 ms) :
  - sur `=` → tout effacer (C)
  - sur `.` → effacer le dernier caractère (←)
- Affichage : expression en haut à gauche, résultat en bas à droite

## 🔧 Matériel

| Composant | Quantité |
|---|---|
| Arduino Uno | 1 |
| Écran LCD 16×2 (HD44780) | 1 |
| Clavier matriciel 4×4 | 1 |

## 🔌 Câblage

**LCD (mode 4 bits)**

| LCD | Arduino |
|---|---|
| RS | D13 |
| E | D12 |
| D4 | D11 |
| D5 | D10 |
| D6 | D9 |
| D7 | D8 |

**Clavier 4×4**

| Clavier | Arduino |
|---|---|
| R1 – R4 (lignes) | D7, D6, D5, D4 |
| C1 – C4 (colonnes) | A0, A1, A2, A3 |

Le schéma complet est dans [`diagram.json`](diagram.json).

## ⌨️ Disposition des touches

|   |   |   |   |
|---|---|---|---|
| `1` | `2` | `3` | `+` |
| `4` | `5` | `6` | `-` |
| `7` | `8` | `9` | `*` |
| `.` | `0` | `=` | `/` |

## ▶️ Lancer le projet

### Option 1 : en ligne (le plus simple)
Ouvrir le [projet Wokwi](https://wokwi.com/projects/477868187497439233) et cliquer sur ▶.

### Option 2 : en local avec VS Code
1. Installer l'extension **Wokwi Simulator** et **PlatformIO** (ou Arduino CLI).
2. Cloner ce dépôt :
   ```bash
   git clone https://github.com/sadkaouinadinejob-stack/calculatrice-arduino.git
   ```
3. Compiler le code, puis lancer la simulation avec `F1` → *Wokwi: Start Simulator*.

## 📚 Bibliothèques

- `LiquidCrystal` (incluse avec Arduino)
- [`Keypad`](https://github.com/Chris--A/Keypad) (voir `libraries.txt`)

## 📁 Structure du projet

```
calculatrice-arduino/
├── sketch.ino       # code principal
├── diagram.json     # schéma du circuit Wokwi
├── libraries.txt    # bibliothèques nécessaires
├── README.md
├── LICENSE
└── .gitignore
```

## ⚠️ Limites

- Expression de 32 caractères maximum, 17 nombres maximum
- Résultat affiché en valeur absolue < 1 000 000 000
- Pas de parenthèses

## 📄 Licence

Distribué sous licence MIT, voir [LICENSE](LICENSE).
