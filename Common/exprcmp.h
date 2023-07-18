#pragma once
#include <string>

bool CmpStringWithMask(const std::string& value, std::string mask);
std::string RegexFromString(std::string mask);
bool FindShiftCtrlAltFxx(std::string input, std::string& key, unsigned char& fnKeyNr);