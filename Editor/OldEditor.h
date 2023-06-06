#pragma once

#include <set>
#include <string>

#include "Blocks.h"
#include "../cppfand/base.h"
#include "../cppfand/constants.h"
#include "../Common/pstring.h"

class Instr;
struct EdExitD;
class Instr_setedittxt;

struct MsgStr
{
	std::string Head, Last, CtrlLast, AltLast, ShiftLast;
};

struct stEditorParams
{
	bool Insert = false, Indent = false, Wrap = false, Just = false;
	char Mode = '\0';
	char TypeT = '\0';
	std::string NameT;
};

bool EditText(char pMode, char pTxtType, std::string pName, std::string pErrMsg,
	LongStr* pLS, WORD pMaxLen, WORD& pInd, int& pScr,
	std::vector<WORD>& break_keys, std::vector<EdExitD*>& pExD, bool& pSrch, bool& pUpdat,
	WORD pLastNr, WORD pCtrlLastNr, MsgStr* pMsgS); // r169
void SimpleEditText(char pMode, std::string pErrMsg, std::string pName, LongStr* TxtPtr,
	WORD MaxLen, WORD& Ind, bool& Updat); // r202
WORD FindTextE(const pstring& PstrScreenStr, pstring Popt, char* PTxtPtr, WORD PLen); // r209
void InitTxtEditor();
void EditTxtFile(std::string* locVar, char Mode, std::string& ErrMsg, std::vector<EdExitD*>& ExD, int TxtPos,
	int Txtxy, WRect* V, WORD Atr, std::string Hd, BYTE WFlags, MsgStr* MsgS);
void ViewPrinterTxt(); // r353
void SetEditTxt(Instr_setedittxt* PD);

void Background();
void DisplLL(WORD Flags);
void WrLLMargMsg(std::string& s, WORD n);
void ScrollPress();
void CleanFrame(std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
WORD SetInd(char* text, size_t len_text, WORD Ind, WORD Pos);
void TestUpdFile();
void DelEndT();
void KodLine();
size_t CountChar(char* text, size_t text_len, char C, size_t first, size_t last);
size_t FindCharPosition(char* text, size_t length, char c, size_t from, size_t n = 1);
size_t GetLineNumber(size_t Ind);
size_t GetLineStartIndex(size_t lineNr);
WORD SetPredI();
void NextLine(bool WrScr);
void HelpLU(char dir);
void BlockLRShift(WORD I1);
void FrameStep(BYTE& odir, PressedKey EvKeyC);
void WrCharE(char Ch);
void Format(WORD& i, int First, int Last, WORD Posit, bool Rep);
void SetPart(int Idx);
bool WordExist();
void NextPartDek();
void NewLine(char Mode);
bool TestLastPos(WORD F, WORD T);
void HelpRD(char dir);
WORD Position(WORD n);
WORD Column(WORD p);
void PreviousLine();
void BlockUDShift(int L1);
void TestKod();
void ClrWord();
bool ModPage(int RLine);
int NewL(int RLine);
void DekFindLine(int Num);
int NewRL(int Line);
bool WordFind(WORD i, WORD& WB, short& WE, WORD& LI);
void SetWord(WORD WB, WORD WE);
WORD WordNo2();
WORD GetArrLineLength();
void RollNext();
void RollPred();
void DekodLine(size_t lineStartIndex);
void SetScreen(WORD Ind, WORD ScrXY, WORD Pos);
bool MyPromptLL(WORD n, std::string& s);
void DelChar();
void PredPart();
void DeleteLine();
void CopyCurrentLineToArr(size_t Ind);
//void TestLenText(char** text, size_t& textLength, size_t i, size_t j);
bool TestOptStr(char c);
bool BlockExist();
void SetBlockBound(int& BBPos, int& EBPos);
void FindReplaceString(int First, int Last);
void PosDekFindLine(int Num, WORD Pos, bool ChScr);
bool BlockHandle(int& fs, HANDLE W1, char Oper);
void BlockCopyMove(char Oper, void* P1, LongStr* sp);
bool BlockGrasp(char Oper, void* P1, LongStr* sp);
bool BlockCGrasp(char Oper, void* P1, LongStr* sp);
void BlockDrop(char Oper, void* P1, LongStr* sp);
void BlockCDrop(char Oper, void* P1, LongStr* sp);
void FillBlank();
void SetPartLine(int Ln);
void MyWrLLMsg(pstring s);
void HMsgExit(pstring s);
void Calculate();
stEditorParams SaveParams();
void RestoreParams(stEditorParams& editorParams);
void OpenTxtFh(char Mode);
bool ReadTextFile();
void SimplePrintHead();
void WrEndT();
WORD WordNo(WORD I);

const int SuccLineSize = 256;

extern size_t LenT;
extern char Arr[SuccLineSize];
extern char* T;
extern bool bScroll;

extern WORD MaxLenT, IndexT, ScrT;
extern WORD ScreenIndex;
extern WORD textIndex, positionOnActualLine, BPos;
extern WORD NextLineStartIndex, PageS, LineS;
extern short TextLineNr, ScreenFirstLineNr;
extern int RScrL;
extern bool Konec;
extern bool EditT, ChangeScr;
extern char TypeT;
extern bool SrchT, UpdatT;
extern int* LocalPPtr;
extern HANDLE TxtFH;
//extern bool AllRd;
extern int AbsLenT;
extern BYTE FrameDir;
extern bool Insert, Indent, Wrap, Just;
extern short LeftMarg, RightMarg;
extern WORD columnOffset, Colu, Row;
extern bool InsPg, ChangePart, TypeB;
extern WORD WordL, LastC, FirstC, FirstR, LastR;
extern bool UpdatedL;
extern std::string FindStr, ReplaceStr;
extern bool Replace, FirstEvent;
extern pstring OptionStr;
extern WORD MargLL[4];

extern Blocks* blocks;


extern std::set<char> Separ;

const BYTE LineMaxSize = 255;
const bool TextBlock = false;
const bool ColBlock = true;

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
