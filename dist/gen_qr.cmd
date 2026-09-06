@echo off
REM Wrapper Windows pour dist\gen_qr.py — évite l'erreur classique
REM "can't open file 'gen_qr'" quand on oublie l'extension .py.
REM
REM Usage identique au script Python :
REM     dist\gen_qr.cmd https://mon.url/installer.cia --out dist\qr.html
REM     dist\gen_qr.cmd --selftest
REM
REM Ce fichier trouve gen_qr.py à côté de lui, quel que soit le CWD.
REM Fallback : essaie "python" puis "py -3" (launcher officiel Windows
REM sur les installs Python.org quand "python" n'est pas dans le PATH).
setlocal
set "SCRIPT_DIR=%~dp0"
where python >nul 2>&1
if %ERRORLEVEL%==0 (
    python "%SCRIPT_DIR%gen_qr.py" %*
) else (
    where py >nul 2>&1
    if %ERRORLEVEL%==0 (
        py -3 "%SCRIPT_DIR%gen_qr.py" %*
    ) else (
        echo ERREUR: ni "python" ni "py" trouves dans le PATH.
        echo Installez Python 3 depuis https://www.python.org/downloads/
        exit /b 1
    )
)
endlocal
