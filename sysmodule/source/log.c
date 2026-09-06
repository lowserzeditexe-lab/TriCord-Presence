#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

#ifdef __3DS__
#include <3ds.h>
typedef LightLock log_lock_t;
#define LOG_LOCK_INIT(l)   LightLock_Init(l)
#define LOG_LOCK(l)        LightLock_Lock(l)
#define LOG_UNLOCK(l)      LightLock_Unlock(l)
#define LOG_TICK_MS()      ((unsigned long long)(svcGetSystemTick() / SYSCLOCK_ARM11 / 1000))
#define LOG_DIR            "sdmc:/3ds/tricord-presence"
#else
/* Compilation hôte (tools/host_log_test) : pas de LightLock, pas de tick 3DS.
 * On garde l'API identique, mais le lock devient un no-op (le test est
 * monothread) et l'horodatage est un compteur monotone. Le chemin de log
 * est repris depuis $TRICORD_LOG_DIR (par défaut /tmp/tricord-log-test). */
#include <stdlib.h>
#include <time.h>
typedef int log_lock_t;
/* Utilise (void)(l) pour éviter le warning "unused parameter" tout en
 * gardant la même API que sur 3DS. */
#define LOG_LOCK_INIT(l)   ((void)(l))
#define LOG_LOCK(l)        ((void)(l))
#define LOG_UNLOCK(l)      ((void)(l))
static unsigned long long s_hostTickMs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
}
#define LOG_TICK_MS()      s_hostTickMs()
#define LOG_DIR            (getenv("TRICORD_LOG_DIR") ? getenv("TRICORD_LOG_DIR") : "/tmp/tricord-log-test")
#endif

/* --------------------------------------------------------------------------
 * Rotation multi-fichier
 * --------------------------------------------------------------------------
 *
 *   log.txt      <- fichier "courant", écrit à chaque logPrintf
 *   log.1.txt    <- rotation précédente
 *   log.2.txt    <- avant-avant
 *   log.3.txt    <- avant-avant-avant (le plus ancien conservé)
 *
 * LOG_MAX = 128 KiB par fichier -> total conservé max = 4 × 128 KiB = 512 KiB.
 * L'ancienne version stockait un seul fichier de 256 KiB puis supprimait tout ;
 * l'historique de log utile sur une console qui tourne longtemps était perdu.
 *
 * La rotation est déclenchée APRÈS chaque écriture qui fait franchir la
 * limite (pas AVANT : ça évite de rester bloqué si la limite est déjà
 * dépassée au démarrage). C'est le comportement de la plupart des loggers
 * "size-based rotation".
 * ------------------------------------------------------------------------ */
#define LOG_MAX          (128 * 1024)
#define LOG_KEEP         3       /* nombre de fichiers .1..N conservés */

static log_lock_t s_lock;

/* Construit un chemin de log : "<LOG_DIR>/log.txt" si index=0,
 * "<LOG_DIR>/log.<index>.txt" sinon. Retourne le pointeur vers buf. */
static char *logPath(char *buf, size_t bufSize, int index) {
    if (index == 0)
        snprintf(buf, bufSize, "%s/log.txt", LOG_DIR);
    else
        snprintf(buf, bufSize, "%s/log.%d.txt", LOG_DIR, index);
    return buf;
}

/* rotate() : cascade log.(N-1).txt -> log.N.txt (le plus ancien saute).
 * Puis log.txt -> log.1.txt. Appelée sous s_lock. */
static void rotate(void) {
    char from[256], to[256];

    /* Supprimer le plus ancien pour libérer son slot. rename() écrase
     * silencieusement sous POSIX/newlib, donc ce remove() serait normalement
     * redondant — mais devoptab sdmc: en libctru ne garantit pas
     * l'écrasement, on le fait explicitement. */
    remove(logPath(from, sizeof(from), LOG_KEEP));

    for (int i = LOG_KEEP - 1; i >= 1; i--) {
        logPath(from, sizeof(from), i);
        logPath(to, sizeof(to), i + 1);
        rename(from, to); /* ignore erreur : fichier absent = OK */
    }

    logPath(from, sizeof(from), 0);
    logPath(to, sizeof(to), 1);
    rename(from, to);
}

void logInit(void) {
    LOG_LOCK_INIT(&s_lock);
    /* Ne PAS supprimer le log courant au démarrage : la rotation naturelle
     * s'en occupera si nécessaire. Ça préserve l'historique du run précédent
     * (utile pour diagnostiquer un crash au boot). */
}

void logPrintf(const char *fmt, ...) {
    LOG_LOCK(&s_lock);

    char path[256];
    logPath(path, sizeof(path), 0);
    FILE *f = fopen(path, "a");
    if (!f) { LOG_UNLOCK(&s_lock); return; }

    fprintf(f, "[%llu] ", LOG_TICK_MS());
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);

    /* Vérifier la taille APRÈS l'écriture : ftell() sur un flux ouvert en
     * mode "a" renvoie la position courante = taille actuelle du fichier
     * (POSIX §7.14.5). Alternative struct stat.st_size, mais ftell évite un
     * appel système supplémentaire pendant qu'on tient déjà le fichier. */
    long size = ftell(f);
    fclose(f);

    if (size > LOG_MAX) rotate();
    LOG_UNLOCK(&s_lock);
}

#ifdef LOG_HOST_TEST
/* Points d'introspection pour le test hôte (tools/host_log_test/). */
long logTestFileSize(int index) {
    char path[256];
    logPath(path, sizeof(path), index);
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (long)st.st_size;
}

const char *logTestPath(int index) {
    static char path[256];
    return logPath(path, sizeof(path), index);
}

int logTestMax(void) { return LOG_MAX; }
int logTestKeep(void) { return LOG_KEEP; }
#endif
