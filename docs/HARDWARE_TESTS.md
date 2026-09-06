# Rapport de tests hardware — TriCord Presence

Ce document liste **chaque `TODO(hardware)` restant dans le code** avec une
procédure de test précise. À remplir sur une vraie 3DS (o3DS ou n3DS,
Luma3DS ≥ 12) et/ou sur Citra-Azahar. Chaque case doit être annotée
`PASS` / `FAIL` / `N/A` avec une note d'observation.

Rappel : **rien** de ce document n'a été validé dans l'environnement de
build de ce squelette (conteneur Linux sans émulateur exécutable, cf.
RAPPORT.md §Environnement).

## Prérequis

- 3DS ou n3DS, Luma3DS ≥ 12, entrée « Enable loading external FIRMs and
  modules » activée dans le config menu Luma (SELECT au boot).
- Rosalina : « Plugin loader » activé (L+Down+Select → Miscellaneous).
- TriCord installé et déjà connecté à un **compte Discord jetable**
  (pas le compte principal — rappel du risque ToS, cf. docs/ARCHITECTURE.md).
- Fichier `sdmc:/3ds/TriCord/accounts` généré par une build TriCord réelle
  (créer un compte dedans, se logguer une fois).
- L'installeur (`tricord-presence-installer.cia`) est passé sans erreur.

## Feuille de tests

Format de chaque ligne : `[ ] Titre — Comment tester — Résultat attendu`.

### 1. Boot / lancement du sysmodule

- [ ] **1.1 `pm:app LaunchTitle` depuis le CIA installeur**
  Après un cold boot : lancer l'installeur `.cia` depuis HOME, cliquer
  « Lancer le sysmodule maintenant ». Attendu : retour `svcSendSyncRequest`
  OK et création de `sdmc:/3ds/tricord-presence/log.txt` avec
  `tricord_presenced start`. FAIL si le processus n'apparaît pas dans
  `Rosalina > Process list`.

- [ ] **1.2 Idem depuis le `.3dsx` sous Homebrew Launcher**
  Même test mais via HBL. Peut échouer : l'accès à `pm:app` dépend des
  droits accordés par `hb:ldr` dans la version de HBL utilisée.

- [ ] **1.3 Persistance sur reboot**
  Après reboot : vérifier que **sans intervention** le sysmodule n'est
  PAS relancé (comportement documenté de Luma mainline). L'utilisateur
  doit relancer l'installeur pour retrouver la présence.
  Cf. `docs/BOOT_AUTORUN.md` pour la méthode alternative « exheader NS ».

### 2. Bridge de compte TriCord (`tricord_account_bridge.c`)

