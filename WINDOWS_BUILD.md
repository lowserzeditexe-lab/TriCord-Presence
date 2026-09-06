# Compiler TriCord Presence sous Windows

## 1. Installer devkitPro

Utilise l'installateur graphique officiel devkitPro sous Windows et coche **3DS Development**. Une connexion Internet est nécessaire pour l'installation.

Après installation, ouvre le terminal **devkitPro MSYS2** fourni avec devkitPro.

## 2. Installer les dépendances du projet

Dans le terminal MSYS2 devkitPro :

```bash
pacman -Syu
pacman -S --needed 3ds-dev 3ds-mbedtls 3ds-wslay 3ds-jansson git make python3
```

Ferme et rouvre MSYS2 si `pacman -Syu` te le demande.

## 3. Compiler

Méthode simple depuis l'Explorateur Windows :

1. Place ce dossier dans un chemin sans caractères exotiques, par exemple `C:\3ds\tricord-presence`.
2. Double-clique sur `build_windows.bat`.
3. Le résultat apparaît dans `dist\`.

Le build produit notamment :

- `dist\tricord-presence-installer.3dsx`
- `dist\tricord-presence-installer.cia`
- `dist\000401300F000102.cxi`
- `dist\tricord_overlay.3gx`
- `dist\titles.txt`
- `dist\qr.html`

## 4. Compilation manuelle depuis MSYS2

```bash
cd /c/3ds/tricord-presence
./build_windows.sh
```

Le script appelle ensuite le `build.sh` du projet.
