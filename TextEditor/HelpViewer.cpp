#include "HelpViewer.h"
#include "../Drivers/constants.h"
#include "../Common/textfunc.h"

HelpViewer::HelpViewer() : TextEditor(EditorMode::Help, TextType::Memo)
{
	TxtColor = screen.colors.hNorm;
	FillChar(ColKey, 8, screen.colors.tCtrl);
	ColKey[5] = screen.colors.hSpec;
	ColKey[3] = screen.colors.hHili;
	ColKey[1] = screen.colors.hMenu;

	brkKeys.push_back(__F1);
	brkKeys.push_back(__F6);
	brkKeys.push_back(__F10);
	brkKeys.push_back(__CTRL_HOME);
	brkKeys.push_back(__CTRL_END);
}

HelpViewer::~HelpViewer()
{
}

void HelpViewer::ViewHelp(std::string& help_text, size_t& text_pos)
{
	bool Srch = false;
	bool Upd = false;
	int Scr = 0;
	bool hardL = false;

	_lines = GetAllLinesWithEnds(help_text, hardL);

	FindAllWords();

	EditText(EditorMode::Help, TextType::Memo, "", "", help_text, 0xFFF0, text_pos, Scr,
		brkKeys, exitD, Srch, Upd, 142, 145, nullptr);
}

