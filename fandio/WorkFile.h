#pragma once
#include "WPage.h"
#include "WRec.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/FieldDescr.h"

class WorkFile
{
public:
	WorkFile();
	virtual ~WorkFile();
	FILE* Handle = nullptr;
	unsigned short RecLen = 0, MaxOnWPage = 0, WPageSize = 0;
	int MaxWPage = 0, WRoot = 0, NChains = 0, PgWritten = 0;
	int WBaseSize = 0;
	WPage* PW = nullptr;
	WPage* PW1 = nullptr;
	WPage* PW2 = nullptr;
	int FreeNr[5]{ 0, 0, 0, 0, 0 };
	unsigned short NFreeNr = 0;
	int IRec = 0, RecNr = 0;
	KeyFldD* KFRoot = nullptr;
	void Reset(KeyFldD* KF, int RestBytes, char Typ, int NRecs);
	void SortMerge();
	virtual bool GetCRec() = 0;
	virtual void Output(WRec* R) = 0;
private:
	void TestErr();
	int GetFreeNr();
	void Merge();
	void Merge2Chains(int Pg1, int Pg2, int Pg, int Nxt);
	void PutFreeNr(int N);
	void ReadWPage(WPage* W, int Pg);
	void WriteWPage(unsigned short N, int Pg, int Nxt, int Chn);
};
