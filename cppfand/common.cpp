#pragma once

#include "common.h"
#include <cmath>
#include "base.h"
#include "legacy.h"


void AddBackSlash(pstring s)
{
	if (s[s.length() - 1] == '\\') return;
	s += '\\';
}

void DelBackSlash(pstring s)
{
	if (s[s.length() - 1] != '\\') return;
	s[s.length() - 1] = '\0';
	s[0] = s.length() - 1;
}

pstring StrPas(const char* Src)
{
	WORD n; pstring s;
	n = 0;
	while ((n < 255) && (Src[n] != '\0')) { s[n + 1] = Src[n]; n++; }
	s[0] = char(n);
	return s;
}

void StrLPCopy(char* Dest, pstring s, WORD MaxL)
{
	auto sLen = s.length();
	int len = (sLen < MaxL) ? sLen : MaxL;
	Move((void*)s.c_str(), Dest, len);
}

integer MinI(integer X, integer Y)
{
	if (X < Y) return X;
	return Y;
}

integer MaxI(integer X, integer Y)
{
	if (X > Y) return X;
	return X;
}

void SplitDate(double R, WORD& d, WORD& m, WORD& y)
{
	WORD i, j;

	longint l = (longint)std::trunc(R);

	if (l == 0) { y = 1; m = 1; d = 1; }
	else {
		y = l / 365; y++; l = l % 365;
		while (l <= OlympYears(y)) { y--; l += 365; }
		l = l - OlympYears(y);
		for (j = 1; j <= 12; j++) {
			i = NoDayInMonth[j];
			if ((j == 2) && OlympYear(y)) i++;
			if (i >= l) goto label1;
			l -= i;
		}
	label1:
		m = j; d = l;
	}
}

bool OlympYear(WORD year)
{
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

WORD OlympYears(WORD year)
{
	if (year < 3) return 0;
	year--;
	return year / 4 - year / 100 + year / 400;
}
