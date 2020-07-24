#pragma once
#include <cstdint>
#include "SDL_pixels.h"
#include <cmath>

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

constexpr int32 blockSize = 32;

const float pi = 3.14159f;
const float tau = 2 * pi;

const SDL_Color Red = { 255, 0, 0, 255 };
const SDL_Color Green = { 0, 255, 0, 255 };
const SDL_Color Blue = { 0, 0, 255, 255 };

const SDL_Color transRed = { 255, 0, 0, 127 };
const SDL_Color transGreen = { 0, 255, 0, 127 };
const SDL_Color transBlue = { 0, 0, 255, 127 };
const SDL_Color transOrange = { 255, 128, 0, 127 };

const SDL_Color lightRed = { 255, 0, 0, 63 };
const SDL_Color lightGreen = { 0, 255, 0, 63 };
const SDL_Color lightBlue = { 0, 0, 255, 63 };

const SDL_Color White = { 255, 255, 255, 255 };
const SDL_Color lightWhite = { 150, 150, 150, 150 };
const SDL_Color Black = { 0, 0, 0, 255 };
const SDL_Color lightBlack = { 0, 0, 0, 150 };
const SDL_Color Brown = { 130, 100, 70, 255 };  //used for dirt block
const SDL_Color Mint = { 0, 255, 127, 255 };   //used for corner block
const SDL_Color Orange = { 255, 127, 0, 255 };   //used for edge block
const SDL_Color Grey = { 127, 127, 127, 255 }; //used for floating block

const SDL_Color HealthBarBackground = { 58, 85, 58, 255 };


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
    int x = 0;
    int y = 0;
};


struct Rectangle {
    Vector bottomLeft;
    Vector topRight;

    float Width()
    {
        return topRight.x - bottomLeft.x;
    }

    float Height()
    {
        return topRight.y - bottomLeft.y;
    }

    bool Collision(VectorInt loc)
    {
        bool result = false;
        if (loc.y > bottomLeft.x && loc.y < topRight.x)
            if (loc.x > bottomLeft.x && loc.x < topRight.x)
                result = true;
        return result;
    }
};

struct Rectangle_Int {
    VectorInt bottomLeft;
    VectorInt topRight;

    int32 Width()
    {
        return topRight.x - bottomLeft.x;
    }

    int32 Height()
    {
        return topRight.y - bottomLeft.y;
    }

    //bool Collision(VectorInt loc)
    //{
    //    bool result = false;
    //    if (loc.y > bottomLeft.x && loc.y < topRight.x)
    //        if (loc.x > bottomLeft.x && loc.x < topRight.x)
    //            result = true;
    //    return result;
    //}
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


float Atan2fToDegreeDiff(float theta);


inline float DotProduct(Vector a, Vector b)
{
    return a.x * b.x + a.y * b.y;
}



inline float Pythags(Vector a)
{
    return sqrtf(powf(a.x, 2) + powf(a.y, 2));
}