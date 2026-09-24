#pragma once
#include <array>
#include <cstdint>

// 10^0 .. 10^20
extern double Power10[21];

double Real48ToDouble(uint8_t* buf);
std::array<uint8_t, 6> DoubleToReal48(double D);
