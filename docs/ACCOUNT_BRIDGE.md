# Pont de compte avec TriCord (`tricord_account_bridge`)

## Pourquoi ce module existe

L'ancienne version de ce projet demandait à l'utilisateur de coller son
token Discord dans `config.txt`. Ce module supprime cette étape : il lit
le compte **déjà connecté dans TriCord** et récupère son token
automatiquement, au démarrage du sysmodule et périodiquement ensuite.

## Comment ça marche réellement

1. TriCord stocke ses comptes dans `sdmc:/3ds/TriCord/accounts` :
   `{"currentIndex":N,"accounts":[{"name":"...","token":"..."},...]}`,
   chiffré en AES-CTR (IV nul) avec `PS_KEYSLOT_0D` — voir
   `TriCord/source/core/config.cpp`, fonction `encrypt_decrypt_data`.
2. `PS_KEYSLOT_0D` est une clé **liée à la console**, pas au compte Discord
   ni à TriCord. N'importe quel process homebrew tournant sur la même 3DS,
   avec accès au service `ps:ps`, peut appeler `PS_EncryptDecryptAes` avec
   ce keyslot — exactement ce que fait TriCord lui-même.
3. `tricord_account_bridge.c` reproduit cet appel sur ce même fichier,
   extrait le token du compte à `currentIndex`, et le donne à
   `discord_gateway.c` pour l'IDENTIFY (op 2).

**Ce n'est donc pas une intégration officielle ni un canal IPC fourni par
TriCord** (TriCord n'expose aucune API de plugin à ce jour) : c'est une
lecture locale, sur le même appareil, du même fichier que TriCord lit et
écrit lui-même, avec le même service système public.

## Ce que ça implique concrètement

- **Aucune saisie de token par l'utilisateur** tant que TriCord est déjà
  connecté : objectif atteint.
- **Deux sessions Gateway simultanées sur le même compte** : celle de
  TriCord (client complet) et celle de ce sysmodule (présence seule).
  Discord gère nativement le multi-session (comme app mobile + desktop en
  même temps) ; ce n'est pas un problème technique, mais ça reste deux
  connexions "self-bot" ouvertes en parallèle sur le même token — voir
  `ARCHITECTURE.md` pour la remarque CGU déjà actée sur ce projet.
- **Dépendance à la structure interne de TriCord.** Si une future version
  de TriCord change le format du fichier `accounts`, le chemin, ou
  l'algorithme de chiffrement, ce module cesse de fonctionner et doit être
  resynchronisé sur le nouveau `config.cpp`. Il n'y a pas de garantie de
  stabilité puisqu'il ne s'agit pas d'une API publique de TriCord.
- **Pas de verrou de fichier entre les deux process.** Une lecture peut
  tomber pile pendant que TriCord réécrit `accounts` (changement de compte,
  déconnexion). D'où le retry avec backoff prévu dans
  `discordGatewayRefreshTokenIfChanged()` plutôt qu'un échec fatal.

## Ce qui reste à faire (TODO(emergent))

- [ ] Vérifier le Title ID exact du module `ps` pour le `Dependency:` du
  RSF (placeholder à corriger dans `tricord_presenced.rsf`, sans quoi
  `GetServiceHandle("ps:ps")` peut bloquer indéfiniment).
- [ ] Tester sur console réelle que `ps:ps` est accessible à un sysmodule
  tiers (non vérifié ici, pas d'émulateur ni de hardware disponibles pour
  ce squelette — cf. `RAPPORT.md` sur les mêmes limites déjà rencontrées
  pour `discord_gateway.c`).
- [ ] Implémenter la reconnexion réelle dans
  `discordGatewayRefreshTokenIfChanged()` (actuellement un stub qui
  détecte le changement mais ne fait qu'un cycle exit/init complet dans
  `main.c`, à affiner : CLOSE propre 1000, pas de RESUME possible après un
  changement de compte).
- [ ] Décider du comportement si `accountBridgeGetToken` réussit mais que
  le token est en fait invalide côté Discord (compte déconnecté d'un
  Discord sans que TriCord l'ait détecté) : aujourd'hui la Gateway
  recevra un close 4004 et s'arrêtera (comportement existant de
  `discord_gateway.c`), à vérifier que ça ne boucle pas en erreur.
