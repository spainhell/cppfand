#pragma once
#include <vector>

class XKey;
class FileD;
class FrmlElem;
struct KeyInD;

struct RprtFDListEl
{
	//RprtFDListEl* Chain = nullptr;
	FileD* FD = nullptr;
	XKey* ViewKey = nullptr;
	FrmlElem* Cond = nullptr;
	std::vector<KeyInD*> KeyIn;
	bool SQLFilter = false;
	void* LVRecPtr = nullptr;
};
