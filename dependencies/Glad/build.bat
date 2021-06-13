@echo off

if not exist lib md lib

cl /c src\glad.c /I include
lib *.obj /out:lib\glad.lib

del *.obj