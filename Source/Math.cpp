#include "Math.h"
#include <cmath>


/*
Atan2f return value:

3pi/4      pi/2        pi/4


            O
+/-pi      /|\          0
           / \


-3pi/4    -pi/2        -pi/4
*/
float Atan2fToDegreeDiff(float theta)
{
    return 360 - RadToDeg(theta);
}