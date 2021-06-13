#pragma once

#include <cmath>
#include "../basic_types.h"
#include "../math.h"

union Vector3
{
    struct
    {
        f32 x, y, z;
    };

    struct
    {
        f32 r, g, b;
    };

    f32 data[3];

    inline f32& operator[](s32 index)
    {
        return data[index];
    }

    Vector3()
    :   x(0.0f), y(0.0f), z(0.0f) {}

    Vector3(f32 val)
    :   x(val), y(val), z(val) {}

    Vector3(f32 x, f32 y, f32 z)
    :   x(x), y(y), z(z) {}

    inline f32 SqrLength() const
    {
        return x * x + y * y + z * z;
    }

    inline f32 Length() const
    {
        return sqrt(x * x + y * y + z * z);
    }

    inline Vector3 Unit() const
    {
        f32 length = Length();

        if (length == 0.0f)
            return Vector3();

        return Vector3(x / length, y / length, z / length);
    }

    inline Vector3& Normalize()
    {
        f32 length = Length();

        if (length == 0.0f)
            return *this;

        x /= length;
        y /= length;
        z /= length;

        return *this;
    }

    inline Vector3& operator-()
    {
        x = -x;
        y = -y;
        z = -z;

        return *this;
    }

    inline Vector3& operator+=(const Vector3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;

        return *this;
    }

    inline Vector3& operator-=(const Vector3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;

        return *this;
    }

    inline Vector3& operator*=(f32 scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;

        return *this;
    }

    inline Vector3& operator/=(f32 scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;

        return *this;
    }

    inline bool operator==(const Vector3& right) const
    {
        return x == right.x && y == right.y && z == right.z;
    }

    inline bool operator!=(const Vector3& right) const
    {
        return x != right.x || y != right.y || z != right.z;
    }
};

inline Vector3 operator+(const Vector3& left, const Vector3& right)
{
    return Vector3(left.x + right.x, left.y + right.y, left.z + right.z);
}

inline Vector3 operator-(const Vector3& left, const Vector3& right)
{
    return Vector3(left.x - right.x, left.y - right.y, left.z - right.z);
}

inline Vector3 operator*(const Vector3& vec, f32 scalar)
{
    return Vector3(vec.x * scalar, vec.y * scalar, vec.z * scalar);
}

inline Vector3 operator*(f32 scalar, const Vector3& vec)
{
    return Vector3(vec.x * scalar, vec.y * scalar, vec.z * scalar);
}

inline Vector3 operator/(const Vector3& vec, f32 scalar)
{
    return Vector3(vec.x / scalar, vec.y / scalar, vec.z / scalar);
}

inline Vector3 operator/(f32 scalar, const Vector3& vec)
{
    return Vector3(vec.x / scalar, vec.y / scalar, vec.z / scalar);
}

inline f32 Dot(const Vector3& left, const Vector3& right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

inline Vector3 Cross(const Vector3& left, const Vector3& right)
{
    Vector3 v;

    v.x = left.y * right.z - left.z * right.y;
    v.y = left.z * right.x - left.x * right.z;
    v.z = left.x * right.y - left.y * right.x;

    return v;
}

inline f32 SqrDistance(const Vector3& v1, const Vector3& v2)
{
    return (v1 - v2).SqrLength();
}

inline f32 Distance(const Vector3& v1, const Vector3& v2)
{
    return (v1 - v2).Length();
}

inline Vector3 Lerp(const Vector3& a, const Vector3& b, f32 t)
{
    t = Clamp(t, 0.0f, 1.0f);
    f32 _1_t = 1.0f - t;
 
    Vector3 res;

    res.x = _1_t * a.x + t * b.x;
    res.y = _1_t * a.y + t * b.y;
    res.z = _1_t * a.z + t * b.z;

    return res;
}