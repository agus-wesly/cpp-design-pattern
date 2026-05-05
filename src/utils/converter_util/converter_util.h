#pragma once
#include <cmath>

class ConverterUtil
{
    public:
        static double degrees_to_radians(double degree)
        {
            return degree * (M_PI / 180);
        }
};
