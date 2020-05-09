#pragma once
#include <vector>
#include "constants.h"
#include "pstring.h"

const BYTE FandFace = 16;

typedef char CharArr[50]; typedef CharArr* CharArrPtr; // ø23
struct LongStr { WORD LL; CharArr A; }; // ø24
typedef LongStr* LongStrPtr; // ø25

struct TMsgIdxItem { WORD Nr; WORD Ofs; BYTE Count; };

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
	static std::vector<std::string> paramstr;
	static pstring OldDir;
	static pstring FandDir;
	static pstring WrkDir;
	static pstring FandOvrName;
	static pstring FandResName;
	static pstring FandWorkName;
	static pstring FandWorkXName;
	static pstring FandWorkTName;
	static pstring CPath;
	static pstring CDir;
	static pstring CName;
	static pstring CExt;
	static pstring CVol;
	//TMsgIdxItem TMsgIdx[100];
	static TResFile ResFile;
	static TMsgIdxItem* MsgIdx;// = TMsgIdx;
	static WORD HandleError; // r229

	static WORD MsgIdxN;
	static longint FrstMsgPos;
	static char AbbrYes;
	static char AbbrNo;
};
