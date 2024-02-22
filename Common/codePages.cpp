#include "codePages.h"

char CharOrdTab[256];
char UpcCharTab[256];

char NoDiakr(char C, TVideoFont fromFont)
{
	BYTE byteC = static_cast<BYTE>(C);
	if (byteC < 0x80) return C;

	switch (fromFont) {
	case TVideoFont::foAscii: {
		return C;
		break;
	}
	case TVideoFont::foLatin2: {
		return static_cast<char>(TabLtN[byteC]);
		break;
	}
	case TVideoFont::foKamen: {
		return static_cast<char>(TabKtN[byteC]);
		break;
	}
	default: {
		return C;
		break;
	}
	}
}

char CurrToKamen(char C)
{
	return C;
}

void ConvToNoDiakr(void* Buf, WORD L, TVideoFont FromFont)
{
	BYTE* c = static_cast<BYTE*>(Buf);
	if (FromFont == TVideoFont::foAscii) return;
	if (FromFont == TVideoFont::foLatin2) {
		for (size_t i = 0; i < L; i++) {
			if (c[i] < 0x80) continue;
			c[i] = TabLtN[c[i]];
		}
	}
	else {
		for (size_t i = 0; i < L; i++) {
			if (c[i] < 0x80) continue;
			c[i] = TabKtN[c[i]];
		}
	}
}

void ConvToNoDiakr(std::string& text, TVideoFont FromFont)
{
	if (FromFont == TVideoFont::foAscii) return;

	if (FromFont == TVideoFont::foLatin2) {
		for (size_t i = 0; i < text.length(); i++) {
			if (static_cast<BYTE>(text[i]) < 0x80) continue;
			text[i] = static_cast<char>(TabLtN[static_cast<BYTE>(text[i])]);
		}
	}
	else {
		for (size_t i = 0; i < text.length(); i++) {
			if (static_cast<BYTE>(text[i]) < 0x80) continue;
			text[i] = static_cast<char>(TabKtN[static_cast<BYTE>(text[i])]);
		}
	}
}

void ConvKamenToCurr(std::string& text, bool diacritic)
{
	BYTE* tab = diacritic ? TabKtL : TabKtN;

	for (size_t i = 0; i < text.length(); i++) {
		short index = (BYTE)text[i] - 0x80;
		if (index > 0) {
			BYTE kam = text[i];
			BYTE lat = tab[kam];
			text[i] = (char)lat;
		}
	}
}

void ConvKamenLatin(void* Buf, WORD L, bool ToLatin)
{
}

char ToggleCS(char C)
{
	char result = 0;
	BYTE input = (BYTE)C;

	if (input == 0) {
		// null stays null
	}
	else if (input > 0 && input < 32) {
		// non-printable char -> stays same
		result = input;
	}
	else {
		result = (char)toggleLatin2[input];
		if (result == 0) {
			// substitute not exists -> return input char
			result = C;
		}
	}
	return result;
}

