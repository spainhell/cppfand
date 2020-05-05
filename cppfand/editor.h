#pragma once

#include "constants.h"
#include "edevent.h"
#include "pstring.h"
#include "rdrun.h"
#include "sort.h"
#include "wwmenu.h"

struct MsgStr
{
	pstring* Head, Last, CtrlLast, AltLast, ShiftLast;
};
typedef MsgStr* MsgStrPtr;

const char TextM = 'T'; const char ViewM = 'V'; const char HelpM = 'H';
const char SinFM = 'S'; const char DouFM = 'D'; const char DelFM = 'F';
const char NotFM = 'N'; const char FileT = 'F'; const char LocalT = 'V';
const char MemoT = 'M';
bool Insert, Indent, Wrap, Just;

const BYTE InterfL = 4; /*sizeof(Insert+Indent+Wrap+Just)*/

const BYTE LineSize = 255; const WORD SuccLineSize = 256; const WORD TextStore = 0x1000;
const BYTE TStatL = 35; /*=10(Col Row)+length(InsMsg+IndMsg+WrapMsg+JustMsg+BlockMsg)*/

set<char> Oddel = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,58,59,60,61,62,63,64,
91,92,93,94,96,123,124,125,126,127 };

const WORD _QY_ = 0x1119; const WORD _QL_ = 0x110C; const WORD _QK_ = 0x110B;
const WORD _QB_ = 0x1102; const WORD _QI_ = 0x1109; const WORD _QF_ = 0x1106;
const WORD _QE_ = 0x1105; const WORD _QX_ = 0x1118; const WORD _QA_ = 0x1101;
const WORD _framesingle_ = 0x112D; const WORD _framedouble_ = 0x113D;
const WORD _delframe_ = 0x112F; const WORD _frmsin_ = 0x2D;
const WORD _frmdoub_ = 0x3D; const WORD _dfrm_ = 0x2F; const WORD _nfrm_ = 0x20;
const WORD _KB_ = 0x0B02; const WORD _KK_ = 0x0B0B; const WORD _KH_ = 0x0B08;
const WORD _KS_ = 0x0B13; const WORD _KY_ = 0x0B19; const WORD _KC_ = 0x0B03;
const WORD _KV_ = 0x0B16; const WORD _KW_ = 0x0B17; const WORD _KR_ = 0x0B12;
const WORD _KP_ = 0x0B10; const WORD _KN_ = 0x0B0E; const WORD _KU_ = 0x0B15;
const WORD _KL_ = 0x0B0C; const WORD _OW_ = 0x0F17; const WORD _OL_ = 0x0F0C;
const WORD _OR_ = 0x0F12; const WORD _OJ_ = 0x0F0A; const WORD _OC_ = 0x0F03;
const WORD _KF_ = 0x0B06;

const BYTE CountC = 7;
pstring CtrlKey({ 19,23,17,4,2,5,1 });
const bool ColBlock = true;
const bool TextBlock = false;



// {**********global param begin for SavePar}  // r85
char Mode;
char TypeT;
pstring NameT;
string ErrMsg;
WORD MaxLenT, LenT, IndT, ScrT;
string Breaks;
EdExitD* ExitD;
bool SrchT, UpdatT;
WORD LastNr, CtrlLastNr;
integer LeftMarg, RightMarg;
bool TypeB;
pstring  LastS, CtrlLastS, ShiftLastS, AltLastS, HeadS;
longint* LocalPPtr;
bool EditT;

// od r101
BYTE ColKey[CountC + 1];
BYTE TxtColor, BlockColor, SysLColor;
pstring InsMsg, nInsMsg, IndMsg, WrapMsg, JustMsg, BlockMsg;
pstring ViewMsg;
char CharPg;
bool InsPg;
longint BegBLn, EndBLn;
WORD BegBPos, EndBPos;
WORD ScrI, LineI, Posi, BPos; // {screen status}
pstring FindStr, ReplaceStr;
bool Replace;
pstring OptionStr;
bool FirstEvent;
WORD PHNum, PPageS; // {strankovani ve Scroll}
struct PartDescr
{
	longint PosP; longint LineP;
	WORD LenP, MovI, MovL;
	bool UpdP;
	ColorOrd ColorP;
} Part;
FILE* TxtFH;
pstring TxtPath;
pstring TxtVol;
bool AllRd;
longint AbsLenT;
bool ChangePart, UpdPHead;
CharArr* T;

longint SavePar(); // r133
void RestorePar(longint l);
void SetEditTxt(Instr* PD);
void GetEditTxt(bool& pInsert, bool& pIndent, bool& pWrap, bool& pJust, bool& pColBlk, 
	integer& pLeftMarg, integer& pRightMarg); // r162
bool EditText(char pMode, char pTxtType, pstring pName, pstring pErrMsg,
	CharArr* pTxtPtr, WORD pMaxLen, WORD& pLen, WORD& pInd, longint pScr,
	pstring pBreaks, EdExitD* pExD, bool& pSrch, bool& pUpdat,
	WORD pLastNr, WORD pCtrlLastNr, MsgStrPtr pMsgS); // r169
void SimpleEditText(char pMode, pstring pErrMsg, pstring pName, CharArr* TxtPtr,
	WORD MaxLen, WORD& Len, WORD& Ind, bool& Updat); // r202
WORD FindText(const pstring& PstrScreenStr, pstring Popt, CharArr* PTxtPtr, WORD PLen); // r209
void EditTxtFile(longint* LP, char Mode, pstring& ErrMsg, EdExitD* ExD, longint TxtPos, 
	longint Txtxy, WRect* V, WORD Atr, const pstring Hd, BYTE WFlags, MsgStrPtr MsgS);
void ViewPrinterTxt(); // r353

// ***********HELP**********  // r351
const BYTE maxStk = 15; WORD iStk = 0;
struct structStk { RdbDPtr Rdb; FileDPtr FD; WORD iR, iT; } Stk[maxStk];
void Help(RdbDPtr R, pstring Name, bool InCWw);
void ViewHelpText(LongStr* S, WORD& TxtPos);
void ClearHelpStkForCRdb();
void InitTxtEditor();
