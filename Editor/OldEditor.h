#pragma once

#include <string>
#include "../cppfand/base.h"
#include "../cppfand/constants.h"
#include "../cppfand/pstring.h"

class Instr;
struct EdExitD;
class Instr_setedittxt;

struct MsgStr
{
	std::string Head, Last, CtrlLast, AltLast, ShiftLast;
};

bool EditText(char pMode, char pTxtType, pstring pName, pstring pErrMsg,
	char* pTxtPtr, WORD pMaxLen, size_t& pLen, WORD& pInd, longint& pScr,
	pstring pBreaks, EdExitD* pExD, bool& pSrch, bool& pUpdat,
	WORD pLastNr, WORD pCtrlLastNr, MsgStr* pMsgS); // r169
void SimpleEditText(char pMode, pstring pErrMsg, pstring pName, char* TxtPtr,
	WORD MaxLen, size_t& Len, WORD& Ind, bool& Updat); // r202
WORD FindTextE(const pstring& PstrScreenStr, pstring Popt, char* PTxtPtr, WORD PLen); // r209
void InitTxtEditor();
void EditTxtFile(longint* LP, char Mode, pstring& ErrMsg, EdExitD* ExD, longint TxtPos,
	longint Txtxy, WRect* V, WORD Atr, pstring Hd, BYTE WFlags, MsgStr* MsgS);
void ViewPrinterTxt(); // r353
void SetEditTxt(Instr_setedittxt* PD);
void GetEditTxt(bool& pInsert, bool& pIndent, bool& pWrap, bool& pJust, bool& pColBlk,
	integer& pLeftMarg, integer& pRightMarg); // r162

void Background();
void DisplLL(WORD Flags);
void WrLLMargMsg(std::string& s, WORD n);
void ScrollPress();
void CleanFrameM();
WORD SetInd(WORD Ind, WORD Pos);
void TestUpdFile();
void DelEndT();
void KodLine();
void SetDekLnCurrI(WORD Ind);
size_t FindChar(char* text, size_t length, char c, size_t from);
void MyDelLine();
void MyInsLine();
WORD SetPredI();
void NextLine(bool WrScr);
void HelpLU(char dir);
void BlockLRShift(WORD I1);



extern pstring Breaks;


const char TextM = 'T'; const char ViewM = 'V'; const char HelpM = 'H';
const char SinFM = 'S'; const char DouFM = 'D'; const char DelFM = 'F';
const char NotFM = 'N'; const char FileT = 'F'; const char LocalT = 'V';
const char MemoT = 'M';

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
