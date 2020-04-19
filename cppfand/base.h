#pragma once
#include <array>
#include "constants.h"
#include "pstring.h"


typedef char CharArr[50]; typedef CharArr* CharArrPtr; // ø23
struct LongStr { WORD LL; CharArr A; }; // ø24
typedef LongStr* LongStrPtr; // ø25

typedef void* PProcedure;

void wait(); // ø. 95

ExitRecord ExitBuf; // r210
// tady je stack, snad nepotøebujeme
WORD BPBound; // r212
bool ExitP, BreakP;

void NewExit(PProcedure POvr, ExitRecord Buf);  // ø. 218


WORD HandleError; // r229
pstring OldDir, FandDir, WrkDir;
pstring FandOvrName, FandResName, FandWorkName, FandWorkXName, FandWorkTName;
pstring CPath; pstring CDir; pstring CName; pstring CExt;
pstring CVol;
bool WasLPTCancel;
FILE* WorkHandle;
const longint MaxWSize = 0; // {currently occupied in FANDWORK.$$$}

//WORD ReadH(WORD handle, WORD bytes, void* buffer);

// ********** MESSAGES **********
WORD F10SpecKey; // ø. 293
BYTE ProcAttr;
bool SetStyleAttr(char c, BYTE a);
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

typedef std::array<BYTE, 4> TPrTimeOut; // ø. 418
TPrTimeOut OldPrTimeOut;
TPrTimeOut PrTimeOut;

const char AbbrYes = 'Y'; const char AbbrNo = 'N';
bool WasInitDrivers = false;
bool WasInitPgm = false;

WORD LANNode; // ø. 431

const BYTE FandFace = 16;

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
	pstring* GetStr(WORD Kod);
};
struct TMsgIdxItem { WORD Nr, Ofs; BYTE Count; };
//TMsgIdxItem TMsgIdx[100];
TResFile ResFile;
TMsgIdxItem* MsgIdx;// = TMsgIdx;
WORD MsgIdxN; longint FrstMsgPos;

WORD StackOvr(); // r482
void NoOvr();

bool CacheLocked = false; // r510

pstring PrTab(WORD N); // r517
void SetCurrPrinter(integer NewPr); // r524
void ExitSave(); //535
void MyExit(); //536
void WrTurboErr(); // 537





// ø. 577
void OpenWorkH();

void OpenOvrFile(); // r632
