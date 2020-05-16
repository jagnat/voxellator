@echo off
cd %~dp0
xcopy "../src/shaders" "shaders" /i /y > nul
set output=..\win\vox.exe
set cflags=/nologo /MT /Zi /EHsc
set libs=LIBCMT.lib User32.lib Gdi32.lib OpenGL32.lib
set lflags=/OUT:%output% /SUBSYSTEM:CONSOLE /INCREMENTAL:NO
set srcs=..\src\win32_layer.c ..\src\main.c ..\src\jobs.c ..\src\noise.c ..\src\render.c ..\src\world.c

cl %srcs% %cflags% /link %libs% %lflags%
