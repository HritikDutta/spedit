#pragma once

#include <string>
#include "containers/darray.h"
#include "math/types.h"

struct AnimationFrame
{
    Vector2 topLeft;
    Vector2 size;
    Vector2 pivot;
};

struct Animation
{
    enum struct LoopType
    {
        NONE,
        CYCLE,
        PING_PONG,
        NUM_TYPES
    };

    std::string name;
    f32 frameRate { 30.0f };
    LoopType loopType { LoopType::NONE };
    gn::darray<AnimationFrame> frames;

    Animation(const std::string& name);

    const char* GetLoopTypeName() const;
};