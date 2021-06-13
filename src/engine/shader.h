#pragma once

#include <string>
#include "containers/hash_table.h"
#include "math/types.h"

struct Shader
{
    // In order according to gl shader codes
    enum class Type
    {
        FRAGMENT_SHADER,
        VERTEX_SHADER,
        NUM_TYPES,
    };

    void LoadShader(const char* filepath, Type type);
    void LoadSource(const char* source, Type type);
    void Compile();
    
    void Bind();

    void SetUniform1f(const std::string& uniformName, f32 value);
    void SetUniform1i(const std::string& uniformName, s32 value);
    void SetUniform1iv(const std::string& uniformName, u32 count, s32* data);

    void SetUniform2f(const std::string& uniformName, f32 v0, f32 v1);
    void SetUniform2fv(const std::string& uniformName, int count, f32* vs);

    void SetUniform3f(const std::string& uniformName, f32 v0, f32 v1, f32 v2);
    void SetUniform3fv(const std::string& uniformName, int count, f32* vs);

    void SetUniform4f(const std::string& uniformName, f32 v0, f32 v1, f32 v2, f32 v3);
    void SetUniform4fv(const std::string& uniformName, int count, f32* vs);

    void SetUniformMatrix4(const std::string& uniformName, bool transpose, const Matrix4& mat);

    u32 shaderIDs[(int) Type::NUM_TYPES];
    u32 program { 0 };
    gn::hash_table<std::string, int, std::hash<std::string>> uniformLocations;
};