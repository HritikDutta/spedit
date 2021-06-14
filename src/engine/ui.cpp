#include "ui.h"

#ifdef DEBUG
#include <iostream>
#endif

#include <string_view>
#include <stb/stb_truetype.h>
#include <stb/stb_image.h>
#include <glad/glad.h>
#include "misc/gn_assert.h"
#include "platform/application.h"
#include "platform/fileio.h"
#include "math/types.h"
#include "shader.h"
#include "standard_shaders.h"

namespace UI
{

static constexpr s32 maxQuadCount = 10000;
static constexpr s32 maxTexCount = 5;

struct Vertex
{
    Vector3 position;
    Vector2 texCoord;
    Vector4 color;
    f32  texIndex;
};

static struct
{
    u32 vao, vbo, ibo;
    Shader quadShader;
    u32 batchQuadCount;
    Vertex* quadVerticesBuffer, *quadVerticesPtr;

    u32 batchTextures[maxTexCount];
    u32 nextActiveTexSlot;  // Should always be lower than max allowed fonts

    ID hot, active;
} uiData;

void Init()
{
    uiData.quadShader.LoadSource(uiQuadVertShader, Shader::Type::VERTEX_SHADER);
    uiData.quadShader.LoadSource(uiQuadFragShader, Shader::Type::FRAGMENT_SHADER);
    uiData.quadShader.Compile();

    uiData.quadVerticesBuffer = new Vertex[maxQuadCount * 4];

    glGenVertexArrays(1, &uiData.vao);
    glBindVertexArray(uiData.vao);

    glGenBuffers(1, &uiData.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, uiData.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * maxQuadCount * 4, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (const void*) offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex), (const void*) offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(Vertex), (const void*) offsetof(Vertex, color));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, false, sizeof(Vertex), (const void*) offsetof(Vertex, texIndex));

    u32 indices[maxQuadCount * 6];
    u32 offset = 0;
    for (int i = 0; i < maxQuadCount * 6; i += 6)
    {
        indices[i + 0] = offset + 0;
        indices[i + 1] = offset + 1;
        indices[i + 2] = offset + 2;
        indices[i + 3] = offset + 2;
        indices[i + 4] = offset + 3;
        indices[i + 5] = offset + 0;
        offset += 4;
    }

    glGenBuffers(1, &uiData.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiData.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    uiData.batchQuadCount = 0;
    uiData.nextActiveTexSlot = 0;

    uiData.hot = uiData.active = UIInvalid();
}

void Begin()
{
    uiData.batchQuadCount = 0;
    uiData.nextActiveTexSlot = 0;
    uiData.quadVerticesPtr = uiData.quadVerticesBuffer;

    glDisable(GL_DEPTH_TEST);
}

void End()
{
    if (uiData.batchQuadCount == 0)
        return;

    uiData.quadShader.Bind();

    for (int i = 0; i < uiData.nextActiveTexSlot; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, uiData.batchTextures[i]);
    }

    s32 activeSlots[maxTexCount] { 0, 1, 2, 3, 4 };
    uiData.quadShader.SetUniform1iv("u_texs", uiData.nextActiveTexSlot, activeSlots);

    glBindVertexArray(uiData.vao);

    // Update Data
    GLsizeiptr size = (u8*)uiData.quadVerticesPtr - (u8*)uiData.quadVerticesBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, uiData.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, uiData.quadVerticesBuffer);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiData.ibo);
    glDrawElements(GL_TRIANGLES, 6 * uiData.batchQuadCount, GL_UNSIGNED_INT, nullptr);

    glEnable(GL_DEPTH_TEST);
}

void Shutdown()
{
    delete[] uiData.quadVerticesBuffer;
}

bool ID::operator==(const ID& other) const
{
    return primary == other.primary && secondary == other.secondary;
}

bool ID::operator!=(const ID& other) const
{
    return primary != other.primary || secondary != other.secondary;
}

