#include "TextEditor.h"
#include <set>
#include <stdexcept>
#include <memory>

#include "../DataEditor/DataEditor.h"
#include "../Core/Compiler.h"
#include "EditorHelp.h"
#include "../Drivers/constants.h"
#include "../Drivers/host.h"
#include "../Core/GlobalVariables.h"
#include "../Drivers/keyboard.h"
#include "../Core/oaccess.h"
#include "../Core/obase.h"
#include "../Core/obaseww.h"
#include "../Core/printtxt.h"
#include "../Core/wwmenu.h"
#include "../Core/wwmix.h"
#include "../Core/models/FrmlElem.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"
#include "../Common/CommonVariables.h"


const int TXTCOLS = 80;
int Timer = 0;

// PROMENNE
bool InsPage;

struct Character {
	char ch = 0;
	uint8_t color = 0;
};

// *** Promenne metody EDIT
WORD NextLineStartIndex = 0;     // index prvniho znaku na dalsim radku
int RScrL = 0;
bool UpdatedL = false;
bool CtrlL = false;
WORD columnOffset = 0;
WORD Colu = 0;
WORD Row = 0;
ColorOrd ColScr;
WORD FirstR = 0, FirstC = 0, LastR = 0, LastC = 0;
WORD MinC = 0, MinR = 0, MaxC = 0, MaxR = 0;
WORD MargLL[4]{ 0, 0, 0, 0 };
WORD PageS = 0, LineS = 0;
bool bScroll = false, FirstScroll = false, HelpScroll = false;
int PredScLn = 0;
WORD PredScPos = 0; // {pozice pred Scroll}
uint8_t FrameDir = 0;
//WORD WordL = 0; // {Mode=HelpM & ctrl-word is on screen}
bool Konec = false;
//WORD i1 = 0, i3 = 0;
//short i2 = 0;
// *** konec promennych

const uint8_t InterfL = 4; /*sizeof(Insert+Indent+Wrap+Just)*/
const WORD TextStore = 0x1000;
const uint8_t TStatL = 35; /*=10(Col Row)+length(InsMsg+IndMsg+WrapMsg+JustMsg+BlockMsg)*/

std::set<char> GlobalSeparators = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,58,59,60,61,62,63,64,
91,92,93,94,96,123,124,125,126,127 };


// {**********global param begin for SaveParams}  // r85
std::string NameT;
std::string ErrMsg;
WORD MaxLenT = 0;
WORD IndexT = 0; // index editovaneho textu (uklada se pro opetovne nacteni na stejne pozici)
WORD ScrT = 0;

//std::vector<EdExitD*> *ExitD = nullptr;

bool SrchT, UpdatT;
WORD LastNr, CtrlLastNr;

bool TypeB;
std::string LastS, CtrlLastS, ShiftLastS, AltLastS, HeadS;
int* LocalPPtr;
bool EditT;

//WORD ScreenIndex = 0; // index of the first char on the screen 0 .. N
WORD textIndex = 0;
WORD positionOnActualLine = 1; // position of the cursor on the actual line (1 .. 255)
WORD BPos = 0; // {screen status}
bool FirstEvent = false;
WORD PHNum = 0, PPageS = 0; // {strankovani ve Scroll}

HANDLE TxtFH = nullptr;
std::string TxtPath;
std::string TxtVol;

int AbsLenT = 0;

//bool ChangePart;
bool UpdPHead;


void MyRunError(std::string s, WORD n)
{
	SetMsgPar(s);
	RunError(n);
}


/// <summary>
/// Find N-th char position in the text
/// </summary>
/// <param name="c">character to find</param>
/// <param name="idx_from">start position 0..n</param>
/// <param name="n">n-th occur 1..n</param>
/// <returns>index of found character, input text length if not found</returns>


WORD FindOrdChar(char C, WORD Pos, WORD Len)
{
	WORD I, K; char cc;
	I = Len; K = Pos - 1; cc = C;
	// TODO: ASM
	return Len - I;
}

WORD FindUpcChar(char C, WORD Pos, WORD Len)
{
	WORD I, K; char cc;
	I = Len; K = Pos - 1; cc = C;
	// TODO: ASM
	return Len - I;
}

bool SEquOrder(pstring S1, pstring S2)
{
	short i;
	if (S1.length() != S2.length()) return false;
	for (i = 1; i <= S1.length(); i++)
		if (CharOrdTab[S1[i]] != CharOrdTab[S2[i]]) return false;
	return true;
}