void HelpViewer::Background()
{
	// UpdStatLine(TextLineNr, positionOnActualLine);

	WORD p = positionOnActualLine;

	if (_word_list[_word_index].start_line == TextLineNr) {
		while (_lines[TextLineNr - 1][p] != 0x11) {
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

void HelpViewer::FindAllWords()
{
	size_t line_index = 0;
	size_t pos_index = 0;

	while (line_index < _lines.size()) {
		std::string line_text = _lines[line_index];
		size_t word_begin = line_text.find(0x13, pos_index);
		if (word_begin != std::string::npos) {
			size_t word_end = line_text.find(0x13, word_begin + 1);
			if (word_end != std::string::npos) {
				// word end is on the same line
				_word_list.push_back({ line_index + 1, word_begin, line_index + 1, word_end });
				pos_index = word_end + 1;
			}
			else {
				// word ends on next lines
				size_t next_line = line_index + 1;
				while (next_line < _lines.size()) {
					std::string next_line_text = _lines[next_line];
					pos_index = 0;
					size_t word_end = next_line_text.find(0x13);
					if (word_end != std::string::npos) {
						_word_list.push_back({ line_index + 1, word_begin, next_line, word_end });
						pos_index = word_end + 1;
						break;
					}
					else {
						next_line++;
						continue;
					}
				}
			}
		}
		else {
			// no more words in this line
			line_index++;
			pos_index = 0;
		}
	}
}

size_t HelpViewer::WordNo(size_t I)
{
	size_t len = GetTotalLength(_lines);

	int32_t last_index = min(len, I - 1);
	size_t count = CountChar(0x13, 0, last_index); // ^S
	return (count + 1) / 2;
}

bool HelpViewer::WordExistsOnActualScreen()
{
	// find word with start_line >= ScreenFirstLineNr
	// and start_line < ScreenFirstLineNr + PageS
	for (size_t i = 0; i < _word_list.size(); i++) {
		if (_word_list[i].start_line >= ScreenFirstLineNr && _word_list[i].start_line < ScreenFirstLineNr + PageS) {
			//_word_index = i;
			return true;
		}
	}

	return false;
}

void HelpViewer::ProcessHelpMove(uint16_t pressed_key)
{
	switch (pressed_key) {
	case __LEFT: {
		ProcessLeftUp('L');
		break;
	}
	case __RIGHT: {
		ProcessRightDown('R');
		break;
	}
	case __UP: {
		ProcessLeftUp('U');
		break;
	}
	case __DOWN: {
		ProcessRightDown('D');
		break;
	}
	case __PAGEUP: {
		ProcessPageUp();
		break;
	}
	case __PAGEDOWN: {
		ProcessPageDown();
		break;
	}
	default: break;
	}
}

WORD HelpViewer::WordNo2()
{
	WORD wNo;
	bool wExists = WordExistsOnActualScreen();

	if (wExists) {
		wNo = WordNo(SetInd(textIndex, positionOnActualLine));
	}
	else {
		wNo = WordNo(TextLineNr);
	}

	return wNo;
}

void HelpViewer::ProcessHelpMode()
{
	if (WordFind(1)) {
		// WordFind should return index of word in vector
		SetWord();
	}

	if (!WordExistsOnActualScreen()) {
		TextLineNr = 1;
		//ScreenIndex = 0;
	}
}

void HelpViewer::ProcessPageUp()
{
	ClrWord();
	TextLineNr = ScreenFirstLineNr;

	//int32_t L1 = blocks->LineAbs(TextLineNr);

	if (bScroll) {
		RScrL = MaxL(1, RScrL - PageS);
		if (ModPage(RScrL)) { RScrL++; }
		ScreenFirstLineNr = NewL(RScrL);
		TextLineNr = ScreenFirstLineNr;
		//DekFindLine(blocks->LineAbs(TextLineNr));
		positionOnActualLine = Position(Colu);
		// TODO: j = CountChar(0x0C, textIndex, ScreenIndex);

		//if ((j > 0) && InsPg) {
		//	DekFindLine(blocks->LineAbs(TextLineNr + j));
		//	ScreenFirstLineNr = TextLineNr;
		//	RScrL = NewRL(ScreenFirstLineNr);
		//}
	}
	else {
		if (ScreenFirstLineNr > PageS) {
			ScreenFirstLineNr -= PageS;
		}
		else {
			ScreenFirstLineNr = 1;
		}

		//DekFindLine(blocks->LineAbs(TextLineNr - PageS));
	}

	//_change_scr = true;

	//ScreenIndex = editor->GetLineStartIndex(editor->ScreenFirstLineNr);
	positionOnActualLine = Position(Colu);

	size_t I1 = 0, I2 = 0;
	//if (WordFind(WordNo2() + 1, I1, I2, _word.start_line) && WordExistsOnActualScreen()) {
	//	SetWord(I1, I2);
	//}
	//else {
	//	_word.start_line = 0;
	//}

}

void HelpViewer::ProcessPageDown()
{
	ClrWord();
	TextLineNr = ScreenFirstLineNr;

	//L1 = editor->blocks->LineAbs(editor->TextLineNr);

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

	//ScreenIndex = GetLineStartIndex(ScreenFirstLineNr);
	positionOnActualLine = Position(Colu);

	size_t I1 = 0, I2 = 0;
	size_t W1 = 0, W2 = 0;

	W1 = WordNo2();

	//if (WordFind(W1 + 1, I1, I2, _word.start_line) && WordExistsOnActualScreen()) {
	//	SetWord(I1, I2);
	//}
	//else if (WordFind(W1, I1, I2, _word.start_line) && WordExistsOnActualScreen()) {
	//	SetWord(I1, I2);
	//}
	//else {
	//	_word.start_line = 0;
	//}
}

bool HelpViewer::WordFind(WORD i)
{
	bool result;
	if (i == 0) {
		result = false;
	}
	else if (i >= _word_list.size()) {
		// there are no so much words
		result = false;
	}
	else {
		_word_index = i - 1;
		result = true;
	}
	return result;
}

void HelpViewer::SetWord()
{
	WordPosition* word = &_word_list[_word_index];
	_lines[word->start_line - 1][word->start_index] = 0x11;
	_lines[word->end_line - 1][word->end_index] = 0x11;
	TextLineNr = word->start_line;
	positionOnActualLine = word->start_index - textIndex + 1;
	Colu = Column(positionOnActualLine);
}

void HelpViewer::ClrWord()
{
	WordPosition* word = &_word_list[_word_index];
	_lines[word->start_line - 1][word->start_index] = 0x13;
	_lines[word->end_line - 1][word->end_index] = 0x13;
}

void HelpViewer::ProcessLeftUp(char dir)
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

	if (WordFind(h2) && (I >= ScreenFirstLineNr - 1)) {
		// WordFind should return index of word in vector
		SetWord();
	}
	else {
		if (WordFind(h1) && (I >= ScreenFirstLineNr)) {
			// WordFind should return index of word in vector
			SetWord();
		}
		else {
			I1 = SetInd(textIndex, positionOnActualLine);
			//_word.start_line = 0;
		}
		I = ScreenFirstLineNr - 1;
	}
	if (I <= ScreenFirstLineNr - 1) {
		DekFindLine(ScreenFirstLineNr);
		RollPred();
	}

	if (WordExistsOnActualScreen()) {
		TextLineNr = GetLineNumber(I1);
	}
}

void HelpViewer::ProcessRightDown(char dir)
{
	size_t I = 0, I1 = 0, I2 = 0;
	uint16_t h2 = 0;

	ClrWord();
	uint16_t h1 = WordNo2();
	if (WordExistsOnActualScreen()) {
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

	if (WordFind(h2) && (I <= ScreenFirstLineNr + PageS)) {
		// WordFind should return index of word in vector
		SetWord();
	}
	else {
		if (WordNo2() > h1) {
			h1++;
		}
		if (WordFind(h1) && (I <= ScreenFirstLineNr + PageS)) {
			// WordFind should return index of word in vector
			SetWord();
		}
		else {
			I1 = SetInd(textIndex, positionOnActualLine);
			//_word.start_line = 0;
		}
		I = ScreenFirstLineNr + PageS;
	}

	if (I >= ScreenFirstLineNr + PageS) {
		DekFindLine(ScreenFirstLineNr + PageS - 1);
		RollNext();
	}

	if (WordExistsOnActualScreen()) {
		TextLineNr = GetLineNumber(I1);
	}
}
