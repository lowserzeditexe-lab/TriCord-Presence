@echo off
setlocal EnableExtensions

rem TriCord Presence - Windows build launcher
rem Requires the official devkitPro Windows installation with MSYS2 and 3DS Development.

set "ROOT=%~dp0"
set "MSYS_BASH=%ProgramFiles%\devkitPro\msys2\usr\bin\bash.exe"
if not exist "%MSYS_BASH%" set "MSYS_BASH=%USERPROFILE%\devkitPro\msys2\usr\bin\bash.exe"
if not exist "%MSYS_BASH%" (
  echo.
  echo [ERREUR] MSYS2 devkitPro introuvable.
  echo Installe devkitPro avec ^"3DS Development^", puis relance ce script.
  echo.
  exit /b 1
)

rem Convert the Windows project path to a POSIX path accepted by MSYS2.
for /f "usebackq delims=" %%P in (`"%MSYS_BASH%" -lc "cygpath -u '%ROOT%'"`) do set "ROOT_POSIX=%%P"
if not defined ROOT_POSIX (
  echo [ERREUR] Impossible de convertir le chemin du projet pour MSYS2.
  exit /b 1
)

"%MSYS_BASH%" -lc "cd '$ROOT_POSIX' && ./build_windows.sh"
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" (
  echo.
  echo Build echoue. Consulte la sortie ci-dessus.
  exit /b %RC%
)

echo.
echo Build termine. Les fichiers sont dans:
echo   %ROOT%dist\
exit /b 0
