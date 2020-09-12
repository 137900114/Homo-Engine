#pragma once
#include <math.h>

#define PI 3.1415926f

inline float clamp(float upper,float lower,float target) {
	if (target > upper) {
		return upper;
	}
	else if (target < lower ) {
		return lower;
	}
	return target;
}


inline float get_angle(float sinangle, float cosangle, bool radius = true) {

	//avoid the sinangle and cosangle value overflow,because of the accurancy of the floating point system
	//when the sin and cos values are close to 1. or -1.,maybe some overflow will occur(some thing like 1.0000012 etc.)
	//clamp the value of sinangle and cosangle between -1.0~1.0 to avoid this condition
	sinangle = clamp(1.,-1.,sinangle);
	cosangle = clamp(1.,-1.,cosangle);
	float angle = asin(sinangle) >= 0. ? acos(cosangle) : 2 * PI - acos(cosangle);
	if (!radius) {
		return angle * 180.f / PI;
	}
	return angle;
}


inline size_t round_up(size_t num,size_t step) {
	return (num + step - 1) & ~(step - 1);
}