#include "math.h"

#include "basic_types.h"

f32 Clamp(f32 val, f32 min, f32 max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;

    return val;
}

f32 ToRad(f32 deg)
{
    return deg * _PI_180;
}

f32 ToDeg(f32 rad)
{
    return rad * _180_PI;
}