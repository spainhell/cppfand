#pragma once
#include <array>

double Real48ToDouble(uint8_t* buf);
std::array<uint8_t, 6> DoubleToReal48(double D);
