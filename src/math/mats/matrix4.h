#pragma once

#include <xmmintrin.h>
#include "../basic_types.h"
#include "../vecs/vector3.h"
#include "../vecs/vector4.h"

union Matrix4
{
    f32 data[4][4];
    __m128 sseColumns[4];

    inline Vector4 operator[](s32 index)
    {
        return Vector4(sseColumns[index]);
    }

    Matrix4(f32 diagonal = 1.0f)
    {
        sseColumns[0] = _mm_setr_ps(diagonal, 0.0f, 0.0f, 0.0f);
        sseColumns[1] = _mm_setr_ps(0.0f, diagonal, 0.0f, 0.0f);
        sseColumns[2] = _mm_setr_ps(0.0f, 0.0f, diagonal, 0.0f);
        sseColumns[3] = _mm_setr_ps(0.0f, 0.0f, 0.0f, diagonal);
    }

    Matrix4(f32 d1, f32 d2, f32 d3, f32 d4 = 1.0f)
    {
        sseColumns[0] = _mm_setr_ps(d1, 0.0f, 0.0f, 0.0f);
        sseColumns[1] = _mm_setr_ps(0.0f, d2, 0.0f, 0.0f);
        sseColumns[2] = _mm_setr_ps(0.0f, 0.0f, d3, 0.0f);
        sseColumns[3] = _mm_setr_ps(0.0f, 0.0f, 0.0f, d4);
    }

    Matrix4(__m128 c0, __m128 c1, __m128 c2, __m128 c3)
    {
        sseColumns[0] = c0;
        sseColumns[1] = c1;
        sseColumns[2] = c2;
        sseColumns[3] = c3;
    }

    static inline Matrix4 Translation(const Vector3& displacement)
    {
        __m128 c0 = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
        __m128 c1 = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
        __m128 c2 = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
        __m128 c3 = _mm_setr_ps(displacement.x, displacement.y, displacement.z, 1.0f);

        return Matrix4(c0, c1, c2, c3);
    }

    static inline Matrix4 Rotation(Vector3 axis, f32 angle)
    {
        axis.Normalize();

        f32 st = sinf(angle);
        f32 ct = cosf(angle);
        f32 _1_ct = 1.0f - ct;

        __m128 c0 = _mm_setr_ps((
            axis.x * axis.x * _1_ct) + ct,
            (axis.x * axis.y * _1_ct) + axis.z * st,
            (axis.x * axis.z * _1_ct) - axis.y * st,
            0.0f
        );

        __m128 c1 = _mm_setr_ps(
            (axis.y * axis.x * _1_ct) - axis.z * st,
            (axis.y * axis.y * _1_ct) + ct,
            (axis.y * axis.z * _1_ct) + axis.x * st,
            0.0f
        );

        __m128 c2 = _mm_setr_ps(
            (axis.z * axis.x * _1_ct) + axis.y * st,
            (axis.z * axis.y * _1_ct) - axis.x * st,
            (axis.z * axis.z * _1_ct) + ct,
            0.0f
        );

        __m128 c3 = _mm_setr_ps(
            0.0f, 0.0f, 0.0f, 1.0f
        );

        return Matrix4(c0, c1, c2, c3);
    }

    static inline Matrix4 Scaling(f32 scale)
    {
        return Matrix4(scale, scale, scale);
    }

    static inline Matrix4 Scaling(const Vector3& scale)
    {
        return Matrix4(scale.x, scale.y, scale.z);
    }

    static inline Matrix4 Perspective(f32 fov, f32 aspectRatio, f32 near, f32 far)
    {
        Matrix4 m(0.0f);

        f32 cotangent = 1.0f / tanf(fov / 2.0f);

        m.data[0][0] = cotangent / aspectRatio;
        m.data[1][1] = cotangent;
        m.data[2][2] = (near + far) / (near - far);
        m.data[2][3] = -1.0f;
        m.data[3][2] = (2.0f * near * far) / (near - far);

        return m;
    }

