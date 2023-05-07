#include "compare.h"
#include "codePages.h"

const unsigned char equ = 0x1;
const unsigned char lt = 0x2;
const unsigned char gt = 0x4;

bool EquUpCase(pstring& S1, pstring& S2)
{
	if (S1.length() != S2.length()) return false;
	for (size_t i = 1; i <= S1.length(); i++) { // Pascal. string -> index od 1
		unsigned char c1 = S1[i];
		unsigned char c2 = S2[i];
		unsigned char upC1 = UpcCharTab[c1];
		unsigned char upC2 = UpcCharTab[c2];
		if (upC1 != upC2) return false;
	}
	return true;
}

bool EquUpCase(const std::string& S1, const std::string& S2)
{
	if (S1.length() != S2.length()) return false;
	for (size_t i = 0; i <= S1.length(); i++) {
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

short CompLongStr(LongStr* S1, LongStr* S2)
{
	if (S1->LL != S2->LL) {
		if (S1->LL < S2->LL) return lt;
		else return gt;
	}
	if (S2->LL == 0) return equ;
	for (size_t i = 0; i < S2->LL; i++)
	{
		if (S1->A[i] == S2->A[i]) continue;
		if (S1->A[i] < S2->A[i]) return lt;
		return gt;
	}
	return equ;
}

short CompLongShortStr(LongStr* S1, pstring* S2)
{
	short result = 0;
	if (S1->LL != (*S2)[0]) {
		if (S1->LL < (*S2)[0]) result = lt;
		else result = gt;
	}
	if ((*S2)[0] == 0) return result;
	for (size_t i = 0; i < (*S2)[0]; i++) {
		if ((unsigned char)S1->A[i] == (*S2)[i + 1]) continue;
		if ((unsigned char)S1->A[i] < (*S2)[i + 1]) return lt;
		return gt;
	}
	return equ;
}

short CompArea(void* A, void* B, size_t L)
{
	int result = memcmp(A, B, L);

	if (result == 0) return equ;
	if (result < 0) return lt;
	return gt;
}

short CompStr(pstring& S1, pstring& S2)
{
	std::string s1 = S1;
	std::string s2 = S2;
	return CompStr(s1, s2);
}

int CompStr(std::string& S1, std::string& S2)
{
	size_t cmpLen = std::min(S1.length(), S2.length());
	for (size_t i = 0; i < cmpLen; i++) {
		if (S1[i] == S2[i]) {
			continue;
		}
		else {
			if ((unsigned char)S1[i] < (unsigned char)S2[i]) return 2; // _lt
			else return 4; // _gt
		}
	}
	if (S1.length() < S2.length()) return lt;
	if (S1.length() > S2.length()) return gt;
	return equ;
}

unsigned short CmpLxStr(char* p1, size_t len1, char* p2, size_t len2)
{
	if (len1 > 0) {
		for (size_t i = len1 - 1; i > 0; i--) {
			if (p1[i] == ' ') { len1--; continue; }
			break;
		}
	}
	if (len2 > 0) {
		for (size_t i = len2 - 1; i > 0; i--) {
			if (p2[i] == ' ') { len1--; continue; }
			break;
		}
	}

	size_t cmpLen = std::min(len1, len2);
	for (size_t i = 0; i < cmpLen; i++) {
		if ((unsigned char)p1[i] == (unsigned char)p2[i]) continue;
		if ((unsigned char)p1[i] < (unsigned char)p2[i]) return lt;
		return gt;
	}
	if (len1 < len2) return lt;
	if (len1 > len2) return gt;
	return equ;
}

unsigned short CompLexLongStr(LongStr* S1, LongStr* S2)
{
	size_t l1 = std::min(S1->LL, (size_t)256);
	char* b1 = new char[l1];
	size_t l2 = std::min(S2->LL, (size_t)256);
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[S1->A[i]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[S2->A[i]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}

unsigned short CompLexLongShortStr(LongStr* S1, pstring& S2)
{
	size_t l1 = std::min(S1->LL, (size_t)256);
	char* b1 = new char[l1];
	size_t l2 = S2[0];
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[S1->A[i]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[S2[i + 1]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}

unsigned short CompLexStr(const pstring& S1, const pstring& S2)
{
	unsigned short l1 = static_cast<pstring>(S1)[0];
	char* b1 = new char[l1];
	unsigned short l2 = static_cast<pstring>(S2)[0];
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[static_cast<pstring>(S1)[i + 1]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[static_cast<pstring>(S2)[i + 1]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}

unsigned short CompLexStr(pstring& S1, pstring& S2)
{
	unsigned short l1 = S1[0];
	char* b1 = new char[l1];
	unsigned short l2 = S2[0];
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[S1[i + 1]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[S2[i + 1]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}

unsigned short CompLexStrings(const std::string& S1, const std::string& S2)
{
	size_t l1 = S1.length();
	char* b1 = new char[l1];
	size_t l2 = S2.length();
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[(unsigned char)S1[i]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[(unsigned char)S2[i]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}