#pragma once
#include <string>
#include <vector>

class FrmlElem;

// One interval of key values for XScan (ScanMode::Interval).
// Bounds are given by expressions FL1 and FL2 (FL2 empty = same as FL1),
// XScan evaluates them into X1 and X2 when the scan is reset.
struct KeyInD
{
	std::vector<FrmlElem*> FL1;
	std::vector<FrmlElem*> FL2;
	int XNrBeg = 0, N = 0;
	std::string X1;
	std::string X2;
};
