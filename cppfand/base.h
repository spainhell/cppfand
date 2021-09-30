#pragma once
#include <array>
#include "constants.h"
#include "OldDrivers.h"
#include "pstring.h"

typedef char CharArr[50];
typedef CharArr* CharArrPtr; // ø23

class LongStr // ø24
{
public:
	LongStr() { A = new char[50]{ 0 }; allocated = 50; LL = 0; }
	LongStr(size_t size) {
		if (size == 0) size = 50;
		A = new char[size] {0};
		allocated = size;
		LL = 0;
	}
	LongStr(char* data, size_t size) {
		A = data;
		allocated = size;
		LL = size;
	}
	~LongStr() { delete[] A; }
	size_t LL;
	char* A;
private:
	size_t allocated = 0;
};

struct WRect { BYTE C1 = 0, R1 = 0, C2 = 0, R2 = 0; }; // r34
struct WordRec { BYTE Lo = 0, Hi = 0; };
struct LongRec { WORD Lo = 0, Hi = 0; };
typedef void* PProcedure;

class Chained 
{
public:
	Chained* Chain = nullptr;
};

bool IsLetter(char C); // r4
void MyMove(void* A1, void* A2, WORD N);
void ChainLast(Chained* Frst, Chained* New); // r13 ASM
Chained* LastInChain(Chained* Frst); // r18 ASM
WORD ListLength(void* P); // r22 ASM
void ReplaceChar(std::string& S, char C1, char C2); // r30 ASM
bool SEquUpcase(std::string S1, std::string S2);
//bool SEquUpcase(pstring S1, pstring S2);
pstring StrPas(const char* Src);
void StrLPCopy(char* Dest, pstring s, WORD MaxL);
WORD SLeadEqu(pstring S1, pstring S2);
bool EqualsMask(void* p, WORD l, pstring Mask); // r86 ASM
bool EquLongStr(LongStr* S1, LongStr* S2);
bool EquArea(void* P1, void* P2, WORD L);
integer MinI(integer X, integer Y);
integer MaxI(integer X, integer Y);
WORD MinW(WORD X, WORD Y);
WORD MaxW(WORD X, WORD Y);
longint MinL(longint X, longint Y);
longint MaxL(longint X, longint Y);
longint SwapLong(longint N);
void ExChange(void* X, void* Y, WORD L);
bool OverlapByteStr(void* p1, void* p2); // ASM
WORD FindCtrlM(LongStr* s, WORD i, WORD n); // r152
WORD SkipCtrlMJ(LongStr* s, WORD i); // r158
void AddBackSlash(std::string& s);
void DelBackSlash(std::string& s);
bool MouseInRect(WORD X, WORD Y, WORD XSize, WORD Size); // r175 ASM
bool MouseInRectProc(WORD X, WORD Y, WORD XSize, WORD Size); // r182 ASM - rozdìleno na txt a graph režim

// *** TIME, DATE ***
void SplitDate(double R, WORD& d, WORD& m, WORD& y);
double AddMonth(double R, double RM);
double DifMonth(double R1, double R2);
double ValDate(pstring Txt, pstring Mask); // r276
pstring StrDate(double R, pstring Mask); //r321
std::string CppToday();
double Today(); // r362
double CurrTime();

// *** DEBUGGING ***
void wait();
#ifndef FandRunV
pstring HexB(BYTE b);
pstring HexW(WORD i);
pstring HexD(longint i);
pstring HexPtr(void* p);
void DispH(void* ad, integer NoBytes);
#endif

// *** MEMORY MANAGEMENT ***

struct CachePage { BYTE Pg3[3]{ 0 }; BYTE Handle = 0; longint HPage = 0; bool Upd = false; BYTE Arr[4096]{ 0 }; };
struct ProcStkD { ProcStkD* ChainBack; void* LVRoot; }; // r199
typedef ProcStkD* ProcStkPtr;

struct ExitRecord {
	void* OvrEx = nullptr; void* mBP = nullptr;
	WORD rBP = 0, rIP = 0, rCS = 0, rSP = 0, rDS = 0;
	bool ExP = false, BrkP = false;
};

void* Normalize(longint L);
longint AbsAdr(void* P);
void* GetStore(WORD Size);
void* GetZStore(WORD Size);
void MarkStore(void* p);
void ReleaseStore(void* pointer);
void ReleaseAfterLongStr(void* p);
int StoreAvail();
void* GetStore2(WORD Size);
void* GetZStore2(WORD Size);
std::string* StoreStr(std::string S);
void MarkStore2(void* p);
void ReleaseStore2(void* p);
void MarkBoth(void* p, void* p2);
void ReleaseBoth(void* p, void* p2);
void AlignLongStr();

void StackOvr(WORD NewBP); // r216
void NewExit(PProcedure POvr, ExitRecord Buf);  // r218
void GoExit();
void RestoreExit(ExitRecord& Buf);
bool OSshell(std::string Path, std::string CmdLine, bool NoCancel, bool FreeMm, bool LdFont, bool TextMd);


