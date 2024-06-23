#pragma once
#include "WPage.h"
#include "WRec.h"
#include "../Core/KeyFldD.h"

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
	void SortMerge(void* record);
	virtual bool GetCRec(void* record) = 0;
	virtual void Output(WRec* R, void* record) = 0;
protected:
	FileD* _parent;
private:
	void TestErr();
	int GetFreeNr();
	void Merge(void* record);
	void Merge2Chains(int Pg1, int Pg2, int Pg, int Nxt, void* record);
	void PutFreeNr(int N);
	void ReadWPage(WPage* W, int Pg);
	void WriteWPage(unsigned short N, int Pg, int Nxt, int Chn, void* record);
};