/**
 * \brief Find control char in the text
 * \param text input text
 * \param textLen input text length
 * \param first first index 0 .. N
 * \param last last index 0 .. N
 * \return index of control char or string::npos if not found
 */
size_t FindCtrlChar(const char* text, size_t textLen, size_t first, size_t last)
{
	if (last > textLen - 1) {
		// koncovy index je za textem
		last = textLen - 1;
	}
	if (first > textLen - 1 || first > last) {
		// pocatecni index je za textem nebo za koncovym indexem
		return std::string::npos; // nenalezeno
	}
	else {
		// ^A ^B ^D ^E ^Q ^S ^W
		std::set<char> pc = { 0x01, 0x02, 0x04, 0x05, 0x11, 0x13, 0x17 };
		for (size_t i = first; i < last; i++) {
			if (pc.count(text[i]) > 0) return i;
		}
		return std::string::npos; // nenalezeno
	}
}

void SimplePrintHead()
{
	//pstring ln;
	PHNum = 0;
	PPageS = 0x7FFF;
}

//void LastLine(char* input, size_t from, size_t num, size_t& Ind, size_t& Count)
//{
//	size_t length = Count;
//	Count = 0;
//	Ind = from;
//	for (int i = from; i < length; i++) {
//		if (input[i] == __ENTER) {
//			Ind = from + i;
//			Count++;
//		}
//	}
//	if (Count > 0 && input[Ind] == __LF) {
//		Ind++; // LF
//	}
//}

bool TextEditor::ReadTextFile()
{
	// kompletne prepsano -> vycte cely soubor do promenne _textT

	_lines.clear();

	auto fileSize = GetFileSize(TxtFH, NULL);
	char* _textT = new char[fileSize];

	DWORD dwBytesRead;
	bool readResult = ReadFile(TxtFH, _textT, fileSize, &dwBytesRead, NULL);
	if (!readResult) {
		HandleError = GetLastError();
		SetMsgPar(TxtPath);
		WrLLF10Msg(700 + HandleError);
	}
	else {
		std::string txt(_textT, fileSize);
		_lines = GetAllLinesWithEnds(txt, HardL);
		HandleError = 0;
	}

	delete[] _textT; _textT = nullptr;

	return false; // return ChangePart
}

//void TextEditor::FirstLine(WORD from, WORD num, WORD& Ind, WORD& Count)
//{
//	char* C = nullptr;
//	WORD* COfs = (WORD*)C;
//	Count = 0; Ind = from - 1; C = &_textT[from];
//	for (WORD i = 0; i < num - 1; i++) {
//		COfs--;
//		if (*C == __ENTER) {
//			Count++;
//			Ind = from - i;
//		}
//	}
//	if ((Count > 0) && (_textT[Ind + 1] == __LF)) Ind++;
//}

//bool RdPredPart()
//{
//	CharArr* ppa;
//	WORD L1, L11, MI;
//	int BL, FSize, Rest, Max, Pos;
//	WORD Pass;
//	Max = MinL(MaxLenT, StoreAvail() + _lenT);
//	Pass = Max - (Max >> 3);
//	Part.MovL = 0;
//	MI = 0;
//	auto result = false;
//	if (Part.PosP == 0) return result;
//	Pos = Part.PosP; BL = Part.LineP;
//	if (_lenT <= (Pass >> 1)) goto label1;
//	FirstLine(_lenT + 1, _lenT - (Pass >> 1), L1, L11);
//	if (L1 < _lenT) {
//		AllRd = false;
//		_lenT = L1;
//		ReleaseStore(&_textT[_lenT + 1]);
//	}
//
//label1:
//	L11 = _lenT;
//	do {
//		if (Pos > 0x1000) L1 = 0x1000;
//		else L1 = Pos;
//		Max = StoreAvail();
//		if (Max > 0x400) Max -= 0x400;
//		if (L1 > Max) L1 = Max;
//		ppa = (CharArr*)GetStore(L1);
//		Move(&_textT[0], &_textT[L1 + 1], _lenT);
//		if (L1 > 0)
//		{
//			SeekH(TxtFH, Pos - L1); ReadH(TxtFH, L1, _textT);
//		}
//		_lenT += L1; Pos -= L1;
//	} while (!((_lenT > Pass) || (Pos == 0) || (L1 == Max)));
//
//	L11 = _lenT - L11; FirstLine(L11 + 1, L11, MI, Part.MovL);
//	if (Pos == 0) MI = L11;
//	else if (Part.MovL > 0) { Part.MovL--; MI = L11 - MI; }
//	L1 = L11 - MI; _lenT -= L1; Pos += L1;
//	if (L1 > 0)
//	{
//		Move(&_textT[L1 + 1], _textT, _lenT);
//		ReleaseStore(&_textT[_lenT + 1]);
//	}
//	/* !!! with Part do!!! */
//	Part.PosP = Pos; Part.LineP = BL - Part.MovL; Part.LenP = _lenT;
//	Part.MovI = MI; Part.UpdP = false;
//	SetColorOrd(Part.ColorP, 1, MI + 1);
//	if ((_lenT == 0)) return result;  /*????????*/
//	result = true;
//	return result;
//}