void Font::Load(const std::string_view& filepath, f32 height)
{
    constexpr int BITMAP_SIZE = 256;

    auto contents = LoadBinaryFile(filepath);

    Byte* bitmap = (Byte*) malloc(BITMAP_SIZE * BITMAP_SIZE * 5);
    stbtt_BakeFontBitmap(contents.data(), 0, height, bitmap, BITMAP_SIZE, BITMAP_SIZE, ' ', 128 - ' ', charData);

    // Make a separate texture setting the alpha value as the bitmap value and rest are 1.0f
    Byte* pixels = bitmap + (BITMAP_SIZE * BITMAP_SIZE);
    for (int i = 0; i < BITMAP_SIZE * BITMAP_SIZE; i++)
    {
        pixels[4 * i + 0] = 255;
        pixels[4 * i + 1] = 255;
        pixels[4 * i + 2] = 255;
        pixels[4 * i + 3] = bitmap[i];
    }

    glGenTextures(1, &bitmapTexID);
    glBindTexture(GL_TEXTURE_2D, bitmapTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, BITMAP_SIZE, BITMAP_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    bitmapWidth = bitmapHeight = BITMAP_SIZE;
    fontHeight  = height;

    free(bitmap);
}

void Font::Free()
{
    glDeleteTextures(1, &bitmapTexID);
}

void Image::SetScale(const Vector2& scale)
{
    scaledWidth  = scale.x * width;
    scaledHeight = scale.y * height;
}

bool Image::Load(const std::string_view& filepath)
{
    s32 bytesPP;
    Byte* pixels = stbi_load(filepath.data(), &width, &height, &bytesPP, 0);
    ASSERT(pixels != nullptr);

    if (pixels == nullptr)
        return false;

    int internalFormat, format;
    switch (bytesPP)
    {
        case 3:
        {
            internalFormat = GL_SRGB8;
            format = GL_RGB;
        } break;

        case 4:
        {
            internalFormat = GL_RGBA8;
            format = GL_RGBA;
        } break;

        default:
        {
            ASSERT_NOT_IMPLEMENTED();
        } break;
    }

    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    scaledWidth = width;
    scaledHeight = height;

    stbi_image_free(pixels);

    return true;
}

void Image::Free()
{
    glDeleteTextures(1, &texID);
}

// @Todo: This doesn't work so FIX IT
void SetActive(ID id)
{
    uiData.active = id;
}

static void AddColouredQuad(Application& app, const Rect& rect, Vector4 color)
{
    if (uiData.batchQuadCount >= maxQuadCount)
    {
        End();
        Begin();
    }

    f32 top    = 1.0f - 2.0f * (rect.topLeft.y / app.refScreenHeight);
    f32 left   = 2.0f * (rect.topLeft.x / app.refScreenWidth) - 1.0f;
    f32 right  = 2.0f * ((rect.topLeft.x + rect.size.x) / app.refScreenWidth) - 1.0f;
    f32 bottom = 1.0f - 2.0f * ((rect.topLeft.y + rect.size.y) / app.refScreenHeight);

    f32 z = rect.topLeft.z;

    uiData.quadVerticesPtr->position = Vector3(left, bottom, z);
    uiData.quadVerticesPtr->color = color;
    uiData.quadVerticesPtr->texIndex = -1.0f;
    uiData.quadVerticesPtr++;

    uiData.quadVerticesPtr->position = Vector3(right, bottom, z);
    uiData.quadVerticesPtr->color = color;
    uiData.quadVerticesPtr->texIndex = -1.0f;
    uiData.quadVerticesPtr++;

    uiData.quadVerticesPtr->position = Vector3(right, top, z);
    uiData.quadVerticesPtr->color = color;
    uiData.quadVerticesPtr->texIndex = -1.0f;
    uiData.quadVerticesPtr++;

    uiData.quadVerticesPtr->position = Vector3(left, top, z);
    uiData.quadVerticesPtr->color = color;
    uiData.quadVerticesPtr->texIndex = -1.0f;
    uiData.quadVerticesPtr++;

    uiData.batchQuadCount++;
}

static void AddTexturedQuad(Application& app, const Rect& rect, Vector4 texCoords, u32 texID, Vector4 color)
{
    if (uiData.batchQuadCount >= maxQuadCount)
    {
        End();
        Begin();
    }

    f32 top    = 1.0f - 2.0f * (rect.topLeft.y / app.refScreenHeight);
    f32 left   = 2.0f * (rect.topLeft.x / app.refScreenWidth) - 1.0f;
    f32 right  = 2.0f * ((rect.topLeft.x + rect.size.x) / app.refScreenWidth) - 1.0f;
    f32 bottom = 1.0f - 2.0f * ((rect.topLeft.y + rect.size.y) / app.refScreenHeight);

    f32 z = rect.topLeft.z;

    // Find if texture has already been set to active
    int textureSlot = uiData.nextActiveTexSlot;
    for (int i = 0; i < uiData.nextActiveTexSlot; i++)
    {
        if (uiData.batchTextures[i] == texID)
        {
            textureSlot = i;
            break;
        }
    }

    if (textureSlot == uiData.nextActiveTexSlot)
    {
#       ifdef DEBUG
        if (uiData.nextActiveTexSlot >= maxTexCount)
        {
            std::cout << "Only " << maxTexCount << " textures allowed in a batch\n";
            return;
        }
#       endif

        uiData.batchTextures[textureSlot] = texID;
        uiData.nextActiveTexSlot++;
    }

    uiData.quadVerticesPtr->position = Vector3(left, bottom, z);
    uiData.quadVerticesPtr->color = color;
    uiData.quadVerticesPtr->texCoord = Vector2(texCoords.s, texCoords.v);
    uiData.quadVerticesPtr->texIndex = (f32) textureSlot;
    uiData.quadVerticesPtr++;

    uiData.quadVerticesPtr->position = Vector3(right, bottom, z);
    uiData.quadVerticesPtr->color = color;
    uiData.quadVerticesPtr->texCoord = Vector2(texCoords.u, texCoords.v);
    uiData.quadVerticesPtr->texIndex = (f32) textureSlot;
    uiData.quadVerticesPtr++;

    uiData.quadVerticesPtr->position = Vector3(right, top, z);
    uiData.quadVerticesPtr->color = color;
    uiData.quadVerticesPtr->texCoord = Vector2(texCoords.u, texCoords.t);
    uiData.quadVerticesPtr->texIndex = (f32) textureSlot;
    uiData.quadVerticesPtr++;

    uiData.quadVerticesPtr->position = Vector3(left, top, z);
    uiData.quadVerticesPtr->color = color;
    uiData.quadVerticesPtr->texCoord = Vector2(texCoords.s, texCoords.t);
    uiData.quadVerticesPtr->texIndex = (f32) textureSlot;
    uiData.quadVerticesPtr++;

    uiData.batchQuadCount++;
}

Vector2 GetRenderedTextSize(const std::string_view& text, const Font& font)
{
    Vector2 size { 0.0f, font.fontHeight * 0.75f };
    f32 lineX = 0.0f;

    int lineStart = 0;
    for (int i = 0; i < text.length(); i++)
    {
        if (text[i] == '\n')
        {
            size.y += font.fontHeight;
            size.x = std::max(lineX, size.x);
            lineX = 0.0f;
            lineStart = i + 1;
            continue;
        }

        if (text[i] == '\r')
        {
            size.x = std::max(lineX, size.x);
            lineX = 0.0f;
            continue;
        }
        
        if (text[i] == '\t')
        {
            f32 x = 0.0f, y = 0.0f;
            stbtt_aligned_quad quad;
            stbtt_GetBakedQuad(font.charData, font.bitmapWidth, font.bitmapHeight, 0,   // That's the index for space
                           &x, &y, &quad, 1);
            lineX += x * (4 - ((i - lineStart) % 4));
            continue;
        }

        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(font.charData, font.bitmapWidth, font.bitmapHeight, text[i] - ' ',
                           &lineX, &size.y, &quad, 1);
    }

    size.x = std::max(lineX, size.x);
    return size;
}

void RenderRect(Application& app, const Rect& rect, Vector4 color)
{
    AddColouredQuad(app, rect, color);
}

void RenderText(Application& app, const std::string_view& text, const Font& font,
                Vector4 color, Vector3 topLeft)
{
    Vector2 position { topLeft.x, topLeft.y };
    position.y += font.fontHeight * 0.65f;

    int lineStart = 0;
    for (int i = 0; i < text.length(); i++)
    {
        if (text[i] == '\n')
        {
            position.y += font.fontHeight;
            position.x = topLeft.x;
            lineStart = i + 1;
            continue;
        }

        if (text[i] == '\r')
        {
            position.x = topLeft.x;
            continue;
        }
        
        if (text[i] == '\t')
        {
            f32 x = 0.0f, y = 0.0f;
            stbtt_aligned_quad quad;
            stbtt_GetBakedQuad(font.charData, font.bitmapWidth, font.bitmapHeight, 0,   // That's the index for space
                           &x, &y, &quad, 1);
            position.x += x * (4 - ((i - lineStart) % 4));
            continue;
        }

        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(font.charData, font.bitmapWidth, font.bitmapHeight, text[i] - ' ',
                           &position.x, &position.y, &quad, 1);

        Rect rect;
        rect.topLeft = Vector3(quad.x0, quad.y0, topLeft.z);
        rect.size = Vector2(quad.x1 - quad.x0, quad.y1 - quad.y0);

        Vector4 texCoords(quad.s0, quad.t0, quad.s1, quad.t1);

        AddTexturedQuad(app, rect, texCoords, font.bitmapTexID, color);
    }
}

bool RenderButton(Application& app, ID id, const Rect& rect, Vector4 defaultColor,
                  Vector4 hoverColor, Vector4 pressedColor)
{
    bool result = false;

    Vector4 color = defaultColor;
    if (app.mouseX >= rect.topLeft.x && app.mouseX <= rect.topLeft.x + rect.size.x &&
        app.mouseY >= rect.topLeft.y && app.mouseY <= rect.topLeft.y + rect.size.y)
    {
        if (uiData.hot != id)
            uiData.hot = id;

        if (app.GetMouseButtonDown(MOUSE(1)))
        {
            result = uiData.active != id;
            uiData.active = id;
        }

        if (uiData.active != id)
            color = hoverColor;
    }
    else
    {
        if (uiData.hot == id)
            uiData.hot = UIInvalid();
    }

    if (uiData.active == id)
    {
        if (app.GetMouseButton(MOUSE(1)))
            color = pressedColor;
        else
            uiData.active = UIInvalid();
    }

    RenderRect(app, rect, color);

    return result;
}

void RenderImage(Application& app, Image& image, Vector3 topLeft, Vector4 tint)
{
    Rect rect;
    rect.topLeft = topLeft;
    rect.size = Vector2(image.scaledWidth, image.scaledHeight);

    Vector4 texCoords(0.0f, 0.0f, 1.0f, 1.0f);
    AddTexturedQuad(app, rect, texCoords, image.texID, tint);
}

void RenderTextBox(Application& app, const std::string_view& text, const Font& font,
                   Vector4 fontColor, Vector4 bgColor,
                   Vector2 padding, Vector3 topLeft)
{
    Vector2 size = GetRenderedTextSize(text, font);
    Rect boxRect { Vector3(topLeft.x, topLeft.y, topLeft.z + 0.01f), size + (padding * 2.0f) };
    RenderRect(app, boxRect, bgColor);
    RenderText(app, text, font, fontColor, topLeft + Vector3(padding.x, padding.y, 0.0f));
}

bool RenderTextButton(Application& app, ID id, const std::string_view& text, const Font& font,
                      Vector2 padding, Vector3 topLeft)
{
    Vector2 size = GetRenderedTextSize(text, font);
    Rect btnRect { Vector3(topLeft.x, topLeft.y, topLeft.z + 0.01f), size + (padding * 2.0f) };

    bool res = RenderButton(app, id, btnRect,
                            { 0.0f, 0.0f, 0.0f, 0.0f },
                            { 0.4f, 0.4f, 0.4f, 0.5f },
                            { 0.2f, 0.2f, 0.2f, 0.5f });

    RenderText(app, text, font, { 0.8f, 0.8f, 0.8f, 1.0f },
               topLeft + Vector3(padding.x, padding.y, 0.0f));

    return res;
}

static inline bool IsPressed(Application& app, int key)
{
    static f64 lastPressTime = -1.0;

    if (app.GetKeyDown(key))
    {
        lastPressTime = app.time + 0.5;
        return true;
    }

    if (app.GetKeyUp(key))
    {
        lastPressTime = -1.0;
        return false;
    }

    if (app.GetKey(key) && (app.time - lastPressTime >= 0.1))
    {
        lastPressTime = app.time;
        return true;
    }

    return false;
}

void RenderTextInput(Application& app, ID id, std::string& text, const Font& font,
                     Vector2 padding, Vector3 topLeft, f32 width)
{
    Rect containerRect;
    containerRect.topLeft = topLeft;
    containerRect.size = Vector2(width, font.fontHeight * 0.65f + 2.0f * padding.y);

    RenderRect(app, containerRect, Vector4(0.5f, 0.5f, 0.5f, 1.0f));

    static char character;
    static bool isTyping = false;
    static bool callbackHasBeenSet = false;

    static s32 selection[2] = {};
    static s32 cursorIndex = 0;
    static f64 lastMoveTime = 0.0;

    static Vector4 textColor = Vector4(1.0f);

    if (app.mouseX >= containerRect.topLeft.x && app.mouseX <= containerRect.topLeft.x + containerRect.size.x &&
        app.mouseY >= containerRect.topLeft.y && app.mouseY <= containerRect.topLeft.y + containerRect.size.y)
    {
        if (uiData.hot != id)
            uiData.hot = id;

        if (app.GetMouseButtonDown(MOUSE(1)))
        {
            uiData.active = id;
        }
    }
    else
    {
        if (uiData.hot == id)
            uiData.hot = UIInvalid();

        if (uiData.active == id && app.GetMouseButtonDown(MOUSE(1)))
        {
            // @Todo: This ignores if this has been set active externally
            uiData.active = UIInvalid();
            app.charCallback = [](Application& app, u32 codepoint) {};
            callbackHasBeenSet = false;

#           ifdef DEBUG
            std::cout << "Focus Removed\n";
#           endif
        }
    }

    if (uiData.active == id && !callbackHasBeenSet)
    {
        callbackHasBeenSet = true;
        selection[0] = 0;
        selection[1] = text.length();
        cursorIndex = 0;
        
        app.charCallback = [](Application& app, u32 codepoint)
        {
            character = (char) codepoint;
            isTyping = true;
        };
    }

    Vector2 size = GetRenderedTextSize(text, font);
    std::string_view textToShow = text;
    f32 avgLetterWidth = size.x / text.length();
    s32 maxLetters = (s32) ((width - 2.0f * padding.x) / avgLetterWidth);

    if (uiData.active != id)
    {
        s32 numLettersToShow = (maxLetters < text.length()) ? maxLetters : text.length();
        Vector3 position = Vector3(topLeft.x + padding.x, topLeft.y + padding.y, topLeft.z);
        RenderText(app, textToShow.substr(0, numLettersToShow), font, Vector4(1.0f), position);
    }
    else
    {
        if (isTyping)
        {
            if (selection[0] != selection[1])
                text.erase(text.begin() + selection[0], text.begin() + selection[1]);

            text.insert(text.begin() + selection[0], character);
            selection[1] = ++selection[0];
            isTyping = false;
        }

        if (IsPressed(app, KEY(BACKSPACE)))
        {
            if (selection[0] != selection[1])
            {
                text.erase(text.begin() + selection[0], text.begin() + selection[1]);
                selection[1] = selection[0];
            }
            else if (app.GetKey(KEY(LEFT_CONTROL)) && selection[0] > 0)
            {
                int start;
                for (start = selection[0]; start > 0 && text[start] == ' '; start--);
                for (; start > 0 && text[start] != ' '; start--);

                text.erase(text.begin() + start, text.begin() + selection[0]);
                selection[0] = selection[1] = start;
            }
            else if (selection[0] > 0)
            {
                selection[0] = --selection[1];
                text.erase(text.begin() + selection[0]);
            }
        }

        if (IsPressed(app, KEY(DELETE)))
        {
            if (selection[0] != selection[1])
            {
                text.erase(text.begin() + selection[0], text.begin() + selection[1]);
                selection[1] = selection[0];
            }
            else if (app.GetKey(KEY(LEFT_CONTROL)) && selection[0] > 0)
            {
                int end;
                for (end = selection[0]; end < text.length() && text[end] == ' '; end++);
                for (; end < text.length() && text[end] != ' '; end++);

                text.erase(text.begin() + selection[0], text.begin() + end);
                selection[1] = selection[0];
            }
            else if (selection[0] < text.length())
            {
                text.erase(text.begin() + selection[0]);
            }
        }

        if (IsPressed(app, KEY(DELETE)) && selection[1] < text.length())
            text.erase(text.begin() + selection[1] + 1);

        if (IsPressed(app, KEY(LEFT)))
        {
            if (selection[0] == selection[1])
                cursorIndex = 0;

            selection[cursorIndex] = selection[cursorIndex] - 1;
            selection[0] = std::max(selection[0], 0);
            selection[1] = std::max(selection[1], selection[0]);

            if (!app.GetKey(KEY(LEFT_SHIFT)) && !app.GetKey(KEY(RIGHT_SHIFT)))
                selection[1 - cursorIndex] = selection[cursorIndex];

            lastMoveTime = app.time;
        }

        if (IsPressed(app, KEY(RIGHT)))
        {
            if (selection[0] == selection[1])
                cursorIndex = 1;

            selection[cursorIndex] = selection[cursorIndex] + 1;
            selection[1] = std::min(selection[1], (int) text.length());
            selection[0] = std::min(selection[0], selection[1]);

            if (!app.GetKey(KEY(LEFT_SHIFT)) && !app.GetKey(KEY(RIGHT_SHIFT)))
                selection[1 - cursorIndex] = selection[cursorIndex];

            lastMoveTime = app.time;
        }

        if (app.GetKeyDown(KEY(ESCAPE)) || app.GetKeyDown(KEY(ENTER)) || app.GetKeyDown(KEY(KP_ENTER)))
        {
            uiData.active = UIInvalid();
            app.charCallback = [](Application& app, u32 codepoint) {};
            callbackHasBeenSet = false;
        }

        s32 numLettersToShow = std::min(maxLetters, (s32) text.length());

        // Place cursor at mouse
        if (app.GetMouseButtonDown(MOUSE(1)))
        {
            f32 mouseXOffset = app.mouseX - topLeft.x - padding.x;
            selection[0] = selection[1] = std::max(std::min((s32) ((mouseXOffset / avgLetterWidth) + 0.3f), numLettersToShow), 0);
        }

        s32 start = std::max(selection[0] - numLettersToShow, 0);

        Vector3 position = Vector3(topLeft.x + padding.x, topLeft.y + padding.y, topLeft.z);
        if (selection[0] == selection[1])
        {
            RenderText(app, textToShow.substr(start, numLettersToShow), font, textColor, position);

            Vector2 prevSize = GetRenderedTextSize(textToShow.substr(start, std::min(selection[0], numLettersToShow)), font);
        
            Rect caret;
            caret.topLeft = Vector3(topLeft.x + padding.x + prevSize.x, topLeft.y + padding.y, topLeft.z);
            caret.size = Vector2(2.0f, 0.7f * font.fontHeight);

            f32 caretAlpha = 0.5 * (sin(10.0 * (app.time - lastMoveTime)) + 1.0);
            RenderRect(app, caret, { 0.0f, 0.0f, 0.0f, caretAlpha });
        }
        else
        {
            RenderText(app, textToShow.substr(start, selection[0] - start), font, textColor, position);
            f32 hoffset = GetRenderedTextSize(textToShow.substr(start, selection[0] - start), font).x;

            RenderTextBox(app, textToShow.substr(selection[0], selection[1] - selection[0]), font,
                          textColor, Vector4(0.3f, 0.3f, 0.3f, 1.0f), Vector2(), position + Vector3(hoffset, 0.0f, 0.0f));
            hoffset += GetRenderedTextSize(textToShow.substr(selection[0], selection[1] - selection[0]), font).x;

            RenderText(app, textToShow.substr(selection[1], numLettersToShow - selection[1]), font, textColor, position + Vector3(hoffset, 0.0f, 0.0f));
        }
    }
}

void RenderNumericInputf(Application& app, ID id, f32& num, std::string& text, const Font& font,
                         Vector2 padding, Vector3 topLeft, f32 width)
{
    Rect containerRect;
    containerRect.topLeft = topLeft;
    containerRect.size = Vector2(width, font.fontHeight * 0.65f + 2.0f * padding.y);

    RenderRect(app, containerRect, Vector4(0.5f, 0.5f, 0.5f, 1.0f));

    static char character;
    static bool isTyping = false;
    static bool callbackHasBeenSet = false;

    static s32 selection[2] = {};
    static s32 cursorIndex = 0;
    static f64 lastMoveTime = 0.0;

    static Vector4 textColor = Vector4(1.0f);

    if (uiData.active != id)
    {
        char buffer[128];
        sprintf(buffer, "%f", num);
        text = buffer;
    }

    if (app.mouseX >= containerRect.topLeft.x && app.mouseX <= containerRect.topLeft.x + containerRect.size.x &&
        app.mouseY >= containerRect.topLeft.y && app.mouseY <= containerRect.topLeft.y + containerRect.size.y)
    {
        if (uiData.hot != id)
            uiData.hot = id;

        if (app.GetMouseButtonDown(MOUSE(1)))
        {
            uiData.active = id;
        }
    }
    else
    {
        if (uiData.hot == id)
            uiData.hot = UIInvalid();

        if (uiData.active == id && app.GetMouseButtonDown(MOUSE(1)))
        {
            // @Todo: This ignores if this has been set active externally
            uiData.active = UIInvalid();
            app.charCallback = [](Application& app, u32 codepoint) {};
            callbackHasBeenSet = false;

#           ifdef DEBUG
            std::cout << "Focus Removed\n";
#           endif
        }
    }

    if (uiData.active == id && !callbackHasBeenSet)
    {
        callbackHasBeenSet = true;
        cursorIndex = 0;
        
        app.charCallback = [](Application& app, u32 codepoint)
        {
            character = (char) codepoint;

            if ((character >= '0' && character <= '9') || character == '.')
                isTyping = true;
        };
    }

    Vector2 size = GetRenderedTextSize(text, font);
    std::string_view textToShow = text;
    f32 avgLetterWidth = size.x / text.length();
    s32 maxLetters = (s32) ((width - 2.0f * padding.x) / avgLetterWidth);

    if (uiData.active != id)
    {
        s32 numLettersToShow = (maxLetters < text.length()) ? maxLetters : text.length();
        Vector3 position = Vector3(topLeft.x + padding.x, topLeft.y + padding.y, topLeft.z);
        RenderText(app, textToShow.substr(0, numLettersToShow), font, Vector4(1.0f), position);
    }
    else
    {
        if (isTyping)
        {
            if (selection[0] != selection[1])
                text.erase(text.begin() + selection[0], text.begin() + selection[1]);

            text.insert(text.begin() + selection[0], character);
            selection[1] = ++selection[0];
            isTyping = false;
        }

        if (IsPressed(app, KEY(BACKSPACE)))
        {
            if (selection[0] != selection[1])
            {
                text.erase(text.begin() + selection[0], text.begin() + selection[1]);
                selection[1] = selection[0];
            }
            else if (app.GetKey(KEY(LEFT_CONTROL)) && selection[0] > 0)
            {
                int start;
                for (start = selection[0]; start > 0 && text[start] == ' '; start--);
                for (; start > 0 && text[start] != ' '; start--);

                text.erase(text.begin() + start, text.begin() + selection[0]);
                selection[0] = selection[1] = start;
            }
            else if (selection[0] > 0)
            {
                selection[0] = --selection[1];
                text.erase(text.begin() + selection[0]);
            }
        }

        if (IsPressed(app, KEY(DELETE)))
        {
            if (selection[0] != selection[1])
            {
                text.erase(text.begin() + selection[0], text.begin() + selection[1]);
                selection[1] = selection[0];
            }
            else if (app.GetKey(KEY(LEFT_CONTROL)) && selection[0] > 0)
            {
                int end;
                for (end = selection[0]; end < text.length() && text[end] == ' '; end++);
                for (; end < text.length() && text[end] != ' '; end++);

                text.erase(text.begin() + selection[0], text.begin() + end);
                selection[1] = selection[0];
            }
            else if (selection[0] < text.length())
            {
                text.erase(text.begin() + selection[0]);
            }
        }

        if (IsPressed(app, KEY(DELETE)) && selection[1] < text.length())
            text.erase(text.begin() + selection[1] + 1);

        if (IsPressed(app, KEY(LEFT)))
        {
            if (selection[0] == selection[1])
                cursorIndex = 0;

            selection[cursorIndex] = selection[cursorIndex] - 1;
            selection[0] = std::max(selection[0], 0);
            selection[1] = std::max(selection[1], selection[0]);

            if (!app.GetKey(KEY(LEFT_SHIFT)) && !app.GetKey(KEY(RIGHT_SHIFT)))
                selection[1 - cursorIndex] = selection[cursorIndex];

            lastMoveTime = app.time;
        }

        if (IsPressed(app, KEY(RIGHT)))
        {
            if (selection[0] == selection[1])
                cursorIndex = 1;

            selection[cursorIndex] = selection[cursorIndex] + 1;
            selection[1] = std::min(selection[1], (int) text.length());
            selection[0] = std::min(selection[0], selection[1]);

            if (!app.GetKey(KEY(LEFT_SHIFT)) && !app.GetKey(KEY(RIGHT_SHIFT)))
                selection[1 - cursorIndex] = selection[cursorIndex];

            lastMoveTime = app.time;
        }

        if (app.GetKeyDown(KEY(ESCAPE)) || app.GetKeyDown(KEY(ENTER)) || app.GetKeyDown(KEY(KP_ENTER)))
        {
            uiData.active = UIInvalid();
            app.charCallback = [](Application& app, u32 codepoint) {};
            callbackHasBeenSet = false;
        }

        s32 numLettersToShow = std::min(maxLetters, (s32) text.length());

        // Place cursor at mouse
        if (app.GetMouseButtonDown(MOUSE(1)))
        {
            f32 mouseXOffset = app.mouseX - topLeft.x - padding.x;
            selection[0] = selection[1] = std::max(std::min((s32) ((mouseXOffset / avgLetterWidth) + 0.3f), numLettersToShow), 0);
        }

        s32 start = std::max(selection[0] - numLettersToShow, 0);

        Vector3 position = Vector3(topLeft.x + padding.x, topLeft.y + padding.y, topLeft.z);
        if (selection[0] == selection[1])
        {
            RenderText(app, textToShow.substr(start, numLettersToShow), font, textColor, position);

            Vector2 prevSize = GetRenderedTextSize(textToShow.substr(start, std::min(selection[0], numLettersToShow)), font);
        
            Rect caret;
            caret.topLeft = Vector3(topLeft.x + padding.x + prevSize.x, topLeft.y + padding.y, topLeft.z);
            caret.size = Vector2(2.0f, 0.7f * font.fontHeight);

            f32 caretAlpha = 0.5 * (sin(10.0 * (app.time - lastMoveTime)) + 1.0);
            RenderRect(app, caret, { 0.0f, 0.0f, 0.0f, caretAlpha });
        }
        else
        {
            RenderText(app, textToShow.substr(start, selection[0] - start), font, textColor, position);
            f32 hoffset = GetRenderedTextSize(textToShow.substr(start, selection[0] - start), font).x;

            RenderTextBox(app, textToShow.substr(selection[0], selection[1] - selection[0]), font,
                          textColor, Vector4(0.3f, 0.3f, 0.3f, 1.0f), Vector2(), position + Vector3(hoffset, 0.0f, 0.0f));
            hoffset += GetRenderedTextSize(textToShow.substr(selection[0], selection[1] - selection[0]), font).x;

            RenderText(app, textToShow.substr(selection[1], numLettersToShow - selection[1]), font, textColor, position + Vector3(hoffset, 0.0f, 0.0f));
        }

        {   // Update number
            sscanf(text.c_str(), "%f", &num);
        }
    }
}

void RenderNumericInputi(Application& app, ID id, s32& num, std::string& text, const Font& font,
                         Vector2 padding, Vector3 topLeft, f32 width)
{
    Rect containerRect;
    containerRect.topLeft = topLeft;
    containerRect.size = Vector2(width, font.fontHeight * 0.65f + 2.0f * padding.y);

    RenderRect(app, containerRect, Vector4(0.5f, 0.5f, 0.5f, 1.0f));

    static char character;
    static bool isTyping = false;
    static bool callbackHasBeenSet = false;

    static s32 selection[2] = {};
    static s32 cursorIndex = 0;
    static f64 lastMoveTime = 0.0;

    static Vector4 textColor = Vector4(1.0f);

    if (uiData.active != id)
    {
        char buffer[128];
        sprintf(buffer, "%d", num);
        text = buffer;
    }

    if (app.mouseX >= containerRect.topLeft.x && app.mouseX <= containerRect.topLeft.x + containerRect.size.x &&
        app.mouseY >= containerRect.topLeft.y && app.mouseY <= containerRect.topLeft.y + containerRect.size.y)
    {
        if (uiData.hot != id)
            uiData.hot = id;

        if (app.GetMouseButtonDown(MOUSE(1)))
        {
            uiData.active = id;
        }
    }
    else
    {
        if (uiData.hot == id)
            uiData.hot = UIInvalid();

        if (uiData.active == id && app.GetMouseButtonDown(MOUSE(1)))
        {
            // @Todo: This ignores if this has been set active externally
            uiData.active = UIInvalid();
            app.charCallback = [](Application& app, u32 codepoint) {};
            callbackHasBeenSet = false;

#           ifdef DEBUG
            std::cout << "Focus Removed\n";
#           endif
        }
    }

    if (uiData.active == id && !callbackHasBeenSet)
    {
        callbackHasBeenSet = true;
        cursorIndex = 0;
        
        app.charCallback = [](Application& app, u32 codepoint)
        {
            character = (char) codepoint;

            if ((character >= '0' && character <= '9') || character == '.')
                isTyping = true;
        };
    }

    Vector2 size = GetRenderedTextSize(text, font);
    std::string_view textToShow = text;
    f32 avgLetterWidth = size.x / text.length();
    s32 maxLetters = (s32) ((width - 2.0f * padding.x) / avgLetterWidth);

    if (uiData.active != id)
    {
        s32 numLettersToShow = (maxLetters < text.length()) ? maxLetters : text.length();
        Vector3 position = Vector3(topLeft.x + padding.x, topLeft.y + padding.y, topLeft.z);
        RenderText(app, textToShow.substr(0, numLettersToShow), font, Vector4(1.0f), position);
    }
    else
    {
        if (isTyping)
        {
            if (selection[0] != selection[1])
                text.erase(text.begin() + selection[0], text.begin() + selection[1]);

            text.insert(text.begin() + selection[0], character);
            selection[1] = ++selection[0];
            isTyping = false;
        }

        if (IsPressed(app, KEY(BACKSPACE)))
        {
            if (selection[0] != selection[1])
            {
                text.erase(text.begin() + selection[0], text.begin() + selection[1]);
                selection[1] = selection[0];
            }
            else if (app.GetKey(KEY(LEFT_CONTROL)) && selection[0] > 0)
            {
                int start;
                for (start = selection[0]; start > 0 && text[start] == ' '; start--);
                for (; start > 0 && text[start] != ' '; start--);

                text.erase(text.begin() + start, text.begin() + selection[0]);
                selection[0] = selection[1] = start;
            }
            else if (selection[0] > 0)
            {
                selection[0] = --selection[1];
                text.erase(text.begin() + selection[0]);
            }
        }

        if (IsPressed(app, KEY(DELETE)))
        {
            if (selection[0] != selection[1])
            {
                text.erase(text.begin() + selection[0], text.begin() + selection[1]);
                selection[1] = selection[0];
            }
            else if (app.GetKey(KEY(LEFT_CONTROL)) && selection[0] > 0)
            {
                int end;
                for (end = selection[0]; end < text.length() && text[end] == ' '; end++);
                for (; end < text.length() && text[end] != ' '; end++);

                text.erase(text.begin() + selection[0], text.begin() + end);
                selection[1] = selection[0];
            }
            else if (selection[0] < text.length())
            {
                text.erase(text.begin() + selection[0]);
            }
        }

        if (IsPressed(app, KEY(DELETE)) && selection[1] < text.length())
            text.erase(text.begin() + selection[1] + 1);

        if (IsPressed(app, KEY(LEFT)))
        {
            if (selection[0] == selection[1])
                cursorIndex = 0;

            selection[cursorIndex] = selection[cursorIndex] - 1;
            selection[0] = std::max(selection[0], 0);
            selection[1] = std::max(selection[1], selection[0]);

            if (!app.GetKey(KEY(LEFT_SHIFT)) && !app.GetKey(KEY(RIGHT_SHIFT)))
                selection[1 - cursorIndex] = selection[cursorIndex];

            lastMoveTime = app.time;
        }

        if (IsPressed(app, KEY(RIGHT)))
        {
            if (selection[0] == selection[1])
                cursorIndex = 1;

            selection[cursorIndex] = selection[cursorIndex] + 1;
            selection[1] = std::min(selection[1], (int) text.length());
            selection[0] = std::min(selection[0], selection[1]);

            if (!app.GetKey(KEY(LEFT_SHIFT)) && !app.GetKey(KEY(RIGHT_SHIFT)))
                selection[1 - cursorIndex] = selection[cursorIndex];

            lastMoveTime = app.time;
        }

        if (app.GetKeyDown(KEY(ESCAPE)) || app.GetKeyDown(KEY(ENTER)) || app.GetKeyDown(KEY(KP_ENTER)))
        {
            uiData.active = UIInvalid();
            app.charCallback = [](Application& app, u32 codepoint) {};
            callbackHasBeenSet = false;
        }

        s32 numLettersToShow = std::min(maxLetters, (s32) text.length());

        // Place cursor at mouse
        if (app.GetMouseButtonDown(MOUSE(1)))
        {
            f32 mouseXOffset = app.mouseX - topLeft.x - padding.x;
            selection[0] = selection[1] = std::max(std::min((s32) ((mouseXOffset / avgLetterWidth) + 0.3f), numLettersToShow), 0);
        }

        s32 start = std::max(selection[0] - numLettersToShow, 0);

        Vector3 position = Vector3(topLeft.x + padding.x, topLeft.y + padding.y, topLeft.z);
        if (selection[0] == selection[1])
        {
            RenderText(app, textToShow.substr(start, numLettersToShow), font, textColor, position);

            Vector2 prevSize = GetRenderedTextSize(textToShow.substr(start, std::min(selection[0], numLettersToShow)), font);
        
            Rect caret;
            caret.topLeft = Vector3(topLeft.x + padding.x + prevSize.x, topLeft.y + padding.y, topLeft.z);
            caret.size = Vector2(2.0f, 0.7f * font.fontHeight);

            f32 caretAlpha = 0.5 * (sin(10.0 * (app.time - lastMoveTime)) + 1.0);
            RenderRect(app, caret, { 0.0f, 0.0f, 0.0f, caretAlpha });
        }
        else
        {
            RenderText(app, textToShow.substr(start, selection[0] - start), font, textColor, position);
            f32 hoffset = GetRenderedTextSize(textToShow.substr(start, selection[0] - start), font).x;

            RenderTextBox(app, textToShow.substr(selection[0], selection[1] - selection[0]), font,
                          textColor, Vector4(0.3f, 0.3f, 0.3f, 1.0f), Vector2(), position + Vector3(hoffset, 0.0f, 0.0f));
            hoffset += GetRenderedTextSize(textToShow.substr(selection[0], selection[1] - selection[0]), font).x;

            RenderText(app, textToShow.substr(selection[1], numLettersToShow - selection[1]), font, textColor, position + Vector3(hoffset, 0.0f, 0.0f));
        }

        {   // Update number
            sscanf(text.c_str(), "%d", &num);
        }
    }
}

} // namespace UI