#pragma once
#include <cstdint>
#include <functional>

using RunErrorCallback = std::function<void(int32_t)>;

class Instr_getindex;

[[nodiscard]] int32_t GetIndex(Instr_getindex* PD, RunErrorCallback err_callback);

