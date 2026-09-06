# Installation TriCord Presence via FBI

FBI est l'outil de référence pour installer un fichier `.cia` sur une 3DS
avec CFW (Luma3DS). Ce guide part du principe que :

- votre 3DS est en **CFW Luma3DS ≥ 12** ;
- **FBI est déjà installé** (via Homebrew Launcher ou en `.cia`) ;
- **TriCord est installé et connecté à un compte** — sinon le sysmodule
  démarrera mais restera en veille (cf. `docs/ACCOUNT_BRIDGE.md`).

## 1. Obtenir le `.cia`

Deux options, au choix :

### Option A — Compiler soi-même (recommandé)

Depuis une machine avec devkitPro installé (Linux ou macOS) :

```bash
# 1. Installer la toolchain devkitPro (une fois)
#    Voir https://devkitpro.org/wiki/Getting_Started
sudo dkp-pacman -S 3ds-dev 3ds-mbedtls 3ds-wslay 3ds-jansson

# 2. Récupérer le code + outils hôte
git clone https://github.com/<user>/<repo> tricord-presence
cd tricord-presence
source tools/env.sh
tools/build_host_tools.sh   # makerom, bannertool, 3gxtool, ctrtool

# 3. Build complet
./build.sh
```

Sortie attendue dans `dist/` :

```
tricord-presence-installer.cia    <- CE FICHIER pour FBI
tricord-presence-installer.3dsx   <- alternative Homebrew Launcher
000401300F000102.cxi              <- sysmodule (embarqué dans le .cia)
tricord_overlay.3gx               <- plugin (embarqué)
titles.txt                        <- base Title ID -> nom de jeu
qr.html                           <- page QR pour FBI Remote Install
```

### Option B — Télécharger une release

Si le mainteneur publie des builds :

```
https://github.com/<user>/<repo>/releases/latest
```

Prendre `tricord-presence-installer.cia`.

## 2. Installer avec FBI

### 2.a. Méthode SD Card