void TextEditor::UpdateFile()
{
	//SeekH(TxtFH, 0);
	//WriteH(TxtFH, _lenT, _textT);
	//if (HandleError != 0) {
	//	SetMsgPar(TxtPath);
	//	WrLLF10Msg(700 + HandleError);
	//}
	//FlushH(TxtFH);
	//TruncH(TxtFH, _lenT);
	//AbsLenT = FileSizeH(TxtFH);
	//if (HandleError != 0) {
	//	SetMsgPar(TxtPath);
	//	WrLLF10Msg(700 + HandleError);
	//}
	std::string txt = JoinLines(_lines);

	DWORD seekResult = SetFilePointer(TxtFH, 0, NULL, FILE_BEGIN);
	if (seekResult == INVALID_SET_FILE_POINTER) {
		HandleError = GetLastError();
		SetMsgPar(TxtPath);
		WrLLF10Msg(700 + HandleError);
		return;
	}
	bool writeFile = WriteFile(TxtFH, txt.c_str(), txt.length(), NULL, NULL);
	if (!writeFile) {
		HandleError = GetLastError();
		SetMsgPar(TxtPath);
		WrLLF10Msg(700 + HandleError);
		return;
	}
	bool setEOF = SetEndOfFile(TxtFH);
	if (!setEOF) {
		HandleError = GetLastError();
		SetMsgPar(TxtPath);
		WrLLF10Msg(700 + HandleError);
		return;
	}
	DWORD fileSize = GetFileSize(TxtFH, NULL);
	if (fileSize != INVALID_FILE_SIZE) {
		AbsLenT = fileSize;
	}
	else {
		HandleError = GetLastError();
		SetMsgPar(TxtPath);
		WrLLF10Msg(700 + HandleError);
		return;
	}
}

void TextEditor::OpenTxtFh(EditorMode e_mode)
{
	FileUseMode UM;
	CPath = TxtPath;
	CVol = TxtVol;
	TestMountVol(CPath[0]);
	if (e_mode == EditorMode::View) {
		UM = RdOnly;
	}
	else {
		UM = Exclusive;
	}
	//TxtFH = OpenH(CPath, _isOldNewFile, UM);
	TxtFH = CreateFile(
		CPath.c_str(),                        // file name
		GENERIC_READ | GENERIC_WRITE,         // write access
		0,                                    // no sharing
		NULL,                                 // default security attributes
		OPEN_ALWAYS,                          // open file or create new
		FILE_ATTRIBUTE_NORMAL,                // normal file
		NULL);                    // no template file

	if (TxtFH == INVALID_HANDLE_VALUE) {
		HandleError = GetLastError();
		SetMsgPar(CPath);
		RunError(700 + HandleError);
	}
	//AbsLenT = FileSizeH(TxtFH);
	AbsLenT = GetFileSize(TxtFH, NULL);
}

/// <summary>
/// Transform string index to screen position.
/// </summary>
/// <param name="line">Line string</param>
/// <param name="string_index">Index of the character in a line 0..N</param>
/// <returns></returns>


size_t TextEditor::ScrPosToStrIndex(const std::string& line, size_t screen_pos)
{
	size_t pos = 0;
	for (size_t i = 0; i < line.length(); i++) {
		if ((uint8_t)line[i] >= 0x20) {
			pos++;
		}
		if (pos == screen_pos) {
			return i + 1;
		}
	}
	return 1;
}

pstring ShortName(pstring Name)
{
	WORD J = Name.length();
	while (!(Name[J] == '\\' || Name[J] == ':') && (J > 0)) {
		J--;
	}
	pstring s = Name.substr(J, Name.length() - J);
	if (Name[2] == ':') {
		s = Name.substr(0, 2) + s;
	}
	return s;
}




/// Inicializuje obrazovku - sirku, vysku editoru


