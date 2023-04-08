#include "compare.h"
#include "codePages.h"


bool EquUpCase(pstring& S1, pstring& S2)
{
	if (S1.length() != S2.length()) return false;
	for (size_t i = 1; i <= S1.length(); i++) // Pascal. string -> index od 1
	{
		unsigned char c1 = S1[i];
		unsigned char c2 = S2[i];
		unsigned char upC1 = UpcCharTab[c1];
		unsigned char upC2 = UpcCharTab[c2];
		if (upC1 != upC2) return false;
	}
	return true;
}

bool EquUpCase(std::string S1, std::string S2)
{
	if (S1.length() != S2.length()) return false;
	for (size_t i = 0; i <= S1.length(); i++)
	{
		unsigned char c1 = S1[i];
		unsigned char c2 = S2[i];
		unsigned char upC1 = UpcCharTab[c1];
		unsigned char upC2 = UpcCharTab[c2];
		if (upC1 != upC2) return false;
	}
	return true;
}

bool EquUpCase(const char* S, pstring& S1)
{
	pstring temp = S;
	return EquUpCase(temp, S1);
}