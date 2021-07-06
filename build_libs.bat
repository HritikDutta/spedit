@echo off
set compile_flags=/O2 /EHsc /std:c++17 /DNDEBUG /cgthreads8 /MP7 /GL
set debug_compile_flags=/Zi /EHsc /std:c++17 /DDEBUG /cgthreads8 /MP7

pushd dependencies

cl %compile_flags% /c stb\src\*.cpp /I stb\include /Fostb\obj\

popd