WORD PColumn(WORD w, char* P)
{
	if (w == 0) {
		return 0;
	}

	WORD ww = 1;
	WORD c = 1;

	while (ww <= w) {
		if (P[ww] >= ' ') {
			c++;
			ww++;
		}
	}

	if (P[w] >= ' ') {
		c--;
	}

	return c;
}

bool MyTestEvent()
{
	if (FirstEvent) return false;
	return TestEvent();
}

void TextEditor::TestUpdFile()
{
	if (TxtFH != nullptr && UpdatT) {
		UpdateFile();
	}
}

/// Counts the number of occurrences of a character;
/// 'first' & 'last' are 0 .. N

/**
 * \brief Ziska cislo radku, ve kterem je znak na indexu
 * \param idx - index 0 .. n
 * \return vraci cislo radku (1 .. N), ve kterem se nachazi index
 */

// vraci index 1. znaku na aktualnim radku (index = 0 .. N)

/// <summary>
/// Load line from text to Arr (without CR/LF), check line length, split line if necessary
/// </summary>

/// ziska index 1. znaku akt. radku, vola DekodLine()

/// vraci cislo radku, na kterem je index


/**
 * \brief Returns order of N-th character in Arr (because of skipping color chars)
 * \param n N-th character (1..256)
 * \return order of the char (1..256) || 0 for 0
 */

/**
 * \brief Returns column for N-th character in Arr (because of skipping color chars)
 * \param p order of the char in the Arr (1..256)
 * \return column on the screen (1..256)
 */

/**
 * \brief Counts Arr Line length (without spaces on the end)
 * \return Arr line length (0 .. 255)
 */

//void NextPart()
//{
//	TestUpdFile();
//	ChangePart = RdNextPart();
//	MoveIdx(1);
//	WrEndT();
//}

/**
 * \brief Get index of the 1st character on the line
 * \param text input text
 * \param text_len input text length
 * \param lineNr line number (1 .. N)
 * \return index of first char on the line (0 .. n), or text length if not found
 */






bool ModPage(int RLine)
{
	return false;
}





/// <summary>
/// Save line from Arr to the vector of lines, adds '\r' or '\r\n' at the end
/// </summary>


int NewL(int RLine)
{
	return RLine; // -Part.LineP;
}

















//void Frame(std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys)
//{
//	pstring FrameString(15);
//	FrameString = "\x50\x48\xB3\x4D\xDA\xC0\xC3\x4B\xBF\xD9\xB4\xC4\xC2\xC1\xC5";
//	pstring FS1(15);
//	FS1 = "\x50\x48\xBA\x4D\xD6\xD3\xC7\x4B\xB7\xBD\xB6\xC4\xD2\xD0\xD7";
//	pstring FS2(15);
//	FS2 = "\x50\x48\xB3\x4D\xD5\xD4\xC6\x4B\xB8\xBE\xB5\xCD\xD1\xCF\xD8";
//	pstring FS3(15);
//	FS3 = "\x50\x48\xBA\x4D\xC9\xC8\xCC\x4B\xBB\xBC\xB9\xCD\xCB\xCA\xCE";
//	uint8_t dir, zn1, zn2, b;
//
//	UpdStatLine(TextLineNr, positionOnActualLine, Mode);
//	screen.CrsBig();
//	uint8_t odir = 0;
//	ClrEvent();
//
//	while (true) /* !!! with Event do!!! */
//	{
//		if (!MyGetEvent(Mode, SysLColor, LastS, LastNr, IsWrScreen, bScroll, ExitD, breakKeys) ||
//			((Event.What == evKeyDown) && (Event.Pressed.KeyCombination() == __ESC)) || (Event.What != evKeyDown)) {
//			ClrEvent();
//			screen.CrsNorm();
//			Mode = TextM;
//			return;
//		}
//		switch (Event.Pressed.KeyCombination()) {
//		case _frmsin_: Mode = SinFM; break;
//		case _frmdoub_: Mode = DouFM; break;
//		case _dfrm_: Mode = DelFM; break;
//		case _nfrm_: Mode = NotFM; break;
//		case __LEFT:
//		case __RIGHT:
//		case __UP:
//		case __DOWN:
//			if (!bScroll) {
//				FrameString[0] = 63;
//				zn1 = FrameString.first(Arr[positionOnActualLine]);
//				zn2 = zn1 & 0x30;
//				zn1 = zn1 & 0x0F;
//				dir = FrameString.first(Hi(Event.Pressed.KeyCombination()));
//				auto dirodir = dir + odir;
//				if (dirodir == 2 || dirodir == 4 || dirodir == 8 || dirodir == 16) odir = 0;
//				if (zn1 == 1 || zn1 == 2 || zn1 == 4 || zn1 == 8) zn1 = 0;
//				char oldzn = Arr[positionOnActualLine];
//				Arr[positionOnActualLine] = ' ';
//				if (Mode == DelFM) b = zn1 && !(odir || dir);
//				else b = zn1 | (odir ^ dir);
//				if (b == 1 || b == 2 || b == 4 || b == 8) b = 0;
//				if ((Mode == DelFM) && (zn1 != 0) && (b == 0)) oldzn = ' ';
//				direction(dir, zn2);
//				direction(odir, zn2);
//				if (Mode == NotFM) b = 0;
//
//				if ((b != 0) && ((Event.Pressed.KeyCombination() == __LEFT) || (Event.Pressed.KeyCombination() == __RIGHT) ||
//					(Event.Pressed.KeyCombination() == __UP) || (Event.Pressed.KeyCombination() == __DOWN)))
//					Arr[positionOnActualLine] = FrameString[zn2 + b];
//				else Arr[positionOnActualLine] = oldzn;
//
//				if ((dir == 1) || (dir == 4)) odir = dir * 2;
//				else odir = dir / 2;
//
//				if (Mode == NotFM) odir = 0;
//				else UpdatedL = true;
//
//				switch (Event.Pressed.KeyCombination()) {
//				case __LEFT: { if (positionOnActualLine > 1) positionOnActualLine--; break; }
//				case __RIGHT: { if (positionOnActualLine < LineMaxSize) positionOnActualLine++; break; }
//				case __UP: { PreviousLine(); break; }
//				case __DOWN: { NextLine(true); break; }
//				default: {};
//				}
//			}
//			break;
//		}
//		ClrEvent();
//		UpdStatLine(TextLineNr, positionOnActualLine, Mode);/*if (not MyTestEvent) */
//		Background();
//	}
//}
















