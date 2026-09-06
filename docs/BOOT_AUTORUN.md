# Lancement au boot de `tricord_presenced`

Luma3DS mainline **ne lance jamais tout seul** un sysmodule custom au boot :
`/luma/sysmodules/<TID>.cxi` n'est utilisé que quand **Process Manager (PM)
demande à lancer ce Title ID**. Il faut donc que quelqu'un déclenche `pm:app
LaunchTitle` pour notre TID `000401300F000102` — soit à chaque démarrage,
soit une fois pour toutes via une dépendance de sysmodule.

Ce document explique les 3 approches connues, leur validation, et pourquoi
`tricord-presence` choisit à ce jour la moins invasive.

## Techniques recensées

### A) Lancement manuel via l'installeur (RETENU par défaut)

C'est ce que fait déjà `installer/source/main.c::launchSysmoduleNow()` :
`svcControlService(STEAL_CLIENT_SESSION, "pm:app")` (SVC 0xB0, extension
Luma3DS, cf. `Luma3DS rosalina/include/csvc.h`) puis `pm:app LaunchTitle`
(cmd `0x0001`, cf. `libctru/services/pmapp.c`).

**Avantages** :
- Aucune modification du système, aucun exheader à générer.
- Fonctionne dès que Luma3DS ≥ 12 est configuré avec « Enable loading
  external FIRMs and modules » (SELECT au boot).
- Se désactive en supprimant simplement `/luma/sysmodules/*.cxi`.
- Réversible sans risque.

**Inconvénient** :
- L'utilisateur doit **relancer l'installeur (bouton "lancer maintenant")
  après chaque redémarrage**. Ce n'est pas un vrai autoboot.

### B) Injection dans les dépendances de NS (le "vrai" autoboot)

`Luma3DS` supporte `/luma/titles/<TID>/exheader.bin` : quand PM lance
`<TID>`, Luma remplace son exheader natif par ce fichier. En modifiant
l'exheader de **NS** (`0004013000008002`, un des tout premiers sysmodules
lancés au boot) pour y ajouter notre TID dans la liste de `Dependencies`,
PM lancera automatiquement notre sysmodule.

**Avantages** :
- Vrai boot automatique, transparent pour l'utilisateur.

**Inconvénients** :
- Nécessite l'**exheader.bin original de NS**, qui est différent par
  firmware (o3DS 4.x/11.x, n3DS 8.x/11.x, TWL, DSi…). L'installeur ne peut
  pas embarquer une copie sans risquer d'écraser un exheader plus récent
  et de rendre NS inutilisable jusqu'à réparation.
- Cet installeur écrivait précédemment un tel exheader
  (`sdmc:/luma/titles/0004013000008002/exheader.bin`, cf. constante
  `EXH_NS_FILE` dans `installer/source/main.c`). L'approche a été
  **abandonnée** dans une révision précédente pour cette raison — le mode
  désinstallation propose d'ailleurs toujours de le supprimer s'il traîne.
- Fragile face aux futures mises à jour firmware (chaque update recompile
  NS avec un nouvel exheader).

L'outil `tools/inject_ns_dep.py` (fourni pour les utilisateurs avancés) sait
faire cette modification proprement. Voir §« Utilisation avancée » ci-dessous.

### C) Fork Luma3DS avec autolaunch (le "vrai" autoboot, mais invasif)

Certains forks de Luma3DS (« Luma3DS-autorun », etc.) ajoutent une liste
`/luma/autolaunch.txt` lue au démarrage. Cette approche est hors périmètre
de ce projet — elle demanderait à l'utilisateur d'installer une build Luma
non-officielle.

## Recommandation

Utiliser **l'approche A** (état actuel du code). Le coût utilisateur est
faible (relancer un `.3dsx` / `.cia` de moins de 200 Ko après chaque
reboot) et le risque de brique est nul.

L'approche B reste disponible pour les utilisateurs qui savent :
- extraire l'exheader NS de leur console (`GodMode9` : `[8:] SD / gm9 /
  scripts / GM9Megascript / More GodMode9 scripts / Extract NAND exheader`),
- vérifier qu'ils ont un accès `luma`/`config` bootable en cas d'échec,
- gérer eux-mêmes la réapplication après chaque mise à jour firmware.

## Utilisation avancée — outil `tools/inject_ns_dep.py`

```bash
# 1. Copier l'exheader NS extrait de votre console dans un fichier local
#    (fichier binaire de 0x400 octets, voir 3dbrew NCCH exheader).
cp /path/to/ns_orig.exh /tmp/ns_orig.exh

# 2. Générer un nouvel exheader avec notre TID en dépendance.
python3 tools/inject_ns_dep.py \
        --in  /tmp/ns_orig.exh \
        --out /tmp/ns_patched.exh \
        --add 000401300F000102

# 3. Copier le résultat sur la SD (avec la 3DS éteinte,
#    OU depuis GodMode9, PAS depuis l'installeur pour éviter tout risque).
mkdir -p /path/to/SD/luma/titles/0004013000008002
cp /tmp/ns_patched.exh /path/to/SD/luma/titles/0004013000008002/exheader.bin
```

**En cas de brique-partielle** (NS refuse de se lancer et la console reste
au logo Luma) : supprimer `/luma/titles/0004013000008002/` depuis GodMode9
ou depuis un PC. La console redémarre normalement.

`inject_ns_dep.py` **valide** que :
- l'entrée fait bien 0x400 octets (taille standard d'un exheader NCCH) ;
- la section `SCI.DependencyList` a bien de la place pour un TID de plus
  (elle en accepte 48 max, cf. 3dbrew NCCH_ExHeader#SCI) ;
- le TID à ajouter n'est pas déjà présent (idempotent) ;
- le hash sha256 de l'ACI est recalculé côté PM — **non**, ce n'est pas
  fait ici, `signed_ncsd_stuff` n'est pas modifié par Luma. Le comportement
  observé sur d'autres projets (BootNTR Selector, PKSM…) est que Luma ne
  vérifie pas la signature de l'exheader qu'il substitue, donc ne pas la
  recalculer suffit.

**TODO(hardware)** : validé aucunement dans cet environnement (pas de
console, pas de vraie build NS). L'outil est fourni « best-effort » pour
les utilisateurs qui savent ce qu'ils font ; tester d'abord sur une
console non-principale avec accès de secours à la SD.
