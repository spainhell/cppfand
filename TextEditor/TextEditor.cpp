#include "TextEditor.h"
#include <set>
#include <stdexcept>
#include <memory>

#include "TextEditorEvents.h"
#include "TextEditorScreen.h"
#include "../DataEditor/DataEditor.h"
#include "../Core/Compiler.h"
#include "EditorHelp.h"
#include "HelpViewer.h"
#include "../Drivers/constants.h"
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


const int TXTCOLS = 80;
int Timer = 0;

// PROMENNE
bool InsPage;

struct Character {
	char ch = 0;
	BYTE color = 0;
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
BYTE FrameDir = 0;
//WORD WordL = 0; // {Mode=HelpM & ctrl-word is on screen}
bool Konec = false;
//WORD i1 = 0, i3 = 0;
//short i2 = 0;
// *** konec promennych

const BYTE InterfL = 4; /*sizeof(Insert+Indent+Wrap+Just)*/
const WORD TextStore = 0x1000;
const BYTE TStatL = 35; /*=10(Col Row)+length(InsMsg+IndMsg+WrapMsg+JustMsg+BlockMsg)*/

std::set<char> Separ = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
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

void MyWrLLMsg(std::string s)
{
	if (HandleError == 4) s = "";
	SetMsgPar(s);
	WrLLF10Msg(700 + HandleError);
}

void MyRunError(std::string s, WORD n)
{
	SetMsgPar(s);
	RunError(n);
}

void HMsgExit(std::string s)
{
	switch (HandleError) {
	case 0: return;
	case 1: {
		s = s[1];
		SetMsgPar(s);
		RunError(700 + HandleError);
		break;
	}
	case 2:
	case 3: {
		SetMsgPar(s);
		RunError(700 + HandleError);
		break;
	}
	case 4: {
		RunError(704);
		break;
	}
	}
}

/// <summary>
/// Find N-th char position in the text
/// </summary>
/// <param name="c">character to find</param>
/// <param name="idx_from">start position 0..n</param>
/// <param name="n">n-th occur 1..n</param>
/// <returns>index of found character, input text length if not found</returns>
size_t TextEditor::FindCharPosition(char c, size_t idx_from, size_t n)
{
	std::string txt = JoinLines(_lines);
	size_t result = std::string::npos; // as not found
	for (size_t j = 0; j < n; j++) {
		for (size_t i = idx_from; i < txt.length(); i++) {
			if (txt[i] == c) {
				result = i;
				break;
			}
		}
		idx_from = result + 1;
	}
	return result == std::string::npos ? txt.length() : result;
}

bool TextEditor::TestOptStr(char c)
{
	return (OptionStr.first(c) != 0) || (OptionStr.first(toupper(c)) != 0);
}

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

bool TextEditor::FindString(WORD& I, WORD Len)
{
	WORD i1 = 0;
	pstring s1, s2;
	char c = '\0';
	auto result = false;
	c = FindStr[1];
	if (!FindStr.empty())
	{
	label1:
		std::string txt = JoinLines(_lines);
		if (TestOptStr('~')) i1 = FindOrdChar(c, I, Len);
		else if (TestOptStr('u')) i1 = FindUpcChar(c, I, Len);
		else {
			i1 = FindCharPosition(c, I);
		}
		I = i1;
		if (I + FindStr.length() > Len) {
			return result;
		}
		s2 = FindStr;
		//Move(&_textT[I], &s1[1], FindStr.length());
		s1[0] = FindStr.length();
		if (TestOptStr('~')) {
			if (!SEquOrder(s1, s2)) {
				I++;
				goto label1;
			}
		}
		else if (TestOptStr('u')) {
			if (!EquUpCase(s1, s2)) {
				I++;
				goto label1;
			}
		}
		else if (s1 != s2) {
			I++;
			goto label1;
		}
		if (TestOptStr('w')) {
			if (I > 1 && !Separ.count(txt[I - 1]) || !Separ.count(txt[I + FindStr.length()])) {
				I++;
				goto label1;
			}
		}
		result = true;
		I += FindStr.length();
	}
	return result;
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

void TextEditor::WrStatusLine()
{
	std::string Blanks;
	if (_mode != EditorMode::Help) {
		if (HeadS.length() > 0) {
			Blanks = AddTrailChars(HeadS, ' ', TXTCOLS);
			size_t i = Blanks.find('_');
			if (i == std::string::npos) {
				Blanks = Blanks.substr(0, TStatL + 3) + Blanks.substr(TStatL + 3 - 1, 252 - TStatL);
				for (size_t j = 0; j < TStatL + 2; j++) {
					Blanks[j] = ' ';
				}
			}
			else {
				while ((i < Blanks.length()) && (Blanks[i] == '_')) {
					Blanks[i] = ' ';
					i++;
				}
			}
		}
		else {
			Blanks = RepeatString(' ', TxtCols);
			std::string s = ShortName(NameT);
			size_t i = TStatL + 3 - 1;
			if (s.length() + i >= TXTCOLS) i = TXTCOLS - s.length() - 2;
			for (size_t j = 0; j < s.length(); j++) {
				Blanks[i + j] = s[j];
			}
		}
		screen.ScrWrStr(1, 1, Blanks, SysLColor);
	}
}

void TextEditor::WriteMargins()
{
	CHAR_INFO LastL[201];

	if ((_mode != EditorMode::Help) && (_mode != EditorMode::View) && Wrap) {
		screen.ScrRdBuf(FirstC, TxtRows, LastL, LineS);
		LastL[MargLL[0]].Attributes = MargLL[1] >> 8;
		LastL[MargLL[0]].Char.AsciiChar = MargLL[1] & 0x00FF;
		LastL[MargLL[2]].Attributes = MargLL[3] >> 8;
		LastL[MargLL[2]].Char.AsciiChar = MargLL[3] & 0x00FF;

		MargLL[0] = MaxI(0, LeftMarg - BPos);
		if (MargLL[0] > 0) {
			MargLL[1] = (LastL[MargLL[0]].Attributes << 8) + LastL[MargLL[0]].Char.AsciiChar;
			LastL[MargLL[0]].Attributes = LastL[LineS].Attributes;
			LastL[MargLL[0]].Char.AsciiChar = 0x10;
		}
		MargLL[2] = MaxI(0, RightMarg - BPos);
		if (MargLL[2] > 0) {
			MargLL[3] = (LastL[MargLL[2]].Attributes << 8) + LastL[MargLL[2]].Char.AsciiChar;
			LastL[MargLL[2]].Attributes = LastL[LineS].Attributes;
			LastL[MargLL[2]].Char.AsciiChar = 0x11;
		}
		screen.ScrWrCharInfoBuf(short(FirstC - 1), short(TxtRows - 1), LastL, LineS);
	}
}

void TextEditor::WrLLMargMsg(std::string& s, WORD n)
{
	if (!s.empty()) {
		MsgLine = s;
		WrLLMsgTxt(s);
	}
	else {
		if (n != 0) WrLLMsg(n);
		else {
			if (!LastS.empty()) {
				MsgLine = LastS;
				WrLLMsgTxt(LastS);
			}
			else {
				WrLLMsg(LastNr);
			}
			if (_mode == EditorMode::Text) {
				WriteMargins();
			}
		}
	}
}

/// Inicializuje obrazovku - sirku, vysku editoru
void TextEditor::InitScr()
{
	FirstR = WindMin.Y;
	FirstC = WindMin.X;
	LastR = WindMax.Y;
	LastC = WindMax.X;

	if ((FirstR == 1) && (_mode != EditorMode::Help)) {
		FirstR++;
	}
	if (LastR == TxtRows) {
		LastR--;
	}
	MinC = FirstC; MinR = FirstR; MaxC = LastC; MaxR = LastR;
	screen.Window(FirstC, FirstR, LastC, LastR);
	FirstR--;
	if ((_mode != EditorMode::Help) && (_mode != EditorMode::View) && Wrap) {
		LastC--;
	}
	PageS = LastR - FirstR;
	LineS = succ(LastC - FirstC);
}

void TextEditor::UpdStatLine(int Row, int Col)
{
	char RowCol[] = "RRRRR:CCCCC";
	char StatLine[] = "                                   ";

	if (!HelpScroll) {
		int lRow = Row; // +Part.LineP;
		snprintf(RowCol, sizeof(RowCol), "%5i:%-5i", lRow, Col);
		memcpy(&StatLine[1], RowCol, 11); // 11 znaku ve format 'RRRRR:CCCCC'

		switch (_mode) { // uses parameter 'mode', not global variable 'Mode'
		case EditorMode::Text: {
			if (Insert) {
				memcpy(&StatLine[10], InsMsg.c_str(), 5);
			}
			else {
				memcpy(&StatLine[10], nInsMsg.c_str(), 5);
			}
			if (Indent) {
				memcpy(&StatLine[15], IndMsg.c_str(), 5);
			}
			if (Wrap) {
				memcpy(&StatLine[20], WrapMsg.c_str(), 5);
			}
			if (Just) {
				memcpy(&StatLine[25], JustMsg.c_str(), 5);
			}
			if (TypeB == ColBlock) {
				memcpy(&StatLine[30], BlockMsg.c_str(), 5);
			}
			break;
		}
		case EditorMode::View: {
			memcpy(&StatLine[10], ViewMsg.c_str(), ViewMsg.length());
			break;
		}
		case EditorMode::FrameSingle: {
			StatLine[12] = '-'; break;
		}
		case EditorMode::FrameDouble: {
			StatLine[12] = '='; break;
		}
		case EditorMode::DeleteFrame: {
			StatLine[12] = '/'; break;
		}
		default: break;
		}

		short i = 1;

		if (!HeadS.empty()) {
			size_t find = HeadS.find('_');

			if (find == std::string::npos) {
				find = 0;
			}

			i = MaxW(1, find);

			if (i > TxtCols - TStatL) {
				i = MaxI((short)(TxtCols)-TStatL, 1);
			}
		}
		screen.ScrWrStr(i, 1, StatLine, SysLColor);
	}
}

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
size_t TextEditor::CountChar(char C, size_t first, size_t last)
{
	std::string txt = JoinLines(_lines);
	size_t count = 0;
	if (first < txt.length()) {
		if (last >= txt.length()) last = txt.length() - 1;
		for (size_t i = first; i < last; i++) {
			if (txt[i] == C) count++;
		}
	}
	else {
		// out of index
	}
	return count;
}

/**
 * \brief Ziska cislo radku, ve kterem je znak na indexu
 * \param idx - index 0 .. n
 * \return vraci cislo radku (1 .. N), ve kterem se nachazi index
 */
size_t TextEditor::GetLine(size_t idx)
{
	return CountChar(__CR, 0, idx) + 1;
}

// vraci index 1. znaku na aktualnim radku (index = 0 .. N)
WORD TextEditor::CurrentLineFirstCharIndex(WORD index)
{
	std::string txt = JoinLines(_lines);
	WORD result = 0;
	while (index > 0) {
		if (txt[index] == __CR) {
			// jsme na '\r' -> prazdny radek
			result = index;
			break;
		}

		if (txt[index - 1] == __CR || txt[index - 1] == __LF) {
			// predchozi znak je '\r' nebo '\n' -> jsme na 1. znaku radku
			//index++;
			//if (_textT[index] == _LF) index++;
			result = index;
			break;
		}

		index--;
	}
	return result;
}

/// <summary>
/// Load line from text to Arr (without CR/LF), check line length, split line if necessary
/// </summary>
void TextEditor::DekodLine()
{
	std::string& line = _lines[TextLineNr - 1];

	// is it the last line? (no CR or LF)
	bool b_last_line = !(line.ends_with('\r') || line.ends_with('\n'));

	if (!b_last_line) {
		// it's not the last line, check if it ends with CR or LF
		HardL = line.ends_with("\r\n");
	}

	if (line.length() > LineMaxSize) {
		line = line.substr(0, LineMaxSize);
		if (_mode == EditorMode::Text) {
			if (PromptYN(402)) {
				// split line into two lines, insert new line to vector of lines
				line = line.substr(0, LineMaxSize - (HardL ? 2 : 1)) + (HardL ? "\r\n" : "\r");
				_lines.insert(_lines.begin() + TextLineNr, line.substr(LineMaxSize - (HardL ? 2 : 1)));
				UpdatT = true;
			}
		}
		else {
			_mode = EditorMode::View;
		}
	}

	FillChar(Arr, LineMaxSize, ' ');
	if (!line.empty()) {
		if (b_last_line) {
			memcpy(Arr, line.c_str(), line.length());
		}
		else {
			// copy line to Arr	without CR/LF
			memcpy(Arr, line.c_str(), line.length() - (HardL ? 2 : 1));
		}
	}

	UpdatedL = false;
}

/// ziska index 1. znaku akt. radku, vola DekodLine()
void TextEditor::CopyCurrentLineToArr(size_t Ind)
{
	textIndex = CurrentLineFirstCharIndex(Ind);
	DekodLine();
}

/// vraci cislo radku, na kterem je index
size_t TextEditor::GetLineNumber(size_t idx)
{
	CopyCurrentLineToArr(idx);
	size_t line = 1;
	if (idx == 0)
	{
		// 1st char is always in 1st line
	}
	else
	{
		line = GetLine(textIndex);
	}
	return line;
}

WORD TextEditor::SetInd(WORD Ind, WORD Pos) // { line, pozice --> index}
{

	return 0;

	// TODO:
	/*std::string txt = JoinLines(_lines);

	WORD P = Ind == 0 ? 0 : Ind - 1;
	if (Ind < txt.length()) {
		while ((Ind - P < Pos) && (txt[Ind - 1] != __CR)) { Ind++; }
	}
	return Ind;*/
}

/**
 * \brief Returns order of N-th character in Arr (because of skipping color chars)
 * \param n N-th character (1..256)
 * \return order of the char (1..256) || 0 for 0
 */
WORD TextEditor::Position(WORD n) // {PosToCol}
{
	if (n == 0) return 0;

	uint16_t c = 1;

	for (size_t i = 0; i < LineMaxSize; i++) {
		if (static_cast<uint8_t>(Arr[i]) < ' ') {
			// non-printable char
			continue;
		}
		else {
			if (c >= n) {
				return i + 1; // return index of the char
			}
			c++;
		}
	}

	return LineMaxSize;
}

/**
 * \brief Returns column for N-th character in Arr (because of skipping color chars)
 * \param p order of the char in the Arr (1..256)
 * \return column on the screen (1..256)
 */
WORD TextEditor::Column(WORD p)
{
	if (p == 0) return 0;

	WORD pp = 1;
	WORD c = 1;

	while (pp <= p) {
		if ((BYTE)Arr[pp - 1] >= ' ') {
			c++;
		}
		pp++;
	}

	if ((BYTE)Arr[p - 1] >= ' ') c--;

	return c;
}

/**
 * \brief Counts Arr Line length (without spaces on the end)
 * \return Arr line length (0 .. 255)
 */
WORD TextEditor::GetArrLineLength()
{
	int LP = LineMaxSize;
	while ((LP > 0) && (Arr[LP - 1] == ' ' || Arr[LP - 1] == '\0')) {
		LP--;
	}
	return LP;
}

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
size_t TextEditor::GetLineStartIndex(size_t lineNr)
{
	// znacne zjednoduseno oproti originalu

	size_t result;

	std::string txt = JoinLines(_lines);

	if (lineNr <= 1) {
		result = 0;
	}
	else {
		// hledame pozici za n-tym vyskytem _CR
		size_t pos = FindCharPosition(__CR, 0, lineNr - 1) + 1;
		// pokud je na nalezene pozici _LF, jdi o 1 dal
		if (pos < txt.length() && txt[pos] == __LF) {
			pos++;
		}
		result = pos;
	}

	if (txt.length() == result) {
		// jsme na konci textu
		TextLineNr = GetLineNumber(txt.length());
	}

	return result;
}

void SetPartLine(int Ln)
{
	//while ((Ln <= Part.LineP) && (Part.PosP > 0)) {
	//	PredPart();
	//}
	//while ((Ln - Part.LineP > 0x7FFF) && !AllRd) {
	//	NextPart();
	//}
}

void TextEditor::DekFindLine(int Num)
{
	TextLineNr = min(Num, _lines.size());
	DekodLine();
}

void TextEditor::PosDekFindLine(int Num, WORD Pos, bool ChScr)
{
	positionOnActualLine = Pos;
	DekFindLine(Num);
	_change_scr = _change_scr || ChScr;
}

void TextEditor::WrEndL(bool Hard, int Row)
{
	if ((_mode != EditorMode::Help) && (_mode != EditorMode::View) && Wrap) {
		WORD w;
		if (Hard) {
			w = 0x11 + static_cast<WORD>(TxtColor << 8);
		}
		else {
			w = ' ' + static_cast<WORD>(TxtColor << 8);
		}
		screen.ScrWrBuf(WindMin.X + LineS, WindMin.Y + Row - 1, &w, 1);
	}
}

void TextEditor::NextPartDek()
{
	ReadTextFile();
	DekodLine();
}

bool ModPage(int RLine)
{
	return false;
}

ColorOrd TextEditor::SetColorOrd(size_t last_line) const
{
	ColorOrd co;

	if (last_line > 0) {
		for (size_t i = 0; i < last_line; i++) {
			const std::string& line = _lines[i];
			size_t index = FindCtrlChar(line.c_str(), line.length(), 0, line.length());
			while (index < line.length()) {
				size_t pp = co.find(line[index]);
				if (pp != std::string::npos) {
					co.erase(pp);
				}
				else {
					co += line[index];
				}
				index = FindCtrlChar(line.c_str(), line.length(), index + 1, line.length());
			}
		}
	}
	else {
		// there are no lines above to check
	}

	return co;
}

void TextEditor::UpdateLine()
{
	// changes only in the current line
	if (bScroll) {
		ColorOrd co1 = SetColorOrd(TextLineNr - 1);
		_screen->ScrollWrline(Arr, columnOffset, TextLineNr - ScreenFirstLineNr + 1, co1, ColKey, TxtColor, InsPage);
	}
	else {
		_screen->EditWrline(std::string(Arr, 255), TextLineNr - ScreenFirstLineNr + 1, ColKey, TxtColor, BlockColor);
	}
}

void TextEditor::UpdScreen()
{
	short r; // row number, starts from 1
	ColorOrd co1;
	//WORD oldScreenIndex = ScreenIndex;
	std::string PgStr;

	InsPage = false;
	if (_change_scr) {
		//if (ChangePart) {
		//	DekodLine(textIndex);
		//}
		_change_scr = false;

		//if (bScroll) {
		//	ScreenIndex = textIndex;
		//}
		//else {
		//	ScreenIndex = GetLineStartIndex(ScreenFirstLineNr);
		//}

		if (HelpScroll) {
			//ColScr = Part.ColorP;
			ColScr = SetColorOrd(TextLineNr - 1);
		}
	}

	if (bScroll) {
		// {tisk aktualniho radku}
		PgStr = std::string(255, CharPg);
		//FillChar(&PgStr[0], 255, CharPg);
		//PgStr[0] = 255;
		co1 = ColScr;
		r = 0;
		while (Arr[r] == 0x0C) {
			r++;
		}
		_screen->ScrollWrline(&Arr[r], columnOffset, 1, co1, ColKey, TxtColor, InsPage);
	}
	else if (_mode == EditorMode::Help) {
		//co1 = Part.ColorP;
		co1 = SetColorOrd(TextLineNr - 1);
		_screen->ScrollWrline(Arr, columnOffset, TextLineNr - ScreenFirstLineNr + 1, co1, ColKey, TxtColor, InsPage);
	}
	else {
		_screen->EditWrline(std::string(Arr, 255), TextLineNr - ScreenFirstLineNr + 1, ColKey, TxtColor, BlockColor);
	}

	WrEndL(HardL, TextLineNr - ScreenFirstLineNr + 1);

	if (MyTestEvent()) return;

	//WORD index = ScreenIndex;

	size_t line_offset = 1;
	short rr = 0;
	WORD w = 1;
	InsPage = false;
	ColorOrd co2 = ColScr;

	//std::string txt = JoinLines(_lines);

	if (bScroll) {
		//while (txt[index] == 0x0C) {
		//	index++;
		//}
	}

	do { // {tisk celeho okna}
		if (MyTestEvent()) return;

		if (bScroll /*&& (index < txt.length())*/) {
			if ((InsPg && (ModPage(line_offset - rr + RScrL - 1))) || InsPage) {
				_screen->EditWrline(PgStr, line_offset, ColKey, TxtColor, BlockColor);
				WrEndL(false, line_offset);
				if (InsPage) rr++;
				InsPage = false;
				goto label1;
			}
		}
		//if (!bScroll && (index == textIndex)) {
		//	index = NextLineStartIndex;
		//	co2 = co1;
		//	goto label1;
		//}
		if (ScreenFirstLineNr + line_offset - 2 < _lines.size()) {
			// index je mensi nez delka textu -> porad je co tisknout
			if (HelpScroll) {
				_screen->ScrollWrline((char*)_lines[ScreenFirstLineNr + line_offset - 2].c_str(), columnOffset, line_offset, co2, ColKey, TxtColor, InsPage);
			}
			else {
				_screen->EditWrline(_lines[ScreenFirstLineNr + line_offset - 2], line_offset, ColKey, TxtColor, BlockColor);
			}

			//if (InsPage) {
			//	// najde konec radku, potrebujeme 1. znak dalsiho radku
			//	index = FindCharPosition(0x0C, index) + 1;
			//}
			//else {
			//	// najde konec radku, potrebujeme 1. znak dalsiho radku
			//	index = FindCharPosition(__CR, index) + 1;
			//}
			//WrEndL((index < txt.length()) && (txt[index] == __LF), r);
			//if (index < txt.length() && txt[index] == __LF) {
			//	index++;
			//}
		}
		else {
			_screen->EditWrline("", line_offset, ColKey, TxtColor, BlockColor);
			WrEndL(false, line_offset);
		}

	label1:
		line_offset++;
		//if (index < txt.length() && bScroll && (txt[index] == 0x0C)) {
		//	InsPage = InsPg;
		//	index++;
		//}
	} while (line_offset <= PageS);
}

void TextEditor::Background()
{
	UpdStatLine(TextLineNr, positionOnActualLine);
	// TODO: musi to tady byt?

	if (positionOnActualLine > LineS) {
		if (positionOnActualLine > BPos + LineS) {
			BPos = positionOnActualLine - LineS;
		}
	}
	if (positionOnActualLine <= BPos) {
		BPos = pred(positionOnActualLine);
	}

	if (TextLineNr < ScreenFirstLineNr) {
		ScreenFirstLineNr = TextLineNr;
		_change_scr = true;
	}
	if (TextLineNr >= ScreenFirstLineNr + PageS) {
		ScreenFirstLineNr = succ(TextLineNr - PageS);
		_change_scr = true;
	}

	if (bScroll || _change_scr) {
		UpdScreen();
	}
	else {
		UpdateLine();
	}

	WriteMargins();
	screen.GotoXY(positionOnActualLine - BPos, TextLineNr - ScreenFirstLineNr + 1);
	IsWrScreen = true;
}

/// <summary>
/// Save line from Arr to the vector of lines, adds '\r' or '\r\n' at the end
/// </summary>
void TextEditor::KodLine()
{
	size_t ArrLineLen = GetArrLineLength(); // Arr bez koncovych mezer
	std::string ArrLine = std::string(Arr, ArrLineLen);

	if (TextLineNr == _lines.size()) {
		// posledni radek (nepridavame konec radku)
	}
	else {
		if (HardL) {
			ArrLine += "\r\n";
		}
		else {
			ArrLine += "\r";
		}
	}

	_lines[TextLineNr - 1] = ArrLine;

	//TestLenText(&_textT, _lenT, NextLineStartIndex, textIndex + LP);
	UpdatT = true;

	//NextLineStartIndex = textIndex + ArrLine.length();
	//LP = NextLineStartIndex - 1;

	UpdatedL = false;
}

int TextEditor::NewRL(int Line)
{
	return LineAbs(Line);
}

int NewL(int RLine)
{
	return RLine; // -Part.LineP;
}

void TextEditor::ScrollPress()
{
	bool old = bScroll;
	const bool fyz = keyboard.GetState(VK_SCROLL) & 0x0001;
	if (fyz == old) FirstScroll = false;
	bScroll = (fyz || FirstScroll) && (_mode != EditorMode::Help);
	HelpScroll = bScroll || (_mode == EditorMode::Help);
	int L1 = LineAbs(ScreenFirstLineNr);
	if (old != bScroll) {
		if (bScroll) {
			WrStatusLine();
			if (UpdatedL) KodLine();
			screen.CrsHide();
			PredScLn = LineAbs(TextLineNr);
			PredScPos = positionOnActualLine;
			if (UpdPHead) {
				//SetPart(1);
				SimplePrintHead();
				DekFindLine(MaxL(L1, PHNum + 1));
			}
			else {
				DekFindLine(MaxL(L1, PHNum + 1));
			}
			ScreenFirstLineNr = TextLineNr;
			RScrL = NewRL(ScreenFirstLineNr);
			if (L1 != LineAbs(ScreenFirstLineNr)) _change_scr = true; // { DekodLine; }
			columnOffset = Column(BPos);
			Colu = Column(positionOnActualLine);
			//ColScr = Part.ColorP;
			ColScr = SetColorOrd(TextLineNr - 1);
		}
		else {
			if ((PredScLn < L1) || (PredScLn >= L1 + PageS)) PredScLn = L1;
			if (!(PredScPos >= BPos + 1 && PredScPos <= BPos + LineS)) PredScPos = BPos + 1;
			PosDekFindLine(PredScLn, PredScPos, false);
			if (_mode == EditorMode::View
				|| _mode == EditorMode::FrameSingle
				|| _mode == EditorMode::FrameDouble
				|| _mode == EditorMode::DeleteFrame
				|| _mode == EditorMode::NotFrame) {
				screen.CrsBig();
			}
			else {
				screen.CrsNorm();
			}
		}
		Background();
	}
}

void TextEditor::DisplLL(WORD Flags)
{
	if ((Flags & 0x04) != 0) // { Ctrl }
		WrLLMargMsg(CtrlLastS, CtrlLastNr);
	else if ((Flags & 0x03) != 0) // { Shift }
		WrLLMargMsg(ShiftLastS, 0);
	else if ((Flags & 0x08) != 0) // { Alt }
		WrLLMargMsg(AltLastS, 0);
}

void TextEditor::RollNext()
{
	std::string txt = JoinLines(_lines);

	if (NextLineStartIndex <= txt.length()) {
		screen.GotoXY(1, 1);
		ScreenFirstLineNr++;
		_change_scr = true;
		if (TextLineNr < ScreenFirstLineNr) {
			if (UpdatedL) KodLine();
			TextLineNr++;
			textIndex = NextLineStartIndex;
			DekodLine();
		}
	}
}

void TextEditor::RollPred()
{
	std::string txt = JoinLines(_lines);

	if (ScreenFirstLineNr > 1) {
		screen.GotoXY(1, 1);
		ScreenFirstLineNr--;
		_change_scr = true;
		if (TextLineNr == ScreenFirstLineNr + PageS) {
			if (UpdatedL) KodLine();
			TextLineNr--;
			if (txt[textIndex - 1] == __LF) { CopyCurrentLineToArr(textIndex - 2); }
			else { CopyCurrentLineToArr(textIndex - 1); }
		}
	}
}

void TextEditor::ProcessPageUp()
{
	size_t I1 = 0, I2 = 0;

	if (UpdatedL) KodLine();


	int32_t L1 = LineAbs(TextLineNr);

	if (bScroll) {
		RScrL = MaxL(1, RScrL - PageS);
		if (ModPage(RScrL)) { RScrL++; }
		ScreenFirstLineNr = NewL(RScrL);
		TextLineNr = ScreenFirstLineNr;
		DekFindLine(LineAbs(TextLineNr));
		positionOnActualLine = Position(Colu);
		int j = 0; // TODO: CountChar(0x0C, textIndex, ScreenIndex);

		if ((j > 0) && InsPg) {
			DekFindLine(LineAbs(TextLineNr + j));
			ScreenFirstLineNr = TextLineNr;
			RScrL = NewRL(ScreenFirstLineNr);
		}
	}
	else {
		if (ScreenFirstLineNr > PageS) {
			ScreenFirstLineNr -= PageS;
		}
		else {
			ScreenFirstLineNr = 1;
		}

		DekFindLine(LineAbs(TextLineNr - PageS));
	}

	_change_scr = true;

	BlockUDShift(L1);
}

void TextEditor::ProcessPageDown()
{
	if (UpdatedL) KodLine();

	int L1 = LineAbs(TextLineNr);

	if (bScroll) {
		RScrL += PageS;
		if (ModPage(RScrL)) {
			RScrL--;
		}
		DekFindLine(LineAbs(NewL(RScrL)));
		positionOnActualLine = Position(Colu);
		// TODO:
		int j = 0; // CountChar(0x0C, ScreenIndex, textIndex);
		if ((j > 0) && InsPg) {
			DekFindLine(LineAbs(TextLineNr - j));
		}
		ScreenFirstLineNr = TextLineNr;
		RScrL = NewRL(ScreenFirstLineNr);
	}
	else {
		DekFindLine(LineAbs(TextLineNr) + PageS);
		if (TextLineNr >= ScreenFirstLineNr + PageS) {
			ScreenFirstLineNr += PageS;
		}
	}

	_change_scr = true;
	BlockUDShift(L1);
}

void TextEditor::ClrWord()
{
	// do nothing for non-help editor mode
}

void TextEditor::ProcessHelpMode()
{
	// do nothing for non-help editor mode
}

bool TextEditor::WordFind(WORD i, size_t& word_begin, size_t& word_end, size_t& line_nr)
{
	// do nothing for non-help editor mode
	return false;
}

void TextEditor::SetWord(size_t word_begin, size_t word_end)
{
	// do nothing for non-help editor mode
}

WORD TextEditor::WordNo2()
{
	// do nothing for non-help editor mode
	return 0;
}

size_t TextEditor::WordNo(size_t I)
{
	// do nothing for non-help editor mode
	return 0;
}

bool TextEditor::WordExist()
{
	// do nothing for non-help editor mode
	return false;
}

void TextEditor::HelpLU(char dir)
{
	// do nothing for non-help editor mode
}

void TextEditor::HelpRD(char dir)
{
	// do nothing for non-help editor mode
}

void TextEditor::direction(BYTE x, BYTE& zn2)
{
	BYTE y = 0x10;
	if (x > 2) { y = y << 1; }
	if (x == 0) { y = 0; }
	if (_mode == EditorMode::FrameDouble) {
		zn2 = zn2 | y;
	}
	else {
		zn2 = zn2 & !y;
	}
}

void TextEditor::MyWriteln()
{
	TextAttr = TxtColor;
	printf("\n");
}

void TextEditor::PreviousLine()
{
	//WORD mi, ml;
	if (UpdatedL) KodLine();
	//if ((TextLineNr == 1) && (Part.PosP > 0)) PredPart();
	if (TextLineNr > 1) {
		TextLineNr--;
		textIndex = GetLineStartIndex(TextLineNr);
		CopyCurrentLineToArr(textIndex);
		//if (_textT[textIndex - 1] == _LF) {
		//	size_t line 
		//	CopyCurrentLineToArr(textIndex - 3);
		//}
		//else {
		//	CopyCurrentLineToArr(textIndex - 2);
		//}
		//TextLineNr--;
		if (TextLineNr < ScreenFirstLineNr) {
			screen.GotoXY(1, 1);
			//MyInsLine();
			ScreenFirstLineNr--;
			_change_scr = true;
			if (bScroll) {
				/*dec(RLineL);*/
				RScrL--;
				/*if (ModPage(RLineL))*/
				if (ModPage(RScrL)) {
					screen.GotoXY(1, 1);
					//MyInsLine();/*dec(RLineL);*/
					RScrL--;
				}
			}
		}
	}
}

void TextEditor::NextLine(bool WrScr)
{
	if (UpdatedL) KodLine();

	if (TextLineNr < _lines.size()) {
		textIndex = NextLineStartIndex;
		TextLineNr++;
		DekodLine();
		if (bScroll) {
			if (PageS > 1) MyWriteln();
			ScreenFirstLineNr++;
			_change_scr = true;
			RScrL++;
			if (ModPage(RScrL)) {
				if (PageS > 1) MyWriteln();
				RScrL++;
			}
		}
		else if (WrScr && (TextLineNr == ScreenFirstLineNr + PageS)) {
			//if (PageS > 1) MyWriteln();
			ScreenFirstLineNr++;
			_change_scr = true;
		}
	}
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
//	BYTE dir, zn1, zn2, b;
//
//	UpdStatLine(TextLineNr, positionOnActualLine, Mode);
//	screen.CrsBig();
//	BYTE odir = 0;
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

void TextEditor::CleanFrame(std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys)
{
	//if (Mode == SinFM || Mode == DouFM || Mode == DelFM || Mode == NotFM) /* !!! with Event do!!! */
	//	if (!MyGetEvent(Mode, SysLColor, LastS, LastNr, IsWrScreen, bScroll, ExitD, breakKeys) ||
	//		((Event.What == evKeyDown) && (Event.Pressed.KeyCombination() == __ESC)) || (Event.What != evKeyDown))
	//	{
	//		ClrEvent();
	//		screen.CrsNorm();
	//		Mode = TextM;
	//		UpdStatLine(TextLineNr, positionOnActualLine, Mode);
	//		return;
	//	}
}

void TextEditor::FrameStep(BYTE& odir, PressedKey EvKeyC)
{
	std::string FrameString = "\x3F\x50\x48\xB3\x4D\xDA\xC0\xC3\x4B\xBF\xD9\xB4\xC4\xC2\xC1\xC5";
	//                                       │       ┌   └   ├       ┐   ┘   ┤   ─   ┬   ┴   ┼
	FrameString += "\x0F\x50\x48\xBA\x4D\xD6\xD3\xC7\x4B\xB7\xBD\xB6\xC4\xD2\xD0\xD7";
	//                                       ║       Í   Ë   ă       Ě   Ż   Â   ─   Ď   đ   Î
	FrameString += "\x0F\x50\x48\xB3\x4D\xD5\xD4\xC6\x4B\xB8\xBE\xB5\xCD\xD1\xCF\xD8";
	//							             │       Ň   ď   Ă       Ş   ż   Á   ═   Đ   ¤   ě
	FrameString += "\x0F\x50\x48\xBA\x4D\xC9\xC8\xCC\x4B\xBB\xBC\xB9\xCD\xCB\xCA\xCE";
	//                                       ║       ╔   ╚   ╠       ╗   ╝   ╣   ═   ╦   ╩   ╬

	switch (EvKeyC.KeyCombination()) {
	case '-': { _mode = EditorMode::FrameSingle; break; }
	case '=': { _mode = EditorMode::FrameDouble; break; }
	case '/': { _mode = EditorMode::DeleteFrame; break; }
	case ' ': { _mode = EditorMode::NotFrame; break; }
	case __ESC: {
		screen.CrsNorm();
		_mode = EditorMode::Text;
		break;
	}
	case __LEFT:
	case __RIGHT:
	case __UP:
	case __DOWN: {
		WORD scanCode = EvKeyC.Key()->wVirtualScanCode;
		size_t idx = FrameString.find_first_of(Arr[positionOnActualLine - 1]);
		BYTE zn1 = (idx == std::string::npos) ? 0 : BYTE(idx);
		BYTE zn2 = zn1 & 0x30;
		zn1 = zn1 & 0x0F;

		idx = FrameString.find_first_of(Lo(scanCode));
		BYTE dir = (idx == std::string::npos) ? 0 : BYTE(idx);
		auto dirodir = dir + odir;
		if (dirodir == 2 || dirodir == 4 || dirodir == 8 || dirodir == 16) {
			odir = 0;
		}
		if (zn1 == 1 || zn1 == 2 || zn1 == 4 || zn1 == 8) {
			zn1 = 0;
		}
		char oldzn = Arr[positionOnActualLine - 1];
		Arr[positionOnActualLine - 1] = ' ';
		BYTE b;
		if (_mode == EditorMode::DeleteFrame) {
			b = zn1 & !(odir | dir);
		}
		else {
			b = zn1 | (odir ^ dir);
		}
		if (b == 1 || b == 2 || b == 4 || b == 8) {
			b = 0;
		}
		if ((_mode == EditorMode::DeleteFrame) && (zn1 != 0) && (b == 0)) {
			oldzn = ' ';
		}
		direction(dir, zn2);
		direction(odir, zn2);
		if (_mode == EditorMode::NotFrame) {
			b = 0;
		}

		if ((b != 0) && ((Event.Pressed.KeyCombination() == __LEFT) || (Event.Pressed.KeyCombination() == __RIGHT) ||
			(Event.Pressed.KeyCombination() == __UP) || (Event.Pressed.KeyCombination() == __DOWN))) {
			Arr[positionOnActualLine - 1] = FrameString[zn2 + b];
		}
		else {
			Arr[positionOnActualLine - 1] = oldzn;
		}

		if ((dir == 1) || (dir == 4)) {
			odir = dir * 2;
		}
		else {
			odir = dir / 2;
		}

		if (_mode == EditorMode::NotFrame) odir = 0;
		else UpdatedL = true;

		switch (Event.Pressed.KeyCombination()) {
		case __LEFT: {
			if (positionOnActualLine > 1) positionOnActualLine--;
			break;
		}
		case __RIGHT: {
			if (positionOnActualLine < LineMaxSize) positionOnActualLine++;
			break;
		}
		case __UP: {
			PreviousLine();
			break;
		}
		case __DOWN: {
			NextLine(true);
			break;
		}
		default:;
		}
	}
			   break;
	}
	UpdStatLine(TextLineNr, positionOnActualLine);
}

void TextEditor::MoveB(WORD& B, WORD& F, WORD& T)
{
	if (F <= T) { if (B > F) B += T - F; }
	else if (B >= F) B -= F - T;
	else if (B > T) B = T; B = MinW(B, GetArrLineLength() + 1);
}

bool TextEditor::TestLastPos(WORD F, WORD T)
{
	WORD LP = GetArrLineLength();
	if (F > LP) F = LP + 1;

	if (LP + T - F <= LineMaxSize) {
		if (LP >= F) {
			memcpy(&Arr[T - 1], &Arr[F - 1], LP - F + 1);
		}

		if (TypeB == TextBlock) {
			if (LineAbs(TextLineNr) == BegBLn) {
				MoveB(BegBPos, F, T);
			}
			if (LineAbs(TextLineNr) == EndBLn) {
				MoveB(EndBPos, F, T);
			}
		}

		if (F > T) {
			if (T <= LP) {
				memset(&Arr[LP + T - F], ' ', F - T);
			}
		}

		UpdatedL = true;
		return true;
	}
	else {
		return false;
	}
}

void TextEditor::DelChar()
{
	WORD LP = 0;
	TestLastPos(positionOnActualLine + 1, positionOnActualLine);
}

void TextEditor::FillBlank()
{
	KodLine();
	WORD I = GetArrLineLength();
	if (positionOnActualLine > I + 1) {
		UpdatT = true;
		// TODO: memset(&_textT[textIndex + I], ' ', positionOnActualLine - I - 1);
		NextLineStartIndex += positionOnActualLine - I - 1;
	}
}

void TextEditor::DeleteLine()
{
	_change_scr = true;
	FillBlank();
	if (_lines.empty()) return;
	if (LineAbs(TextLineNr) + 1 <= BegBLn) {
		BegBLn--;
		if ((LineAbs(TextLineNr) == BegBLn) && (TypeB == TextBlock))
			BegBPos += GetArrLineLength();
	}
	if (LineAbs(TextLineNr) + 1 <= EndBLn) {
		EndBLn--;
		if ((LineAbs(TextLineNr) == EndBLn) && (TypeB == TextBlock))
			EndBPos += GetArrLineLength();
	}
	if (TextLineNr < _lines.size()) {
		// the line isn't the last one
		size_t EoL_length = HardL ? 2 : 1;

		if (Event.Pressed.Char == '\b') {
			// if BACKSPACE was pressed we will delete line below cursor (which was already moved)
			if (TextLineNr < 1) return;
			_lines[TextLineNr - 1] = _lines[TextLineNr - 1].substr(0, _lines[TextLineNr - 1].length() - EoL_length) + _lines[TextLineNr];
			_lines.erase(_lines.begin() + TextLineNr);
		}
		else {
			if (TextLineNr < 1 || TextLineNr == _lines.size()) {
				// cursor is on the last line, there is nothing more to move
				return;
			}
			else {
				_lines[TextLineNr - 1] = _lines[TextLineNr - 1].substr(0, _lines[TextLineNr - 1].length() - EoL_length) + _lines[TextLineNr];
				_lines.erase(_lines.begin() + TextLineNr);
			}
		}
	}
	else {
		// the line is the last one -> cannot be deleted
	}
	DekodLine();
}

void TextEditor::NewLine(char Mode)
{
	KodLine();
	WORD LP = textIndex + MinI(GetArrLineLength(), positionOnActualLine - 1);

	std::string EoL = HardL ? "\r\n" : "\r";

	_lines.insert(_lines.begin() + TextLineNr, "");

	//TestLenText(&_textT, _lenT, LP, LP + 2);
	UpdatT = true;
	_change_scr = true;

	// vse od aktualni pozice zkopirujeme na dalsi radek (nove vytvoreny)
	_lines[TextLineNr] = _lines[TextLineNr - 1].substr(positionOnActualLine - 1);
	// na puvodnim radku zustane vse pred pozici kurzoru a pridame ukonceni radku
	_lines[TextLineNr - 1] = _lines[TextLineNr - 1].substr(0, positionOnActualLine - 1) + EoL;

	if (LineAbs(TextLineNr) <= BegBLn) {
		if (LineAbs(TextLineNr) < BegBLn) {
			BegBLn++;
		}
		else if ((BegBPos > positionOnActualLine) && (TypeB == TextBlock)) {
			BegBLn++;
			BegBPos -= positionOnActualLine - 1;
		}
	}
	if (LineAbs(TextLineNr) <= EndBLn) {
		if (LineAbs(TextLineNr) < EndBLn) {
			EndBLn++;
		}
		else if ((EndBPos > positionOnActualLine) && (TypeB == TextBlock)) {
			EndBLn++;
			EndBPos -= positionOnActualLine - 1;
		}
	}

	if (Mode == 'm') {
		TextLineNr++;
		textIndex = LP + 1;
	}

	DekodLine();
}

WORD TextEditor::SetPredI()
{
	WORD result;
	std::string txt = JoinLines(_lines);

	if (textIndex <= 1) {
		result = textIndex;
	}
	else if (txt[textIndex - 1] == __LF) {
		result = CurrentLineFirstCharIndex(textIndex - 2);
	}
	else {
		result = CurrentLineFirstCharIndex(textIndex - 1);
	}

	return result;
}

void TextEditor::WrCharE(char Ch)
{
	if (Insert) {
		if (TestLastPos(positionOnActualLine, positionOnActualLine + 1)) {
			Arr[positionOnActualLine - 1] = Ch;
			if (positionOnActualLine < LineMaxSize) {
				positionOnActualLine++;
			}
		}
	}
	else {
		Arr[positionOnActualLine - 1] = Ch;
		UpdatedL = true;
		if (positionOnActualLine < LineMaxSize) {
			positionOnActualLine++;
		}
	}
}

void TextEditor::Format(WORD& i, int First, int Last, WORD Posit, bool Rep)
{
	size_t lst, ii1;
	short ii;
	char A[260];
	bool bBool;
	WORD rp, nb, nw, n;
	WORD RelPos;

	//SetPart(First);
	WORD fst = First; // -Part.PosP;
	int llst = Last; // -Part.PosP;

	std::string txt = JoinLines(_lines);

	if (llst > txt.length()) lst = txt.length();
	else lst = llst;

	do {
		if (txt.length() > 0x400) ii1 = txt.length() - 0x400;
		else ii1 = 0;

		i = fst; ii1 = i;
		if ((i < 2) || (txt[i - 1] == __LF)) {
			while (txt[ii1] == ' ') ii1++; Posit = MaxW(Posit, ii1 - i + 1);
		}
		ii1 = i; RelPos = 1;
		if (Posit > 1) {
			Move(&txt[i], A, Posit);
			for (ii = 1; ii <= Posit - 1; i++) {
				if (!CtrlKey.contains(txt[i])) RelPos++;
				if (txt[i] == __CR) A[ii] = ' ';
				else i++;
			}
			if ((txt[i] == ' ') && (A[Posit - 1] != ' ')) {
				Posit++; RelPos++;
			}
		}
		while (i < lst) {
			bBool = true; nw = 0; nb = 0;
			if (RelPos < LeftMarg)
				if ((Posit == 1) || (A[Posit - 1] == ' '))
				{
					ii = LeftMarg - RelPos; FillChar(&A[Posit], ii, 32);
					Posit += ii; RelPos = LeftMarg;
				}
				else while (RelPos < LeftMarg)
				{
					Posit++;
					if (!CtrlKey.contains(txt[i])) RelPos++;
					if (txt[i] != __CR) i++;
					if (txt[i] == __CR) A[Posit] = ' ';
					else A[Posit] = txt[i];
				}
			while ((RelPos <= RightMarg) && (i < lst)) {
				if ((txt[i] == __CR) || (txt[i] == ' ')) {
					while (((txt[i] == __CR) || (txt[i] == ' ')) && (i < lst))
						if (txt[i + 1] == __LF) lst = i;
						else { txt[i] = ' '; i++; }
					if (!bBool) { nw++; if (i < lst) i--; };
				}
				if (i < lst) {
					bBool = false;
					A[Posit] = txt[i];
					if (!CtrlKey.contains(A[Posit])) RelPos++;
					i++; Posit++;
				}
			}
			if ((i < lst) && (txt[i] != ' ') && (txt[i] != __CR)) {
				ii = Posit - 1;
				if (CtrlKey.contains(A[ii])) ii--;
				rp = RelPos; RelPos--;
				while ((A[ii] != ' ') && (ii > LeftMarg)) {
					if (!CtrlKey.contains(A[ii])) RelPos--; ii--;
				}
				if (RelPos > LeftMarg) {
					nb = rp - RelPos;
					i -= (Posit - ii);
					Posit = ii;
				}
				else
				{
					while ((txt[i] != ' ') && (txt[i] != __CR) && (Posit < LineMaxSize)) {
						A[Posit] = txt[i]; i++; Posit++;
					}
					while (((txt[i] == __CR) || (txt[i] == ' ')) && (i < lst)) {
						if (txt[i + 1] == __LF) {
							lst = i;
						}
						else {
							txt[i] = ' ';
							i++;
						}
					}
				}
			}
			if (Just)
			{
				ii = LeftMarg;
				while ((nb > 0) and (nw > 1))
				{
					while (A[ii] == ' ') ii++;
					while (A[ii] != ' ') ii++;
					nw--; n = nb / nw;
					if ((nw % nb != 0) && (nw % 2) && (nb > n)) n++;
					if (Posit - ii > 0) Move(&A[ii], &A[ii + n], Posit - ii + 1);
					FillChar(&A[ii], n, 32); Posit += n;
					nb -= n;
				}
			}
			ii = 1;
			while (A[ii] == ' ') ii++;
			if (ii >= Posit) Posit = 1;
			if (i < lst) A[Posit] = __CR; else Posit--;
			//TestLenText(&_textT, _lenT, i, int(ii1) + Posit);
			UpdatT = true;
			if (Posit > 0) Move(A, &txt[ii1], Posit);
			ii = ii1 + Posit - i; i = ii1 + Posit; lst += ii; llst += ii;
			Posit = 1; RelPos = 1; ii1 = i;
		}
		if (Rep)
		{
			while ((txt[i] == __CR) || (txt[i] == __LF)) i++; fst = i; Rep = i < llst;
			if (llst > txt.length()) lst = txt.length(); else lst = llst;
		}
	} while (Rep);

	_lines = GetAllLinesWithEnds(txt, HardL);
	BegBLn = 1; BegBPos = 1; EndBLn = 1; EndBPos = 1; TypeB = TextBlock;
}

void TextEditor::Calculate()
{
	wwmix ww;
	FrmlElem* Z = nullptr;
	std::string txt;
	size_t I;
	std::string Msg;
	void* p = nullptr;
	char FTyp;
	double R;
	bool Del;
	MarkStore(p);
	//NewExit(Ovr(), er);
	//goto label2;
	try {
		gc->ResetCompilePars();
		//ptrRdFldNameFrml = RdFldNameFrmlT;
		gc->rdFldNameType = FieldNameType::T;
	label0:
		txt = CalcTxt;
		Del = true; I = 1;
	label1:
		ww.PromptLL(114, txt, I, Del, true, true);
		if (Event.Pressed.KeyCombination() == 'U') goto label0;
		if (Event.Pressed.KeyCombination() == __ESC || txt.empty()) goto label3;
		CalcTxt = txt;
		if (Event.Pressed.KeyCombination() == __CTRL_F4 && _mode == EditorMode::Text && !bScroll) {
			if (txt.length() > LineMaxSize - GetArrLineLength()) {
				I = LineMaxSize - GetArrLineLength();
				WrLLF10Msg(419);
				goto label1;
			}
			if (positionOnActualLine <= GetArrLineLength()) {
				TestLastPos(positionOnActualLine, positionOnActualLine + txt.length());
			}
			memcpy(&Arr[positionOnActualLine], txt.c_str(), txt.length());
			UpdatedL = true;
			goto label3;
		}
		gc->SetInpStr(txt);
		gc->RdLex();
		Z = gc->RdFrml(FTyp, nullptr);
		if (gc->Lexem != 0x1A) gc->Error(21);

		switch (FTyp) {
		case 'R': {
			R = RunReal(CFile, Z, CRecPtr);
			str(R, 30, 10, txt);
			txt = LeadChar(' ', TrailChar(txt, '0'));
			if (txt[txt.length() - 1] == '.') {
				txt = txt.substr(0, txt.length() - 1);
			}
			break;
		}
		case 'S': {
			/* wie RdMode fuer _textT ??*/
			txt = RunString(CFile, Z, CRecPtr);
			break;
		}
		case 'B': {
			if (RunBool(CFile, Z, CRecPtr)) txt = AbbrYes;
			else txt = AbbrNo;
			break;
		}
		}
		I = 1;
		goto label1;
	}
	catch (std::exception& e) {
		//label2:
		Msg = MsgLine;
		I = gc->input_pos;
		SetMsgPar(Msg);
		WrLLF10Msg(110);
		IsCompileErr = false;
		ReleaseStore(&p);
		Del = false;
		// TODO: goto label1;
	}
label3:
	ReleaseStore(&p);
}

bool TextEditor::BlockExist()
{
	if (TypeB == TextBlock)
		return (BegBLn < EndBLn) || (BegBLn == EndBLn) && (BegBPos < EndBPos);
	return (BegBLn <= EndBLn) && (BegBPos < EndBPos);
}

void TextEditor::SetBlockBound(int& BBPos, int& EBPos)
{
	SetPartLine(EndBLn);
	short i = EndBLn; // -Part.LineP;
	size_t nextLineIdx = GetLineStartIndex(i);
	EBPos = SetInd(nextLineIdx, EndBPos); // +Part.PosP;
	SetPartLine(BegBLn);
	i = BegBLn; // -Part.LineP;
	nextLineIdx = GetLineStartIndex(i);
	BBPos = SetInd(nextLineIdx, BegBPos); // +Part.PosP;
}

void TextEditor::ResetPrint(TextEditor* editor, char Oper, int& fs, HANDLE W1, int LenPrint, ColorOrd* co, WORD& I1, bool isPrintFile, char* p)
{
	//*co = Part.ColorP;
	*co = SetColorOrd(I1 - 1); // TODO: fix this
	isPrintFile = false;
	fs = co->length();
	LenPrint += fs;
	if (Oper == 'p') LenPrint++;
	if ((MemoryAvailable() > LenPrint) && (LenPrint < 0xFFF0)) {
		char* t = new char[LenPrint];
		p = t;
		Move(&co[1], p, co->length());
	}
	else {
		isPrintFile = true;
		W1 = WorkHandle;
		SeekH(W1, 0);
		WriteH(W1, co->length(), &co[1]);
		HMsgExit(CPath);
	}
}

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

bool TextEditor::BlockHandle(int& fs, HANDLE W1, char Oper)
{
	WORD i, I1;
	int LL1, LL2;
	ColorOrd co;
	bool isPrintFile = false;
	char* p = nullptr;
	bool tb; char c = '\0';

	if (UpdatedL) KodLine();
	int Ln = LineAbs(TextLineNr);
	WORD Ps = positionOnActualLine;
	if (Oper == 'p') {
		tb = TypeB;
		TypeB = TextBlock;
	}
	else if (!BlockExist()) {
		return false;
	}
	screen.CrsHide();
	auto result = true;
	if (TypeB == TextBlock) {
		WORD I2;
		std::string txt = JoinLines(_lines);
		if (Oper == 'p') {
			LL2 = AbsLenT + txt.length(); // -Part.LenP;
			LL1 = SetInd(textIndex, positionOnActualLine); // +Part.PosP;
		}
		else {
			SetBlockBound(LL1, LL2);
		}
		I1 = LL1; // -Part.PosP;
		if (toupper(Oper) == 'P') {
			ResetPrint(this, Oper, fs, W1, LL2 - LL1, &co, I1, isPrintFile, p);
		}
		do {
			if (LL2 > /*Part.PosP +*/ txt.length()) I2 = txt.length();
			else I2 = LL2; // -Part.PosP;
			switch (Oper) {
			case 'Y': {
				//TestLenText(&_textT, _lenT, I2, I1);
				UpdatT = true;
				LL2 -= I2 - I1;
				break;
			}
			case 'U': {
				for (i = I1; i <= I2 - 1; i++) txt[i] = UpcCharTab[txt[i]];
				LL1 += I2 - I1;
				break;
			}
			case 'L': {
				for (i = I1; i <= I2 - 1; i++) LowCase(txt[i]);
				LL1 += I2 - I1;
				break;
			}
			case 'p':
			case 'P': {
				if (isPrintFile) {
					WriteH(W1, I2 - I1, &txt[I1]);
					HMsgExit(CPath);
				}
				else {
					Move(&txt[I1], &p[fs + 1], I2 - I1);
					fs += I2 - I1; LL1 += I2 - I1;
				}
				break;
			}
			case 'W': {
				SeekH(W1, fs);
				WriteH(W1, I2 - I1, &txt[I1]);
				HMsgExit(CPath);
				fs += I2 - I1;
				LL1 += I2 - I1;
				break;
			}
			}
			if (Oper == 'U' || Oper == 'L' || Oper == 'Y') {
				//SetUpdat();
				UpdatT = true;
			}
			if ((Oper == 'p') /* && AllRd*/) LL1 = LL2;
			//if (!AllRd && (LL1 < LL2))
			//{
			//	I1 = _lenT;
			//	NextPart();
			//	//I1 -= Part.MovI;
			//}
		} while (LL1 != LL2);
		_lines = GetAllLinesWithEnds(txt, HardL);
	}
	else              /*ColBlock*/
	{
		PosDekFindLine(BegBLn, BegBPos, false);
		I1 = EndBPos - BegBPos;
		LL1 = (EndBLn - BegBLn + 1) * (I1 + 2);
		LL2 = 0;
		if (Oper == 'P') ResetPrint(this, Oper, fs, W1, LL1, &co, I1, isPrintFile, p);
		do {
			switch (Oper) {
			case 'Y': { TestLastPos(EndBPos, BegBPos); break; }
			case 'U': {
				for (i = BegBPos; i <= EndBPos - 1; i++) {
					Arr[i] = UpcCharTab[Arr[i]];
				}
				UpdatedL = true;
				break;
			}
			case 'L': {
				for (i = BegBPos; i <= EndBPos - 1; i++) {
					LowCase(Arr[i]);
				}
				UpdatedL = true;
				break;
			}
			case 'W':
			case 'P': {
				char* a = nullptr;
				Move(&Arr[BegBPos], a, I1);
				a[I1 + 1] = __CR;
				a[I1 + 2] = __LF;
				if ((Oper == 'P') && !isPrintFile) {
					Move(a, &p[fs + 1], I1 + 2);
				}
				else {
					WriteH(W1, I1 + 2, a);
					HMsgExit(CPath);
				}
				fs += I1 + 2;
				break;
			}
			}
			LL2 += I1 + 2;
			NextLine(false);
		} while (LL2 != LL1);

	}
	if (toupper(Oper) == 'P') {
		if (isPrintFile) {
			std::string txt = JoinLines(_lines);
			WriteH(W1, 0, txt.c_str());/*truncH*/
			PrintFandWork();
		}
		else {
			PrintArray(p, fs, false);
			delete[] p; p = nullptr;
		}
	}
	if (Oper == 'p') { TypeB = tb; }
	if (Oper == 'Y') { PosDekFindLine(BegBLn, BegBPos, true); }
	else {
		if (Oper == 'p') {
			//SetPart(1);
		}
		PosDekFindLine(Ln, Ps, true);
	}
	if (!bScroll) screen.CrsShow();
	return result;
}

void DelStorClpBd(void* P1, LongStr* sp)
{
	TWork.Delete(ClpBdPos);
	std::string data = std::string(sp->A, sp->LL);
	ClpBdPos = TWork.Store(data);
	ReleaseStore(&P1);
}

void MarkRdClpBd(void* P1, LongStr* sp)
{
	MarkStore(P1);
	//sp = TWork.ReadLongStr(ClpBdPos);
}

bool TextEditor::BlockGrasp(char Oper, void* P1, LongStr* sp)
{
	int L, L1, L2, ln;
	WORD I1;
	auto result = false;
	if (!BlockExist()) return result;
	L = /*Part.PosP +*/ textIndex + positionOnActualLine - 1;
	ln = LineAbs(TextLineNr);
	if (Oper == 'G' && UpdatedL) KodLine();
	SetBlockBound(L1, L2);
	if ((L > L1) and (L < L2) and (Oper != 'G')) return result;
	L = L2 - L1; if (L > 0x7FFF) { WrLLF10Msg(418); return result; }
	//if (L2 > /*Part.PosP +*/ _lenT) MovePart(L1 /* - Part.PosP*/);
	I1 = L1 /* - Part.PosP*/;
	MarkStore(P1);
	sp = new LongStr(L + 2);
	sp->LL = L;
	//Move(&_textT[I1], sp->A, L);
	if (Oper == 'M') {
		//TestLenText(&_textT, _lenT, I1 + L, I1);
		UpdatT = true;
		/*   if (L1>Part.PosP+I1) dec(L1,L);*/
		if (EndBLn <= ln)
		{
			if ((EndBLn == ln) && (positionOnActualLine >= EndBPos))
				positionOnActualLine = BegBPos + positionOnActualLine - EndBPos;
			ln -= EndBLn - BegBLn;
		}
	}
	if (Oper == 'G') DelStorClpBd(P1, sp);
	PosDekFindLine(ln, positionOnActualLine, false);
	result = true;
	return result;
}

void TextEditor::BlockDrop(char Oper, void* P1, LongStr* sp)
{
	WORD I, I2;
	if (Oper == 'D') MarkRdClpBd(P1, sp);
	if (sp->LL == 0) return;
	/* hlidani sp->LL a StoreAvail, MaxLenT, dela TestLenText, prip.SmallerPart */
	if (Oper == 'D') FillBlank();
	I = textIndex + positionOnActualLine - 1; I2 = sp->LL;
	BegBLn = LineAbs(TextLineNr);
	BegBPos = positionOnActualLine;
	//NullChangePart();
	//TestLenText(&_textT, _lenT, I, int(I) + I2);
	UpdatT = true;
	//if (ChangePart) I -= Part.MovI;
	//Move(sp->A, &_textT[I], I2);
	ReleaseStore(&P1);
	TextLineNr = GetLineNumber(I + I2);
	EndBLn = /*Part.LineP +*/ TextLineNr;
	EndBPos = succ(I + I2 - textIndex);
	PosDekFindLine(BegBLn, BegBPos, true); /*ChangeScr = true;*/
}

bool TextEditor::BlockCGrasp(char Oper, void* P1, LongStr* sp)
{
	WORD i, I2;
	int L;
	char* a = nullptr;

	auto result = false;
	if (!BlockExist()) return result;
	if (UpdatedL) KodLine();
	L = LineAbs(TextLineNr);
	if ((L >= BegBLn && L <= EndBLn) && (positionOnActualLine >= BegBPos + 1 && positionOnActualLine <= EndBPos - 1) && (Oper != 'G')) return result;
	int l1 = (EndBLn - BegBLn + 1) * (EndBPos - BegBPos + 2);
	if (l1 > 0x7FFF) { WrLLF10Msg(418); return result; }
	MarkStore(P1);
	sp = new LongStr(l1 + 2);
	sp->LL = l1;
	PosDekFindLine(BegBLn, positionOnActualLine, false);
	I2 = 0;
	i = EndBPos - BegBPos;
	do {
		Move(&Arr[BegBPos], a, i); a[i + 1] = __CR; a[i + 2] = __LF;
		if (Oper == 'M') TestLastPos(EndBPos, BegBPos);
		Move(a, &sp->A[I2 + 1], i + 2); I2 += i + 2;
		if (UpdatedL) KodLine();
		NextLine(false);
	} while (I2 != sp->LL);
	if ((Oper == 'M') && (L >= BegBLn && L <= EndBLn) && (positionOnActualLine > EndBPos)) {
		positionOnActualLine -= EndBPos - BegBPos;
	}
	if (Oper == 'G') DelStorClpBd(P1, sp); PosDekFindLine(L, positionOnActualLine, false);
	result = true;
	return result;
}

void TextEditor::InsertLine(WORD& i, WORD& I1, WORD& I3, WORD& ww, LongStr* sp)
{
	i = MinW(I1 - I3, LineMaxSize - GetArrLineLength());
	if (i > 0) {
		TestLastPos(ww, ww + i);
		Move(&sp->A[I3], &Arr[ww], i);
	}
	if (UpdatedL) KodLine();
}

void TextEditor::BlockCDrop(char Oper, void* P1, LongStr* sp)
{
	WORD i, I1, I3, ww;
	if (Oper == 'D') MarkRdClpBd(P1, sp);
	if (sp->LL == 0) return;
	/* hlidani sp->LL a StoreAvail, MaxLenT
		dela NextLine - prechazi mezi segmenty */
	if (Oper != 'R') {
		EndBPos = positionOnActualLine;
		BegBPos = positionOnActualLine;
		BegBLn = TextLineNr /* + Part.LineP*/;
	}
	ww = BegBPos; I1 = 1; I3 = 1;
	do {
		if (sp->A[I1] == __CR) {
			InsertLine(i, I1, I3, ww, sp);
			ww = BegBPos;
			EndBPos = MaxW(ww + i, EndBPos);
			std::string txt = JoinLines(_lines);
			if ((NextLineStartIndex > txt.length()) && ((_text_type != TextType::File) || true /*AllRd*/)) {
				//TestLenText(&_textT, _lenT, _lenT, (int)_lenT + 2);
				UpdatT = true;
				txt[txt.length() - 2] = __CR;
				txt[txt.length() - 1] = __LF;
				NextLineStartIndex = txt.length();
				_lines = GetAllLinesWithEnds(txt, HardL);
			}
			NextLine(false);
		}
		if (sp->A[I1] == __CR || sp->A[I1] == __LF || sp->A[I1] == 0x1A) I3 = I1 + 1;
		I1++;
	} while (I1 <= sp->LL);
	if (I3 < I1) InsertLine(i, I1, I3, ww, sp);
	if (Oper != 'R') {
		EndBLn = /*Part.LineP +*/ TextLineNr - 1;
		ReleaseStore(&P1);
		PosDekFindLine(BegBLn, BegBPos, true);
	}
}

void TextEditor::BlockCopyMove(char Oper, void* P1, LongStr* sp)
{
	bool b = false;
	if (!BlockExist()) return;
	FillBlank();
	if (TypeB == TextBlock) {
		if (BlockGrasp(Oper, P1, sp)) { BlockDrop(Oper, P1, sp); }
	}
	else if (BlockCGrasp(Oper, P1, sp)) { BlockCDrop(Oper, P1, sp); }
}

bool TextEditor::ColBlockExist()
{
	bool b = false;
	if ((TypeB == ColBlock) && (BegBPos == EndBPos) && (BegBLn < EndBLn)) return true;
	else return BlockExist();
}

void TextEditor::NewBlock1(WORD& I1, int& L2)
{
	if (I1 != positionOnActualLine) {
		BegBLn = L2;
		EndBLn = L2;
		BegBPos = MinW(I1, positionOnActualLine);
		EndBPos = MaxW(I1, positionOnActualLine);
	}
}

void TextEditor::BlockLRShift(WORD I1)
{
	if (!bScroll && (_mode != EditorMode::Help) && ((KbdFlgs & 0x03) != 0))   /*Shift*/
	{
		int L2 = LineAbs(TextLineNr);
		if (!ColBlockExist()) NewBlock1(I1, L2);
		else
			switch (TypeB) {
			case TextBlock: {
				if ((BegBLn == EndBLn) && (L2 == BegBLn) && (EndBPos == BegBPos)
					&& (I1 == BegBPos)) if (I1 > positionOnActualLine) BegBPos = positionOnActualLine;
					else EndBPos = positionOnActualLine;
				else if ((L2 == BegBLn) && (I1 == BegBPos)) BegBPos = positionOnActualLine;
				else if ((L2 == EndBLn) && (I1 == EndBPos)) EndBPos = positionOnActualLine;
				else NewBlock1(I1, L2);
				break;
			}
			case ColBlock: {
				if ((L2 >= BegBLn) && (L2 <= EndBLn))
					if ((EndBPos == BegBPos) && (I1 == BegBPos))
						if (I1 > positionOnActualLine) BegBPos = positionOnActualLine;
						else EndBPos = positionOnActualLine;
					else if (I1 == BegBPos) BegBPos = positionOnActualLine;
					else if (I1 == EndBPos) EndBPos = positionOnActualLine;
					else NewBlock1(I1, L2);
				else NewBlock1(I1, L2);
				break;
			}
			}
	}
}

void TextEditor::NewBlock2(int& L1, int& L2)
{
	if (L1 != L2) {
		BegBPos = positionOnActualLine; EndBPos = positionOnActualLine;
		BegBLn = MinL(L1, L2); EndBLn = MaxL(L1, L2);
	}
}

void TextEditor::BlockUDShift(int L1)
{
	int L2;
	if (!bScroll && (_mode != EditorMode::Help) && ((KbdFlgs & 0x03) != 0))   /*Shift*/
	{
		L2 = LineAbs(TextLineNr);
		if (!ColBlockExist()) NewBlock2(L1, L2);
		else {
			switch (TypeB) {
			case TextBlock: {
				if ((BegBLn == EndBLn) && (L1 == BegBLn))
					if ((positionOnActualLine >= BegBPos) && (positionOnActualLine <= EndBPos))
						if (L1 < L2) { EndBLn = L2; EndBPos = positionOnActualLine; }
						else { BegBLn = L2; BegBPos = positionOnActualLine; }
					else NewBlock2(L1, L2);
				else if ((L1 == BegBLn) && (BegBPos == positionOnActualLine)) BegBLn = L2;
				else if ((L1 == EndBLn) && (EndBPos == positionOnActualLine)) EndBLn = L2;
				else NewBlock2(L1, L2);
				break;
			}
			case ColBlock:
				if ((positionOnActualLine >= BegBPos) && (positionOnActualLine <= EndBPos))
					if ((BegBLn == EndBLn) && (L1 == BegBLn))
						if (L1 < L2) EndBLn = L2;
						else BegBLn = L2;
					else if (L1 == BegBLn) BegBLn = L2;
					else if (L1 == EndBLn) EndBLn = L2;
					else NewBlock2(L1, L2);
				else NewBlock2(L1, L2);
			}
		}
	}
}

bool MyPromptLL(WORD n, std::string& s)
{
	wwmix ww;
	ww.PromptLL(n, s, 1, true, false, false);
	return Event.Pressed.KeyCombination() == __ESC;
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

void TextEditor::SetScreen(WORD Ind, WORD ScrXY, WORD Pos)
{
	TextLineNr = 1; // TODO: GetLineNumber(Ind);
	DekodLine();
	positionOnActualLine = MinI(LineMaxSize, MaxI(MaxW(1, Pos), Ind - textIndex + 1));
	if (ScrXY > 0) {
		ScreenFirstLineNr = TextLineNr - (ScrXY >> 8) + 1;
		positionOnActualLine = MaxW(positionOnActualLine, ScrXY & 0x00FF);
		BPos = positionOnActualLine - (ScrXY & 0x00FF);
		_change_scr = true;
	}
	Colu = Column(positionOnActualLine);
	columnOffset = Column(BPos);
	if (bScroll) {
		RScrL = NewRL(ScreenFirstLineNr);
		TextLineNr = MaxI(PHNum + 1, LineAbs(TextLineNr)); // -Part.LineP;
		int rl = NewRL(TextLineNr);
		if ((rl >= RScrL + PageS) || (rl < RScrL)) {
			if (rl > 10) RScrL = rl - 10;
			else RScrL = 1;
			_change_scr = true; ScreenFirstLineNr = NewL(RScrL);
		}
		TextLineNr = ScreenFirstLineNr;
		DekFindLine(LineAbs(TextLineNr));
	}
	else {
		if ((TextLineNr >= ScreenFirstLineNr + PageS) || (TextLineNr < ScreenFirstLineNr)) {
			if (TextLineNr > 10) ScreenFirstLineNr = TextLineNr - 10;
			else ScreenFirstLineNr = 1;
			_change_scr = true;
		}
	}
}

void TextEditor::ReplaceString(WORD& J, WORD& fst, WORD& lst, int& Last)
{
	size_t r = ReplaceStr.length();
	size_t f = FindStr.length();
	//TestLenText(&_textT, _lenT, J, int(J) + r - f);
	UpdatT = true;
	//ChangeP(fst);
	//if (TestLastPos(positionOnActualLine, positionOnActualLine + r - f));
	//if (!ReplaceStr.empty()) Move(&ReplaceStr[1], &_textT[J - f], r);
	J += r - f;
	SetScreen(J, 0, 0);
	lst += r - f;
	Last += r - f;
}

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

void TextEditor::FindReplaceString(int First, int Last)
{
	WORD lst;
	if (First >= Last) {
		if ((_text_type == TextType::Memo) && TestOptStr('e')) {
			SrchT = true;
			Konec = true;
		}
		return;
	}
	FirstEvent = false;
	//SetPart(First);
	WORD fst = First;
	//NullChangePart();
label1:
	size_t len = GetTotalLength(_lines);
	if (Last > len) lst = len - 1;
	else lst = Last;
	//ChangeP(fst);            /* Background muze volat NextPart */
	if (FindString(fst, lst)) {
		SetScreen(fst, 0, 0);
		if (Replace) {
			if (TestOptStr('n')) {
				ReplaceString(fst, fst, lst, Last);
				UpdStatLine(TextLineNr, positionOnActualLine);/*BackGround*/
			}
			else {
				FirstEvent = true;
				Background();
				FirstEvent = false;
				char c = MyVerifyLL(408, "");
				if (c == AbbrYes) ReplaceString(fst, fst, lst, Last);
				else if (c == __ESC) return;
			}
			if (TestOptStr('g') || TestOptStr('e') || TestOptStr('l')) goto label1;
		}
	}
	else {                       /* !FindString */
		//if (!AllRd && (Last > Part.PosP + _lenT)) {
		//	NextPart();
		//	goto label1;
		//}
		//else {
		if (TestOptStr('e') && (_text_type == TextType::Memo)) {
			SrchT = true; Konec = true;
		}
		else {
			SetScreen(lst, 0, 0);
		}
		//}
	}
	/* BackGround; */
}

void TextEditor::CursorWord()
{
	std::set<char> O;
	std::string word;

	WORD pp = positionOnActualLine;

	if (_mode == EditorMode::Help) {
		O.insert(0x11);
	}
	else {
		O = Separ;
		if (O.count(Arr[pp - 1]) > 0) { pp--; }
	}

	while ((pp > 0) && !O.count(Arr[pp - 1])) { pp--; }

	pp++;

	while ((pp <= GetArrLineLength()) && !O.count(Arr[pp - 1])) {
		word += Arr[pp - 1];
		pp++;
	}

	gc->LexWord = word;
}

void TextEditor::Edit(std::string& text, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys)
{
	//blocks = new Blocks();
	InitScr();
	IsWrScreen = false;
	//WrEndT();
	IndexT = min(IndexT, text.length() - 1);
	BegBLn = 1;
	EndBLn = 1;
	BegBPos = 1;
	EndBPos = 1;
	ScreenFirstLineNr = 1;
	//ScreenIndex = 0;
	RScrL = 1;
	PredScLn = 1;
	PredScPos = 1;
	UpdPHead = false;
	if (_text_type != TextType::File) {
		AbsLenT = text.length() - 1;
		SimplePrintHead();
	}

	FirstScroll = _mode == EditorMode::View;
	const bool keybScroll = GetKeyState(VK_SCROLL) & 0x0001;
	bScroll = (keybScroll || FirstScroll) && (_mode != EditorMode::Help);
	if (bScroll)
	{
		ScreenFirstLineNr = NewL(RScrL);
		_change_scr = true;
	}
	HelpScroll = bScroll || (_mode == EditorMode::Help);
	if (HelpScroll) {
		screen.CrsHide();
	}
	else {
		screen.CrsNorm();
	}
	columnOffset = 0;
	BPos = 0;
	SetScreen(IndexT, ScrT, positionOnActualLine);
	Konec = false;

	if (_mode == EditorMode::Help) {
		ProcessHelpMode();
	}

	FillChar((char*)MargLL, sizeof(MargLL), 0);
	//ColScr = Part.ColorP;
	WrStatusLine();
	TextAttr = TxtColor;
	ClrScr(TextAttr);
	Background();
	FirstEvent = false;

	// {!!!!!!!!!!!!!!}
	if (!ErrMsg.empty()) {
		SetMsgPar(ErrMsg);
		F10SpecKey = 0xffff;
		WrLLF10Msg(110);
		ClearKbdBuf();
		AddToKbdBuf(Event.Pressed.KeyCombination());
	}
	FillChar((char*)MargLL, sizeof(MargLL), 0);
	WrLLMargMsg(LastS, LastNr);

	do {
		//if (TypeT == FileT) {
		//	NullChangePart();
		//}
		_events->HandleEvent(this, _mode, IsWrScreen, SysLColor, LastS, LastNr, ExitD, breakKeys);
		if (!(Konec || IsWrScreen)) {
			Background();
		}
	} while (!Konec);

	if (bScroll && (_mode != EditorMode::Help)) {
		positionOnActualLine = BPos + 1;
		TextLineNr = ScreenFirstLineNr;
		//textIndex = ScreenIndex;
	}

	IndexT = SetInd(textIndex, positionOnActualLine);
	ScrT = ((TextLineNr - ScreenFirstLineNr + 1) << 8) + positionOnActualLine - BPos;

	if (_mode != EditorMode::Help) {
		TxtXY = ScrT + ((int)positionOnActualLine << 16);
	}

	CursorWord();

	if (_mode == EditorMode::Help) {
		ClrWord();
	}

	screen.CrsHide();
	screen.Window(MinC, MinR, MaxC, MaxR);
	TestUpdFile();

	//delete blocks;
	//blocks = nullptr;
}

void TextEditor::SetEditTxt(Instr_setedittxt* PD)
{
	if (PD->Insert != nullptr) Insert = !RunBool(CFile, PD->Insert, CRecPtr);
	if (PD->Indent != nullptr) Indent = RunBool(CFile, PD->Indent, CRecPtr);
	if (PD->Wrap != nullptr) Wrap = RunBool(CFile, PD->Wrap, CRecPtr);
	if (PD->Just != nullptr) Just = RunBool(CFile, PD->Just, CRecPtr);
	if (PD->ColBlk != nullptr) TypeB = RunBool(CFile, PD->ColBlk, CRecPtr);
	if (PD->Left != nullptr) LeftMarg = MaxI(1, RunInt(CFile, PD->Left, CRecPtr));
	if (PD->Right != nullptr) RightMarg = MaxI(LeftMarg, MinI(255, RunInt(CFile, PD->Right, CRecPtr)));
}

void TextEditor::GetEditTxt(bool& pInsert, bool& pIndent, bool& pWrap, bool& pJust, bool& pColBlk, short& pLeftMarg, short& pRightMarg)
{
	pInsert = Insert; pIndent = Indent; pWrap = Wrap; pJust = Just; pColBlk = TypeB;
	pLeftMarg = LeftMarg; pRightMarg = RightMarg;
}

TextEditor::TextEditor(EditorMode e_mode, TextType text_type)
{
	InitTxtEditor();
	_mode = e_mode;
	_text_type = text_type;
	this->_events = new TextEditorEvents();
	this->_screen = new TextEditorScreen(this, TXTCOLS, CtrlKey);
}

TextEditor::~TextEditor()
{
	delete this->_events;
	this->_events = nullptr;

	delete this->_screen;
	this->_screen = nullptr;
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
	FirstEvent = !SrchT;
	if (SrchT) {
		SrchT = false;
		keyboard.AddToFrontKeyBuf(0x0C); // '^L' .. '\f' .. #12
		/*pstring OldKbdBuffer = KbdBuffer;
		KbdBuffer = 0x0C;
		KbdBuffer += OldKbdBuffer;*/
		Event.Pressed.UpdateKey('L');
		IndexT = 0;
	}

	Edit(pErrMsg, pExD, break_keys);
	if (_mode != EditorMode::Help) { TextAttr = TxtColor; }
	pUpdat = UpdatT;
	pSrch = SrchT;

	text = JoinLines(_lines);

	pInd = IndexT;
	pScr = ScrT + ((int)positionOnActualLine << 16);
	EdOk = oldEdOK;
	return EditT;
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

WORD TextEditor::FindTextE(const pstring& Pstr, pstring Popt, char* PTxtPtr, WORD PLen)
{
	return 0;
	//auto* origT = _textT;
	//_textT = (char*)PTxtPtr;
	//pstring f = FindStr;
	//pstring o = OptionStr;
	//bool r = Replace;
	//FindStr = Pstr;
	//OptionStr = Popt;
	//Replace = false;
	//WORD I = 1;
	//WORD result;
	//if (FindString(I, PLen + 1)) result = I;
	//else result = 0;
	//FindStr = f;
	//OptionStr = o;
	//Replace = r;
	//_textT = origT;
	//return result;
}

void TextEditor::EditTxtFile(std::string* locVar, EditorMode e_mode, std::string& ErrMsg, std::vector<EdExitD*>& ExD,
	int TxtPos, int Txtxy, WRect* V, WORD Atr, const std::string Hd, BYTE WFlags, MsgStr* MsgS)
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

void TextEditor::InitHelpViewEditor()
{
	TxtColor = screen.colors.hNorm;
	FillChar(ColKey, 8, screen.colors.tCtrl);
	ColKey[5] = screen.colors.hSpec;
	ColKey[3] = screen.colors.hHili;
	ColKey[1] = screen.colors.hMenu;
}

size_t TextEditor::LineAbs(int Ln)
{
	size_t result = Ln; // Part.LineP + Ln;
	if (Ln < 1) {
		result = 1;
	}
	return result;
}

bool TextEditor::LineInBlock(int Ln)
{
	if ((LineAbs(Ln) > BegBLn) && (LineAbs(Ln) < EndBLn)) {
		return true;
	}
	else {
		return false;
	}
}

bool TextEditor::LineBndBlock(int Ln)
{
	if ((LineAbs(Ln) == BegBLn) || (LineAbs(Ln) == EndBLn)) {
		return true;
	}
	else {
		return false;
	}
}