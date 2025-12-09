#pragma once
#include "XKey.h"
#include "XScan.h"
#include "XXPage.h"

class XWorkFile
{
public:
	XWorkFile(FileD* parent, XScan* AScan, std::vector<XKey*>& AK);
	~XWorkFile();

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

	void Main(OperationType oper_type, uint8_t* record);
	void CopyIndex(XKey* K, std::vector<KeyFldD*>& KF, OperationType oper_type, uint8_t* record);
	bool GetCRec(uint8_t* record);
	void Output(XKey* xKey, WRec* R, uint8_t* record);
	void Reset(std::vector<KeyFldD*>& KF, int RestBytes, OperationType oper_type, int NRecs);
	void SortMerge(XKey* xKey, uint8_t* record);

private:
	FileD* _parent;
	std::vector<XKey*> x_keys_;
	XXPage* xxPage = nullptr;
	XScan* xScan = nullptr;

	void TestErr();
	void FinishIndex(XKey* xKey);
	int GetFreeNr();
	void ReadWPage(WPage* W, int Pg);
	void WriteWPage(XKey* xKey, unsigned short N, int Pg, int Nxt, int Chn, uint8_t* record);
	void Merge(XKey* xKey, uint8_t* record);
	void Merge2Chains(XKey* xKey, int Pg1, int Pg2, int Pg, int Nxt, uint8_t* record);
	void PutFreeNr(int N);
};
