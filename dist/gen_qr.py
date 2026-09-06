#!/usr/bin/env python3
"""
gen_qr.py — génère dist/qr.html, page autonome contenant le QR code de
téléchargement de tricord-presence-installer.cia pour FBI "Remote Install".

Usage :
    python3 dist/gen_qr.py [URL] [--out dist/qr.html]
    QR_URL=https://.../installer.cia ./build.sh

Si aucune URL n'est fournie, on injecte un placeholder :
    https://github.com/USER/REPO/releases/latest/download/tricord-presence-installer.cia
qu'il faudra remplacer avant distribution (rappel visible dans la page).

Dépendance : `pip install qrcode` (module Python pur, aucun binaire).

Rendu : QR en SVG inline (pas d'image PNG externe), tout est dans un seul
fichier HTML < 10 KiB, ouvrable hors ligne. Aucun réseau, aucun tracker.

Comment FBI "Remote Install" lit ce QR :
    - FBI ouvre l'appareil photo, scanne le QR
    - Le contenu du QR est une URL HTTPS pointant vers un .cia
    - FBI télécharge et installe (redirections 302 tolérées)
    - Voir docs/FBI_INSTALL.md pour la procédure utilisateur complète
"""

import argparse
import sys
from pathlib import Path

DEFAULT_URL = "https://github.com/USER/REPO/releases/latest/download/tricord-presence-installer.cia"
PLACEHOLDER_MARKER = "USER/REPO"  # détection du placeholder pour l'avertissement

HTML_TEMPLATE = """<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<title>TriCord Presence — QR Remote Install (FBI)</title>
<style>
  body {{ font-family: system-ui, -apple-system, sans-serif; margin: 2em auto; max-width: 640px; text-align: center; color: #222; }}
  .qr {{ display: inline-block; padding: 1em; background: #fff; border: 1px solid #ddd; border-radius: 8px; }}
  .qr svg {{ display: block; width: 320px; height: 320px; }}
  .warn {{ margin: 1em 2em; padding: 1em; background: #fff4d1; border: 1px solid #e0c060; border-radius: 6px; color: #6a5000; }}
  code {{ background: #eee; padding: 0 4px; border-radius: 3px; font-size: 0.9em; }}
</style>
</head>
<body>
<h1>TriCord Presence — Installation via FBI</h1>
<p>Scannez ce QR code avec FBI &rsaquo; <em>Remote Install</em> pour installer <code>tricord-presence-installer.cia</code>.</p>

<div class="qr">{qr_svg}</div>

<p><small>URL cible : <code>{url_escaped}</code></small></p>
{warn_html}

<h2>Après installation</h2>
<ol style="text-align: left; max-width: 480px; margin: 1em auto;">
  <li>Ouvrez « TriCord Presence Installer » depuis le menu HOME.</li>
  <li>Appuyez sur A pour installer (copie du sysmodule + plugin sur la SD).</li>
  <li>Activez « Plugin loader » dans Rosalina (L + Bas + Select).</li>
  <li>Activez « Enable loading external FIRMs and modules » dans le config
      menu de Luma (SELECT au boot).</li>
</ol>
<p><small>Voir <code>docs/FBI_INSTALL.md</code> et <code>README.md</code> pour la procédure complète.</small></p>
</body>
</html>
"""

WARN_HTML = """<div class="warn"><strong>⚠️ URL placeholder détectée.</strong> Régénérez ce QR avec la vraie URL de release avant de le distribuer.
<br><br><strong>Linux / macOS :</strong>
<br><code>python3 dist/gen_qr.py https://vraie.url/installer.cia --out dist/qr.html</code>
<br><br><strong>Windows (cmd / PowerShell) :</strong>
<br><code>python dist\\gen_qr.py https://vraie.url/installer.cia --out dist\\qr.html</code>
<br>ou plus court : <code>dist\\gen_qr.cmd https://vraie.url/installer.cia --out dist\\qr.html</code>
<br><br><em>Note : sur Windows, ne pas oublier le suffixe <code>.py</code> ni utiliser des antislash <code>\\</code>. Le wrapper <code>gen_qr.cmd</code> évite les deux pièges.</em></div>"""


def escape_html(s: str) -> str:
    return (s.replace("&", "&amp;").replace("<", "&lt;")
              .replace(">", "&gt;").replace('"', "&quot;"))


def make_qr_svg(url: str) -> str:
    """Retourne un SVG inline (chaîne) qui encode l'URL en QR.

    Utilise le module `qrcode` (pur Python) avec le backend SvgPathImage,
    qui rend un <path> unique — beaucoup plus léger qu'un raster PNG et
    scalable sans perte."""
    try:
        import qrcode
        import qrcode.image.svg
    except ImportError:
        sys.stderr.write("ERREUR: le module Python 'qrcode' est manquant. `pip install qrcode`.\n")
        sys.exit(2)

    factory = qrcode.image.svg.SvgPathImage
    img = qrcode.make(url, image_factory=factory, box_size=10, border=2)

    import io
    buf = io.BytesIO()
    img.save(buf)
    svg = buf.getvalue().decode("utf-8")
    # Retirer la déclaration XML/DOCTYPE (on est dans du HTML5), garder juste <svg>...</svg>.
    idx = svg.find("<svg")
    if idx >= 0:
        svg = svg[idx:]
    return svg


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("url", nargs="?", default=None,
                    help="URL du .cia à distribuer (défaut : placeholder GitHub)")
    ap.add_argument("--out", default="dist/qr.html", help="chemin de sortie HTML")
    args = ap.parse_args()

    url = args.url or DEFAULT_URL
    warn = WARN_HTML if PLACEHOLDER_MARKER in url else ""

    svg = make_qr_svg(url)

    html = HTML_TEMPLATE.format(
        qr_svg=svg,
        url_escaped=escape_html(url),
        warn_html=warn,
    )

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(html, encoding="utf-8")

    print(f"écrit : {out_path} ({out_path.stat().st_size} octets) — URL: {url}")


# ---------------------------------------------------------------------------
# Selftest sans réseau ni dépendance
# ---------------------------------------------------------------------------
def _selftest():
    """python3 gen_qr.py --selftest : vérifie que la génération HTML+SVG
    tient debout sur une URL bidon (nécessite le paquet `qrcode`)."""
    import tempfile, os
    with tempfile.TemporaryDirectory() as td:
        out = os.path.join(td, "qr.html")
        # Simuler argv
        sys.argv = ["gen_qr.py", "https://example.com/installer.cia", "--out", out]
        main()
        html = open(out, encoding="utf-8").read()
        assert "<svg" in html and "</svg>" in html, "SVG manquant"
        assert "https://example.com/installer.cia" in html, "URL manquante dans le HTML"
        assert "USER/REPO" not in html, "placeholder marker à tort"
        assert "warn" not in html.lower() or "avant de le distribuer" not in html, "avertissement placeholder à tort"

        # Test placeholder -> avertissement présent
        sys.argv = ["gen_qr.py", "--out", out]
        main()
        html = open(out, encoding="utf-8").read()
        assert "avant de le distribuer" in html, "avertissement placeholder manquant"

    print("selftest OK")


if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "--selftest":
        _selftest()
    else:
        main()