void LowCase(unsigned char& c)
{
	if ((c >= 'A') && (c <= 'Z')) { c = c + 0x20; return; }
	for (size_t i = 128; i <= 255; i++)
		if (((unsigned char)UpcCharTab[i] == c) && (i != c)) { c = i; return; }
}

void LowCase(char& c)
{
	if ((c >= 'A') && (c <= 'Z')) { c = c + 0x20; return; }
	for (size_t i = 128; i <= 255; i++)
		if ((UpcCharTab[i] == c) && (i != c)) { c = i; return; }
}


void DelStorClpBd(uint8_t* P1, LongStr* sp)
{
	//TWork.Delete(ClpBdPos);
	//std::string data = std::string(sp->A, sp->LL);
	//ClpBdPos = TWork.Store(data);
	//ReleaseStore(&P1);
}

void MarkRdClpBd(uint8_t* P1, LongStr* sp)
{
	MarkStore(P1);
	//sp = TWork.ReadLongStr(ClpBdPos);
}













//void ChangeP(WORD& fst)
//{
//	if (ChangePart) {
//		//if (fst <= Part.MovI) fst = 1;
//		//else fst -= Part.MovI;
//		/* if (Last>Part.PosP+_lenT) lst = _lenT-1 else lst = Last-Part.PosP; */
//		//NullChangePart();
//	}
//}



char MyVerifyLL(WORD n, std::string s)
{
	char cc;
	WORD c2 = screen.WhereX() + FirstC - 1;
	WORD r2 = screen.WhereY() + FirstR;
	int w = PushW(1, 1, TxtCols, TxtRows);
	screen.GotoXY(1, TxtRows);
	TextAttr = screen.colors.pTxt;
	ClrEol(TextAttr);
	SetMsgPar(s);
	WriteMsg(n);
	WORD c1 = screen.WhereX();
	WORD r1 = screen.WhereY();
	TextAttr = screen.colors.pNorm;
	printf(" ");
	screen.CrsNorm();
	int t = Timer + 15;
	WORD r = r1;
	do {
		while (!KbdPressed())
			if (Timer >= t) {
				t = Timer + 15;
				if (r == r1) { screen.GotoXY(c2, r2); r = r2; }
				else { screen.GotoXY(c1, r1); r = r1; }
			}
		cc = toupper(ReadKbd());
	} while (!(cc == AbbrYes || cc == AbbrNo || cc == __ESC));
	PopW(w);
	return cc;
}


