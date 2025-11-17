#pragma once
#include <array>

double Real48ToDouble(void* buf);
std::array<unsigned char, 6> DoubleToReal48(double D);
