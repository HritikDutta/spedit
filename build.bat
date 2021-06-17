@echo off

set includes= /I dependencies\Glad\include     ^
              /I dependencies\GLFW\include     ^
              /I dependencies\stb\include      ^
              /I src

set libs= dependencies\GLFW\lib\glfw3.lib ^
          dependencies\Glad\lib\glad.lib  ^
          Shell32.lib                     ^
          User32.lib                      ^
          Gdi32.lib                       ^
          OpenGL32.lib                    ^
          msvcrt.lib                      ^
          Comdlg32.lib

set objs= dependencies\stb\obj\*.obj


set compile_flags=/O2 /EHsc /std:c++17 /DNDEBUG /cgthreads8 /MP7 /GL
set link_flags=/NODEFAULTLIB:LIBCMT /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /LTCG

set debug_compile_flags=/Zi /EHsc /std:c++17 /DDEBUG /cgthreads8 /MP7
set debug_link_flags=/NODEFAULTLIB:LIBCMT /DEBUG

rem Source
cl %compile_flags% /c src/misc/*.cpp %includes% & ^
cl %compile_flags% /c src/math/*.cpp %includes% & ^
cl %compile_flags% /c src/engine/*.cpp %includes% & ^
cl %compile_flags% /c src/platform/*.cpp %includes% & ^
cl %compile_flags% /c src/program/*.cpp %includes% & ^
cl %compile_flags% /c src/json/*.cpp %includes% & ^
cl %compile_flags% /c src/main.cpp %includes% & ^

rem Resources
rc resources.rc

rem Link and Make Executable
link *.obj %objs% %libs% *.res /OUT:spedit.exe %link_flags%

rem Delete Intermediate Files
del *.obj
del *.res