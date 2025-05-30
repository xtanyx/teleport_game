@echo off
set COMPILER_FLAGS=/MTd /Od /Zi /FC /nologo /W4 /wd4100 /wd4189
cl %COMPILER_FLAGS% win32_teleport_game.c user32.lib gdi32.lib winmm.lib