#pragma once

#include <set>
#include <string>

#include "Blocks.h"
#include "../Core/base.h"
#include "../Common/pstring.h"

class FrmlElem;
class MergeReportBase;
class Instr;
struct EdExitD;
class Instr_setedittxt;

typedef std::string ColorOrd;

struct MsgStr
{
	std::string Head, Last, CtrlLast, AltLast, ShiftLast;
};


bool ModPage(int RLine);
int NewL(int RLine);
bool MyPromptLL(WORD n, std::string& s);

void SetPartLine(int Ln);
void MyWrLLMsg(std::string s);
void HMsgExit(std::string s);
void SimplePrintHead();

const int SuccLineSize = 256;

//extern char Arr[SuccLineSize];
extern bool bScroll;

extern WORD MaxLenT, IndexT, ScrT;
//extern WORD ScreenIndex;
extern WORD textIndex, positionOnActualLine, BPos;
extern WORD NextLineStartIndex, PageS, LineS;
extern int RScrL;
extern bool Konec;
extern bool EditT;
extern bool SrchT, UpdatT;
extern int* LocalPPtr;
extern HANDLE TxtFH;
//extern bool AllRd;
extern int AbsLenT;
extern BYTE FrameDir;
extern WORD columnOffset, Colu, Row;
extern bool ChangePart, TypeB;
extern WORD LastC, FirstC, FirstR, LastR;
extern bool UpdatedL;
extern bool FirstEvent;
extern WORD MargLL[4];

//extern Blocks* blocks;

extern std::set<char> Separ;

const BYTE LineMaxSize = 255;
const bool TextBlock = false;
const bool ColBlock = true;

//const char TextM = 'T'; const char ViewM = 'V'; const char HelpM = 'H';
//const char SinFM = 'S'; const char DouFM = 'D'; const char DelFM = 'F';
//const char NotFM = 'N'; const char FileT = 'F'; const char LocalT = 'V';
//const char MemoT = 'M';

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

class TextEditorEvents;
class TextEditorScreen;

enum class EditorMode {	Unknown, Normal, Text, Help, View, Edit, FrameSingle, FrameDouble, DeleteFrame, NotFrame };
enum class TextType { Unknown, File, Local, Memo };

class TextEditor
{
public:
	friend class TextEditorEvents;
	friend class TextEditorScreen;

	TextEditor(EditorMode e_mode, TextType text_type);
	virtual ~TextEditor();

	bool EditText(EditorMode e_mode, TextType text_type, std::string pName, std::string pErrMsg,
		std::string& text, size_t pMaxLen, size_t& pInd, int& pScr,
		std::vector<WORD>& break_keys, std::vector<EdExitD*>& pExD, bool& pSrch, bool& pUpdat,
		WORD pLastNr, WORD pCtrlLastNr, MsgStr* pMsgS);
	void SimpleEditText(EditorMode editor_mode, std::string pErrMsg, std::string pName, std::string& text,
		size_t MaxLen, size_t& Ind, bool& Updat);
	void EditTxtFile(std::string* locVar, EditorMode e_mode, std::string& ErrMsg, std::vector<EdExitD*>& ExD, int TxtPos,
		int Txtxy, WRect* V, WORD Atr, std::string Hd, BYTE WFlags, MsgStr* MsgS);
	void ViewPrinterTxt();
	void SetEditTxt(Instr_setedittxt* PD);
	void GetEditTxt(bool& pInsert, bool& pIndent, bool& pWrap, bool& pJust, bool& pColBlk, short& pLeftMarg,
	                short& pRightMarg);

	void InitTxtEditor();
	void InitHelpViewEditor();

	uint8_t ColKey[CountC + 1]{ 0 };
	std::string InsMsg, nInsMsg, IndMsg, WrapMsg, JustMsg, BlockMsg;

protected:
	virtual void Background();
	virtual void UpdStatLine(int Row, int Col);
	void UpdScreen();
	void WriteMargins();
	size_t FindCharPosition(char c, size_t from, size_t n = 1);
	size_t CountChar(char C, size_t first, size_t last);
	WORD SetInd(WORD Ind, WORD Pos);
	size_t GetLine(size_t idx);
	size_t GetLineNumber(size_t Ind);
	WORD Column(WORD p);
	WORD Position(WORD n);
	void DekFindLine(int Num);
	void NextLine(bool WrScr);
	void RollNext();
	void RollPred();

	// *** methods for HELP viewer ***
	virtual void ClrWord();
	virtual void ProcessHelpMode();
	virtual bool WordFind(WORD i, size_t& word_begin, size_t& word_end, size_t& line_nr);
	virtual void SetWord(size_t word_begin, size_t word_end);
	virtual WORD WordNo2();
	virtual size_t WordNo(size_t I);
	virtual bool WordExist();
	virtual void HelpLU(char dir);
	virtual void HelpRD(char dir);
	// ***

