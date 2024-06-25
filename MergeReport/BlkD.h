#pragma once
#include <string>
#include <vector>

class FrmlElemSum;
class FrmlElem;
struct RFldD;
struct AssignD;

class BlkD
{
public:
	FrmlElem* Bool = nullptr;
	std::vector<FrmlElemSum*>* Sum = nullptr;
	size_t lineLength = 0; // total length of line after printing this block
	bool AbsLine = false, SetPage = false, NotAtEnd = false, FF1 = false, FF2 = false;
	FrmlElem* LineBound = nullptr;
	FrmlElem* LineNo = nullptr;
	FrmlElem* PageNo = nullptr;
	uint16_t NTxtLines = 0;
	uint16_t NBlksFrst = 0; // pozice 1. bloku na radku; pred nim jsou mezery
	uint16_t DHLevel = 0;
	std::vector<RFldD*> ReportFields; // vektor jednotlivych poli formulare
	std::vector<AssignD*> BeforeProc; // prikazy provedene pred blokem
	std::vector<AssignD*> AfterProc; // prikazy provedene po bloku
	std::vector<std::string> lines; // vektor jednotlivych radku
};

extern std::vector<BlkD*> RprtHd;
extern std::vector<BlkD*> PageHd;
extern std::vector<BlkD*> PageFt;
