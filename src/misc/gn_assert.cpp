#ifdef DEBUG

#include <iostream>

bool DebugMessage(const char* message, const char* file, int line)
{
    std::cout << file << '(' << line << ") Assertion Failed: " << message << '\n';
    __debugbreak();
    return false;
}

#endif