#pragma once

#include "animation.h"
#include "containers/darray.h"
#include "engine/ui.h"
#include "misc/gn_assert.h"

struct Context
{
    // File Data
    std::string filename = "Empty";
    std::string fullpath;

    // Image
    UI::Image image;
    bool imageLoaded = false;
    bool imageLoadError = false;

    // Animations
    gn::darray<Animation> animations;
    s32 selectedAnimationIndex = -1;
    s32 selectedFrameIndex = -1;

    Animation& CurrentAnimation()
    {
        ASSERT(selectedAnimationIndex != -1);
        return animations[selectedAnimationIndex];
    }

    AnimationFrame& CurrentAnimationFrame()
    {
        ASSERT(selectedAnimationIndex != -1);
        ASSERT(selectedFrameIndex != -1);
        return CurrentAnimation().frames[selectedFrameIndex];
    }
};