#pragma once
#include "../cppfand/access-structs.h"
#include "../cppfand/constants.h"
#include "XWKey.h"
#include "../cppfand/models/FrmlElem.h"

struct KeyInD;

class XScan
{
public:
	~XScan();
	FileD* FD = nullptr;
	XKey* Key = nullptr;
	FrmlElem* Bool = nullptr;
	BYTE Kind = 0;
	longint NRecs = 0, IRec = 0, RecNr = 0;
	bool hasSQLFilter = false, eof = false;
	XScan(FileD* aFD, XKey* aKey, KeyInD* aKIRoot, bool aWithT);
	void Reset(FrmlElem* ABool, bool SQLFilter);
	void ResetSort(KeyFldD* aSK, FrmlPtr& BoolZ, LockMode OldMd, bool SQLFilter);
	void SubstWIndex(WKeyDPtr WK);
	void ResetOwner(XString* XX, FrmlPtr aBool);
	void ResetOwnerIndex(LinkD* LD, LocVar* LV, FrmlPtr aBool);
#ifdef FandSQL
	void ResetSQLTxt(FrmlPtr Z);
#endif
	void ResetLV(void* aRP);
	void Close();
	void SeekRec(longint I);
	void GetRec();
private:
	KeyInD* KIRoot = nullptr;
	LocVar* OwnerLV = nullptr;
	KeyFldD* SK = nullptr;
	//XItem* X = nullptr;
	size_t _item = 1;
	XPage* P = nullptr;
	WORD NOnPg = 0;
	KeyInD* KI = nullptr;
	longint NOfKI = 0, iOKey = 0;
	bool TempWX = false, NotFrst = false, withT = false;
	void* Strm = nullptr; // {SQLStreamPtr or LVRecPtr}
	void SeekOnKI(longint I);
	void SeekOnPage(longint Page, WORD I);
	void NextIntvl();
};