	std::vector<std::string> _lines;
	char Arr[LineMaxSize]{ '\0' };  // dekodovany 1 radek
	BYTE TxtColor = 0;
	short TextLineNr = 0;          // cislo radku v celem textu (1 .. N)
	short ScreenFirstLineNr = 0;   // cislo radku, ktery je na obrazovce zobrazen jako prvni (1 .. N)
	bool IsWrScreen = false;

private:
	EditorMode _mode = EditorMode::Unknown;
	TextType _text_type = TextType::Unknown;

	TextEditorEvents* _events = nullptr;
	TextEditorScreen* _screen = nullptr;
	Blocks* blocks = nullptr;

	void FindReplaceString(int First, int Last);
	void ScrollPress();
	void DisplLL(WORD Flags);
	void WrStatusLine();
	void WrLLMargMsg(std::string& s, WORD n);
	void InitScr();
	void CleanFrame(std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
	void SetBlockBound(int& BBPos, int& EBPos);
	void ResetPrint(TextEditor* editor, char Oper, int& fs, HANDLE W1, int LenPrint, ColorOrd* co, WORD& I1, bool isPrintFile, char* p);
	bool BlockHandle(int& fs, HANDLE W1, char Oper);
	void BlockCopyMove(char Oper, void* P1, LongStr* sp);
	bool BlockGrasp(char Oper, void* P1, LongStr* sp);
	bool BlockCGrasp(char Oper, void* P1, LongStr* sp);
	void BlockDrop(char Oper, void* P1, LongStr* sp);
	void BlockCDrop(char Oper, void* P1, LongStr* sp);
	void TestUpdFile();
	void KodLine();
	void DekodLine();
	void FrameStep(BYTE& odir, PressedKey EvKeyC);
	void Format(WORD& i, int First, int Last, WORD Posit, bool Rep);
	void NewLine(char Mode);
	WORD SetPredI();
	void MyWriteln();
	void PreviousLine();
	void FillBlank();
	void DeleteLine();
	bool TestOptStr(char c);
	size_t GetLineStartIndex(size_t lineNr);
	void CopyCurrentLineToArr(size_t Ind);
	void PosDekFindLine(int Num, WORD Pos, bool ChScr);
	void WrEndL(bool Hard, int Row);
	void SetScreen(WORD Ind, WORD ScrXY, WORD Pos);
	void Edit(std::string& text, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
	void UpdateLine();
	void InsertLine(WORD& i, WORD& I1, WORD& I3, WORD& ww, LongStr* sp);
	WORD CurrentLineFirstCharIndex(WORD index);
	void NextPartDek();
	ColorOrd SetColorOrd(size_t last_line) const;
	void ReplaceString(WORD& J, WORD& fst, WORD& lst, int& Last);
	bool FindString(WORD& I, WORD Len);
	bool ReadTextFile();
	//void FirstLine(WORD from, WORD num, WORD& Ind, WORD& Count);
	void UpdateFile();
	bool TestLastPos(WORD F, WORD T);
	void MoveB(WORD& B, WORD& F, WORD& T);
	void DelChar();
	void WrCharE(char Ch);
	void Calculate();
	void BlockLRShift(WORD I1);
	void BlockUDShift(int L1);
	int NewRL(int Line);
	bool BlockExist();
	bool ColBlockExist();
	void NewBlock1(WORD& I1, int& L2);
	void NewBlock2(int& L1, int& L2);
	WORD FindTextE(const pstring& PstrScreenStr, pstring Popt, char* PTxtPtr, WORD PLen);
	void CursorWord();
	WORD GetArrLineLength();
	void direction(BYTE x, BYTE& zn2);
	void OpenTxtFh(EditorMode mode);

	bool Insert, Indent, Wrap, Just;
	pstring OptionStr;
	std::string FindStr, ReplaceStr;
	BYTE BlockColor = 0, SysLColor = 0;
	bool Replace = false;
	std::string ViewMsg;
	short LeftMarg, RightMarg;
	char CharPg = '\0';
	bool InsPg = false;
	bool HardL = false; // actual line (Arr) ended with CRLF "\r\n" - otherwise only with CR "\r"
	bool _change_scr = true; // true if screen needs to be updated (e.g. after scrolling or changes on more lines)

	///  { ^s - underline, ^w - italic, ^q - expanded, ^d - double, ^b - bold, ^e - compressed, ^a - ELITE }
	std::set<char> CtrlKey = { '\x13', '\x17', '\x11', '\x04', '\x02', '\x05', '\x01' };
};