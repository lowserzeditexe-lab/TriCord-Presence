#pragma once
// Journal texte sur SD (sdmc:/3ds/tricord-presence/log.txt) : un sysmodule
// n'a aucune sortie visible, c'est le seul moyen de déboguer sur console.
//
// Rotation : quand log.txt dépasse 128 KiB, il est renommé en log.1.txt,
// et les rotations précédentes cascadent (log.1 -> log.2, log.2 -> log.3,
// log.3 supprimé). Au maximum 512 KiB conservés sur la SD.
// Cf. log.c pour le détail (macros LOG_MAX / LOG_KEEP).
void logInit(void);
void logPrintf(const char *fmt, ...);