// ***  VIRTUAL HANDLES  ***
enum FileOpenMode { _isnewfile = 0, _isoldfile = 1, _isoverwritefile = 2, _isoldnewfile = 3 }; // poradi se nesmi zmenit!!!
enum FileUseMode { Closed = 0, RdOnly = 1, RdShared = 2, Shared = 3, Exclusive = 4 }; // poradi se nesmi zmenit!!!

bool IsNetCVol();
bool CacheExist();
bool SaveCache(WORD ErrH, FILE* f);
//void ClearCacheH(FILE* h);
void SetUpdHandle(FILE* H);
void ResetUpdHandle(FILE* H);
bool IsUpdHandle(FILE* H);
longint PosH(FILE* handle);
int SeekH(FILE* handle, longint pos);
longint FileSizeH(FILE* handle);
FILE* OpenH(FileOpenMode Mode, FileUseMode UM);
size_t ReadH(FILE* handle, size_t bytes, void* buffer);
void WriteH(FILE* handle, WORD bytes, void* buffer);
void TruncH(FILE* handle, longint N);
void FlushH(FILE* handle);
void FlushHandles();
void CloseH(FILE** handle);
void CloseClearH(FILE** h);
void SetFileAttr(WORD Attr);
WORD GetFileAttr();
void RdWrCache(bool ReadOp, FILE* Handle, bool NotCached, longint Pos, WORD N, void* Buf);
void MyDeleteFile(pstring path);
void RenameFile56(pstring OldPath, pstring NewPath, bool Msg);
std::string MyFExpand(pstring Nm, pstring EnvName);

// *** DISPLAY ***

//WORD LenStyleStr(pstring s);
WORD LogToAbsLenStyleStr(pstring s, WORD l);
//void WrStyleStr(pstring s, WORD Attr);
//void WrLongStyleStr(LongStr* S, WORD Attr);


void SetMsgPar(pstring s);
void SetMsgPar(pstring s1, pstring s2);
void SetMsgPar(pstring s1, pstring s2, pstring s3);
void SetMsgPar(pstring s1, pstring s2, pstring s3, pstring s4);
void RdMsg(integer N);
void WriteMsg(WORD N);
void ClearLL(BYTE attr);


enum TKbdConv { OrigKbd, CsKbd, CaKbd, SlKbd, DtKbd };

struct Spec // r.319
{
	BYTE UpdCount;
	BYTE AutoRprtWidth;
	BYTE AutoRprtLimit;
	BYTE CpLines;
	bool AutoRprtPrint;
	bool ChoosePrMsg;
	bool TxtInsPg;
	char TxtCharPg;
	bool ESCverify;
	bool Prompt158;
	bool F10Enter;
	bool RDBcomment;
	char CPMdrive;
	WORD RefreshDelay;
	WORD NetDelay;
	BYTE LockDelay;
	BYTE LockRetries;
	bool Beep;
	bool LockBeepAllowed;
	WORD XMSMaxKb;
	bool NoCheckBreak;
	TKbdConv KbdTyp;
	bool NoMouseSupport;
	bool MouseReverse;
	BYTE DoubleDelay;
	BYTE RepeatDelay;
	BYTE CtrlDelay;
	bool OverwrLabeledDisk;
	WORD ScreenDelay;
	BYTE OffDefaultYear;
	bool WithDiskFree;
};
extern Spec spec;

struct Video // ø. 345
{
	WORD address;
	BYTE TxtRows;
	bool ChkSnow;	// {not used }
	WORD CursOn, CursOff, CursBig;
};
extern Video video;

struct Fonts // r350
{
	TVideoFont VFont = TVideoFont::foLatin2;
	bool LoadVideoAllowed = false;
	bool NoDiakrSupported = false;
};
extern Fonts fonts;

std::string PrTab(WORD printerNr, WORD value);
void SetCurrPrinter(integer NewPr);

struct Printer {
	std::string Strg; char Typ, Kod; BYTE Lpti, TmOut;
	bool OpCls, ToHandle, ToMgr; WORD Handle;
};

typedef std::array<BYTE, 4> TPrTimeOut; // ø. 418

void OpenWorkH();

//FILE* GetOverHandle(FILE* fptr, int diff);
void NonameStartFunction(); // r639 BASE.PAS - kam to patøí?

struct wdaystt { BYTE Typ = 0; WORD Nr = 0; };

class TResFile // r. 440
{
public:
	std::string FullName;
	FILE* Handle;
	struct st
	{
		longint Pos;
		WORD Size;
	} A[FandFace];
	WORD Get(WORD Kod, void** P);
	std::string Get(WORD Kod);
	LongStr* GetStr(WORD Kod);
};

struct TMsgIdxItem { WORD Nr; WORD Ofs; BYTE Count; };
