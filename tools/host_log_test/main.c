/*
 * Test hôte de la rotation de logs (sysmodule/source/log.c).
 *
 * Scénarios validés :
 *   1) init idempotent (2 appels d'affilée ne cassent pas)
 *   2) écriture normale (log.txt existe, taille > 0, autres absents)
 *   3) rotation simple : après avoir écrit LOG_MAX + un peu, log.txt est
 *      recréé petit et log.1.txt existe avec l'ancien contenu
 *   4) rotation cascade : après ~5 × LOG_MAX écrits, on a log.txt,
 *      log.1.txt, log.2.txt, log.3.txt, mais PAS log.4.txt (LOG_KEEP = 3)
 *   5) le plus vieux (log.3.txt) est effectivement rotaté hors du set
 *      (son contenu change entre 2 rotations)
 *   6) init sans reset : après un run précédent qui a laissé du contenu,
 *      un nouveau logInit + une écriture APPEND au log.txt existant, pas
 *      d'écrasement.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log.h"

/* Helpers exposés par log.c compilé avec -DLOG_HOST_TEST. */
long        logTestFileSize(int index);
const char *logTestPath(int index);
int         logTestMax(void);
int         logTestKeep(void);

static int failures = 0;
#define CHECK(cond, msg, ...) do { \
    if (!(cond)) { printf("FAIL: " msg "\n", ##__VA_ARGS__); failures++; } \
    else { printf("OK:   " msg "\n", ##__VA_ARGS__); } \
} while (0)

/* Écrit une ligne d'environ ~90 octets pour saturer rapidement. */
static void spam(int n) {
    for (int i = 0; i < n; i++) {
        logPrintf("iteration %06d - lorem ipsum dolor sit amet consectetur adipiscing elit sed do", i);
    }
}

/* Lit la première ligne d'un fichier (pour vérifier qu'on a bien roté du
 * "vieux" contenu vers .1, pas du récent). */
static void readFirstLine(int index, char *out, size_t outSize) {
    FILE *f = fopen(logTestPath(index), "r");
    if (!f) { snprintf(out, outSize, "(absent)"); return; }
    if (!fgets(out, (int)outSize, f)) snprintf(out, outSize, "(vide)");
    fclose(f);
    /* strip \n */
    size_t n = strlen(out);
    if (n && out[n - 1] == '\n') out[n - 1] = '\0';
    if (n > 50) out[50] = '\0'; /* truncate for readability */
}

int main(void) {
    printf("=== Test rotation log.c ===\n");
    printf("LOG_MAX = %d octets, LOG_KEEP = %d fichiers\n",
           logTestMax(), logTestKeep());

    /* Scénario 1 : init idempotent. */
    logInit();
    logInit();
    CHECK(1, "logInit() appelé 2 fois sans crash");

    /* Scénario 2 : écriture normale. */
    logPrintf("hello world");
    CHECK(logTestFileSize(0) > 0,      "log.txt existe et a du contenu (%ld o)", logTestFileSize(0));
    CHECK(logTestFileSize(1) == -1,    "log.1.txt absent avant rotation");

    /* Scénario 3 : rotation simple. Chaque logPrintf écrit ~120 octets,
     * il faut donc ~1100 écritures pour dépasser 128 KiB. On en fait 1500
     * pour être sûr. */
    spam(1500);
    long s0 = logTestFileSize(0);
    long s1 = logTestFileSize(1);
    CHECK(s1 > 0,                      "log.1.txt existe après rotation (%ld o)", s1);
    CHECK(s0 >= 0 && s0 < logTestMax(),
                                       "log.txt est reparti à petite taille (%ld o < %d)", s0, logTestMax());

    /* Sauvegarde de la première ligne du plus vieux fichier pour le scénario 5. */
    char oldest_before[128];
    readFirstLine(logTestKeep(), oldest_before, sizeof(oldest_before));

    /* Scénario 4 : rotation cascade. On envoie l'équivalent de 5 × LOG_MAX. */
    spam(6000);
    CHECK(logTestFileSize(0) > 0,      "log.txt (0) présent après cascade");
    CHECK(logTestFileSize(1) > 0,      "log.1.txt (1) présent après cascade");
    CHECK(logTestFileSize(2) > 0,      "log.2.txt (2) présent après cascade");
    CHECK(logTestFileSize(3) > 0,      "log.3.txt (3) présent après cascade");
    CHECK(logTestFileSize(4) == -1,    "log.4.txt (4) ABSENT (LOG_KEEP=%d bien respecté)", logTestKeep());

    /* Scénario 5 : le plus vieux a effectivement changé. */
    char oldest_after[128];
    readFirstLine(logTestKeep(), oldest_after, sizeof(oldest_after));
    CHECK(strcmp(oldest_before, oldest_after) != 0,
                                       "log.3.txt a été rotaté hors du set (contenu remplacé)");

    /* Chaque fichier roté doit tenir sous LOG_MAX + une ligne de "queue"
     * (une écriture peut légèrement dépasser avant qu'on rote). Marge : 1 KiB. */
    long allowed = logTestMax() + 1024;
    for (int i = 1; i <= logTestKeep(); i++) {
        long sz = logTestFileSize(i);
        CHECK(sz <= allowed,           "log.%d.txt <= %ld (mesuré %ld)", i, allowed, sz);
    }

    /* Scénario 6 : ré-init sans reset. On note la taille courante, on
     * réinit, on écrit une ligne, on vérifie que la taille a augmenté (pas
     * remise à zéro comme le faisait l'ancienne version). */
    long before = logTestFileSize(0);
    logInit();
    logPrintf("marker after reinit");
    long after = logTestFileSize(0);
    CHECK(after > before,              "logInit() ne détruit pas l'historique (avant=%ld, après=%ld)", before, after);

    printf("\n=== %s (%d échec(s)) ===\n", failures ? "ECHEC" : "SUCCES", failures);
    return failures ? 1 : 0;
}
