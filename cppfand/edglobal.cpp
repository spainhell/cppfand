#include "edglobal.h"



#include "common.h"
#include "editor.h"
#include "kbdww.h"
#include "lexanal.h"

FrmlPtr RdFldNameFrmlT(char& FTyp)
{
	Error(8);
	return nullptr;
}

void MyWrLLMsg(pstring s)
{
	if (HandleError == 4) s = ""; SetMsgPar(s); WrLLF10Msg(700 + HandleError);
}

void MyRunError(pstring s, WORD n)
{
	SetMsgPar(s); RunError(n);
}

void HMsgExit(pstring s)
{
	switch (HandleError) {
	case 0: return;
	case 1: { s = s[1]; SetMsgPar(s); RunError(700 + HandleError); break; }
	case 2:
	case 3: { SetMsgPar(s); RunError(700 + HandleError); break; }
	case 4: RunError(704); break;
	}
}

WORD FindChar(WORD& Num, char C, WORD Pos, WORD Len)
{
	// ASM
	return 0;
}

bool TestOptStr(char c)
{
	return (OptionStr.first(c) != 0) || (OptionStr.first(toupper(c)) != 0);
}

bool FindString(WORD& I, WORD Len)
{
	WORD j, i1;
	pstring s1, s2;
	char c;
	auto result = false;
	c = FindStr[1];
	if (FindStr != "")
	{
	label1:
		if (TestOptStr('~')) i1 = FindOrdChar(c, I, Len);
		else if (TestOptStr('u')) i1 = FindUpcChar(c, I, Len);
		else { j = 1; i1 = FindChar(j, c, I, Len); }
		I = i1;
		if (I + FindStr.length() > Len) return result; s2 = FindStr;
		Move(T[I], &s1[1], FindStr.length()); s1[0] = FindStr.length();
		if (TestOptStr('~'))
		{
			if (!SEquOrder(s1, s2))
			{
				I++; goto label1;
			}
		}
		else if (TestOptStr('u'))
		{
			if (!SEquUpcase(s1, s2))
			{
				I++; goto label1;
			}
		}
		else if (s1 != s2) { I++; goto label1; }
		if (TestOptStr('w'))
			if (I > 1 && !Oddel.count(*T[I - 1]) ||	!Oddel.count(*T[I + FindStr.length()])) 
			{
				I++;
				goto label1;
			}
		result = true; I += FindStr.length();
	}
	return result;
}

WORD FindUpcChar(char C, WORD Pos, WORD Len)
{
	WORD I, K; char cc;
	I = Len; K = Pos - 1; cc = C;
	// TODO: ASM
	return Len - I;
}

WORD FindOrdChar(char C, WORD Pos, WORD Len)
{
	WORD I, K; char cc;
	I = Len; K = Pos - 1; cc = C;
	// TODO: ASM
	return Len - I;
}

bool SEquOrder(pstring S1, pstring S2)
{
	integer i;
	if (S1.length() != S2.length()) return false;
	for (i = 1; i < S1.length(); i++)
		if (CharOrdTab[S1[i]] != CharOrdTab[S2[i]]) return false;
	return true;
}

void SetColorOrd(ColorOrd CO, WORD First, WORD Last)
{
	WORD I, pp;
	BYTE*len = (BYTE*)&CO;
	I = FindCtrl(First, Last);
	while (I < Last)
	{
		pp = CO.first(*T[I]);
		if (pp > 0) CO = CO.substr(1, pp - 1) + CO.substr(pp + 1, *len - pp);
		else  CO = CO + *T[I];
		I = FindCtrl(I + 1, Last);
	}
}

WORD FindCtrl(WORD F, WORD L)
{
	WORD I, K;
	I = L; K = F - 1;
	// ASM
	return L - I + 1;
}

void SimplePrintHead()
{
	pstring ln;
	PHNum = 0; PPageS = 0x7FFF;
}
