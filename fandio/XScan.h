#pragma once
#include "../cppfand/access-structs.h"
#include "../cppfand/models/FrmlElem.h"

struct KeyInD;

class XScan
{
public:
	~XScan();
	FileD* FD = nullptr;
	XKey* Key = nullptr;
	FrmlElem* Bool = nullptr;
	unsigned char Kind = 0;
	int NRecs = 0, IRec = 0, RecNr = 0;
	bool hasSQLFilter = false, eof = false;
	XScan(FileD* aFD, XKey* aKey, KeyInD* aKIRoot, bool aWithT);
	void Reset(FrmlElem* ABool, bool SQLFilter);
	void ResetSort(KeyFldD* aSK, FrmlElem* BoolZ, LockMode OldMd, bool SQLFilter);
	void SubstWIndex(XWKey* WK);
	void ResetOwner(XString* XX, FrmlElem* aBool);
	void ResetOwnerIndex(LinkD* LD, LocVar* LV, FrmlElem* aBool);
#ifdef FandSQL
	void ResetSQLTxt(FrmlElem* Z);
#endif
	void ResetLV(void* aRP);
	void Close();
	void SeekRec(int I);
	void GetRec(void* record);
private:
	KeyInD* KIRoot = nullptr;
	LocVar* OwnerLV = nullptr;
	KeyFldD* SK = nullptr;
	size_t _item = 1;
	XPage* page_ = nullptr;
	unsigned short items_on_page_ = 0;
	KeyInD* KI = nullptr;
	int NOfKI = 0, iOKey = 0;
	bool TempWX = false;
	bool NotFrst = false;
	bool withT = false;
	void* Strm = nullptr; // {SQLStreamPtr or LVRecPtr}
	void SeekOnKI(int I);
	void SeekOnPage(int pageNr, unsigned short i);
	void NextIntvl();
};