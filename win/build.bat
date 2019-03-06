@echo off
cd %~dp0
xcopy "../src/shaders" "shaders" /i /y > nul
set output=..\win\vox.exe
set cflags=/nologo /MT /Zi /EHsc
set libs=LIBCMT.lib User32.lib Gdi32.lib OpenGL32.lib
set lflags=/OUT:%output% /SUBSYSTEM:CONSOLE /INCREMENTAL:NO
set srcs=win32_vox.c vox_main.c vox_jobs.c vox_mesher.c vox_noise.c vox_render.c vox_world.c

pushd ..\src\
cl %srcs% %cflags% /link %libs% %lflags%
popd
