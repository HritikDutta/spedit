#include "background.h"

#include <cstdlib>
#include "math/types.h"
#include "engine/ui.h"

void BackgroundTexture::Create(f32 width, f32 height)
{
    u8* pixels = (u8*) malloc(width * height * 4);

    u8 colors[][4] = {
        { 100, 100, 100, 200 },
        {  50,  50,  50, 200 },
    };

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            u32 idx = (y * width + x) * 4;
            u8* color = colors[(x + y) % 2];

            pixels[idx + 0] = color[0];
            pixels[idx + 1] = color[1];
            pixels[idx + 2] = color[2];
            pixels[idx + 3] = color[3];
        }
    }

    glGenTextures(1, &image.texID);
    glBindTexture(GL_TEXTURE_2D, image.texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    image.width  = width;
    image.height = height;

    free(pixels);
}

void BackgroundTexture::CreateDefault()
{
    Create(128, 128);
}

void BackgroundTexture::Free()
{
    image.Free();
}

void BackgroundTexture::SetScale(const Vector2& scale)
{
    image.SetScale(scale);
}