#pragma once

#include "hash_table.h"

namespace gn
{

template <>
struct hash<f32>
{
    size_t operator()(f32 const& key) const
    {
        u8* bytes = (u8*) &key;
        u8 h[4];

        h[0] = bytes[3];
        h[1] = bytes[2];
        h[2] = bytes[1];
        h[3] = bytes[0];

        u32 hash = *(u32*)h;
        return hash;
    }
};

template <>
struct hash<u64>
{
    size_t operator()(u64 const& key) const
    {
        return key;
    }
};

} // namespace gn