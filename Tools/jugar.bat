@echo off
rem Lanza BLACKLINE en modo juego (sin editor) en la misión 1 "Amanecer roto" (nivel gris).
rem Uso: doble clic, o "Tools\jugar.bat [Mapa]". Mapa de pruebas: Tools\jugar_pruebas.bat
set "UE=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "MAP=%~1"
if "%MAP%"=="" set "MAP=/Game/Maps/M01/L_M01_AmanecerRoto"
start "" "%UE%" "%~dp0..\Blackline.uproject" %MAP% -game -windowed -ResX=1600 -ResY=900 -nosplash