std::string TextEditor::CursorWord()
{
	std::set<char> sep;

	const std::string line = _lines[TextLineNr - 1];
	size_t start_index = ScrPosToStrIndex(line, positionOnActualLine);
	size_t end_index = start_index + 1;

	if (_mode == EditorMode::Help) {
		sep.insert(0x11);
	}
	else {
		sep = GlobalSeparators;
		if (start_index > 0 && !sep.contains(line[start_index - 1])) {
			start_index--;
		}
	}

	if (!line.empty()) {
		while ((start_index > 1) && !sep.contains(line[start_index - 1])) {
			// find beginning of the word
			start_index--;
		}

		while ((end_index < line.length() - 1) && !sep.contains(line[end_index + 1])) {
			end_index++;
		}
	}

	return line.substr(start_index, end_index - start_index + 1);
}


void TextEditor::SetEditTxt(Instr_setedittxt* PD)
{
	if (PD->Insert != nullptr) Insert = !RunBool(nullptr, PD->Insert, nullptr);
	if (PD->Indent != nullptr) Indent = RunBool(nullptr, PD->Indent, nullptr);
	if (PD->Wrap != nullptr) Wrap = RunBool(nullptr, PD->Wrap, nullptr);
	if (PD->Just != nullptr) Just = RunBool(nullptr, PD->Just, nullptr);
	if (PD->ColBlk != nullptr) TypeB = RunBool(nullptr, PD->ColBlk, nullptr);
	if (PD->Left != nullptr) LeftMarg = MaxI(1, RunInt(nullptr, PD->Left, nullptr));
	if (PD->Right != nullptr) RightMarg = MaxI(LeftMarg, MinI(255, RunInt(nullptr, PD->Right, nullptr)));
}


TextEditor::TextEditor(EditorMode e_mode, TextType text_type)
{
	InitTxtEditor();
	_mode = e_mode;
	_text_type = text_type;
}

TextEditor::~TextEditor()
{

}

bool TextEditor::EditText(EditorMode e_mode, TextType text_type, std::string pName, std::string pErrMsg, std::string& text, size_t pMaxLen,
	size_t& pInd, int& pScr, std::vector<WORD>& break_keys, std::vector<EdExitD*>& pExD, bool& pSrch, bool& pUpdat, WORD pLastNr,
	WORD pCtrlLastNr, MsgStr* pMsgS)
{
	bool oldEdOK = EdOk;
	EditT = true;
	_mode = e_mode;
	_text_type = text_type;
	NameT = pName;
	ErrMsg = pErrMsg;

	// parse input text to lines
	_lines = GetAllLinesWithEnds(text, HardL);

	MaxLenT = pMaxLen;
	IndexT = pInd;
	ScrT = pScr & 0xFFFF;
	positionOnActualLine = pScr >> 16;
	//Breaks = break_keys;
	//ExitD = pExD;
	SrchT = pSrch; UpdatT = pUpdat;
	LastNr = pLastNr; CtrlLastNr = pCtrlLastNr;
	if (pMsgS != nullptr) {
		LastS = pMsgS->Last;
		CtrlLastS = pMsgS->CtrlLast;
		ShiftLastS = pMsgS->ShiftLast;
		AltLastS = pMsgS->AltLast;
		HeadS = pMsgS->Head;
	}
	else {
		/*LastS = nullptr; CtrlLastS = nullptr; ShiftLastS = nullptr; AltLastS = nullptr; HeadS = nullptr;*/
		LastS = ""; CtrlLastS = ""; ShiftLastS = ""; AltLastS = ""; HeadS = "";
	}
	if (_mode != EditorMode::Help) TxtColor = TextAttr;

	// Hostitelsky rezim: celou editaci vcetne napovedy muze prevzit okno hostitele.
	if (FandHost::TextEditEnabled()) {
		FandHost::TextEditRequest req;
		req.Mode = static_cast<int>(e_mode);
		req.TextType = static_cast<int>(text_type);
		req.Pos = static_cast<int>(IndexT);
		req.Scroll = pScr;
		req.Scrolling = (_mode == EditorMode::View) ? 1 : 0;
		req.ReadOnly = (_mode == EditorMode::View) ? 1 : 0;
		memcpy(req.ColKey, ColKey, sizeof(req.ColKey));
		req.TxtColor = TxtColor;
		req.BlockColor = BlockColor;
		strncpy_s(req.Name, NameT.c_str(), sizeof(req.Name) - 1);
		req.Text = text;
		req.BreakKeys = break_keys;

		FandHost::TextEditResult res;
		if (FandHost::RunTextEdit(req, res)) {
			text = res.Text;
			pInd = res.Pos;
			pScr = res.Scroll;
			pUpdat = res.Updated != 0;
			pSrch = false;
			// Volajici ukoncovaci klavesu necte z navratove hodnoty, ale
			// z globalniho Event.Pressed -- viz DataEditor.cpp:3989
			// (c = Event.Pressed.KeyCombination()) nebo EditorHelp.cpp:143.
			// UpdateKey je inverzni funkce ke KeyCombination (keyboard.cpp:419).
			if (res.Key != 0) Event.Pressed.UpdateKey(res.Key);

			// Puvodni editor tady dela gc->LexWord = CursorWord(). V napovede je
			// to zvoleny odkaz, ze ktereho EditorHelp.cpp:195 udela nazev dalsi
			// kapitoly; jinde slovo pod kurzorem (DataEditor.cpp:3986).
			if (!res.Word.empty()) gc->LexWord = res.Word;

			EdOk = oldEdOK;
			return true;
		}
	}

	// Bez hostitele uz editor neni. Vykreslovani, kurzor a obsluha klaves byly
	// vyhozene, kdyz editaci vcetne napovedy prevzalo okno hostitele; konzolova
	// cppfand.exe proto text needituje, jen ho vrati nezmeneny.
	pUpdat = false;
	pSrch = false;
	EdOk = oldEdOK;
	return false;
}

