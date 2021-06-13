#pragma once

#include <xmmintrin.h>
#include "../basic_types.h"
#include "../vecs/vector3.h"
#include "../vecs/vector4.h"
#include "../mats/matrix4.h"

union Quaternion;
extern f32 Dot(const Quaternion& left, const Quaternion& right);

// @Todo: Complete this
union Quaternion
{
    struct
    {
        union
        {
            Vector3 xyz;
            struct
            {
                f32 x, y, z;
            };
        };

        f32 w;
    };

    f32 data[4];
    __m128 sseData;

    inline f32& operator[](s32 index)
    {
        return data[index];
    }

    Quaternion()
    :   sseData(_mm_setzero_ps()) {}

    Quaternion(f32 x, f32 y, f32 z, f32 w)
    :   sseData(_mm_setr_ps(x, y, z, w)) {}

    Quaternion(__m128 sse)
    :   sseData(sse) {}

    Quaternion(const Vector4& vec)
    :   sseData(vec.sseData) {}

    Quaternion(const Vector3& axis, f32 angle)
    {
        xyz = axis.Unit() * sinf(angle / 2.0f);
        w = cosf(angle / 2.0f);
    }

    inline f32 SqrMagnitude() const
    {
        return Dot(*this, *this);
    }

    inline f32 Magnitude() const
    {
        return sqrt(Dot(*this, *this));
    }

    inline Quaternion Unit() const
    {
        return Quaternion(_mm_div_ps(sseData, _mm_set1_ps(Magnitude())));
    }

    inline Quaternion& Normalize()
    {
        sseData = _mm_div_ps(sseData, _mm_set1_ps(Magnitude()));
        return *this;
    }

    inline Quaternion Inverse() const
    {
        __m128 conjugate = _mm_setr_ps(-x, -y, -z, w);
        return Quaternion(_mm_div_ps(conjugate, _mm_set1_ps(abs(SqrMagnitude()))));
    }

    // Stores the result of inverse itself
    inline Quaternion& Invert()
    {
        __m128 conjugate = _mm_setr_ps(-x, -y, -z, w);
        sseData = _mm_div_ps(conjugate, _mm_set1_ps(abs(SqrMagnitude())));
        return *this;
    }

    inline Matrix4 GetMatrix4() const
    {
        Quaternion normalized = Unit();

        f32 xx, yy, zz,
            xy, xz, yz,
            wx, wy, wz;

        xx = normalized.x * normalized.x;
        yy = normalized.y * normalized.y;
        zz = normalized.z * normalized.z;

        xy = normalized.x * normalized.y;
        xz = normalized.x * normalized.z;
        yz = normalized.y * normalized.z;

        wx = normalized.w * normalized.x;
        wy = normalized.w * normalized.y;
        wz = normalized.w * normalized.z;

        __m128 c0 = _mm_setr_ps(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f);
        __m128 c1 = _mm_setr_ps(2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f);
        __m128 c2 = _mm_setr_ps(2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f);
        __m128 c3 = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);

        return Matrix4(c0, c1, c2, c3);
    }

    inline Quaternion& operator-()
    {
        sseData = _mm_xor_ps(sseData, _mm_set1_ps(-0.0f));
        return *this;
    }

    inline Quaternion& operator+=(const Quaternion& other)
    {
        sseData = _mm_add_ps(sseData, other.sseData);
        return *this;
    }

    inline Quaternion& operator-=(const Quaternion& other)
    {
        sseData = _mm_sub_ps(sseData, other.sseData);
        return *this;
    }

    inline Quaternion& operator*=(f32 scalar)
    {
        sseData = _mm_mul_ps(sseData, _mm_set1_ps(scalar));
        return *this;
    }

    inline Quaternion& operator/=(f32 scalar)
    {
        sseData = _mm_div_ps(sseData, _mm_set1_ps(scalar));
        return *this;
    }

    inline Quaternion& operator*=(const Quaternion& other)
    {
        // Multiply each component with it's respective multiplicatives
        // The xor changes the sign for the result

        __m128 elem = _mm_xor_ps(_mm_shuffle_ps(sseData, sseData, _MM_SHUFFLE(0, 0, 0, 0)),
                                 _mm_setr_ps(0.0f, -0.0f, 0.0f, -0.0f));
        __m128 mul  = _mm_shuffle_ps(other.sseData, other.sseData, _MM_SHUFFLE(0, 1, 2, 3));
        __m128 res  = _mm_mul_ps(elem, mul);

        elem = _mm_xor_ps(_mm_shuffle_ps(sseData, sseData, _MM_SHUFFLE(1, 1, 1, 1)),
                          _mm_setr_ps(0.0f, 0.0f, -0.0f, -0.0f));
        mul  = _mm_shuffle_ps(other.sseData, other.sseData, _MM_SHUFFLE(1, 0, 3, 2));
        res  = _mm_add_ps(res, _mm_mul_ps(elem, mul));

        elem = _mm_xor_ps(_mm_shuffle_ps(sseData, sseData, _MM_SHUFFLE(2, 2, 2, 2)),
                          _mm_setr_ps(-0.0f, 0.0f, 0.0f, -0.0f));
        mul  = _mm_shuffle_ps(other.sseData, other.sseData, _MM_SHUFFLE(2, 3, 0, 1));
        res  = _mm_add_ps(res, _mm_mul_ps(elem, mul));

        elem = _mm_shuffle_ps(sseData, sseData, _MM_SHUFFLE(3, 3, 3, 3));
        mul  = _mm_shuffle_ps(other.sseData, other.sseData, _MM_SHUFFLE(3, 2, 1, 0));
        sseData  = _mm_add_ps(res, _mm_mul_ps(elem, mul));

        return *this;
    }
};

