#pragma once
#include <string>

bool CmpStringWithMask(std::string Value, std::string Mask);
std::string RegexFromString(std::string Mask);
bool FindShiftCtrlAltFxx(std::string input, std::string& key, unsigned char& fnKeyNr);