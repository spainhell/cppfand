#pragma once
#include <string>
#include "pstring.h"

bool EquUpCase(pstring& S1, pstring& S2); // r274 ASM
bool EquUpCase(std::string S1, std::string S2);
bool EquUpCase(const char* S, pstring& S1);