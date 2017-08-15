@echo off
cd %~dp0
del *.exe *.ilk *.obj *.pdb *.suo
rmdir shaders /s /q