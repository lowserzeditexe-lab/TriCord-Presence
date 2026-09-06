#pragma once
#include "presence_state.h"
#ifndef __3DS__
#include <stdint.h>
typedef int32_t Result;
#define R_FAILED(r) ((r) < 0)
#endif

// Connexion websocket persistante à la Gateway Discord
// (wss://gateway.discord.gg/?v=10&encoding=json), dans un thread dédié du
// sysmodule. Pile : sockets libctru (soc:U) + mbedtls (TLS, port devkitPro
// 3ds-mbedtls) + wslay (WebSocket, port devkitPro 3ds-wslay) + jansson (JSON,
// port devkitPro 3ds-jansson) — la même combinaison wslay/TLS que le client
// Discord 3DS non-officiel de yourWaifu (Sleepy Discord).
//
// Flux implémenté (cf discord_gateway.c) :
//  - HELLO (op 10) -> intervalle de heartbeat, premier battement avec jitter
//  - IDENTIFY (op 2) : token récupéré via tricord_account_bridge (compte déjà
//    connecté dans TriCord, sdmc:/3ds/TriCord/accounts) — repli sur
//    sdmc:/3ds/tricord-presence/config.txt en dev/test uniquement, voir
//    loadToken() dans le .c. JAMAIS en dur dans le binaire ; pas de champ
//    "intents" (token utilisateur, pas bot)
//  - HEARTBEAT (op 1) / HEARTBEAT_ACK (op 11), détection de connexion zombie
//  - READY -> session_id + resume_gateway_url ; RESUME (op 6) après coupure,
//    RECONNECT (op 7), INVALID_SESSION (op 9)
//  - UPDATE PRESENCE (op 3) : en jeu = status "online" + activité type 0
//    "Playing <jeu>" ; menu HOME = status "idle" + statut personnalisé
//    (type 4) "Sur le menu HOME"
//  - codes de fermeture 4004/4010-4014 = fatals (pas de reconnexion)
//
// Rappel : token utilisateur -> usage "self-bot", risque ToS assumé.
//
// Le fichier compile aussi sur hôte Linux (-DGATEWAY_HOST_TEST, voir
// tools/host_gateway_test/) pour valider TLS + WebSocket + HELLO/IDENTIFY
// contre la vraie Gateway sans console.

Result discordGatewayInit(void);
void discordGatewayUpdatePresence(const presence_state_t *state);
void discordGatewayExit(void);

// À appeler périodiquement (ex. toutes les 10-15s) depuis main.c : détecte
// un changement de compte / une déconnexion côté TriCord. Renvoie true si
// un changement a été détecté (la fonction a alors déjà écrit le nouveau
// token, remis à zéro session_id/seq/resume_on_reconnect et positionné
// want_close ; le thread réseau ferme la session courante avec CLOSE 4000
// et reconnecte au tour suivant avec un IDENTIFY neuf).
// Cas déconnexion complète (accounts absent/illisible) : le token est vidé
// et le thread réseau retombe en "veille active" (attente 5s + retry) au
// lieu de mourir sur 4004. Le prochain refresh qui trouve un token relance
// une session normale.
#include <stdbool.h>
bool discordGatewayRefreshTokenIfChanged(void);
