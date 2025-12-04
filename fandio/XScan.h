#pragma once
#include "XWKey.h"
#include "../Common/LinkD.h"
#include "../Common/LocVar.h"

struct KeyInD;

class XScan
{
public:
	XScan(FileD* aFD, XKey* aKey, std::vector<KeyInD*>& aKIRoot, bool aWithT);
	~XScan();
	FileD* FD = nullptr;
	XKey* Key = nullptr;
	FrmlElem* Bool = nullptr;
	unsigned char Kind = 0;
	int NRecs = 0, IRec = 0, RecNr = 0;
	bool hasSQLFilter = false, eof = false;
	void Reset(FrmlElem* ABool, bool SQLFilter, uint8_t* record);
	void ResetSort(std::vector<KeyFldD*>& aSK, FrmlElem* BoolZ, LockMode OldMd, bool SQLFilter, uint8_t* record);
	void SubstWIndex(XWKey* WK);
	void ResetOwner(XString* XX, FrmlElem* aBool);
	[[nodiscard]] int32_t ResetOwnerIndex(LinkD* LD, LocVar* LV, FrmlElem* aBool);
#ifdef FandSQL
	void ResetSQLTxt(FrmlElem* Z);
#endif
	void ResetLV(void* aRP);
	void Close();
	void SeekRec(int I);
	void GetRec(uint8_t* record);
private:
	std::vector<KeyInD*> KIRoot;
	LocVar* OwnerLV = nullptr;
	std::vector<KeyFldD*> SK;
	size_t _item = 1;
	XPage* page_ = nullptr;
	unsigned short items_on_page_ = 0;
	std::vector<KeyInD*>::iterator KI;
	int NOfKI = 0, iOKey = 0;
	bool TempWX = false;
	bool NotFrst = false;
	bool withT = false;
	void* Strm = nullptr; // {SQLStreamPtr or LVRecPtr}
	void SeekOnKI(int I);
	void SeekOnPage(int pageNr, unsigned short i);
	void NextIntvl();
};