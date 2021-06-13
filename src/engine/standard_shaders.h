#pragma once

constexpr char uiQuadVertShader[] =
"#version 330 core\n"

"layout(location = 0) in vec3 position;\n"
"layout(location = 1) in vec2 texCoord;\n"
"layout(location = 2) in vec4 color;\n"
"layout(location = 3) in float texIndex;\n"

"uniform sampler2D u_texs[5];\n"

"out vec2 v_texCoord;\n"
"out vec4 v_color;\n"
"out float v_texIndex;\n"

"void main()\n"
"{\n"
"    v_texCoord = texCoord;\n"
"    v_color = color;\n"
"    v_texIndex = texIndex;\n"
"    gl_Position = vec4(position, 1.0);\n"
"}"
;

constexpr char uiQuadFragShader[] =
"#version 330 core\n"

"in vec2 v_texCoord;\n"
"in vec4 v_color;\n"
"in float v_texIndex;\n"

"uniform sampler2D u_texs[5];\n"

"out vec4 color;\n"

"void main()\n"
"{\n"
"    color = v_color;\n"

"    int index = int(v_texIndex);\n"
"    if (index >= 0)\n"
"        color *= texture(u_texs[index], v_texCoord);\n"
"}"
;

constexpr char meshVertShader[] =
"#version 330 core\n"

"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 normal;\n"
"layout (location = 2) in vec2 texCoord;\n"

"uniform mat4 u_mvp;\n"
"uniform sampler2D u_texs[32];\n"

"out vec2 v_texCoord;\n"

"void main()\n"
"{\n"
"    gl_Position = u_mvp * vec4(position, 1.0);\n"
"    v_texCoord = texCoord;\n"
"}"
;

constexpr char meshFragShader[] =
"#version 330 core\n"

"in vec2 v_texCoord;\n"

"uniform mat4 u_mvp;\n"
"uniform sampler2D u_texs[32];\n"

"out vec4 color;\n"

"void main()\n"
"{\n"
"    color = texture(u_texs[0], v_texCoord);\n"
"}"
;

constexpr char skyboxVertShader[] =
"#version 440 core\n"

"layout (location = 0) in vec3 position;\n"

"layout (std140, binding = 0) uniform u_Camera\n"
"{\n"
"    mat4 projection;\n"
"    mat4 view;\n"
"    mat4 viewProjection;\n"
"};\n"

"uniform mat4 u_viewWithoutTranslation;\n"

"out vec3 v_texCoord;\n"

"void main()\n"
"{\n"
"    v_texCoord = position;\n"
"    vec4 pos = projection * u_viewWithoutTranslation * vec4(position, 1);\n"
"    gl_Position = pos.xyww;\n"
"}"
;

constexpr char skyboxFragShader[] =
"#version 440 core\n"

"in vec3 v_texCoord;\n"

"layout (std140, binding = 0) uniform u_Camera\n"
"{\n"
"    mat4 projection;\n"
"    mat4 view;\n"
"    mat4 viewProjection;\n"
"};\n"

"uniform samplerCube u_skybox;\n"

"out vec4 o_color;\n"

"void main()\n"
"{\n"
"    o_color = texture(u_skybox, v_texCoord);\n"
"}"
;