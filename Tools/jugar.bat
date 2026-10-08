@echo off
rem Lanza BLACKLINE en modo juego (sin editor): menú principal (desde ahí, la misión 1 "Amanecer roto").
rem Uso: doble clic, o "Tools\jugar.bat [Mapa]". Mapa de pruebas: Tools\jugar_pruebas.bat
set "UE=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "MAP=%~1"
if "%MAP%"=="" set "MAP=/Game/Maps/Menu/L_MainMenu"
start "" "%UE%" "%~dp0..\Blackline.uproject" %MAP% -game -windowed -ResX=1600 -ResY=900 -nosplash
