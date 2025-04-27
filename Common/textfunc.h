#pragma once
#include <vector>
#include <string>

/// rozdeli vstup na jednotlive radky podle CR, LF nebo max. delky radku (0 = neomezeno); zohlednuje tisknutelne znaky
std::vector<std::string> GetAllLines(const std::string& input, size_t maxLineLen = 0, bool skipLastEmptyLine = false);

/// rozdeli vstup na jednolive radky a zachova v nich CR a LF; nezohlednuje tisknutelne znaky
std::vector<std::string> GetAllLinesWithEnds(std::string& input, bool& contains_LF);

/// sestavi retezec z vektoru retezcu
std::string JoinLines(const std::vector<std::string>& lines);

/// spocita celkovou delku retezcu ve vektoru
size_t GetTotalLength(const std::vector<std::string>& lines);

/// odstrani nebo nahradi zadane znaky na konci retezce
std::string TrailChar(std::string& input, char c_old, char c_new = '\0');

/// odstrani nebo nahradi zadane znaky na zacatku retezce
std::string LeadChar(std::string& input, char c_old, char c_new = '\0');

/// prida zadane znaky na konec retezce
std::string AddTrailChars(std::string& input, char c, size_t totalLength);

/// vrati 1 radek; cislo radku zacina 1 .. N, pocet 1 .. N; da se nastavit oddelovac
std::string GetNthLine(std::string& input, size_t from, size_t count, char delimiter = '\r');

/// vrati pocet casti v retezci po rozdeleni vstupnim znakem
size_t CountLines(std::string& input, char delimiter);

/// vytvori ze vstupu formatovany retez o maximalnim poctu zobrazitelnych znaku; vstup prochazi od pozice 'from'
std::string GetStyledStringOfLength(std::string& input, size_t from, size_t length);

/// vrati delku formatovaneho retezce (pocet zobrazitelnych znaku)
size_t GetLengthOfStyledString(std::string& input);

std::string RepeatString(std::string& input, int32_t count);
std::string RepeatString(char input, int32_t count);

std::string lowerCaseString(std::string input);
std::string upperCaseString(std::string input);

size_t ReplaceChar(std::string& text, char oldChar, char newChar);

// old functions:
unsigned short CountDLines(void* Buf, unsigned short L, char C); // r139 ASM

std::vector<std::string> SplitString(const std::string& str, char delimiter);