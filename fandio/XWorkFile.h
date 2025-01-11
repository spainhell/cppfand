#pragma once
#include "XKey.h"
#include "XScan.h"
#include "XXPage.h"
#include "../Core/KeyFldD.h"

class XWorkFile
{
public:
	XWorkFile(FileD* parent, XScan* AScan, std::vector<XKey*>& AK);

	HANDLE Handle = nullptr;
	unsigned short RecLen = 0, MaxOnWPage = 0, WPageSize = 0;
	int MaxWPage = 0, WRoot = 0, NChains = 0, PgWritten = 0;
	int WBaseSize = 0;
	FandXFile* xwFile = nullptr;
	XPage* xPage = nullptr;
	int nextXPage = 0;
	bool msgWritten = false;
	int IRec = 0, RecNr = 0;
	std::vector<KeyFldD*> KFRoot;
	unsigned short NFreeNr = 0;
	WPage* PW = nullptr;
	WPage* PW1 = nullptr;
	WPage* PW2 = nullptr;
	int FreeNr[5]{ 0, 0, 0, 0, 0 };

	void Main(char Typ, void* record);
	void CopyIndex(XKey* K, std::vector<KeyFldD*>& KF, char Typ, void* record);
	bool GetCRec(void* record);
	void Output(XKey* xKey, WRec* R, void* record);
	void Reset(std::vector<KeyFldD*>& KF, int RestBytes, char Typ, int NRecs);
	void SortMerge(XKey* xKey, void* record);

private:
	FileD* _parent;
	std::vector<XKey*> x_keys_;
	XXPage* xxPage = nullptr;
	XScan* xScan = nullptr;

	void TestErr();
	void FinishIndex(XKey* xKey);
	int GetFreeNr();
	void ReadWPage(WPage* W, int Pg);
	void WriteWPage(XKey* xKey, unsigned short N, int Pg, int Nxt, int Chn, void* record);
	void Merge(XKey* xKey, void* record);
	void Merge2Chains(XKey* xKey, int Pg1, int Pg2, int Pg, int Nxt, void* record);
	void PutFreeNr(int N);
};
