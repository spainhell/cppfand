#pragma once
#include <vector>
#include <string>

#include "../cppfand/constants.h"

/// rozdeli vstup na jednotlive radky podle CR, LF nebo max delky radku (0 = neomezeno); zohlednuje tisknutelne znaky
std::vector<std::string> GetAllRows(std::string input, size_t maxLineLen = 0);
std::string TrailChar(std::string& input, char c);
std::string LeadChar(std::string& input, char c);
/// cislo radku zacina 1 .. N, pocet 1 .. N; da se nastavit oddelovac
std::string GetNthLine(std::string& input, size_t from, size_t count, char delimiter = '\r');
/// vrati seznam casti v retezci po rozdeleni vstupnim znakem
size_t CountLines(std::string& input, char delimiter);
/// vytvori ze vstupu formatovany retez o maximalnim poctu zobrazitelnych znaku; vstup prochazi od pozice 'from'
std::string GetStyledStringOfLength(std::string& input, size_t from, size_t length);

std::string RepeatString(std::string& input, size_t count);
std::string RepeatString(char input, size_t count);

// old functions:
WORD CountDLines(void* Buf, WORD L, char C); // r139 ASM
//pstring GetDLine(void* Buf, WORD L, char C, WORD I); // r144 ASM