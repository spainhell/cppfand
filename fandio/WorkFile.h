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
	WORD RecLen = 0, MaxOnWPage = 0, WPageSize = 0;
	longint MaxWPage = 0, WRoot = 0, NChains = 0, PgWritten = 0;
	longint WBaseSize = 0;
	WPage* PW = nullptr;
	WPage* PW1 = nullptr;
	WPage* PW2 = nullptr;
	longint FreeNr[5]{ 0, 0, 0, 0, 0 };
	WORD NFreeNr = 0;
	longint IRec = 0, RecNr = 0;
	KeyFldD* KFRoot = nullptr;
	void Reset(KeyFldD* KF, longint RestBytes, char Typ, longint NRecs);
	void SortMerge();
	virtual bool GetCRec() = 0;
	virtual void Output(WRec* R) = 0;
private:
	void TestErr();
	longint GetFreeNr();
	void Merge();
	void Merge2Chains(longint Pg1, longint Pg2, longint Pg, longint Nxt);
	void PutFreeNr(longint N);
	void ReadWPage(WPage* W, longint Pg);
	void WriteWPage(WORD N, longint Pg, longint Nxt, longint Chn);
};
