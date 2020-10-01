#pragma once
#include <cstdint>
#include "SDL_pixels.h"
#include <cmath>
#include "gb_math.h"

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
//using uint64 = uint64_t;
//typedef uint64_t uint64;
//#define uint64_t uint64

struct Color {
    float r;
    float g;
    float b;
    float a;
};

const Color Red = { 1.0f, 0.0f, 0.0f, 1.0f };
const Color Green = { 0.0f, 1.0f, 0.0f, 1.0f };
const Color Blue = { 0.0f, 0.0f, 1.0f, 1.0f };

const Color transRed = { 1.0f, 0.0f, 0.0f, 0.5f };
const Color transGreen = { 0.0f, 1.0f, 0.0f, 0.5f };
const Color transBlue = { 0.0f, 0.0f, 1.0f, 0.5f };
const Color transOrange = { 1.0f, 0.5f, 0.0f, 0.5f };

const Color lightRed = { 1.0f, 0.0f, 0.0f, 0.25f };
const Color lightGreen = { 0.0f, 1.0f, 0.0f, 0.25f };
const Color lightBlue = { 0.0f, 0.0f, 1.0f, 0.25f };

const Color White = { 1.0f, 1.0f, 1.0f, 1.0f };
const Color lightWhite = { 0.58f, 0.58f, 0.58f, 0.58f };
const Color Black = { 0.0f, 0.0f, 0.0f, 1.0f };
const Color lightBlack = { 0.0f, 0.0f, 0.0f, 0.58f };
const Color Brown = { 0.5f, 0.4f, 0.25f, 1.0f };  //used for dirt block
const Color Mint = { 0.0f, 1.0f, 0.5f, 1.0f };   //used for corner block
const Color Orange = { 1.0f, 0.5f, 0.0f, 1.0f };   //used for edge block
const Color Grey = { 0.5f, 0.5f, 0.5f, 1.0f }; //used for floating block

const Color HealthBarBackground = { 0.25f, 0.33f, 0.25f, 1.0f};
constexpr int32 blockSize = 32;

const float pi = 3.14159f;
const float tau = 2 * pi;
const float inf = 0x7f800000;

const uint32 CollisionNone = 0;
const uint32 CollisionTop = 1;
const uint32 CollisionBot = 2;
const uint32 CollisionRight = 4;
const uint32 CollisionLeft = 8;


struct Vector {
    float x = 0;
    float y = 0;
};


struct VectorInt {
    int32 x = 0;
    int32 y = 0;
};


struct Rectangle {
    Vector botLeft;
    Vector topRight;

    float Width()
    {
        return topRight.x - botLeft.x;
    }

    float Height()
    {
        return topRight.y - botLeft.y;
    }

    bool Collision(VectorInt loc)
    {
        bool result = false;
        if (loc.y > botLeft.x && loc.y < topRight.x)
            if (loc.x > botLeft.x && loc.x < topRight.x)
                result = true;
        return result;
    }
};

struct Rectangle_Int {
    VectorInt bottomLeft;
    VectorInt topRight;

    int32 Width() const
    {
        return topRight.x - bottomLeft.x;
    }

    int32 Height() const
    {
        return topRight.y - bottomLeft.y;
    }
};


inline Vector operator-(const Vector& lhs, const Vector& rhs)
{
    return { lhs.x - rhs.x, lhs.y - rhs.y };
}
inline Vector operator-(const Vector& lhs, const float rhs)
{
    return { lhs.x - rhs, lhs.y - rhs };
}

inline Vector operator+(const Vector& lhs, const Vector& rhs)
{
    return { lhs.x + rhs.x, lhs.y + rhs.y };
}


inline Vector operator*(const Vector& a, const float b)
{
    return { a.x * b,  a.y * b };
}
inline VectorInt operator*(const VectorInt& a, const float b)
{
    return { int(a.x * b),  int(a.y * b) };
}


inline const Vector& operator+=(Vector& lhs, const Vector& rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}



template <typename T>
T Min(T a, T b)
{
    return a < b ? a : b;
}

template <typename T>
T Max(T a, T b)
{
    return a > b ? a : b;
}

template <typename T>
T Clamp(T v, T min, T max)
{
    return Max(min, Min(max, v));
}

inline float RadToDeg(float angle)
{
    return ((angle) / (tau)) * 360;
}

inline float DegToRad(float angle)
{
    return (angle / 360 ) * (tau);
}


inline float VectorDistance(Vector A, Vector B)
{
    return sqrtf(powf(B.x - A.x, 2) + powf(B.y - A.y, 2));
}


inline Vector Normalize(Vector v)
{
    float hyp = sqrtf(powf(v.x, 2) + powf(v.y, 2));
    return { (v.x / hyp), (v.y / hyp) };
}


/*
Atan2f return value:

3pi/4      pi/2        pi/4


            O
+/-pi      /|\          0
           / \


-3pi/4    -pi/2        pi/4
*/

inline float DotProduct(Vector a, Vector b)
{
    return a.x * b.x + a.y * b.y;
}

inline float Pythags(Vector a)
{
    return sqrtf(powf(a.x, 2) + powf(a.y, 2));
}

void Swap(void* a, void* b, const int size);