    static inline Matrix4 Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
    {
        Matrix4 m(1.0f);

        m.data[0][0] = 2.0f / (right - left);
        m.data[1][1] = 2.0f / (top - bottom);
        m.data[2][2] = 2.0f / (near - far);
        m.data[3][3] = 1.0f;

        m.data[3][0] = (left + right) / (left - right);
        m.data[3][1] = (bottom + top) / (bottom - top);
        m.data[3][2] = (far + near) / (near - far);

        return m;
    }

    static inline Matrix4 LookAt(Vector3 eye, Vector3 center, Vector3 up)
    {
        Vector3 f = (center - eye).Normalize();
        Vector3 s = Cross(f, up).Normalize();
        Vector3 u = Cross(s, f);

        __m128 c1 = _mm_setr_ps(s.x, u.x, -f.x, 0.0f);
        __m128 c2 = _mm_setr_ps(s.y, u.y, -f.y, 0.0f);
        __m128 c3 = _mm_setr_ps(s.z, u.z, -f.z, 0.0f);
        __m128 c4 = _mm_setr_ps(-Dot(s, eye), -Dot(u, eye), Dot(f, eye), 1.0f);

        return Matrix4(c1, c2, c3, c4);
    }

    inline Matrix4& operator*=(f32 scalar)
    {
        sseColumns[0] = _mm_mul_ps(sseColumns[0], _mm_set1_ps(scalar));
        sseColumns[1] = _mm_mul_ps(sseColumns[1], _mm_set1_ps(scalar));
        sseColumns[2] = _mm_mul_ps(sseColumns[2], _mm_set1_ps(scalar));
        sseColumns[3] = _mm_mul_ps(sseColumns[3], _mm_set1_ps(scalar));
        return *this;
    }

    inline Matrix4& operator/=(f32 scalar)
    {
        sseColumns[0] = _mm_div_ps(sseColumns[0], _mm_set1_ps(scalar));
        sseColumns[1] = _mm_div_ps(sseColumns[1], _mm_set1_ps(scalar));
        sseColumns[2] = _mm_div_ps(sseColumns[2], _mm_set1_ps(scalar));
        sseColumns[3] = _mm_div_ps(sseColumns[3], _mm_set1_ps(scalar));
        return *this;
    }

    inline Matrix4 operator+(const Matrix4& right) const
    {
        __m128 c1 = _mm_add_ps(sseColumns[0], right.sseColumns[0]);
        __m128 c2 = _mm_add_ps(sseColumns[1], right.sseColumns[1]);
        __m128 c3 = _mm_add_ps(sseColumns[2], right.sseColumns[2]);
        __m128 c4 = _mm_add_ps(sseColumns[3], right.sseColumns[3]);

        return Matrix4(c1, c2, c3, c4);
    }

    inline Matrix4 operator-(const Matrix4& right) const
    {
        __m128 c1 = _mm_sub_ps(sseColumns[0], right.sseColumns[0]);
        __m128 c2 = _mm_sub_ps(sseColumns[1], right.sseColumns[1]);
        __m128 c3 = _mm_sub_ps(sseColumns[2], right.sseColumns[2]);
        __m128 c4 = _mm_sub_ps(sseColumns[3], right.sseColumns[3]);
        
        return Matrix4(c1, c2, c3, c4);
    }

    inline Matrix4& operator+=(const Matrix4& right)
    {
        sseColumns[0] = _mm_add_ps(sseColumns[0], right.sseColumns[0]);
        sseColumns[1] = _mm_add_ps(sseColumns[1], right.sseColumns[1]);
        sseColumns[2] = _mm_add_ps(sseColumns[2], right.sseColumns[2]);
        sseColumns[3] = _mm_add_ps(sseColumns[3], right.sseColumns[3]);
    }

    inline Matrix4& operator-=(const Matrix4& right)
    {
        sseColumns[0] = _mm_sub_ps(sseColumns[0], right.sseColumns[0]);
        sseColumns[1] = _mm_sub_ps(sseColumns[1], right.sseColumns[1]);
        sseColumns[2] = _mm_sub_ps(sseColumns[2], right.sseColumns[2]);
        sseColumns[3] = _mm_sub_ps(sseColumns[3], right.sseColumns[3]);
    }

