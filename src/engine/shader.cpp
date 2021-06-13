#include "shader.h"

#include <iostream>
#include <string>
#include <glad/glad.h>
#include "math/types.h"
#include "platform/fileio.h"

void Shader::LoadShader(const char* filepath, Shader::Type type)
{
    std::string source = LoadFile(filepath);
    const char* src = source.c_str();
    GLenum glShaderType = GL_FRAGMENT_SHADER + (int) type;

    u32 shader = glCreateShader(glShaderType);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus != GL_TRUE)
    {
        GLsizei logLength = 0;
        GLchar message[1024];
        glGetShaderInfoLog(shader, 1024, &logLength, message);
        std::cout << message << '\n';
        return;
    }

    shaderIDs[(int) type] = shader;
}

void Shader::LoadSource(const char* source, Shader::Type type)
{
    GLenum glShaderType = GL_FRAGMENT_SHADER + (int) type;

    u32 shader = glCreateShader(glShaderType);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus != GL_TRUE)
    {
        GLsizei logLength = 0;
        GLchar message[1024];
        glGetShaderInfoLog(shader, 1024, &logLength, message);
        std::cout << message << '\n';
        return;
    }

    shaderIDs[(int) type] = shader;
}

void Shader::Compile()
{
    program = glCreateProgram();
    glAttachShader(program, shaderIDs[0]);
    glAttachShader(program, shaderIDs[1]);
    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE)
    {
        GLsizei logLength = 0;
        GLchar message[1024];
        glGetProgramInfoLog(program, 1024, &logLength, message);
        std::cout << message << '\n';
    }

    glDeleteShader(shaderIDs[0]);
    glDeleteShader(shaderIDs[1]);
}

void Shader::Bind()
{
    glUseProgram(program);
}

static int GetUniformLocation(Shader* shader, const std::string& uniformName)
{
    if (shader->uniformLocations.find(uniformName) != shader->uniformLocations.end())
        return shader->uniformLocations[uniformName];
        
    int uniformLocation = glGetUniformLocation(shader->program, uniformName.c_str());
    shader->uniformLocations[uniformName] = uniformLocation;
    return uniformLocation;
}

void Shader::SetUniform1f(const std::string& uniformName, f32 value)
{
    glUniform1f(GetUniformLocation(this, uniformName), value);
}

void Shader::SetUniform1i(const std::string& uniformName, s32 value)
{
    glUniform1i(GetUniformLocation(this, uniformName), value);
}

void Shader::SetUniform1iv(const std::string& uniformName, u32 count, s32* data)
{
    glUniform1iv(GetUniformLocation(this, uniformName), count, data);
}

void Shader::SetUniform2f(const std::string& uniformName, f32 v0, f32 v1)
{
    glUniform2f(GetUniformLocation(this, uniformName), v0, v1);
}

void Shader::SetUniform2fv(const std::string& uniformName, int count, f32* vs)
{
    glUniform2fv(GetUniformLocation(this, uniformName), count, vs);
}

void Shader::SetUniform3f(const std::string& uniformName, f32 v0, f32 v1, f32 v2)
{
    glUniform3f(GetUniformLocation(this, uniformName), v0, v1, v2);
}

void Shader::SetUniform3fv(const std::string& uniformName, int count, f32* vs)
{
    glUniform3fv(GetUniformLocation(this, uniformName), count, vs);
}

void Shader::SetUniform4f(const std::string& uniformName, f32 v0, f32 v1, f32 v2, f32 v3)
{
    glUniform4f(GetUniformLocation(this, uniformName), v0, v1, v2, v3);
}

void Shader::SetUniform4fv(const std::string& uniformName, int count, f32* vs)
{
    glUniform4fv(GetUniformLocation(this, uniformName), count, vs);
}

void Shader::SetUniformMatrix4(const std::string& uniformName, bool transpose, const Matrix4& mat)
{
    glUniformMatrix4fv(GetUniformLocation(this, uniformName), 1, transpose, (f32*) mat.data);
}