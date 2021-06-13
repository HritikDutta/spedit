#pragma once

#include "basic_types.h"

#define _PI       3.14159265358979323846f   // pi
#define _PI_180   0.01745329251994329577f   // pi / 180
#define _180_PI   57.2957795130823208768f   // 180 / pi

f32 Clamp(f32 val, f32 min, f32 max);
f32 ToRad(f32 deg);
f32 ToDeg(f32 rad);