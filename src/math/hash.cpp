#include "hash.h"

#include "containers/hash_table.h"
#include "vecs/vector2.h"
#include "vecs/vector3.h"
#include "vecs/vector4.h"

static void CombineHash(size_t& seed, size_t hash)
{
    // Copied from glm
    hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= hash;
}

namespace gn
{
    size_t hash<Vector2>::operator()(Vector2 const& vec) const
    {
        size_t seed = 0;
        hash<f32> hasher;
        CombineHash(seed, hasher(vec.x));
        CombineHash(seed, hasher(vec.y));
        return seed;
    }

    size_t hash<Vector3>::operator()(Vector3 const& vec) const
    {
        size_t seed = 0;
        hash<f32> hasher;
        CombineHash(seed, hasher(vec.x));
        CombineHash(seed, hasher(vec.y));
        CombineHash(seed, hasher(vec.z));
        return seed;
    }

    size_t hash<Vector4>::operator()(Vector4 const& vec) const
    {
        size_t seed = 0;
        hash<f32> hasher;
        CombineHash(seed, hasher(vec.x));
        CombineHash(seed, hasher(vec.y));
        CombineHash(seed, hasher(vec.z));
        CombineHash(seed, hasher(vec.w));
        return seed;
    }
}