void TextEditor::SimpleEditText(EditorMode editor_mode, std::string pErrMsg, std::string pName, std::string& text, size_t MaxLen, size_t& Ind, bool& Updat)
{
	bool Srch = false;
	int Scr = 0;
	std::vector<WORD> emptyBreakKeys;
	std::vector<EdExitD*> emptyExitD;
	EditText(editor_mode, TextType::Local, std::move(pName), std::move(pErrMsg), text, MaxLen, Ind, Scr,
		emptyBreakKeys, emptyExitD, Srch, Updat, 0, 0, nullptr);
}

void TextEditor::EditTxtFile(std::string* locVar, EditorMode e_mode, std::string& ErrMsg, std::vector<EdExitD*>& ExD,
	int TxtPos, int Txtxy, WRect* V, WORD Atr, const std::string Hd, uint8_t WFlags, MsgStr* MsgS)
{
	bool Srch = false, Upd = false;
	int Size = 0; // , L = 0;
	int w1 = 0;
	bool Loc = false;
	size_t Ind = 0, oldInd = 0;
	int oldTxtxy = 0;
	std::string text;
	std::string compErrTxt;

	if (Atr == 0) {
		Atr = screen.colors.tNorm;
	}
	int w2 = 0;
	int w3 = 0;
	if (V != nullptr) {
		w1 = PushW(1, 1, TxtCols, 1, (WFlags & WPushPixel) != 0, false);
		w2 = PushW(1, TxtRows, TxtCols, TxtRows, (WFlags & WPushPixel) != 0, false);
		w3 = PushWFramed(V->C1, V->R1, V->C2, V->R2, Atr, Hd, "", WFlags);
	}
	else {
		w1 = PushW(1, 1, TxtCols, TxtRows);
		TextAttr = Atr;
	}

	try {
		Loc = (locVar != nullptr);

		if (!Loc) {
			MaxLenT = 0xFFF0;
			TxtPath = CPath;
			TxtVol = CVol;
			// zacatek prace se souborem
			OpenTxtFh(e_mode);
			ReadTextFile();
			SimplePrintHead();
			Ind = TxtPos;
		}
		else {
			Ind = TxtPos;
			text = *locVar;
		}

		oldInd = Ind;
		oldTxtxy = Txtxy;

		while (true) {
			Srch = false;
			Upd = false;
			if (!Loc) {
				// TODO: nahraj text ze souboru
				// std::string LS2 = std::string(_textT, _lenT);
				std::string LS2 = JoinLines(_lines); // ? neni uz nekde cely v pameti ?
				std::vector<WORD> brkKeys = { __F1, __F6, __F9, __ALT_F10 };
				EditText(e_mode, TextType::File, TxtPath, ErrMsg, LS2, 0xFFF0, Ind, Txtxy,
					brkKeys, ExD, Srch, Upd, 126, 143, MsgS);

				// TODO: ulozit jej zpatky?
				/*delete[] _textT;
				_lenT = LS2.length();
				_textT = new char[_lenT];
				memcpy(_textT, LS2.c_str(), _lenT);*/
			}
			else {
				std::vector<WORD> brkKeys = { __F1, __F6 };
				EditText(e_mode, TextType::Local, "", ErrMsg, text, MaxLStrLen, Ind, Txtxy,
					brkKeys, ExD, Srch, Upd, 126, 143, MsgS);
			}
			TxtPos = Ind; // +Part.PosP;
			if (Upd) EdUpdated = true;
			WORD KbdChar = Event.Pressed.KeyCombination();

			if ((KbdChar == __ALT_EQUAL) || (KbdChar == 'U')) {
				// UNDO CHANGES
				text = *locVar;

				if (KbdChar == __ALT_EQUAL) {
					Event.Pressed.UpdateKey(__ESC);
					goto label4;
				}
				else {
					Ind = oldInd;
					Txtxy = oldTxtxy;
					continue;
				}
			}

			if (!Loc) {
				// v originale: ReleaseStore(T) - tady ale smazani pri opetovnem spusteni editoru zpusobuje chybu
				// napr. po navratu z Helpu ...
				// delete[] _textT;
				// _textT = nullptr;
			}

			if (EdBreak == 0xFFFF) {
				switch (KbdChar) {
				case __F9: {
					if (Loc) {

						*locVar = text;
					}
					else {
						//RdPart();
					}
					continue;
				}
				case __F10: {
					if (Event.Pressed.Alt()) {
						Help(nullptr, "", false);
						goto label2;
					}
					break;
				}
				case __F1: {
					ReadMessage(6);
					FandHelp(HelpFD, MsgLine, false);
				label2:
					if (!Loc) {
						// RdPart();
					}
					continue;
				}
				}
			}
			if (!Loc) {
				//Size = FileSizeH(TxtFH);
				Size = GetFileSize(TxtFH, NULL);
				//CloseH(&TxtFH);
				CloseHandle(TxtFH);
				TxtFH = NULL;
			}
			if ((EdBreak == 0xFFFF) && (KbdChar == __F6)) {
				if (Loc) {
					PrintArray(text, false);
					continue;
				}
				else {
					CPath = TxtPath;
					CVol = TxtVol;
					PrintTxtFile(0);
					OpenTxtFh(e_mode);
					// RdPart();
					continue;
				}
			}

			if (!Loc && (Size < 1)) {
				MyDeleteFile(TxtPath);
			}

			if (Loc && (KbdChar == __ESC)) {
				// TODO: why? s.length() = _lenT;
			}

		label4:
			if (IsCompileErr) {
				IsCompileErr = false;
				compErrTxt = MsgLine;
				SetMsgPar(compErrTxt);
				WrLLF10Msg(110);
			}
			if (Loc) {
				*locVar = text;
			}
			if (w3 != 0) {
				PopW(w3, (WFlags & WNoPop) == 0);
			}
			if (w2 != 0) {
				PopW(w2);
			}
			PopW(w1);
			LastTxtPos = Ind; // +Part.PosP;
			break;
		}
	}
	catch (std::exception& e) {
		// TODO: log error
	}
}

