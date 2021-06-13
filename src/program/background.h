#pragma once

#include "math/types.h"
#include "engine/ui.h"

struct BackgroundTexture
{
    UI::Image image;

    void CreateDefault();
    void Create(f32 width, f32 height);
    void Free();
    void SetScale(const Vector2& scale);
};