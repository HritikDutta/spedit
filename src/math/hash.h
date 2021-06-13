#pragma once

// Hash functions for all vectors

#include "containers/hash_table.h"

#include "vecs/vector2.h"
#include "vecs/vector3.h"
#include "vecs/vector4.h"

namespace gn
{
    template<>
    struct hash<Vector2>
    {
        size_t operator()(Vector2 const& vec) const;
    };

    template<>
    struct hash<Vector3>
    {
        size_t operator()(Vector3 const& vec) const;
    };

    template<>
    struct hash<Vector4>
    {
        size_t operator()(Vector4 const& vec) const;
    };
}