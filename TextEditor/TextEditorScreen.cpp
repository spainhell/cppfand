#include "../Drivers/constants.h"
#include "TextEditorScreen.h"

TextEditorScreen::TextEditorScreen(TextEditor* editor, size_t TextColumns, Blocks* blocks, std::set<char> ctrlKey)
{
	_editor = editor;
	_textColumns = TextColumns;
	_blocks = blocks;
	_ctrlKey = ctrlKey;
}

TextEditorScreen::~TextEditorScreen()
{
}

void TextEditorScreen::EditWrline(const std::string& text_line, int Row, BYTE ColKey[], BYTE TxtColor, BYTE BlockColor)
{
	WORD BuffLine[256]{ 0 };
	BYTE nv2;

	WORD Line = _editor->ScreenFirstLineNr + Row - 1;
	if (_blocks->LineInBlock(Line) && (TypeB == TextBlock)) {
		nv2 = BlockColor;
	}
	else {
		nv2 = TxtColor;
	}

	size_t i = 0;
	for (; i < LineMaxSize - 1; i++) {
		if (text_line[i] == '\0' || text_line[i] == __CR || text_line[i] == __LF) {
			break;
		}
		uint8_t c = text_line[i];
		if (c < 32) {
			// control char -> convert to letter and color
			BuffLine[i] = ((text_line[i] + 64) & 0x00FF) + (Color(text_line[i], ColKey) << 8);
		}
		else {
			// normal char
			BuffLine[i] = (nv2 << 8) + c;
		}
	}

	for (; i < BPos + LineS; i++) {
		// all characters after last char will be spaces (to the end of screen)
		BuffLine[i] = (nv2 << 8) + ' ';
	}

	if (_blocks->BegBLn <= _blocks->EndBLn) {
		if (_blocks->LineBndBlock(Line) || ((TypeB == ColBlock) && _blocks->LineInBlock(Line))) {
			short B, E;
			if ((_blocks->BegBLn == _blocks->LineAbs(Line)) || (TypeB == ColBlock)) {
				B = MinI(_blocks->BegBPos, LineS + BPos + 1);
			}
			else { B = 1; }
			if ((_blocks->EndBLn == _blocks->LineAbs(Line)) || (TypeB == ColBlock)) {
				E = MinI(_blocks->EndBPos, LineS + BPos + 1);
			}
			else { E = LineS + BPos + 1; }
			for (i = B; i <= pred(E); i++) {
				if (i < 0 || i > 255) throw std::exception("Index");
				BuffLine[i] = (BuffLine[i] & 0x00FF) + (BlockColor << 8);
			}
		}
	}

	// both 'WindMin.Y' and 'Row' are counted from 1 -> that's why -2 
	screen.ScrWrBuf(WindMin.X - 1, WindMin.Y + Row - 2, &BuffLine[BPos], LineS);
}

void TextEditorScreen::ScrollWrline(char* P, size_t offsetX, int Row, ColorOrd& CO, BYTE ColKey[], BYTE TxtColor, bool& InsPage)
{
	std::set<char> GrafCtrl = { 3,6,9,11,15,16,18,21,22,24,25,26,29,30,31 };
	BYTE len = 15; // GrafCtrl has 15 members

	WORD BuffLine[256]{ 0 };
	BYTE nv1;
	BYTE nv2;

	bool IsCtrl = false;
	BYTE Col = Color(CO, ColKey, TxtColor);
	nv2 = Col;

	short I = 0;
	short J = 0;
	char cc = P[I];
	while (cc != '\0' && !(cc == __CR || cc == __LF) && I < LineMaxSize && !InsPage) {
		if (((BYTE)cc >= 32) || (GrafCtrl.count(cc) > 0)) {
			nv1 = cc;
			BuffLine[J] = (nv2 << 8) + nv1;
			J++;
		}
		else {
			if (_ctrlKey.contains(cc)) IsCtrl = true;
			else {
				if (bScroll && (cc == 0x0C)) { InsPage = _editor->InsPg; I++; }
			}
		}
		I++;
		cc = P[I];
	}

	short LP = I - 1;   // index of last character (before CR)
	nv1 = ' ';

	while (J < offsetX + LineS) {
		BuffLine[J] = (nv2 << 8) + nv1;
		J++;
	}
	if (IsCtrl) {
		I = 0; J = 0;
		while (I <= LP) {
			cc = P[I];
			if (((BYTE)cc >= 32) || (GrafCtrl.count(cc) > 0)) {
				BuffLine[J] = (BuffLine[J] & 0x00FF) + (Col << 8);
				J++;
			}
			else if (_ctrlKey.contains(cc)) {
				size_t pp = CO.find(cc);
				if (pp != std::string::npos) {
					CO = CO.substr(0, pp) + CO.substr(pp + 1, len - pp + 1);
				}
				else {
					CO += cc;
				}
				Col = Color(CO, ColKey, TxtColor);
			}
			else if (cc == 0x0C) {
				BuffLine[J] = 219 + (Col << 8);
			}
			I++;
		}
		while (J <= offsetX + LineS) {
			BuffLine[J] = (BuffLine[J] & 0x00FF) + (Col << 8);
			J++;
		}
	}
	// both 'WindMin.Y' and 'Row' are counted from 1 -> that's why -2 
	screen.ScrWrBuf(WindMin.X - 1, WindMin.Y + Row - 2, &BuffLine[offsetX], LineS);
}

BYTE TextEditorScreen::Color(char c, BYTE ColKey[])
{
	size_t index = 0;
	switch (c) {
	case '\x13': index = 1; break;
	case '\x17': index = 2; break;
	case '\x11': index = 3; break;
	case '\x04': index = 4; break;
	case '\x02': index = 5; break;
	case '\x05': index = 6; break;
	case '\x01': index = 7; break;
	default: index = 0; break;
		// default case is not needed, but added for safety
		// if the last color is not in the list, return the default text color
	}
	return ColKey[index];
}

BYTE TextEditorScreen::Color(ColorOrd CO, BYTE ColKey[], BYTE TxtColor)
{
	if (CO.empty()) {
		return TxtColor;
	}

	return Color(CO[CO.length() - 1], ColKey);
}
