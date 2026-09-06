#!/usr/bin/env python3
"""
inject_ns_dep.py — ajoute un Title ID à la DependencyList de l'exheader d'un
sysmodule 3DS. Prévu spécifiquement pour patcher l'exheader de NS
(0004013000008002) afin que PM lance automatiquement notre sysmodule au boot,
mais fonctionne sur n'importe quel exheader NCCH.

Layout : 3dbrew NCCH_ExHeader#SCI
    offset 0x000..0x1FF : SCI (System Control Info)
        0x00 : appTitle (8 octets)
        0x08 : reserved (5 octets)
        0x0D : flag (1 octet)
        0x0E : remasterVersion (u16)
        0x10 : textCodeSetInfo (12 octets)
        0x1C : stackSize (u32)
        0x20 : readOnlyCodeSetInfo (12 octets)
        0x2C : reserved (4 octets)
        0x30 : dataCodeSetInfo (12 octets)
        0x3C : bssSize (u32)
        0x40 : DependencyList : 48 u64 = 0x180 octets  <-- ce qu'on modifie
        0x1C0 : SystemInfo (SaveDataSize u64, JumpId u64, reserved 0x30)
    offset 0x200..0x3FF : ACI (Access Control Info)
    ...

La signature (accessDescSignature, 0x100 octets à offset 0x400) N'EST PAS
recalculée : Luma3DS substitue l'exheader avant vérification par PM (comme le
font BootNTR Selector, PKSM…). C'est cohérent avec le fait que Luma n'a
manifestement pas la clé RSA privée pour signer.

Références :
- 3dbrew NCCH_ExHeader : https://www.3dbrew.org/wiki/NCCH/Extended_Header
- Luma3DS `k11_extension/source/svc/GetProcessInfo.c` (usage du TID)
- BootNTR Selector : équivalent de cet outil, historique de la méthode

TODO(hardware) : jamais testé sur console dans cet environnement. À valider
sur une 3DS non-principale AVEC accès de secours (GodMode9 amorçable).
"""

import argparse, struct, sys

EXH_SIZE = 0x400
DEP_LIST_OFFSET = 0x40
DEP_LIST_COUNT = 48
DEP_LIST_END = DEP_LIST_OFFSET + DEP_LIST_COUNT * 8  # 0x1C0

def parse_tid(s: str) -> int:
    """Accepte '000401300F000102' ou '0004013000008002' ou '0x...'. Retourne u64."""
    s = s.strip().lower()
    if s.startswith("0x"):
        s = s[2:]
    if len(s) != 16 or any(c not in "0123456789abcdef" for c in s):
        raise argparse.ArgumentTypeError(f"TID invalide : {s!r} (attendu : 16 hex)")
    return int(s, 16)

def load_exh(path: str) -> bytearray:
    with open(path, "rb") as f:
        data = bytearray(f.read())
    if len(data) != EXH_SIZE:
        raise SystemExit(f"exheader {path} : taille {len(data)} != {EXH_SIZE}")
    return data

def get_dep_list(exh: bytearray) -> list[int]:
    tids = []
    for i in range(DEP_LIST_COUNT):
        (tid,) = struct.unpack_from("<Q", exh, DEP_LIST_OFFSET + i * 8)
        tids.append(tid)
    return tids

def set_dep_list(exh: bytearray, tids: list[int]) -> None:
    assert len(tids) == DEP_LIST_COUNT
    for i, tid in enumerate(tids):
        struct.pack_into("<Q", exh, DEP_LIST_OFFSET + i * 8, tid)

def inject(exh: bytearray, add_tid: int, verbose: bool = True) -> bool:
    """Ajoute add_tid à la première case libre (== 0). Idempotent. Retourne
    True si une modif a été faite."""
    deps = get_dep_list(exh)

    if add_tid in deps:
        if verbose:
            print(f"note : {add_tid:016X} est déjà présent (rien à faire)")
        return False

    try:
        slot = deps.index(0)
    except ValueError:
        raise SystemExit(f"aucun slot libre dans DependencyList (max {DEP_LIST_COUNT})")

    deps[slot] = add_tid
    set_dep_list(exh, deps)
    if verbose:
        print(f"OK : {add_tid:016X} injecté au slot {slot} de la DependencyList")
    return True

def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--in", dest="src", required=True, help="exheader.bin d'origine (0x400 o)")
    ap.add_argument("--out", dest="dst", required=True, help="exheader.bin patché à écrire")
    ap.add_argument("--add", dest="add", required=True, type=parse_tid,
                    help="Title ID (hex 16 car.) à ajouter en dépendance")
    ap.add_argument("--dump", action="store_true",
                    help="Affiche la DependencyList avant/après")
    args = ap.parse_args()

    exh = load_exh(args.src)
    if args.dump:
        print("Avant :")
        for i, tid in enumerate(get_dep_list(exh)):
            if tid:
                print(f"  [{i:2d}] {tid:016X}")
    inject(exh, args.add)
    if args.dump:
        print("Après :")
        for i, tid in enumerate(get_dep_list(exh)):
            if tid:
                print(f"  [{i:2d}] {tid:016X}")
    with open(args.dst, "wb") as f:
        f.write(exh)
    print(f"écrit : {args.dst} ({len(exh)} octets)")

# ------------------------------------------------------------------------
# Tests unitaires basiques (le fichier est aussi runnable en mode test)
# ------------------------------------------------------------------------
def _selftest():
    """python3 inject_ns_dep.py --selftest : vérifie sur des fixtures que la
    logique parse_tid / inject / idempotence est correcte (aucune console
    requise)."""
    # 1) parse_tid
    assert parse_tid("000401300F000102") == 0x000401300F000102
    assert parse_tid("0x000401300F000102") == 0x000401300F000102
    try:
        parse_tid("nope")
    except argparse.ArgumentTypeError:
        pass
    else:
        raise AssertionError("parse_tid aurait dû rejeter 'nope'")

    # 2) fixture : exheader synthétique avec 2 deps déjà présentes
    exh = bytearray(EXH_SIZE)
    struct.pack_into("<Q", exh, DEP_LIST_OFFSET + 0 * 8, 0x0004013000001A02)  # cfg (exemple)
    struct.pack_into("<Q", exh, DEP_LIST_OFFSET + 1 * 8, 0x0004013000002402)  # ac
    deps = get_dep_list(exh)
    assert deps[0] == 0x0004013000001A02
    assert deps[1] == 0x0004013000002402
    assert all(d == 0 for d in deps[2:])

    # 3) inject : ajoute au slot 2
    assert inject(exh, 0x000401300F000102, verbose=False) is True
    deps = get_dep_list(exh)
    assert deps[2] == 0x000401300F000102

    # 4) idempotence
    assert inject(exh, 0x000401300F000102, verbose=False) is False
    deps = get_dep_list(exh)
    assert deps[2] == 0x000401300F000102
    assert deps[3] == 0

    # 5) plein : refuser d'ajouter si aucun slot libre
    for i in range(2, DEP_LIST_COUNT):
        struct.pack_into("<Q", exh, DEP_LIST_OFFSET + i * 8, 0xDEADBEEF00000000 | i)
    try:
        inject(exh, 0xCAFEBABE12345678, verbose=False)
    except SystemExit:
        pass
    else:
        raise AssertionError("inject aurait dû refuser un exheader plein")

    print("selftest OK")

if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "--selftest":
        _selftest()
    else:
        main()
