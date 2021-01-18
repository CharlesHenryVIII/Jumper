#include "Math.h"
#include <cmath>

void Swap(void* a, void* b, const int size)
{

    char* c = (char*)a;
    char* d = (char*)b;
    for (int i = 0; i < size; i++)
    {

		char temp = c[i];
		c[i] = d[i];
		d[i] = temp;
    }
}

Vector CreateRandomVector(const Vector& min, const Vector& max)
{
    return { Random<float>(min.x, max.x),
             Random<float>(min.y, max.y) };
}

float LinearToAngularVelocity(Vector centerOfCircle, Vector position, Vector velocity)
{

	Vector toCenter = centerOfCircle - position;
	Vector tangent = { toCenter.y, -toCenter.x };
	float velocityScale = DotProduct(NormalizeZero(tangent), NormalizeZero(velocity));
	float radius = Distance(centerOfCircle, position);
	return Pythags(velocity) * velocityScale / (tau * radius);
}

