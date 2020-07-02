#pragma once
#include <cmath>


inline double Real48ToDouble(void* buf)
{
	unsigned char* real48 = (unsigned char*)buf;
	if (real48[0] == 0) return 0.0; // Null exponent = 0
	double exponent = real48[0] - 129.0;
	double mantissa = 0.0;
	for (int i = 1; i < 5; i++) // loop through bytes 1 - 4
	{
		mantissa += real48[i];
		mantissa *= 0.00390625; // mantissa /= 256
	}

	mantissa += (real48[5] & 0x7F);
	mantissa *= 0.0078125; // mantissa /= 128
	mantissa += 1.0;

	if ((real48[5] & 0x80) == 0x80) // Sign bit check
		mantissa = -mantissa;

	return mantissa * pow(2.0, exponent);
}