inline Quaternion operator+(const Quaternion& left, const Quaternion& right)
{
    return Quaternion(_mm_add_ps(left.sseData, right.sseData));
}

inline Quaternion operator-(const Quaternion& left, const Quaternion& right)
{
    return Quaternion(_mm_sub_ps(left.sseData, right.sseData));
}

inline Quaternion operator*(const Quaternion& quat, f32 scalar)
{
    return Quaternion(_mm_mul_ps(quat.sseData, _mm_set1_ps(scalar)));
}

inline Quaternion operator*(f32 scalar, const Quaternion& quat)
{
    return Quaternion(_mm_mul_ps(quat.sseData, _mm_set1_ps(scalar)));
}

inline Quaternion operator*(const Quaternion& left, const Quaternion& right)
{
    // Multiply each component with it's respective multiplicatives
    // The xor changes the sign for the result

    __m128 elem = _mm_xor_ps(_mm_shuffle_ps(left.sseData, left.sseData, _MM_SHUFFLE(0, 0, 0, 0)),
                                _mm_setr_ps(0.0f, -0.0f, 0.0f, -0.0f));
    __m128 mul  = _mm_shuffle_ps(right.sseData, right.sseData, _MM_SHUFFLE(0, 1, 2, 3));
    __m128 res  = _mm_mul_ps(elem, mul);

    elem = _mm_xor_ps(_mm_shuffle_ps(left.sseData, left.sseData, _MM_SHUFFLE(1, 1, 1, 1)),
                        _mm_setr_ps(0.0f, 0.0f, -0.0f, -0.0f));
    mul  = _mm_shuffle_ps(right.sseData, right.sseData, _MM_SHUFFLE(1, 0, 3, 2));
    res  = _mm_add_ps(res, _mm_mul_ps(elem, mul));

    elem = _mm_xor_ps(_mm_shuffle_ps(left.sseData, left.sseData, _MM_SHUFFLE(2, 2, 2, 2)),
                        _mm_setr_ps(-0.0f, 0.0f, 0.0f, -0.0f));
    mul  = _mm_shuffle_ps(right.sseData, right.sseData, _MM_SHUFFLE(2, 3, 0, 1));
    res  = _mm_add_ps(res, _mm_mul_ps(elem, mul));

    elem = _mm_shuffle_ps(left.sseData, left.sseData, _MM_SHUFFLE(3, 3, 3, 3));
    mul  = _mm_shuffle_ps(right.sseData, right.sseData, _MM_SHUFFLE(3, 2, 1, 0));
    res  = _mm_add_ps(res, _mm_mul_ps(elem, mul));

    return Quaternion(res);
}

inline Quaternion operator/(const Quaternion& quat, f32 scalar)
{
    return Quaternion(_mm_div_ps(quat.sseData, _mm_set1_ps(scalar)));
}

inline f32 Dot(const Quaternion& left, const Quaternion& right)
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

inline Quaternion NLerp(const Quaternion& a, const Quaternion& b, f32 t)
{
    return Quaternion(_mm_add_ps(
        _mm_mul_ps(a.sseData, _mm_set1_ps(1.0f - t)),
        _mm_mul_ps(b.sseData, _mm_set1_ps(t))
    ));
}

inline Quaternion SLerp(const Quaternion& a, const Quaternion& b, f32 t)
{
    f32 cosTheta = Dot(a, b);
    f32 angle = acosf(cosTheta);

    f32 sin_1_tTheta = sinf((1.0f - t) * angle);
    f32 sin_tTheta   = sinf(t * angle);

    Quaternion left  = a * sin_1_tTheta;
    Quaternion right = b * sin_tTheta;

    return (left + right) / sinf(angle);
}