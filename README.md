# TriCord Presence

Extension "Rich Presence" pour TriCord (client Discord non-officiel 3DS),
composée de 3 briques distinctes :

```
tricord-presence/
├── sysmodule/   → daemon résident (Luma3DS, /luma/sysmodules/000401300F000102.cxi)
│                  détecte l'état console (idle / en jeu) via APT et pousse
│                  les mises à jour de présence sur la Gateway Discord (TLS+WS)
├── plugin/      → plugin Luma3DS (.3gx, CTRPluginFramework), injecté dans le
│                  jeu en cours, affiche l'overlay "Vous jouez a <jeu>"
├── installer/   → homebrew CLI (.3dsx + .cia) : copie les fichiers sur la SD,
│                  base de titres — plus de saisie de token : le sysmodule
│                  récupère le compte déjà connecté dans TriCord au démarrage
│                  (voir sysmodule/source/tricord_account_bridge.c et
│                  docs/ACCOUNT_BRIDGE.md)
├── tools/       → env.sh, build des outils hôte, générateurs (titres, assets)
├── dist/        → artefacts + gen_qr.py / qr.html (QR "Remote Install" FBI)
├── docs/        → notes d'architecture
├── RAPPORT.md   → ce qui est fait / TODO(hardware) / points devinés + sources
└── build.sh     → compile les 3 composants dans l'ordre
```

## ⚠️ État

Le code compile de bout en bout et le client Gateway Discord a été validé
sur hôte Linux contre la vraie gateway, mais **rien n'a été exécuté sur
console ni sur Citra/Azahar** — y compris le nouveau pont de compte
(`tricord_account_bridge`), qui n'a jamais été testé contre un vrai fichier
`accounts` généré par TriCord. Les zones `// TODO(hardware)` et
`TODO(emergent)` listent ce qui reste à valider (voir `RAPPORT.md` et
`docs/ACCOUNT_BRIDGE.md`). Testez d'abord sur une console de test, avec un
compte Discord jetable — pas votre compte principal, vu le risque ToS déjà
documenté dans `docs/ARCHITECTURE.md`.

Prérequis fonctionnel : **TriCord doit déjà être installé et connecté à un
compte** (`sdmc:/3ds/TriCord/accounts` doit exister) avant de lancer ce
sysmodule pour la première fois. Sans ça, il tourne mais reste en attente
(voir la boucle de retry dans `sysmodule/source/main.c`).

## Prérequis de build

- devkitARM + devkitPro : `dkp-pacman -S 3ds-dev 3ds-mbedtls 3ds-wslay 3ds-jansson`
- Outils hôte : `tools/build_host_tools.sh` (makerom, ctrtool, 3gxtool,
  bannertool ; nécessite `libyaml-cpp-dev`)
- CTRPluginFramework : installé automatiquement par `build.sh` si absent
- Python 3 (+ `pip install qrcode` pour le QR)

## Build

```bash
source tools/env.sh
./build.sh                                   # → dist/
QR_URL=https://…/tricord-presence-installer.cia ./build.sh   # + QR avec l'URL réelle
```

## Installation utilisateur final

**Procédure détaillée : voir [`docs/FBI_INSTALL.md`](docs/FBI_INSTALL.md)**
(guide complet en français, du build du `.cia` à l'installation FBI, en
passant par le QR Remote Install et les étapes système Luma/Rosalina).

Résumé rapide :

1. Luma3DS ≥ 12 : activer "Enable loading external FIRMs and modules"
   (SELECT au boot) ; Rosalina : activer "Plugin loader".
2. Installer `dist/tricord-presence-installer.cia` (FBI / QR `dist/qr.html`)
   ou lancer le `.3dsx` via Homebrew Launcher.
3. Suivre l'installeur (copie des fichiers, lancement du sysmodule) — se
   connecter dans TriCord avant ou après, le sysmodule attend et se
   connecte tout seul dès qu'un compte existe (voir "État" ci-dessus).
4. Luma3DS ne relance pas un sysmodule custom au boot : relancer l'installeur
   après chaque redémarrage (voir `docs/BOOT_AUTORUN.md` pour les alternatives).

Fichiers écrits sur la SD : `/luma/sysmodules/000401300F000102.cxi`,
`/luma/plugins/default.3gx`, `/3ds/tricord-presence/{titles.txt,log.txt}`
(`config.txt` optionnel, dépannage uniquement — voir docs/ACCOUNT_BRIDGE.md).

## Build sans devkitPro local

Le dépôt contient deux workflows GitHub Actions dans `.github/workflows/`. Ils utilisent directement l'image officielle `devkitpro/devkitarm` pour compiler le projet sur GitHub, sans installer devkitARM sur le PC local.

1. Crée un dépôt GitHub et pousse ce dossier dedans.
2. Ouvre l'onglet **Actions**.
3. Lance **Build TriCord Presence (3DS)** avec **Run workflow**.
4. À la fin du job, récupère l'artifact `tricord-presence-3ds-...` dans la section **Artifacts**.

Le workflow `Package release ZIP` permet en plus de produire un ZIP distributable complet.

## Build Windows

Voir `WINDOWS_BUILD.md` pour la procédure Windows. Pour lancer le build directement depuis l'Explorateur, double-cliquer sur `build_windows.bat`.

## GitHub Actions

Le workflow GitHub Actions installe `libctrpf` depuis le dépôt de paquets ThePixellizerOSS et ne reconstruit pas CTRPluginFramework depuis ses sous-modules. Cela évite les échecs CI liés à `libcwav`.
