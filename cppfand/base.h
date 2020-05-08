#pragma once
#include <array>
#include "constants.h"
#include "drivers.h"
#include "pstring.h"


longint const UserLicNrShow = 999001; // 160188
const char Version[] = { '4', '.', '2', '0', '\0' };
const WORD FDVersion = 0x0411;
const WORD ResVersion = 0x0420;
const char CfgVersion[] = { '4', '.', '2', '0', '\0' };
const BYTE DMLVersion = 41;
const WORD NoDayInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const bool HasCoproc = true;

typedef char CharArr[50]; typedef CharArr* CharArrPtr; // ø23
struct LongStr { WORD LL; CharArr A; }; // ø24
typedef LongStr* LongStrPtr; // ø25

struct WRect { BYTE C1, R1, C2, R2; }; // r34
struct WordRec { BYTE Lo = 0, Hi = 0; };
struct LongRec { WORD Lo = 0, Hi = 0; };
// struct PtrRec { WORD Ofs, Seg; }


const WORD MaxLStrLen = 65000;
const BYTE WShadow = 0x01; // window flags
const BYTE WNoClrScr = 0x02;
const BYTE WPushPixel = 0x04;
const BYTE WNoPop = 0x08;
const BYTE WHasFrame = 0x10;
const BYTE WDoubleFrame = 0x20;

typedef void* PProcedure;

void wait(); // ø. 95

#ifndef FandRunV
pstring HexB(BYTE b);
pstring HexW(WORD i);
pstring HexD(longint i);
pstring HexPtr(void* p);
void DispH(void* ad, integer NoBytes);
#endif


/* MEMORY MANAGEMENT */
WORD CachePageSize;
void* AfterCatFD; // r108
const BYTE CachePageShft = 12;
const WORD NCachePages = 0;
const WORD XMSCachePages = 0;
struct CachePage { BYTE Pg3[3]; BYTE Handle; longint HPage; bool Upd; BYTE Arr[4096]; };
struct ProcStkD { ProcStkD* ChainBack; void* LVRoot; }; // r199
typedef ProcStkD* ProcStkPtr;

struct ExitRecord {
	void* OvrEx = nullptr; void* mBP = nullptr;
	WORD rBP = 0, rIP = 0, rCS = 0, rSP = 0, rDS = 0;
	bool ExP = false, BrkP = false;
} ExitBuf; // r202 - r210
ProcStkD* MyBP; ProcStkD* ProcMyBP;
WORD BPBound; // r212
bool ExitP, BreakP;
longint LastExitCode = 0; // r215
void StackOvr(WORD NewBP); // r216
void NewExit(PProcedure POvr, ExitRecord Buf);  // r218
/* konec */

/*  VIRTUAL HANDLES  */
enum FileOpenMode { _isnewfile, _isoldfile, _isoverwritefile, _isoldnewfile }; // poradi se nesmi zmenit!!!
enum FileUseMode { Closed, RdOnly, RdShared, Shared, Exclusive }; // poradi se nesmi zmenit!!!
WORD HandleError; // r229
pstring OldDir, FandDir, WrkDir;
pstring FandOvrName, FandResName, FandWorkName, FandWorkXName, FandWorkTName;
pstring CPath; pstring CDir; pstring CName; pstring CExt;
pstring CVol;
bool WasLPTCancel;
FILE* WorkHandle;
longint MaxWSize = 0; // {currently occupied in FANDWORK.$$$}
/* konec */

// ********** MESSAGES **********
WORD F10SpecKey; // ø. 293
BYTE ProcAttr;
// bool SetStyleAttr(char c, BYTE& a); // je v KBDWW
pstring MsgLine;
pstring MsgPar[4];
void SetMsgPar(pstring s);
void Set2MsgPar(pstring s1, pstring s2);
void Set3MsgPar(pstring s1, pstring s2, pstring s3);
void Set4MsgPar(pstring s1, pstring s2, pstring s3, pstring s4);
void RdMsg(integer N);
void WriteMsg(WORD N);
void ClearLL(BYTE attr);

// ********** DML **********
void* FandInt3f; // ø. 311
FILE* OvrHandle;
WORD Fand_ss, Fand_sp, Fand_bp, DML_ss, DML_sp, DML_bp;
const longint _CallDMLAddr = 0; // {passed to FANDDML by setting "DMLADDR="in env.}
enum TKbdConv { OrigKbd, CsKbd, CaKbd, SlKbd, DtKbd };

struct Spec // r.319
{
	BYTE UpdCount;
	BYTE AutoRprtWidth, AutoRprtLimit, CpLines;
	bool AutoRprtPrint;
	bool ChoosePrMsg;
	bool TxtInsPg;
	char TxtCharPg;
	bool ESCverify;
	bool Prompt158;
	bool F10Enter;
	bool RDBcomment;
	char CPMdrive;
	WORD RefreshDelay, NetDelay;
	BYTE LockDelay, LockRetries;
	bool Beep;
	bool LockBeepAllowed;
	WORD XMSMaxKb;
	bool NoCheckBreak;
	TKbdConv KbdTyp;
	bool NoMouseSupport, MouseReverse;
	BYTE DoubleDelay, RepeatDelay, CtrlDelay;
	bool OverwrLabeledDisk;
	WORD ScreenDelay;
	BYTE OffDefaultYear;
	bool WithDiskFree;
} spec;