- [ ] **2.1 `psInit()` accessible à un sysmodule tiers**
  Chercher dans `log.txt` la ligne `psInit ...`. Attendu : rc=0.
  FAIL si `rc=D8E06406` (`RD_ACCESS_DENIED`) — dans ce cas, ajouter la
  déclaration `ps:ps` au bon endroit du `.rsf` (déjà fait, TID
  `0x0004013000003102` : vérifier que `makerom` ne l'a pas fait sauter).

- [ ] **2.2 `PS_EncryptDecryptAes(PS_KEYSLOT_0D, ...)` sur un vrai `accounts`**
  Se connecter dans TriCord avec un compte. `log.txt` doit ensuite montrer :
  `Token récupéré depuis le compte TriCord actif`. FAIL si :
  - `PS_EncryptDecryptAes a échoué (rc=...)` → keyslot refusé, revoir la
    doc `PS:PS`.
  - `JSON invalide après déchiffrement` → mauvais algo/format,
    resynchroniser sur la version courante de
    `TriCord/source/core/config.cpp::encrypt_decrypt_data`.

- [ ] **2.3 Changement de compte à chaud**
  Sysmodule tourne, présence active. Dans TriCord : se déconnecter puis
  reconnecter avec un autre compte. Sous 15 s : voir dans `log.txt`
  `Changement de compte détecté` + `close reçu, code ...` + `READY`
  avec un nouveau `session_id`. La présence Discord doit refléter le
  nouveau compte.

- [ ] **2.4 Déconnexion complète → veille active**
  Sysmodule tourne. Dans TriCord : « Se déconnecter » du compte actif
  (accounts vidé). Sous 15 s : `log.txt` doit montrer
  `TriCord déconnecté` + `veille : pas de token TriCord actif`.
  **PAS de `fatal=1` ni de `4004`**.

### 3. APT / détection du jeu au premier plan

- [ ] **3.1 Plan A : `APT:GetAppletInfo(0x300)` accepté**
  Lancer un jeu physique (cartouche) ou eShop. `log.txt` : `state kind=1
  tid=<xxxx> name=<jeu>`. FAIL si tid=0 ou rc≠0 → passer au 3.2.

- [ ] **3.2 Plan B : fallback `svcGetProcessList`**
  Si 3.1 échoue : vérifier que le fallback ajouté déclenche. La ligne
  `state` doit apparaître quand même. Retester avec le firmware réel
  du système (le type 0x10001 de `svcGetProcessInfo` est une extension
  Luma3DS k11 : ne pas fonctionne pas sur Citra sans mode Luma).

- [ ] **3.3 Sortie de jeu → menu HOME**
  Presser HOME depuis un jeu, sans quitter. `log.txt` doit rester
  `kind=1` (jeu suspendu = toujours "en jeu", cf. §4 RAPPORT.md).
  Quitter le jeu depuis HOME : `kind=0` (idle).

- [ ] **3.4 Applet bibliothèque (clavier, erreur…) au premier plan**
  Ouvrir un jeu qui utilise `swkbd`. Vérifier que la présence reste sur
  le jeu (kind=1) même si l'applet bibliothèque est active.

### 4. Gateway Discord (voir aussi RAPPORT.md §0)

- [ ] **4.1 Handshake complet avec un vrai token**
  Chaîne complète : TCP → TLS handshake → WS upgrade → HELLO → IDENTIFY
  → **READY** → heartbeat/ACK. Pas de close 4004 (i.e. token accepté).
  Nouveau depuis la révision « veille active » : la ligne
  `ws-accept` doit maintenant faire partie du chemin nominal (vérifiée
  silencieusement, une seule mention en cas d'ÉCHEC).

- [ ] **4.2 Update Presence op 3 visible côté Discord**
  Lancer un jeu → la présence Discord doit afficher `Joue à <jeu>`.
  Retourner au HOME → statut passe `idle` avec `Sur le menu HOME`.

- [ ] **4.3 RESUME après coupure WiFi brève**
  Couper le WiFi 10 s puis rétablir : `log.txt` doit montrer un cycle
  `heartbeat non acquitté` → `RESUME envoyé` → `RESUMED` (pas
  `READY`). Aucun réenvoi complet de session.

- [ ] **4.4 Reconnexion après veille console (couvercle fermé)**
  Fermer le couvercle 30 s (WiFi coupé par le système), rouvrir.
  Reconnexion propre attendue, RESUME si le seq est encore valide.

- [ ] **4.5 Certificat racine expiré**
  Retourner la 3DS à une date d'il y a > 10 ans (system settings). Le
  handshake TLS doit échouer avec un log `certificat refusé:
  MBEDTLS_X509_BADCERT_FUTURE/EXPIRED`. **Ne pas** contourner la
  vérification, remonter le message clair vers l'utilisateur.

### 5. IPC sysmodule ↔ plugin

- [ ] **5.1 `svcConnectToPort("presence:d")` depuis un jeu**
  Lancer un jeu où le plugin `.3gx` est chargé. Ouvrir Rosalina →
  Process list, vérifier que le plugin est bien injecté. `log.txt`
  du sysmodule doit montrer une session IPC active.

- [ ] **5.2 Requête `GetState` (cmd 0x0001)**
  Depuis le plugin, en cours de jeu : le toast doit apparaître à
  l'entrée dans le jeu (`Vous jouez a <jeu>`), une seule fois.

- [ ] **5.3 Session tuée quand le sysmodule redémarre**
  Tuer le sysmodule (via Rosalina → Kill process) pendant qu'un jeu
  tourne. Vérifier que le plugin passe en retry (pas de crash du jeu).

### 6. Rendu overlay (`overlay_draw.cpp`)

- [ ] **6.1 Rendu classique 2D**
  Jeu 2D standard (ex. eShop de démo). Le toast doit s'afficher en
  haut de l'écran du haut avec animation de glissement.

- [ ] **6.2 Jeu 3D stéréo**
  Activer la 3D physique sur un jeu compatible (Mario 3D Land, etc.).
  Observation attendue : CTRPF ne dessine que le framebuffer gauche
  → le toast apparaît « dédoublé » en 3D. Documenter le comportement,
  ne pas cacher le TODO.

- [ ] **6.3 Wide mode (800 px)**
  Jeu compatible wide (Metroid Prime Federation Force, VC…). Toast
  positionné correctement ? Ou coupé à droite ?

### 7. Empreinte mémoire

- [ ] **7.1 Heap 3 MiB suffisant**
  Sur o3DS (mémoire application 64 MiB) : lancer un jeu gros consommateur
  (Monster Hunter 3G, Xenoblade). Vérifier qu'aucun `malloc` n'échoue
  côté sysmodule (`log.txt` : chercher `-2` retourné par
  `discordGatewayInit`).

- [ ] **7.2 Cohabitation Rosalina + plugin loader**
  Sysmodule + plugin + Rosalina + jeu chargés en même temps. Aucun
  crash / freeze pendant 15 min de jeu.

### 8. Installeur / désinstallation

- [ ] **8.1 Install propre (CIA)**
  Depuis FBI → « Load ticket ». Vérifier que tous les fichiers sont
  écrits (log de l'installeur).

- [ ] **8.2 Install propre (.3dsx via HBL)**
  Idem via HBL.

- [ ] **8.3 Désinstallation complète**
  Menu X « Désinstaller ». Vérifier que `/luma/sysmodules/`,
  `/luma/plugins/default.3gx`, `/3ds/tricord-presence/` sont propres
  après reboot.

- [ ] **8.4 exheader.bin résiduel — cas B de `docs/BOOT_AUTORUN.md`**
  Si l'utilisateur a testé la méthode B (injection NS), la
  désinstallation doit proposer et exécuter la suppression de
  `/luma/titles/0004013000008002/exheader.bin`.

- [ ] **8.5 Rotation des logs — session longue**
  Laisser tourner le sysmodule au moins 24 h sur une console active
  (jeux + HOME menu alternés). Vérifier ensuite sur SD :
  - `sdmc:/3ds/tricord-presence/log.txt` doit exister (fichier courant)
  - `log.1.txt`, `log.2.txt`, `log.3.txt` doivent exister s'il y a eu
    beaucoup d'activité (chacun ≤ ~130 KiB, cf. `LOG_MAX` dans `log.c`)
  - `log.4.txt` doit être ABSENT (LOG_KEEP=3)
  - Un `rename()` mid-rotation ne doit pas se solder par 2 fichiers
    identiques (bug potentiel sur newlib devoptab sdmc: non-atomique) —
    inspecter les tailles et les premières lignes pour valider.

---

## Comment remplir

```
| # | Résultat | Console | Firmware | Notes |
|---|---|---|---|---|
| 1.1 | PASS | n3DS XL | 11.17.0-50U | rc=0, log OK |
| 1.2 | FAIL | idem | idem | rc=D8E06406 depuis HBL |
| 1.3 | PASS | idem | idem | pas relancé, conforme |
| 2.1 | PASS | idem | idem | psInit rc=0 |
| ... | ... | ... | ... | ... |
```

Coller les 5-10 dernières lignes de `sdmc:/3ds/tricord-presence/log.txt` en
annexe pour chaque test qui échoue.

---

## À faire de l'agent (Emergent) pour ce document

- [x] Fournir la liste exhaustive des TODO(hardware) restants du code.
- [x] Décrire la procédure exacte de test pour chacun.
- [ ] **Exécuter ces tests** — hors périmètre (pas d'accès hardware).
- [ ] Mettre à jour ce fichier avec les résultats — à faire par
      l'utilisateur ou un mainteneur avec accès console.
