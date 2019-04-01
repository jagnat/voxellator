@echo off
cd %~dp0
xcopy "../src/shaders" "shaders" /i /y > nul
set output=..\win\vox.exe
set cflags=/nologo /MT /Zi /EHsc
set libs=LIBCMT.lib User32.lib Gdi32.lib OpenGL32.lib
set lflags=/OUT:%output% /SUBSYSTEM:CONSOLE /INCREMENTAL:NO
set srcs=..\src\win32_vox.c ..\src\vox_main.c ..\src\vox_jobs.c ..\src\vox_mesher.c ..\src\vox_noise.c ..\src\vox_render.c ..\src\vox_world.c

cl %srcs% %cflags% /link %libs% %lflags%
