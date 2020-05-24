#include "Math.h"
#include <cmath>

float Atan2fToDegreeDiff(float theta)
{
    float result = 0;
    if (theta < 0)
        return RadToDeg(fabsf(theta));
    else
        return 360 + 360 - RadToDeg(theta);
}