#include "HelpViewer.h"
#include "../Drivers/constants.h"
#include "../Common/textfunc.h"

HelpViewer::HelpViewer(std::string help_text, size_t text_pos) : TextEditor(EditorMode::Help, TextType::Memo)
{
	InitHelpViewEditor(help_text, text_pos);
}

HelpViewer::~HelpViewer()
{
}

void HelpViewer::InitHelpViewEditor(std::string& help_text, size_t& text_pos)
{
	bool Srch = false;
	bool Upd = false;
	int Scr = 0;

	TxtColor = screen.colors.hNorm;
	FillChar(ColKey, 8, screen.colors.tCtrl);
	ColKey[5] = screen.colors.hSpec;
	ColKey[3] = screen.colors.hHili;
	ColKey[1] = screen.colors.hMenu;

	std::vector<WORD> brkKeys;
	brkKeys.push_back(__F1);
	brkKeys.push_back(__F6);
	brkKeys.push_back(__F10);
	brkKeys.push_back(__CTRL_HOME);
	brkKeys.push_back(__CTRL_END);
	std::vector<EdExitD*> emptyExitD;

	EditText(EditorMode::Help, TextType::Memo, "", "", help_text, 0xFFF0, text_pos, Scr,
		brkKeys, emptyExitD, Srch, Upd, 142, 145, nullptr);
}

void HelpViewer::Background()
{
	UpdStatLine(TextLineNr, positionOnActualLine);

	WORD p = positionOnActualLine;

	if (_word.start_line == TextLineNr) {
		while (Arr[p] != 0x11) {
			p++;
		}
	}

	if (Column(p) - columnOffset > LineS) {
		columnOffset = Column(p) - LineS;
	}

	if (Column(positionOnActualLine) <= columnOffset) {
		columnOffset = Column(positionOnActualLine) - 1;
	}
	BPos = Position(columnOffset);


	if (TextLineNr < ScreenFirstLineNr) {
		ScreenFirstLineNr = TextLineNr;
	}

	if (TextLineNr >= ScreenFirstLineNr + PageS) {
		ScreenFirstLineNr = TextLineNr - PageS + 1;
	}

	// always update whole screen, not only line in the help viewer
	UpdScreen();

	WriteMargins();
	screen.GotoXY(positionOnActualLine - BPos, TextLineNr - ScreenFirstLineNr + 1);
	//IsWrScreen = true;
}

size_t HelpViewer::WordNo(size_t I)
{
	size_t len = GetTotalLength(_lines);

	int32_t last_index = min(len, I - 1);
	size_t count = CountChar(0x13, 0, last_index); // ^S
	return (count + 1) / 2;
}

bool HelpViewer::WordExist()
{
	return (_word.start_line >= ScreenFirstLineNr) && (_word.start_line < ScreenFirstLineNr + PageS);
}

WORD HelpViewer::WordNo2()
{
	WORD wNo;
	bool wExists = WordExist();

	if (wExists) {
		wNo = WordNo(SetInd(textIndex, positionOnActualLine));
	}
	else {
		wNo = WordNo(TextLineNr);
	}

	return wNo;
}

void HelpViewer::ClrWord()
{
	std::string txt = JoinLines(_lines);

	size_t word_begin = FindCharPosition(0x11, 0);
	if (word_begin < txt.length()) {
		txt[word_begin] = 0x13;
	}

	size_t word_end = FindCharPosition(0x11, word_begin + 1);
	if (word_end < txt.length()) {
		txt[word_end] = 0x13;
	}
}

void HelpViewer::ProcessHelpMode()
{
	//word_line = 0;
	//ScreenIndex = SetInd(textIndex, positionOnActualLine) - 1;
	size_t begin_index;
	size_t end_index;
	size_t line_number;
	uint16_t i = WordNo2() + 1;

	if (WordFind(i, begin_index, end_index, line_number)) {
		SetWord(begin_index, end_index);
	}

	if (!WordExist()) {
		TextLineNr = GetLineNumber(IndexT);
		//ScreenIndex = 0;
	}
}

bool HelpViewer::WordFind(WORD i, size_t& word_begin, size_t& word_end, size_t& line_nr)
{
	size_t len = GetTotalLength(_lines);

	bool result = false;
	if (i == 0) return result;
	i = i * 2 - 1;

	word_begin = FindCharPosition(0x13, 0, i);
	if (word_begin >= len) return result;

	word_end = FindCharPosition(0x13, word_begin + 1);
	if (word_end >= len) return result;

	line_nr = GetLine(word_begin); // TODO: +1 ?;
	result = true;
	return result;
}

void HelpViewer::SetWord(size_t word_begin, size_t word_end)
{
	std::string txt = JoinLines(_lines);
	txt[word_begin] = 0x11;
	txt[word_end] = 0x11;
	bool hardL = false;
	_lines = GetAllLinesWithEnds(txt, hardL); // HardL);
	TextLineNr = GetLineNumber(word_begin);
	_word.start_line = TextLineNr;
	positionOnActualLine = word_begin - textIndex + 1;
	Colu = Column(positionOnActualLine);
}

void HelpViewer::HelpLU(char dir)
{
	size_t I = 0, I1 = 0, I2 = 0;
	uint16_t h2 = 0;
	ClrWord();
	uint16_t h1 = WordNo2();

	if (dir == 'U') {
		DekFindLine(TextLineNr - 1);
		positionOnActualLine = Position(Colu);
		h2 = MinW(h1, WordNo2() + 1);
	}
	else {
		h2 = h1;
	}

	if (WordFind(h2, I1, I2, I) && (I >= ScreenFirstLineNr - 1)) {
		SetWord(I1, I2);
	}
	else {
		if (WordFind(h1 + 1, I1, I2, I) && (I >= ScreenFirstLineNr)) {
			SetWord(I1, I2);
		}
		else {
			I1 = SetInd(textIndex, positionOnActualLine);
			_word.start_line = 0;
		}
		I = ScreenFirstLineNr - 1;
	}
	if (I <= ScreenFirstLineNr - 1) {
		DekFindLine(ScreenFirstLineNr);
		RollPred();
	}

	if (WordExist()) {
		TextLineNr = GetLineNumber(I1);
	}
}

void HelpViewer::HelpRD(char dir)
{
	size_t I = 0, I1 = 0, I2 = 0;
	uint16_t h2 = 0;

	ClrWord();
	uint16_t h1 = WordNo2();
	if (WordExist()) {
		h1++;
	}

	if (dir == 'D') {
		NextLine(false);
		positionOnActualLine = Position(Colu);
		while ((positionOnActualLine > 0) && (Arr[positionOnActualLine - 1] != 0x13)) positionOnActualLine--;
		positionOnActualLine++;
		h2 = MaxW(h1 + 1, WordNo2() + 1);
	}
	else {
		h2 = h1 + 1;
	}

	if (WordFind(h2, I1, I2, I) && (I <= ScreenFirstLineNr + PageS)) {
		SetWord(I1, I2);
	}
	else {
		if (WordNo2() > h1) {
			h1++;
		}
		if (WordFind(h1, I1, I2, I) && (I <= ScreenFirstLineNr + PageS)) {
			SetWord(I1, I2);
		}
		else {
			I1 = SetInd(textIndex, positionOnActualLine);
			_word.start_line = 0;
		}
		I = ScreenFirstLineNr + PageS;
	}

	if (I >= ScreenFirstLineNr + PageS) {
		DekFindLine(ScreenFirstLineNr + PageS - 1);
		RollNext();
	}

	if (WordExist()) {
		TextLineNr = GetLineNumber(I1);
	}
}