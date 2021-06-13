#include "animation.h"

#ifdef DEBUG
#include <iostream>
#endif

#include <string>
#include <cstdlib>
#include "misc/gn_assert.h"
#include "containers/darray.h"
#include "math/types.h"

Animation::Animation(const std::string& name)
:   name(name), frameRate(30.0f),
    loopType(LoopType::NONE) {}

const char* Animation::GetLoopTypeName() const
{
    switch (loopType)
    {
        case LoopType::NONE:      return "None";
        case LoopType::CYCLE:     return "Cycle";
        case LoopType::PING_PONG: return "Ping Pong";
        
        default: return "Invalid";
    }
}