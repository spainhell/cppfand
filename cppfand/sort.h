#pragma once
#include "access.h"
#include "rdrun.h"
#include "XPage.h"
#include "XScan.h"

class XXPage;
class Instr_getindex;

class WPage /* ca. 64k pages in work file */
{
public:
	longint NxtChain = 0;
	longint Chain = 0;
	WORD NRecs = 0;
	BYTE A[65535]{ 0 };
	void Sort(WORD N, WORD RecLen);
};

class WRec /* record on WPage */
{
public:
	WRec() = default;
	WRec(WPage* wp);
	BYTE N[3]{ 0, 0, 0 };
	BYTE IR[3]{ 0, 0, 0 };
	XString X;
	longint GetN(); // ASM
	void PutN(longint NN); // ASM
	void PutIR(longint II); // ASM
	WORD Comp(WRec* R); // ASM
	WORD Compare(const WRec& w) const;
	void Deserialize(unsigned char* data);
	size_t Serialize(unsigned char* buffer);
	bool operator == (const WRec& w) const;
	bool operator < (const WRec& w) const;
	bool operator > (const WRec& w) const;
};

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
	virtual bool GetCRec();
	virtual void Output(WRec* R);
private:
	void TestErr();
	longint GetFreeNr();
	void Merge();
	void Merge2Chains(longint Pg1, longint Pg2, longint Pg, longint Nxt);
	void PutFreeNr(longint N);
	void ReadWPage(WPage* W, longint Pg);
	void WriteWPage(WORD N, longint Pg, longint Nxt, longint Chn);
};

class XWorkFile : public WorkFile
{
public:
	XWorkFile(XScan* AScan, XKey* AK);
	XXPage* PX = nullptr;
	KeyD* KD = nullptr;
	XScan* Scan = nullptr;
	bool MsgWritten = false;
	longint NxtXPage = 0;
	XWFile* XF = nullptr;
	XPage* XPP = nullptr;
	void Main(char Typ);
	void CopyIndex(KeyD* K, KeyFldD* KF, char Typ);
	bool GetCRec() override;
	void Output(WRec* R) override;
private:
	void FinishIndex();
};

class XXPage /* for building XPage */
{
public:
	XXPage* Chain = nullptr;
	XWorkFile* XW = nullptr;
	WORD Off = 0, MaxOff = XPageSize - 4; // XPageOverHead - 1;
	pstring LastIndex;
	longint LastRecNr = 0, Sum = 0;
	bool IsLeaf = false;
	longint GreaterPage = 0;
	WORD NItems = 0;
	//BYTE A[XPageSize - XPageOverHead];
	BYTE A[XPageSize - 4];
	void Reset(XWorkFile* OwnerXW);
	void PutN(longint* N); // ASM
	void PutDownPage(longint DownPage); // ASM
	void PutMLX(BYTE M, BYTE L); // ASM
	void ClearRest(); // ASM
	void PageFull();
	void AddToLeaf(WRec* R, KeyDPtr KD);
	void AddToUpper(XXPage* P, longint DownPage);
};

/* the index is sorted by key value && input order(IR) !! */

void CreateWIndex(XScan* Scan, XWKey* K, char Typ); // r508
void ScanSubstWIndex(XScan* Scan, KeyFldD* SK, char Typ); // r518
void SortAndSubst(KeyFldD* SK); // r534
void GetIndexSort(Instr_getindex* PD);
void CopyIndex(WKeyDPtr K, KeyDPtr FromK); // r581
