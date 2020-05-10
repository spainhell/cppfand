#pragma once
#include <vector>
#include "constants.h"
#include "pstring.h"

class FileD;
struct EFldD;
const BYTE FandFace = 17;

typedef char CharArr[50];
typedef CharArr* CharArrPtr; // ø23
struct LongStr { WORD LL; CharArr A; }; // ø24
typedef LongStr* LongStrPtr; // ø25

struct TMsgIdxItem { WORD Nr; WORD Ofs; BYTE Count; };
struct wdaystt { BYTE Typ = 0; WORD Nr = 0; };

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
	LongStr* GetStr(WORD Kod);
};

class globconf
{
public:
	static globconf* GetInstance()
	{
		static globconf INSTANCE;
		return &INSTANCE;
	}
	std::vector<std::string> paramstr;
	pstring OldDir;
	pstring FandDir;
	pstring WrkDir;
	pstring FandOvrName;
	pstring FandResName;
	pstring FandWorkName;
	pstring FandWorkXName;
	pstring FandWorkTName;
	pstring CPath;
	pstring CDir;
	pstring CName;
	pstring CExt;
	pstring CVol;
	//TMsgIdxItem TMsgIdx[100];
	TResFile ResFile;
	TMsgIdxItem* MsgIdx;// = TMsgIdx;
	WORD HandleError; // r229

	WORD MsgIdxN;
	longint FrstMsgPos;
	char AbbrYes = 'Y';
	char AbbrNo = 'N';

	// ********** MESSAGES **********
	WORD F10SpecKey; // ø. 293
	BYTE ProcAttr;
	// bool SetStyleAttr(char c, BYTE& a); // je v KBDWW
	pstring MsgLine;
	pstring MsgPar[4];

	wdaystt WDaysTabType;
	WORD NWDaysTab;
	float WDaysFirst;
	float WDaysLast;
	wdaystt* WDaysTab;

	bool TxtEdCtrlUBrk, TxtEdCtrlF4Brk;
	EFldD* CFld;
private:
	globconf() {}
};
