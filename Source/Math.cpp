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
