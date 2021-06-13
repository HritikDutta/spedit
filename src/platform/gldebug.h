#ifdef DEBUG

#pragma once

#include <iostream>
#include <glad/glad.h>

#define hd_assert(x) if (!(x)) { std::cout << "ASSERTION FAILED (" << __FILE__ << ":" << __LINE__ << "): " << #x << std::endl; __debugbreak(); }

void APIENTRY GLDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity,
                            GLsizei length, const char* message, const void* userParam);

#endif