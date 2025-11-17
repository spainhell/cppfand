#include "realDouble.h"


double Real48ToDouble(void* buf)
{
	unsigned char* real48 = (unsigned char*)buf;
	if (real48[0] == 0) return 0.0; // null exponent = 0
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

	if ((real48[5] & 0x80) == 0x80) // sign bit check
		mantissa = -mantissa;

	return mantissa * pow(2.0, exponent);
}

std::array<unsigned char, 6> DoubleToReal48(double D)
{
	std::array<unsigned char, 6> real48{ 0, 0, 0, 0, 0, 0 };
	unsigned char* da = (unsigned char*)&D;

	// copy negative flag
	real48[5] |= da[7] & 0x80;
	// get exponent
	unsigned char b1 = da[7] & 0x7F;
	unsigned short n = (unsigned short)(b1 << 4);
	unsigned char b2 = da[6] & 0xF0;
	b2 >>= 4;
	n |= b2;
	if (n == 0) return real48;

	unsigned char ex = (unsigned char)(n - 1023);
	real48[0] = (unsigned char)(ex + 129);

	// copy the Mantissa
	real48[5] |= (unsigned char)((da[6] & 0x0f) << 3); //Get the last four bits
	real48[5] |= (unsigned char)((da[5] & 0xe0) >> 5); //Get the first three bits

	real48[4] = (unsigned char)((da[5] & 0x1f) << 3); //Get the last 5 bits
	real48[4] |= (unsigned char)((da[4] & 0xe0) >> 5); //Get the first three bits

	real48[3] = (unsigned char)((da[4] & 0x1f) << 3); //Get the last 5 bits
	real48[3] |= (unsigned char)((da[3] & 0xe0) >> 5); //Get the first three bits

	real48[2] = (unsigned char)((da[3] & 0x1f) << 3); //Get the last 5 bits
	real48[2] |= (unsigned char)((da[2] & 0xe0) >> 5); //Get the first three bits

	real48[1] = (unsigned char)((da[2] & 0x1f) << 3); //Get the last 5 bits
	real48[1] |= (unsigned char)((da[1] & 0xe0) >> 5); //Get the first three bits

	return real48;
}