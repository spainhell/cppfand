#include "../Drivers/constants.h"
#include "TextEditorScreen.h"

TextEditorScreen::TextEditorScreen(size_t TextColumns, Blocks* blocks, std::string ctrlKey)
{
	_textColumns = TextColumns;
	_blocks = blocks;
	_ctrlKey = ctrlKey;
}

TextEditorScreen::~TextEditorScreen()
{
}

void TextEditorScreen::WriteEditLine(std::string& text_line, size_t row)
{
}

void TextEditorScreen::WriteScrollLine(std::string& text_line, size_t offset, size_t row)
{
}

void TextEditorScreen::EditWrline(char* input_text, size_t text_len, int Row, BYTE ColKey[], BYTE TxtColor, BYTE BlockColor)
{
	WORD BuffLine[256]{ 0 };
	BYTE nv1;
	BYTE nv2;
	bool IsCtrl = false;

	WORD Line = pred(ScreenFirstLineNr + Row);
	if (_blocks->LineInBlock(Line) && (TypeB == TextBlock)) {
		nv2 = BlockColor;
	}
	else {
		nv2 = TxtColor;
	}
	short i = 0;
	while (i < text_len && input_text[i] != '\0' && !(input_text[i] == __CR || input_text[i] == __LF) && i < LineMaxSize - 1) {
		nv1 = input_text[i];
		if (i < 0 || i > 255) throw std::exception("Index");
		BuffLine[i] = (nv2 << 8) + nv1;
		if (nv1 < 32) IsCtrl = true;
		i++;
	}

	short LP = i - 1;  // index of last character (before CR)
	nv1 = ' ';

	for (i = LP + 1; i <= BPos + LineS; i++) {
		// all characters after last char will be spaces (to the end of screen)
		if (i < 0 || i > 255) throw std::exception("Index");
		BuffLine[i] = (nv2 << 8) + nv1;
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
	if (IsCtrl) {
		// retezec obsahuje kontrolni znaky
		// -> budou zmeneny na pismena a prebarveny
		for (i = BPos; i <= LP; i++) {
			if ((unsigned char)input_text[i] < 32) {
				if (i < 0 || i > 254) throw std::exception("Index");
				BuffLine[i] = ((input_text[i] + 64) & 0x00FF) + (Color(input_text[i], ColKey) << 8);
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
			if (_ctrlKey.find(cc) != std::string::npos) IsCtrl = true;
			else {
				if (bScroll && (cc == 0x0C)) { InsPage = InsPg; I++; }
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
			else if (_ctrlKey.find(cc) != std::string::npos) {
				size_t pp = CO.find(cc);
				if (pp != std::string::npos) {
					// TODO: nevim, jak to ma presne fungovat
					// original: if pp>0 then CO:=copy(CO,1,pp-1)+copy(CO,pp+1,len-pp)
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
	const size_t indexOfKey = _ctrlKey.find(c);
	return ColKey[indexOfKey + 1];
}

BYTE TextEditorScreen::Color(ColorOrd CO, BYTE ColKey[], BYTE TxtColor)
{
	if (CO.length() == 0) {
		return TxtColor;
	}
	const char lastColor = CO[CO.length() - 1];
	return Color(lastColor, ColKey);
}
