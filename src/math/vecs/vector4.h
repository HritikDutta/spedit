#pragma once

#include <cmath>
#include <xmmintrin.h>
#include "../basic_types.h"
#include "../math.h"

union Vector4;
extern f32 Dot(const Vector4& left, const Vector4& right);

union Vector4
{
    struct
    {
        f32 x, y, z, w;
    };

    struct
    {
        f32 r, g, b, a;
    };

    struct
    {
        f32 s, t, u, v;
    };

    f32 data[4];

    __m128 sseData;

    inline f32& operator[](s32 index)
    {
        return data[index];
    }

    Vector4()
    :   x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}

    Vector4(__m128 sseData)
    :   sseData(sseData) {}

    Vector4(f32 val)
    :   x(val), y(val), z(val), w(val) {}

    Vector4(f32 x, f32 y, f32 z, f32 w)
    :   x(x), y(y), z(z), w(w) {}

    inline f32 SqrLength() const
    {
        return Dot(*this, *this);
    }

    inline f32 Length() const
    {
        return sqrt(Dot(*this, *this));
    }

    inline Vector4 Unit() const
    {
        f32 length = Length();

        if (length == 0.0f)
            return Vector4();

        return Vector4(x / length, y / length, z / length, w / length);
    }

    inline Vector4& Normalize()
    {
        f32 length = Length();

        if (length == 0.0f)
            return *this;

        x /= length;
        y /= length;
        z /= length;
        w /= length;

        return *this;
    }

    inline Vector4& operator-()
    {
        sseData = _mm_xor_ps(sseData, _mm_set1_ps(-0.0f));
        return *this;
    }

    inline Vector4& operator+=(const Vector4& other)
    {
        sseData = _mm_add_ps(sseData, other.sseData);
        return *this;
    }

    inline Vector4& operator-=(const Vector4& other)
    {
        sseData = _mm_sub_ps(sseData, other.sseData);
        return *this;
    }

    inline Vector4& operator*=(f32 scalar)
    {
        _mm_mul_ps(sseData, _mm_set1_ps(scalar));
        return *this;
    }

    inline Vector4& operator/=(f32 scalar)
    {
        _mm_div_ps(sseData, _mm_set1_ps(scalar));
        return *this;
    }

    inline bool operator==(const Vector4& right) const
    {
        return x == right.x && y == right.y && z == right.z && w == right.w;
    }

    inline bool operator!=(const Vector4& right) const
    {
        return x != right.x || y != right.y || z != right.z || w != right.w;
    }
};

inline Vector4 operator+(const Vector4& left, const Vector4& right)
{
    return Vector4(_mm_add_ps(left.sseData, right.sseData));
}

inline Vector4 operator-(const Vector4& left, const Vector4& right)
{
    return Vector4(_mm_sub_ps(left.sseData, right.sseData));
}

inline Vector4 operator*(const Vector4& vec, f32 scalar)
{
    return Vector4(_mm_mul_ps(vec.sseData, _mm_set1_ps(scalar)));
}

inline Vector4 operator*(f32 scalar, const Vector4& vec)
{
    return Vector4(_mm_mul_ps(vec.sseData, _mm_set1_ps(scalar)));
}

inline Vector4 operator/(const Vector4& vec, f32 scalar)
{
    return Vector4(_mm_div_ps(vec.sseData, _mm_set1_ps(scalar)));
}

inline Vector4 operator/(f32 scalar, const Vector4& vec)
{
    return Vector4(_mm_div_ps(vec.sseData, _mm_set1_ps(scalar)));
}

inline f32 Dot(const Vector4& left, const Vector4& right)
{
    f32 res;    

    // Copied from HandmadeMath
    __m128 SSEResultOne = _mm_mul_ps(left.sseData, right.sseData);
    __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    _mm_store_ss(&res, SSEResultOne);

    return res;
}

inline f32 SqrDistance(const Vector4& v1, const Vector4& v2)
{
    return (v1 - v2).SqrLength();
}

inline f32 Distance(const Vector4& v1, const Vector4& v2)
{
    return (v1 - v2).Length();
}

inline Vector4 Lerp(const Vector4& a, const Vector4& b, f32 t)
{
    return Vector4(_mm_add_ps(
        _mm_mul_ps(a.sseData, _mm_set1_ps(1.0f - t)),
        _mm_mul_ps(b.sseData, _mm_set1_ps(t))
    ));
}