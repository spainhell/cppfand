#include "XString.h"
#include "access.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "runfrml.h"
#include "../pascal/real48.h"

void XString::Clear()
{
	this->S.clean();
}

void XString::StoreReal(double R, KeyFldD* KF)
{
	BYTE A[20];
	// pole urcuje pocet Bytu, ve kterych bude ulozeno cislo
	const BYTE TabF[18] = { 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8 };
	auto X = KF->FldD;

	if (X->Typ == 'R' || X->Typ == 'D') {
		bool b = KF->Descend;
		if (R < 0) { b = !b; R = -R; }
		auto r48 = DoubleToReal48(R);
		BYTE date[6];
		for (size_t i = 0; i < 6; i++) {
			date[i] = r48[i];
		}
		StoreD(date, b);
		return;
	}
	if ((X->Flg & f_Comma) == 0) R = R * Power10[X->M];
	WORD n = X->L - 1;
	if (X->M > 0) n--;
	n = TabF[n - 1];
	FixFromReal(R, A, n);
	StoreF(A, n, KF->Descend);
}

void XString::StoreStr(std::string V, KeyFldD* KF)
{
	WORD n = 0;
	auto X = KF->FldD;
	while (V.length() < X->L) {
		if (X->M == LeftJust) V += ' ';
		else {
			V = " " + V;
		}
	}
	if (X->Typ == 'N') {
		for (size_t i = 0; i < X->L; i++) {
			// kolikaty byte zapisujeme?
			size_t iB = i / 2;
			// zapisujeme levou nebo pravou cast?
			if (i % 2 == 0) {
				V[iB] = (V[i] - 0x30) << 4;
			}
			else {
				V[iB] += V[i] - 0x30;
			}
		}
		n = (X->L + 1) / 2;
		//V = V.substr(0, n); // neni potreba, do fce vstupuje delka ...
		StoreN(&V[0], n, KF->Descend);
	}
	else {
		StoreA(&V[0], X->L, KF->CompLex, KF->Descend);
	}
}

void XString::StoreBool(bool B, KeyFldD* KF)
{
	StoreN(&B, 1, KF->Descend);
}

void XString::StoreKF(KeyFldD* KF)
{
	FieldDescr* F = KF->FldD;
	switch (F->FrmlTyp) {
	case 'S': StoreStr(_ShortS(F), KF); break;
	case 'R': StoreReal(_R(F), KF); break;
	case 'B': StoreBool(_B(F), KF); break;
	}
}

void XString::PackKF(KeyFldD* KF)
{
	Clear();
	while (KF != nullptr) {
		StoreKF(KF);
		KF = (KeyFldD*)KF->Chain;
	}
}

bool XString::PackFrml(FrmlListEl* FL, KeyFldD* KF)
{
	FrmlElem* Z;
	Clear();
	while (FL != nullptr) {
		Z = FL->Frml;
		switch (KF->FldD->FrmlTyp) {
		case 'S':StoreStr(RunShortStr(Z), KF); break;
		case 'R':StoreReal(RunReal(Z), KF); break;
		case 'B':StoreBool(RunBool(Z), KF); break;
		}
		KF = (KeyFldD*)KF->Chain;
		FL = (FrmlListEl*)FL->Chain;
	}
	return KF != nullptr;
}

void XString::StoreD(void* R, bool Descend)
{
	unsigned char* data = (unsigned char*)R;
	unsigned char origLen = S[0];
	if (origLen + 6 < origLen) return; // proc to v ASM je? kvuli preteceni?
	S[0] = origLen + 6; // nova delka
	unsigned char D0 = data[0]; // AL
	unsigned char D5 = data[5]; // AH
	unsigned char b5 = (D5 <= 0xF0) ? 0x80 : 0; // nevyssi bit z d5 (AH) - negovany		
	D5 = D5 << 1;
	unsigned char b0 = (D0 & 0x01) << 7; // tento nejnizsi bit z d0 (AL) pujde na nejvyssi v d5
	D0 = (D0 >> 1) + b5; // rcr d0 (AL) + stav neg. CF
	D5 = (D5 >> 1) + b0;

	S[origLen + 1] = D0; // data zacinaji za origLen (jeste je tam 1B delka retezce)
	S[origLen + 2] = D5;

	S[origLen + 3] = data[3];
	S[origLen + 4] = data[4];

	S[origLen + 5] = data[1];
	S[origLen + 6] = data[2];

	if (Descend) {
		// neguj vsechny bity v datech
		for (size_t i = 0; i < 6; i++) {
			S[origLen + 1 + i] = ~S[origLen + 1 + i];
		}
	}
}

void XString::StoreN(void* N, WORD Len, bool Descend)
{
	std::string inpStr((char*)N, Len);
	pstring inpPStr = inpStr;
	S += inpPStr;
}

void XString::StoreF(void* F, WORD Len, bool Descend)
{
	unsigned char* data = (unsigned char*)F;
	unsigned char origLen = S[0];
	if (origLen + Len < Len) return; // proc to v ASM je? kvuli preteceni?
	S[0] = origLen + Len;
	if (data[0] <= 0x0F) S[origLen + 1] = data[0] | 0x80; // 1. bit bude '1'
	else S[origLen + 1] = data[0] & 0x7F; // 1. bit bude '0'
	unsigned char newIndex = origLen + 2; // zacneme zapisovat do S za puvodni data
	// dokopirujeme zbytek dat (1. Byte uz mame, pokracujeme od 2.)
	for (size_t i = 1; i < Len; i++) {
		S[newIndex++] = data[i];
	}
}

void XString::StoreA(void* A, WORD Len, bool CompLex, bool Descend)
{
	int endSpaces = 0; // pocet mezer na konci retezce
	char* p = (char*)A;
	if (CompLex) {
		std::string cplx = TranslateOrd(p);
		Len = cplx.length();
		memcpy(p, cplx.c_str(), Len);
	}
	// nahradi mezery na konci retezce za '0x1F'
	for (int i = Len - 1; i >= 0; i--) {
		if (p[i] == ' ') {
			p[i] = 0x1F;
			endSpaces++;
			continue;
		}
		break;
	}
	// pokud je koncovych mezer vic, kopirujeme jen jednu
	if (endSpaces > 1) {
		Len = Len - (endSpaces - 1);
	}
	auto oldLen = S[0];
	S[0] = oldLen + Len;
	memcpy(&S[oldLen + 1], p, Len);
	if (Descend) {
		// call NegateESDI;
	}
}