#pragma once
#include <array>
#include "constants.h"
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
struct WordRec { BYTE Lo, Hi; };
struct LongRec { WORD Lo, Hi; };
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
void GoExit();
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
const longint MaxWSize = 0; // {currently occupied in FANDWORK.$$$}
/* konec */

// ********** MESSAGES **********
WORD F10SpecKey; // ø. 293
BYTE ProcAttr;
bool SetStyleAttr(char c, BYTE& a);
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

struct wdaystt { BYTE Typ; WORD Nr; } WDaysTabType;
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

WORD StackOvr(); // r482 ASM
void NoOvr(); // ASM

bool CacheLocked = false; // r510

pstring PrTab(WORD N); // r517 ASM
void SetCurrPrinter(integer NewPr); // r524 
void (*ExitSave)(); //535
void MyExit(); //536
void WrTurboErr(); // 537
void OpenResFile(); // 571
void OpenWorkH(); // ø. 577
void InitOverlays(); // r614
void OpenOvrFile(); //  r632

