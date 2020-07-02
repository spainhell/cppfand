#include "pch.h"
#include "random.h"
#include <cmath>

unsigned int RandSeed = 0;

float Random()
{
	const unsigned int a = 134775813 * RandSeed + 1;
	const unsigned int b = 0xFFFFFFFF;
	RandSeed = a % b;
	float X = (float)(RandSeed / pow(2, 16));
	while (X > 1) X = X / 10;
	return X;
}

unsigned Random(int Maximum)
{
	const unsigned int a = 134775813 * RandSeed + 1;
	const unsigned int b = 0xFFFFFFFF;
	RandSeed = a % b;
	const auto X = static_cast<unsigned int>(RandSeed / pow(2, 16));
	return X % Maximum;
}
