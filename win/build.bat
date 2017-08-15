@echo off
cd %~dp0
xcopy "../src/shaders" "shaders" /i /y > nul
set output=vox.exe
set cflags=/nologo /MT /Zi
set libs=LIBCMT.lib User32.lib Gdi32.lib OpenGL32.lib
set lflags=/OUT:%output% /SUBSYSTEM:CONSOLE /INCREMENTAL:NO

cl ..\src\win32_vox.c %cflags% /link %libs% %lflags%
