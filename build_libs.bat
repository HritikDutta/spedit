@echo off
set compile_flags=/O2 /EHsc /std:c++17 /DNDEBUG /cgthreads8 /MP7 /GLz

pushd dependencies

cl %compile_flags% /c stb\src\*.cpp /I stb\include /Fostb\obj\

popd