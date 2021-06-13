#pragma once

#include <cmath>
#include "../math.h"
#include "../basic_types.h"

union Vector2
{
    struct
    {
        f32 x, y;
    };

    struct
    {
        f32 u, v;
    };

    struct
    {
        f32 width, height;
    };

    f32 data[2];

    inline f32& operator[](s32 index)
    {
        return data[index];
    }

    Vector2()
    :   x(0.0f), y(0.0f) {}

    Vector2(f32 val)
    :   x(val), y(val) {}

    Vector2(f32 x, f32 y)
    :   x(x), y(y) {}

    inline f32 SqrLength() const
    {
        return x * x + y * y;
    }

    inline f32 Length() const
    {
        return sqrt(x * x + y * y);
    }

    inline Vector2 Unit() const
    {
        f32 length = Length();

        if (length == 0.0f)
            return Vector2();

        return Vector2(x / length, y / length);
    }

    inline Vector2& Normalize()
    {
        f32 length = Length();

        if (length == 0.0f)
            return *this;

        x /= length;
        y /= length;

        return *this;
    }

    inline Vector2& operator-()
    {
        x = -x;
        y = -y;

        return *this;
    }

    inline Vector2& operator+=(const Vector2& other)
    {
        x += other.x;
        y += other.y;

        return *this;
    }

    inline Vector2& operator-=(const Vector2& other)
    {
        x -= other.x;
        y -= other.y;

        return *this;
    }

    inline Vector2& operator*=(f32 scalar)
    {
        x *= scalar;
        y *= scalar;

        return *this;
    }

    inline Vector2& operator/=(f32 scalar)
    {
        x /= scalar;
        y /= scalar;

        return *this;
    }

    inline bool operator==(const Vector2& right) const
    {
        return x == right.x && y == right.y;
    }

    inline bool operator!=(const Vector2& right) const
    {
        return x != right.x || y != right.y;
    }
};

inline Vector2 operator+(const Vector2& left, const Vector2& right)
{
    return Vector2(left.x + right.x, left.y + right.y);
}

inline Vector2 operator-(const Vector2& left, const Vector2& right)
{
    return Vector2(left.x - right.x, left.y - right.y);
}

inline Vector2 operator*(const Vector2& vec, f32 scalar)
{
    return Vector2(vec.x * scalar, vec.y * scalar);
}

inline Vector2 operator*(f32 scalar, const Vector2& vec)
{
    return Vector2(vec.x * scalar, vec.y * scalar);
}

inline Vector2 operator/(const Vector2& vec, f32 scalar)
{
    return Vector2(vec.x / scalar, vec.y / scalar);
}

inline f32 Dot(const Vector2& left, const Vector2& right)
{
    return left.x * right.x + left.y * right.y;
}

inline f32 SqrDistance(const Vector2& v1, const Vector2& v2)
{
    return (v1 - v2).SqrLength();
}

inline f32 Distance(const Vector2& v1, const Vector2& v2)
{
    return (v1 - v2).Length();
}

inline Vector2 Lerp(const Vector2& a, const Vector2& b, f32 t)
{
    t = Clamp(t, 0.0f, 1.0f);
    f32 _1_t = 1.0f - t;
 
    Vector2 res;

    res.x = _1_t * a.x + t * b.x;
    res.y = _1_t * a.y + t * b.y;

    return res;
}