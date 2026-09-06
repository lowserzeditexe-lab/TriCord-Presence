# Prompt pour Emergent (Fable 5) — finalisation du projet TriCord Presence

Copie-colle (et adapte si besoin) le bloc ci-dessous dans Emergent.

---

Je te fournis le squelette d'un projet homebrew Nintendo 3DS (CFW
Luma3DS) appelé "TriCord Presence", en 3 composants distincts. Ton
rôle est de COMPLÉTER le code là où des commentaires `TODO(hardware)`
l'indiquent, PAS de tout réécrire depuis zéro — respecte l'architecture
et les fichiers fournis.

## Environnement à installer d'abord

- devkitARM + devkitPro complet, groupe `3ds-dev` (via `dkp-pacman`)
- Packages : `3ds-libctru`, `3ds-citro3d`, `general-tools`, `3dstools`,
  `bannertool`, `makerom`
- Port `mbedtls` pour 3ds (TLS) et port `wslay` pour 3ds (WebSocket) —
  si aucun port officiel n'existe déjà pour devkitARM, cherche des
  sources tierces déjà utilisées par des homebrews Discord existants
  (ex: le projet "Sleepy Discord" / "yourWaifu/sleepy-discord" compile
  déjà pour 3DS et peut servir de référence ou être réutilisé tel quel)
- `3gxtool` (https://github.com/Nanquitas/3gxtool) pour compiler le
  plugin Luma3DS `.3gx`

## Contraintes à respecter impérativement

1. **L'installeur reste en mode CLI pur pour cette version** : pas
   d'interface graphique, pas de romfs avec assets visuels, juste la
   console texte `consoleInit` déjà en place dans
   `installer/source/main.c`. N'ajoute pas de banner/icône soignée ni
   de menu graphique — ce sera fait dans une itération future.
2. Ne supprime aucun commentaire `TODO(hardware)` sans l'avoir
   réellement résolu et testé — si tu ne peux pas valider un point sur
   hardware/émulateur, laisse le TODO en place plutôt que de deviner
   silencieusement une implémentation qui a l'air correcte.
3. Le sysmodule et le plugin touchent à l'OS bas niveau de la 3DS
   (exheader, IPC custom, hook GSP) : documente clairement, pour
   chaque point incertain que tu résous, la source que tu as utilisée
   (3dbrew.org, code d'un projet homebrew existant, etc.) pour qu'on
   puisse le revérifier.
4. **Ne jamais demander de token à l'utilisateur.** Depuis cette révision,
   le token vient en priorité de `tricord_account_bridge.c` (compte déjà
   connecté dans TriCord, `sdmc:/3ds/TriCord/accounts`) — voir
   `docs/ACCOUNT_BRIDGE.md` pour le mécanisme complet. `config.txt` n'est
   qu'un repli de dev/test (tests hôte sans console, dépannage manuel) :
   ne construis aucun flux d'installeur qui présente `config.txt` ou une
   saisie de token comme le fonctionnement normal pour l'utilisateur final.
5. Le risque ToS Discord (connexion permanente avec un token
   utilisateur, pas un bot) est un choix déjà assumé — pas la peine de
   proposer une alternative "bot officiel", juste implémenter proprement.
6. Ne code jamais de token/secret en dur dans le code, et ne le fais
   jamais transiter par un service réseau externe autre que la Gateway
   Discord elle-même (tout le pont de compte reste on-device).

## Ce qu'il reste à finaliser (par priorité)

1. `sysmodule/source/tricord_account_bridge.c` (nouveau) : c'est la
   priorité n°1 de cette itération. Vérifier sur hardware/Citra-Azahar
   que le service `ps:ps` est bien accessible à un sysmodule tiers avec
   les droits listés dans `tricord_presenced.rsf`, et que
   `PS_EncryptDecryptAes(..., PS_KEYSLOT_0D, ...)` déchiffre bien un vrai
   `accounts` généré par une build TriCord réelle (comparer avec le
   comportement de `TriCord/source/core/config.cpp::Config::load()`).
   Corriger le `Dependency: ps:` placeholder dans le `.rsf` (Title ID
   exact du module PS, non confirmé dans ce squelette — voir le TODO
   dans le fichier).
2. `sysmodule/source/discord_gateway.c`, fonction
   `discordGatewayRefreshTokenIfChanged` : actuellement un stub qui
   détecte un changement de compte/déconnexion côté TriCord mais ne fait
   qu'un cycle exit/init brutal dans `main.c`. Implémenter une fermeture
   propre (CLOSE 1000) avant réIDENTIFY, et décider du comportement quand
   TriCord se déconnecte complètement (repasser en mode "attente de
   compte" sans tuer le sysmodule).
3. `sysmodule/source/apt_monitor.c` : implémenter
   `aptMonitorGetActiveTitleId` (Title ID du jeu au premier plan) —
   vérifier les codes de commande exacts sur 3dbrew.org (services APT,
   AM, éventuellement pm:app).
4. `sysmodule/source/discord_gateway.c` (Gateway elle-même) : finir de
   valider HELLO/IDENTIFY/HEARTBEAT/RESUME/Update Presence contre un vrai
   compte (le rapport précédent, `RAPPORT.md`, n'a validé que jusqu'à
   IDENTIFY faute de token disponible dans cet environnement — avec le
   pont de compte, un vrai token est enfin disponible sur hardware réel).
5. `plugin/source/overlay_draw.c` : implémenter le hook GSP réel pour
   dessiner un HUD "🎮 <jeu> vient d'être lancé" par-dessus le jeu hôte au
   moment d'un changement d'état (c'est la brique pour les futures pop-up
   demandées par l'utilisateur). Inspire-toi de plugins `.3gx` open-source
   existants qui font déjà de l'overlay texte plutôt que de partir de zéro.
6. `plugin/Makefile` : brancher la vraie chaîne de compilation 3gxtool.
7. `sysmodule/source/ipc_server.c` et `plugin/source/presence_client.c` :
   implémenter la requête/réponse IPC réelle entre les deux (actuellement
   stubs qui ne transportent pas de vraies données) — c'est ce canal qui
   portera l'événement "jeu lancé" jusqu'au HUD du point 5.
8. `sysmodule/source/title_db.c` : charger une vraie base Title ID →
   nom de jeu (s'inspirer de la base utilisée par le projet 3DS-RPC,
   qui utilise une version modifiée de 3dsdb).
9. `installer/source/main.c` : implémenter réellement `copyFile` et
   `ensureDir` (actuellement stubs qui ne font que logger). L'installeur
   n'a plus besoin d'écran de saisie de token pour l'usage normal ; garder
   au maximum un écran optionnel "mode dépannage" qui écrit
   `config.txt` pour les cas où `tricord_account_bridge` échoue (TriCord
   non installé, format de compte non reconnu, etc.).

## Livrable attendu

- Le même arbre de fichiers, complété (pas de refonte de structure)
- `build.sh` qui compile les 3 composants sans erreur avec devkitARM installé
- Un rapport texte listant : ce qui a été implémenté, ce qui reste en
  TODO faute de pouvoir être validé sans hardware/Citra réel, et tout
  point où tu as dû deviner un comportement non documenté.
