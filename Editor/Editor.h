#pragma once

#include <string>
#include "../cppfand/base.h"
#include "../cppfand/constants.h"
#include "../cppfand/pstring.h"
#include "../cppfand/Rdb.h"

class Instr;
struct EdExitD;
class Instr_setedittxt;

struct MsgStr
{
	std::string Head, Last, CtrlLast, AltLast, ShiftLast;
};

bool EditText(char pMode, char pTxtType, pstring pName, pstring pErrMsg,
	char* pTxtPtr, WORD pMaxLen, WORD& pLen, WORD& pInd, longint& pScr,
	pstring pBreaks, EdExitD* pExD, bool& pSrch, bool& pUpdat,
	WORD pLastNr, WORD pCtrlLastNr, MsgStr* pMsgS); // r169
void SimpleEditText(char pMode, pstring pErrMsg, pstring pName, char* TxtPtr,
	WORD MaxLen, WORD& Len, WORD& Ind, bool& Updat); // r202
WORD FindTextE(const pstring& PstrScreenStr, pstring Popt, char* PTxtPtr, WORD PLen); // r209
void Help(RdbDPtr R, pstring Name, bool InCWw);
void InitTxtEditor();
void EditTxtFile(longint* LP, char Mode, pstring& ErrMsg, EdExitD* ExD, longint TxtPos,
	longint Txtxy, WRect* V, WORD Atr, pstring Hd, BYTE WFlags, MsgStr* MsgS);
void ViewPrinterTxt(); // r353
void SetEditTxt(Instr_setedittxt* PD);
void GetEditTxt(bool& pInsert, bool& pIndent, bool& pWrap, bool& pJust, bool& pColBlk,
	integer& pLeftMarg, integer& pRightMarg); // r162
void ClearHelpStkForCRdb();

const char TextM = 'T'; const char ViewM = 'V'; const char HelpM = 'H';
const char SinFM = 'S'; const char DouFM = 'D'; const char DelFM = 'F';
const char NotFM = 'N'; const char FileT = 'F'; const char LocalT = 'V';
const char MemoT = 'M';
