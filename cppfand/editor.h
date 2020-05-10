#pragma once

#include "constants.h"
#include "pstring.h"
#include "sort.h"

struct Instr;
struct EdExitD;

struct MsgStr
{
	pstring* Head, Last, CtrlLast, AltLast, ShiftLast;
};
typedef MsgStr* MsgStrPtr;

bool EditText(char pMode, char pTxtType, pstring pName, pstring pErrMsg,
	CharArr* pTxtPtr, WORD pMaxLen, WORD& pLen, WORD& pInd, longint pScr,
	pstring pBreaks, EdExitD* pExD, bool& pSrch, bool& pUpdat,
	WORD pLastNr, WORD pCtrlLastNr, MsgStrPtr pMsgS); // r169
void SimpleEditText(char pMode, pstring pErrMsg, pstring pName, CharArr* TxtPtr,
	WORD MaxLen, WORD& Len, WORD& Ind, bool& Updat); // r202
WORD FindText(const pstring& PstrScreenStr, pstring Popt, CharArr* PTxtPtr, WORD PLen); // r209
void Help(RdbDPtr R, pstring Name, bool InCWw);
void InitTxtEditor();
void EditTxtFile(longint* LP, char Mode, pstring& ErrMsg, EdExitD* ExD, longint TxtPos,
	longint Txtxy, WRect* V, WORD Atr, pstring Hd, BYTE WFlags, MsgStrPtr MsgS);
void ViewPrinterTxt(); // r353
void SetEditTxt(Instr* PD);
void GetEditTxt(bool& pInsert, bool& pIndent, bool& pWrap, bool& pJust, bool& pColBlk,
	integer& pLeftMarg, integer& pRightMarg); // r162
void ClearHelpStkForCRdb();

const char TextM = 'T'; const char ViewM = 'V'; const char HelpM = 'H';
const char SinFM = 'S'; const char DouFM = 'D'; const char DelFM = 'F';
const char NotFM = 'N'; const char FileT = 'F'; const char LocalT = 'V';
const char MemoT = 'M';
