#pragma once

#include <string_view>
#include <stb/stb_truetype.h>
#include "platform/application.h"
#include "math/types.h"

namespace UI
{

void Init();
void Begin();
void End();
void Shutdown();

struct ID
{
    s32 primary;
    s32 secondary;

    bool operator==(const ID& other) const;
    bool operator!=(const ID& other) const;
};

struct Rect
{
    Vector3 topLeft;
    Vector2 size;
};

struct Font
{
    u32 bitmapTexID;
    u32 bitmapWidth, bitmapHeight;
    f32 fontHeight;

    stbtt_bakedchar charData[128 - ' '];

    void Load(const std::string_view& filepath, f32 height);
    void Free();
};

struct Image
{
    u32 texID;
    s32 width, height;
    s32 scaledWidth, scaledHeight;

    void SetScale(const Vector2& scale);
    bool Load(const std::string_view& filepath);
    void Free();
};

void SetActive(ID id);

Vector2 GetRenderedTextSize(const std::string_view& text, const Font& font);

void RenderRect(Application& app, const Rect& rect, Vector4 color);
void RenderText(Application& app, const std::string_view& text, const Font& font,
                Vector4 color, Vector3 topLeft);
bool RenderButton(Application& app, ID id, const Rect& rect, Vector4 defaultColor,
                  Vector4 hoverColor, Vector4 pressedColor);

void RenderImage(Application& app, Image& image, Vector3 topLeft, Vector4 tint = Vector4(1.0f));

void RenderTextBox(Application& app, const std::string_view& text, const Font& font,
                   Vector4 fontColor, Vector4 rectColor,
                   Vector2 padding, Vector3 topLeft);

// Using default colors
bool RenderTextButton(Application& app, ID id, const std::string_view& text, const Font& font,
                      Vector2 padding, Vector3 topLeft);

void RenderTextInput(Application& app, ID id, std::string& text, const Font& font,
                     Vector2 padding, Vector3 topLeft, f32 width);

void RenderNumericInputf(Application& app, ID id, f32& num, std::string& text, const Font& font,
                         Vector2 padding, Vector3 topLeft, f32 width);

void RenderNumericInputi(Application& app, ID id, s32& num, std::string& text, const Font& font,
                         Vector2 padding, Vector3 topLeft, f32 width);

} // namespace UI

#define GenUIID() (UI::ID { __LINE__, 0 })
#define GenUIIDWithSec(x) (UI::ID { __LINE__, (x) })
#define UIInvalid() (UI::ID { -1, -1 })