    inline __m128 operator*(const __m128& sse) const
    {
        __m128 res = _mm_mul_ps(sseColumns[0], _mm_shuffle_ps(sse, sse, 0x00));
        res = _mm_add_ps(res, _mm_mul_ps(sseColumns[1], _mm_shuffle_ps(sse, sse, 0x55)));
        res = _mm_add_ps(res, _mm_mul_ps(sseColumns[2], _mm_shuffle_ps(sse, sse, 0xaa)));
        res = _mm_add_ps(res, _mm_mul_ps(sseColumns[3], _mm_shuffle_ps(sse, sse, 0xff)));
        return res;
    }

    inline Vector4 operator*(const Vector4& vec) const
    {
        return Vector4(*this * vec.sseData);
    }

    inline Vector3 operator*(const Vector3& vec) const
    {
        Vector4 res = *this * Vector4(vec.x, vec.y, vec.z, 1.0f);
        return Vector3(res.x, res.y, res.z);
    }

    inline Matrix4 operator*(const Matrix4& right) const
    {
        __m128 c0 = *this * right.sseColumns[0];
        __m128 c1 = *this * right.sseColumns[1];
        __m128 c2 = *this * right.sseColumns[2];
        __m128 c3 = *this * right.sseColumns[3];
        return Matrix4(c0, c1, c2, c3);
    }

    // A *= B == B * A; This way makes more sense to me
    inline Matrix4& operator*=(const Matrix4& right)
    {
        *this = right * (*this);
        return *this;
    }

    inline Matrix4& Translate(const Vector3& displacement)
    {
        *this *= Translation(displacement);
        return *this;
    }

    inline Matrix4& Rotate(const Vector3& axis, f32 angle)
    {
        *this *= Rotation(axis, angle);
        return *this;
    }

    inline Matrix4& Scale(const Vector3& scale)
    {
        *this *= Scaling(scale);
        return *this;
    }

    inline Matrix4& Scale(f32 scale)
    {
        *this *= Scaling(scale);
        return *this;
    }
};

inline Matrix4 operator*(const Matrix4& mat, f32 scalar)
{
    __m128 c1 = _mm_mul_ps(mat.sseColumns[0], _mm_set1_ps(scalar));
    __m128 c2 = _mm_mul_ps(mat.sseColumns[1], _mm_set1_ps(scalar));
    __m128 c3 = _mm_mul_ps(mat.sseColumns[2], _mm_set1_ps(scalar));
    __m128 c4 = _mm_mul_ps(mat.sseColumns[3], _mm_set1_ps(scalar));
    return Matrix4(c1, c2, c3, c4);
}

inline Matrix4 operator*(f32 scalar, const Matrix4& mat)
{
    __m128 c1 = _mm_mul_ps(mat.sseColumns[0], _mm_set1_ps(scalar));
    __m128 c2 = _mm_mul_ps(mat.sseColumns[1], _mm_set1_ps(scalar));
    __m128 c3 = _mm_mul_ps(mat.sseColumns[2], _mm_set1_ps(scalar));
    __m128 c4 = _mm_mul_ps(mat.sseColumns[3], _mm_set1_ps(scalar));
    return Matrix4(c1, c2, c3, c4);
}

inline Matrix4 operator/(const Matrix4& mat, f32 scalar)
{
    __m128 c1 = _mm_div_ps(mat.sseColumns[0], _mm_set1_ps(scalar));
    __m128 c2 = _mm_div_ps(mat.sseColumns[1], _mm_set1_ps(scalar));
    __m128 c3 = _mm_div_ps(mat.sseColumns[2], _mm_set1_ps(scalar));
    __m128 c4 = _mm_div_ps(mat.sseColumns[3], _mm_set1_ps(scalar));
    return Matrix4(c1, c2, c3, c4);
}