struct Video // ø. 345
{
	WORD address;
	BYTE TxtRows;
	bool ChkSnow;	// {not used }
	WORD CursOn, CursOff, CursBig;
} video;

struct Fonts // r350
{
	TVideoFont VFont = TVideoFont::foLatin2;
	bool LoadVideoAllowed = false;
	bool NoDiakrSupported = false;
} fonts;

struct Colors
{
	BYTE userColor[16];
	BYTE mNorm, mHili, mFirst, mDisabled; // menu
	BYTE sNorm, sHili, sMask; // select
	BYTE pNorm, pTxt; // prompt, verify, password
	BYTE zNorm; // message
	BYTE lNorm, lFirst, lSwitch; // last line
	BYTE fNorm; // first line
	BYTE tNorm, tCtrl, tBlock; // text edit
	BYTE tUnderline, tItalic, tDWidth, tDStrike, tEmphasized, tCompressed, tElite; // data edit
	BYTE dNorm, dHili, dSubset, dTxt, dDeleted, dSelect; // -"-
	BYTE uNorm; // user screen
	BYTE hNorm, hHili, hMenu, hSpec;
	BYTE nNorm;
	BYTE ShadowAttr;
	BYTE DesktopColor;
} colors;

char CharOrdTab[256]; // after Colors /FANDDML/ // ø. 370
char UpcCharTab[256]; // TODO: v obou øádcích bylo 'array[char] of char;' - WTF?
WORD TxtCols, TxtRows;

// konstanty
const BYTE prName = 0; const BYTE prUl1 = 1; const BYTE prUl2 = 2; const BYTE prKv1 = 3;
const BYTE prKv2 = 4; const BYTE prBr1 = 5; const BYTE prBr2 = 6; const BYTE prDb1 = 7;
const BYTE prDb2 = 8; const BYTE prBd1 = 9; const BYTE prBd2 = 10; const BYTE prKp1 = 11;
const BYTE prKp2 = 12; const BYTE prEl1 = 13; const BYTE prEl2 = 14; const BYTE prReset = 15;
const BYTE prMgrFileNm = 15; const BYTE prMgrProg = 16; const BYTE prMgrParam = 17;
const BYTE prPageSizeNN = 16; const BYTE prPageSizeTrail = 17; const BYTE prLMarg = 18;
const BYTE prLMargTrail = 19; const BYTE prUs11 = 20; const BYTE prUs12 = 21;
const BYTE prUs21 = 22; const BYTE prUs22 = 23; const BYTE prUs31 = 24;
const BYTE prUs32 = 25; const BYTE prLine72 = 26; const BYTE prLine216 = 27;
const BYTE prDen60 = 28; const BYTE  prDen120 = 29; const BYTE prDen240 = 30;
const BYTE  prColor = 31; const BYTE prClose = 32;

integer prCurr, prMax;
struct Printer {
	void* Strg; char Typ, Kod; BYTE Lpti, TmOut;
	bool OpCls, ToHandle, ToMgr; WORD Handle;
} printer[10];
typedef std::array<BYTE, 4> TPrTimeOut; // ø. 418
TPrTimeOut OldPrTimeOut;
TPrTimeOut PrTimeOut;  // absolute 0:$478;

struct wdaystt { BYTE Typ = 0; WORD Nr = 0; } WDaysTabType;
WORD NWDaysTab; float WDaysFirst; float WDaysLast;
wdaystt* WDaysTab;

const char AbbrYes = 'Y'; const char AbbrNo = 'N';
bool WasInitDrivers = false;
bool WasInitPgm = false;

WORD LANNode; // ø. 431

const BYTE RMsgIdx = 0; const BYTE BgiEgaVga = 1; const BYTE BgiHerc = 2;
const BYTE ChrLittKam = 3; const BYTE ChrTripKam = 4; const BYTE Ega8x14K = 5;
const BYTE Vga8x16K = 6; const BYTE Vga8x19K = 7; const BYTE Ega8x14L = 8;
const BYTE Vga8x16L = 9; const BYTE Vga8x19L = 10; const BYTE ChrLittLat = 11;
const BYTE ChrTripLat = 12; const BYTE LatToWinCp = 13; const BYTE KamToWinCp = 14;
const BYTE WinCpToLat = 15; const BYTE FandFace = 16;

