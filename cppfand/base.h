#pragma once
#include <array>
#include "constants.h"
#include "drivers.h"
#include "pstring.h"

extern char Version[5];

typedef char CharArr[50];
typedef CharArr* CharArrPtr; // ø23

class LongStr // ø24
{
public:
	LongStr() { A = new char[50]{ 0 }; allocated = 50; }
	LongStr(WORD size) { if (size == 0) size = 50; A = new char[size] {0}; allocated = size; }
	~LongStr() { delete[] A; }
	WORD LL;
	char* A;
private:
	WORD allocated = 0;
};

typedef LongStr* LongStrPtr; // ø25

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
void ReplaceChar(pstring S, char C1, char C2); // r30 ASM
bool SEquUpcase(std::string S1, std::string S2);
bool SEquUpcase(pstring S1, pstring S2);
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
WORD CountDLines(void* Buf, WORD L, char C); // r139 ASM
pstring GetDLine(void* Buf, WORD L, char C, WORD I); // r144 ASM
WORD FindCtrlM(LongStr* s, WORD i, WORD n); // r152
WORD SkipCtrlMJ(LongStrPtr s, WORD i); // r158
void AddBackSlash(pstring& s);
void DelBackSlash(pstring& s);
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
extern WORD CachePageSize;
extern void* AfterCatFD; // r108
struct CachePage { BYTE Pg3[3]{ 0 }; BYTE Handle = 0; longint HPage = 0; bool Upd = false; BYTE Arr[4096]{ 0 }; };
struct ProcStkD { ProcStkD* ChainBack; void* LVRoot; }; // r199
typedef ProcStkD* ProcStkPtr;

struct ExitRecord {
	void* OvrEx = nullptr; void* mBP = nullptr;
	WORD rBP = 0, rIP = 0, rCS = 0, rSP = 0, rDS = 0;
	bool ExP = false, BrkP = false;
};
extern ExitRecord ExitBuf; // r202 - r210
extern ProcStkD* MyBP;
extern ProcStkD* ProcMyBP;
extern WORD BPBound; // r212
extern bool ExitP, BreakP;
extern longint LastExitCode; // r215

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
pstring* StoreStr(pstring S);
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
extern WORD HandleError; // r229
extern pstring OldDir;
extern pstring FandDir;
extern pstring WrkDir;
extern pstring FandOvrName;
extern pstring FandResName;
extern pstring FandWorkName;
extern pstring FandWorkXName;
extern pstring FandWorkTName;
extern pstring CPath;
extern pstring CDir;
extern pstring CName;
extern pstring CExt;
extern pstring CVol;

extern bool WasLPTCancel;
extern FILE* WorkHandle;
extern longint MaxWSize; // {currently occupied in FANDWORK.$$$}

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
WORD ReadH(FILE* handle, WORD bytes, void* buffer);
void WriteH(FILE* handle, WORD bytes, void* buffer);
void TruncH(FILE* handle, longint N);
void FlushH(FILE* handle);
void FlushHandles();
void CloseH(FILE* handle);
void CloseClearH(FILE* h);
void SetFileAttr(WORD Attr);
WORD GetFileAttr();
void RdWrCache(bool ReadOp, FILE* Handle, bool NotCached, longint Pos, WORD N, void* Buf);
void MyDeleteFile(pstring path);
void RenameFile56(pstring OldPath, pstring NewPath, bool Msg);
pstring MyFExpand(pstring Nm, pstring EnvName);

// *** DISPLAY ***

WORD LenStyleStr(pstring s);
WORD LogToAbsLenStyleStr(pstring s, WORD l);
//void WrStyleStr(pstring s, WORD Attr);
//void WrLongStyleStr(LongStr* S, WORD Attr);

// *** MESSAGES ***
extern WORD F10SpecKey; // ø. 293
extern BYTE ProcAttr;
// extern bool SetStyleAttr(char c, BYTE& a); // je v KBDWW
extern pstring MsgLine;
extern pstring MsgPar[4];
void SetMsgPar(pstring s);
void Set2MsgPar(pstring s1, pstring s2);
void Set3MsgPar(pstring s1, pstring s2, pstring s3);
void Set4MsgPar(pstring s1, pstring s2, pstring s3, pstring s4);
void RdMsg(integer N);
void WriteMsg(WORD N);
void ClearLL(BYTE attr);

// ********** DML **********
extern void* FandInt3f; // ø. 311
extern FILE* OvrHandle;
extern WORD Fand_ss, Fand_sp, Fand_bp, DML_ss, DML_sp, DML_bp;
extern longint _CallDMLAddr; // {passed to FANDDML by setting "DMLADDR="in env.}
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

extern char CharOrdTab[256]; // after Colors /FANDDML/ // ø. 370
extern char UpcCharTab[256]; // TODO: v obou øádcích bylo 'array[char] of char;' - WTF?
extern WORD TxtCols, TxtRows;

pstring PrTab(WORD N);
void SetCurrPrinter(integer NewPr);

extern integer prCurr, prMax;
struct Printer {
	void* Strg; char Typ, Kod; BYTE Lpti, TmOut;
	bool OpCls, ToHandle, ToMgr; WORD Handle;
};
extern Printer printer[10];
typedef std::array<BYTE, 4> TPrTimeOut; // ø. 418
extern TPrTimeOut OldPrTimeOut;
extern TPrTimeOut PrTimeOut;  // absolute 0:$478;
extern bool WasInitDrivers;
extern bool WasInitPgm;
extern WORD LANNode; // ø. 431
extern void (*CallOpenFandFiles)(); // r453
extern void (*CallCloseFandFiles)(); // r454

extern double userToday;

void OpenWorkH();

//FILE* GetOverHandle(FILE* fptr, int diff);
void NonameStartFunction(); // r639 BASE.PAS - kam to patøí?

struct wdaystt { BYTE Typ = 0; WORD Nr = 0; };

extern wdaystt WDaysTabType;
extern WORD NWDaysTab;
extern double WDaysFirst;
extern double WDaysLast;
extern wdaystt* WDaysTab;

extern char AbbrYes;
extern char AbbrNo;

class TResFile // ø. 440
{
public:
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

//TMsgIdxItem TMsgIdx[100];
extern TResFile ResFile;
extern TMsgIdxItem* MsgIdx;// = TMsgIdx;
extern WORD MsgIdxN;
extern longint FrstMsgPos;
