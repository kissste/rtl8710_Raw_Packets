@echo off
call paths.bat
start JLink.exe -Device CORTEX-M3 -If SWD -Speed 1000 flasher\RTL_RunRAM.JLinkScript