class TResFile // ø. 440
{
public:
	FILE* Handle;
	struct st
	{
		longint Pos;
		WORD Size;
	} A[FandFace];
	WORD Get(WORD Kod, void* P);
	LongStrPtr GetStr(WORD Kod);
};
struct TMsgIdxItem { WORD Nr, Ofs; BYTE Count; };
//TMsgIdxItem TMsgIdx[100];
TResFile ResFile;
TMsgIdxItem* MsgIdx;// = TMsgIdx;
WORD MsgIdxN; longint FrstMsgPos;

void (*CallOpenFandFiles)(); // r453
void (*CallCloseFandFiles)(); // r454

double userToday;

WORD ReadH(FILE* handle, WORD bytes, void* buffer);
pstring MyFExpand(pstring Nm, pstring EnvName);
void CloseH(FILE* handle);
FILE* OpenH(FileOpenMode Mode, FileUseMode UM);
void MarkStore2(void* p);
void ReleaseStore2(void* p);
void MarkStore(void* p);
void DelBackSlash(pstring s);
void RestoreExit(ExitRecord& Buf);
void SeekH(FILE* handle, longint pos);
longint PosH(FILE* handle);
void SetCurrPrinter(integer NewPr);
void AddBackSlash(pstring s);
FILE* GetOverHandle(FILE* fptr, int diff);
void OpenWorkH();
bool SEquUpcase(pstring S1, pstring S2);
void ReleaseStore(void* pointer);
bool OSshell(pstring Path, pstring CmdLine, bool NoCancel, bool FreeMm, bool LdFont, bool TextMd);
void GoExit();
longint FileSizeH(FILE* handle);
void RdWrCache(bool ReadOp, FILE* Handle, bool NotCached, longint Pos, WORD N, void* Buf);
longint SwapLong(longint N);
void* GetStore(WORD Size);
void* GetStore2(WORD Size);
void SetUpdHandle(FILE* H);
WORD SLeadEqu(pstring S1, pstring S2);
void* GetZStore(WORD Size);
bool SaveCache(WORD ErrH);
void* GetZStore2(WORD Size);

integer MinI(integer X, integer Y);
integer MaxI(integer X, integer Y);
WORD MinW(WORD X, WORD Y);
WORD MaxW(WORD X, WORD Y);
longint MinL(longint X, longint Y);
longint MaxL(longint X, longint Y);
longint StoreAvail();
void WriteH(FILE* handle, WORD bytes, void* buffer);
void TruncH(FILE* handle, longint N);
void FlushH(FILE* handle);
void DeleteFile(pstring path);

WORD FindCtrlM(LongStrPtr s, WORD i, WORD n); // r152
WORD SkipCtrlMJ(LongStrPtr s, WORD i); // r158

pstring PrTab(WORD N);
pstring StrPas(const char* Src);

void ChainLast(void* Frst, void* New); // r13 ASM
void* LastInChain(void* Frst); // r18 ASM
pstring* StoreStr(pstring S);
void CloseClearH(FILE* h);
bool CacheExist();
void FlushHandles();
bool IsNetCVol();
WORD GetFileAttr();
void SetFileAttr(WORD Attr);
void ClearCacheH(FILE* h);
void RenameFile56(pstring OldPath, pstring NewPath, bool Msg);
void ResetUpdHandle(FILE* H);
//bool IsHandle(FILE* H);
bool IsUpdHandle(FILE* H);
void StrLPCopy(char* Dest, pstring s, WORD MaxL);
void SplitDate(double R, WORD& d, WORD& m, WORD& y);
double Today(); // r362
double CurrTime();
bool MouseInRect(WORD X, WORD Y, WORD XSize, WORD Size); // r175 ASM
double ValDate(const pstring& Txtpstring, pstring Mask); // r276
pstring StrDate(double R, pstring Mask); //r321
double AddMonth(double R, double RM);
double DifMonth(double R1, double R2);
void MyMove(void* A1, void* A2, WORD N);
void ReleaseAfterLongStr(void* p);
WORD CountDLines(void* Buf, WORD L, char C); // r139 ASM
pstring GetDLine(void* Buf, WORD L, char C, WORD I); // r144 ASM
bool OverlapByteStr(void* p1, void* p2); // ASM
bool MouseInRectProc(WORD X, WORD Y, WORD XSize, WORD Size); // r182 ASM - rozdìleno na txt a graph režim
bool EqualsMask(void* p, WORD l, pstring Mask); // r86 ASM
WORD ListLength(void* P); // r22 ASM
void AlignLongStr();
void MarkBoth(void* p, void* p2);
void ReleaseBoth(void* p, void* p2);
void* Normalize(longint L);
longint AbsAdr(void* P);
void ExChange(void* X, void* Y, WORD L);
void ReplaceChar(pstring S, char C1, char C2); // r30 ASM
bool SetStyleAttr(char C, BYTE& a);
WORD LenStyleStr(pstring s);
void WrStyleStr(pstring s, WORD Attr);
void WrLongStyleStr(LongStr* S, WORD Attr);
WORD LogToAbsLenStyleStr(pstring s, WORD l);
