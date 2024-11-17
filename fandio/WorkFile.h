#pragma once
#include "WPage.h"
#include "WRec.h"
#include "../Core/KeyFldD.h"

class XKey;
class FileD;

class WorkFile
{
public:
	WorkFile(FileD* parent);
	virtual ~WorkFile();
	HANDLE Handle = nullptr;
	unsigned short RecLen = 0, MaxOnWPage = 0, WPageSize = 0;
	int MaxWPage = 0, WRoot = 0, NChains = 0, PgWritten = 0;
	int WBaseSize = 0;
	WPage* PW = nullptr;
	WPage* PW1 = nullptr;
	WPage* PW2 = nullptr;
	int FreeNr[5]{ 0, 0, 0, 0, 0 };
	unsigned short NFreeNr = 0;
	int IRec = 0, RecNr = 0;
	std::vector<KeyFldD*> KFRoot;
	void Reset(std::vector<KeyFldD*>& KF, int RestBytes, char Typ, int NRecs);
	void SortMerge(XKey* xKey, void* record);
	virtual bool GetCRec(void* record) = 0;
	virtual void Output(XKey* xKey, WRec* R, void* record) = 0;
protected:
	FileD* _parent;
private:
	void TestErr();
	int GetFreeNr();
	void Merge(XKey* xKey, void* record);
	void Merge2Chains(XKey* xKey, int Pg1, int Pg2, int Pg, int Nxt, void* record);
	void PutFreeNr(int N);
	void ReadWPage(WPage* W, int Pg);
	void WriteWPage(XKey* xKey, unsigned short N, int Pg, int Nxt, int Chn, void* record);
};
