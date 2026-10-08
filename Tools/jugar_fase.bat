@echo off
rem Empieza la misión 1 en una fase concreta (sin briefing, con el objetivo de esa fase).
rem Uso: Tools\jugar_fase.bat Fase2   (Fase1 inserción, Fase2 patio, Fase3 control, Fase4 calle, Objetivo local)
set "UE=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "FASE=%~1"
if "%FASE%"=="" (
  echo Fases: Fase1 ^(insercion^)  Fase2 ^(patio^)  Fase3 ^(control^)  Fase4 ^(calle^)  Objetivo ^(local^)
  set /p FASE=Fase: 
)
start "" "%UE%" "%~dp0..\Blackline.uproject" /Game/Maps/M01/L_M01_AmanecerRoto -game -windowed -ResX=1600 -ResY=900 -nosplash -BLStart=%FASE%