1. Copiez `tricord-presence-installer.cia` **à la racine de la SD**
   (n'importe où marche, mais la racine est plus rapide à naviguer).
2. Sur la 3DS : lancez FBI (par HBL ou .cia).
3. `SD` → naviguez jusqu'au fichier → A → « Install and delete CIA »
   (ou juste « Install » si vous voulez garder la copie sur SD).
4. Confirmez. L'installation prend < 5 secondes (petit fichier).

Une icône « TriCord Presence Installer » apparaît dans le menu HOME.

### 2.b. Méthode QR Remote Install (sans SD, sans PC)

1. Récupérez `dist/qr.html` (auto-généré par `build.sh`, contient un QR
   scannable pointant vers l'URL de release).
2. Affichez la page sur un PC ou téléphone.
3. Sur la 3DS : FBI → `Remote Install` → `Scan QR Code`.
4. Pointez la caméra vers le QR. FBI télécharge et installe automatiquement.

Attention : la 3DS doit avoir une connexion WiFi active et l'URL du QR
doit être **HTTPS** (FBI refuse le HTTP en clair depuis la 11.4).

Régénérer le QR avec une URL réelle (une seule fois avant distribution) :

**Linux / macOS :**
```bash
python3 dist/gen_qr.py https://example.com/installer.cia --out dist/qr.html
# ou (via build.sh) :
QR_URL=https://example.com/installer.cia ./build.sh
```

**Windows (cmd, PowerShell, ou Windows Terminal) :**
```cmd
python dist\gen_qr.py https://example.com/installer.cia --out dist\qr.html
```

Ou plus court, en utilisant le wrapper `.cmd` fourni :
```cmd
dist\gen_qr.cmd https://example.com/installer.cia --out dist\qr.html
```

> **Pièges Windows classiques** : ne pas oublier le suffixe `.py`
> (`python dist\gen_qr` sans `.py` échoue avec « can't open file »), et
> utiliser des antislash `\` (pas des slash `/`) pour les chemins. Le
> wrapper `gen_qr.cmd` évite ces deux problèmes en trouvant automatiquement
> le `.py` à côté de lui.

Prérequis Python (identique sur les 3 OS) : `pip install qrcode`.

## 3. Utiliser l'installeur (côté 3DS)

Après installation par FBI, ouvrez « TriCord Presence Installer » depuis le
menu HOME. C'est une **console texte** (pas de GUI, contrainte assumée
pour cette version — cf. `docs/ARCHITECTURE.md` §CLI).

Actions proposées à l'ouverture (contrôles listés à l'écran) :

- **A — Installer** : copie du sysmodule vers `/luma/sysmodules/`, du plugin
  overlay vers `/luma/plugins/default.3gx`, de la base de titres et de la
  config. **Aucun token à saisir** (le sysmodule récupère automatiquement
  le compte déjà connecté dans TriCord — cf. `docs/ACCOUNT_BRIDGE.md`).
- **X — Désinstaller** : retire proprement tous les fichiers écrits par
  l'installeur (avec confirmations par étape).
- **B — Quitter** : sans rien toucher.

À la fin de l'installation, l'installeur propose de **lancer immédiatement
le sysmodule sans reboot** (via `pm:app LaunchTitle`, méthode Plug-n-play).

## 4. Étapes système à faire *une seule fois*

Avant que le sysmodule + plugin fonctionnent réellement :

1. **Luma3DS** : au boot, maintenir SELECT pour entrer dans le config menu,
   activer :
   - `Enable loading external FIRMs and modules`
2. **Rosalina** (`L + Down + Select` en jeu ou depuis HOME) :
   - `Miscellaneous options` → `Plugin loader` → activé.
3. Redémarrer la console une fois.

## 5. Vérifier que ça marche

- Lancer un jeu compatible : au bout de ~5 s, un toast « Vous jouez a
  &lt;nom du jeu&gt; » doit apparaître en haut de l'écran du haut.
- Ouvrir Discord depuis un autre appareil : le compte lié dans TriCord doit
  afficher « Joue à &lt;nom du jeu&gt; » dans son statut.
- En cas de problème, consulter `sdmc:/3ds/tricord-presence/log.txt`
  (+ éventuellement `log.1.txt`, `log.2.txt`, `log.3.txt` — cf. rotation
  documentée dans `RAPPORT.md` §M).

## 6. Rappel important — ToS Discord

Ce projet utilise un **token utilisateur** (pas un bot Discord officiel).
Discord considère la connexion permanente avec un token utilisateur comme
un usage « self-bot », techniquement contraire aux CGU. **Le risque de
suspension du compte est réel.** À utiliser avec un compte jetable, pas le
compte principal. Cf. `docs/ARCHITECTURE.md` §« Contrainte ToS Discord ».

## 7. Après un reboot

Luma3DS mainline **ne relance pas** un sysmodule custom au boot :
- Le plus simple : rouvrir « TriCord Presence Installer » et cliquer
  « Lancer le sysmodule maintenant » (aucun reboot nécessaire).
- Alternative avancée : injecter la dépendance dans l'exheader NS. Voir
  `docs/BOOT_AUTORUN.md` pour la procédure et les risques.

## 8. Désinstaller

- Depuis l'installeur : ouvrir → X → suivre les 4 étapes.
- Depuis FBI : `Title Manager` → chercher `000400000F000200` → Delete.
- Nettoyage manuel de la SD :
  - `sdmc:/luma/sysmodules/000401300F000102.cxi`
  - `sdmc:/luma/plugins/default.3gx`
  - `sdmc:/3ds/tricord-presence/`
  - éventuellement `sdmc:/luma/titles/0004013000008002/` (si vous aviez
    tenté l'autoboot via NS, cf. `docs/BOOT_AUTORUN.md`).