void TextEditor::ViewPrinterTxt()
{
	WRect V = { 1, 2, 80, 24 };
	if (!PrintView) return;
	SetPrintTxtPath();
	V.C2 = TxtCols;
	V.R2 = TxtRows - 1;
	std::string ErrMsg;
	std::vector<EdExitD*> emptyExitD;
	EditTxtFile(nullptr, EditorMode::Unknown, ErrMsg, emptyExitD, 1, 0, &V, 0, "", WPushPixel, nullptr);
}

void TextEditor::InitTxtEditor()
{
	FindStr = ""; ReplaceStr = "";
	OptionStr[0] = 0; Replace = false;

	TxtColor = screen.colors.tNorm;
	BlockColor = screen.colors.tBlock;
	SysLColor = screen.colors.fNorm;

	ColKey[0] = screen.colors.tCtrl;
	ColKey[1] = screen.colors.tUnderline;
	ColKey[2] = screen.colors.tItalic;
	ColKey[3] = screen.colors.tDWidth;
	ColKey[4] = screen.colors.tDStrike;
	ColKey[5] = screen.colors.tEmphasized;
	ColKey[6] = screen.colors.tCompressed;
	ColKey[7] = screen.colors.tElite;

	InsMsg = ReadMessage(411);
	nInsMsg = ReadMessage(412);
	IndMsg = ReadMessage(413);
	WrapMsg = ReadMessage(414);
	JustMsg = ReadMessage(415);
	BlockMsg = ReadMessage(417);
	ViewMsg = ReadMessage(416);

	Insert = true; Indent = true; Wrap = false; Just = false; TypeB = false;
	LeftMarg = 1; RightMarg = 78;
	CharPg = /*char(250)*/ spec.TxtCharPg;
	InsPg = /*true*/ spec.TxtInsPg;
}




