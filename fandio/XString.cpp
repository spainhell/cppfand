#include "XString.h"
#include "../Core/access.h"
#include "../Core/FieldDescr.h"
#include "../Core/GlobalVariables.h"
#include "../Core/KeyFldD.h"
#include "../Core/runfrml.h"
#include "../pascal/real48.h"

void XString::Clear()
{
	this->S.clean();
}

void XString::StoreReal(double R, KeyFldD* KF)
{
	unsigned char A[20];
	// pole urcuje pocet Bytu, ve kterych bude ulozeno cislo
	const unsigned char TabF[18] = { 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8 };
	FieldDescr* X = KF->FldD;

	if (X->field_type == FieldType::REAL || X->field_type == FieldType::DATE) {
		bool b = KF->Descend;
		if (R < 0) { b = !b; R = -R; }
		auto r48 = DoubleToReal48(R);
		unsigned char date[6];
		for (size_t i = 0; i < 6; i++) {
			date[i] = r48[i];
		}
		StoreD(date, b);
		return;
	}
	if ((X->Flg & f_Comma) == 0) R = R * Power10[X->M];
	unsigned short n = X->L - 1;
	if (X->M > 0) n--;
	n = TabF[n - 1];
	FixFromReal(R, A, n);
	StoreF(A, n, KF->Descend);
}

void XString::StoreStr(std::string V, KeyFldD* KF)
{
	unsigned short n = 0;
	FieldDescr* X = KF->FldD;
	while (V.length() < X->L) {
		if (X->M == LeftJust) {
			V += ' ';
		}
		else {
			V = " " + V;
		}
	}
	if (X->field_type == FieldType::NUMERIC) {
		for (size_t i = 0; i < X->L; i++) {
			// kolikaty byte zapisujeme?
			size_t iB = i / 2;
			// je znak cislo? pokud ne, nahradime nulou
			if (!isdigit(V[i])) {
				V[i] = '0';
			}
			// zapisujeme levou nebo pravou cast?
			if (i % 2 == 0) {
				V[iB] = static_cast<char>((V[i] - 0x30) << 4);
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

void XString::StoreKF(FileD* file_d, KeyFldD* KF, void* record)
{
	FieldDescr* F = KF->FldD;
	switch (F->frml_type) {
	case 'S': {
		StoreStr(file_d->loadS(F, record), KF);
		break;
	}
	case 'R': {
		StoreReal(file_d->loadR(F, record), KF);
		break;
	}
	case 'B': {
		StoreBool(file_d->loadB(F, record), KF);
		break;
	}
	}
}

//void XString::PackKF(FileD* file_d, KeyFldD* KF, void* record)
//{
//	Clear();
//	while (KF != nullptr) {
//		StoreKF(file_d, KF, record);
//		KF = KF->pChain;
//	}
//}

void XString::PackKF(FileD* file_d, std::vector<KeyFldD*>& KF, void* record)
{
	Clear();
	for (KeyFldD* k : KF) {
		StoreKF(file_d, k, record);
	}
}

bool XString::PackFrml(FileD* file_d, std::vector<FrmlElem*>& FL, std::vector<KeyFldD*>& KF, void* record)
{
	Clear();

	for (size_t i = 0; i < FL.size(); i++) {
		FrmlElem* Z = FL[i];
		KeyFldD* key_field = KF[i];
		switch (key_field->FldD->frml_type) {
		case 'S': {
			StoreStr(RunShortStr(file_d, Z, record), key_field);
			break;
		}
		case 'R': {
			StoreReal(RunReal(file_d, Z, record), key_field);
			break;
		}
		case 'B': {
			StoreBool(RunBool(file_d, Z, record), key_field);
			break;
		}
		}
		//KF = KF->pChain;
		//FL = FL->pChain;
	}

	//return KF != nullptr;
	return KF.size() > FL.size();
}

void XString::StoreD(void* R, bool descend)
{
	unsigned char* data = (unsigned char*)R;
	unsigned char origLen = S[0];
	if (origLen + 6 < origLen) return; // proc to v ASM je? kvuli preteceni?
	S[0] = origLen + 6; // nova delka
	unsigned char D0 = data[0]; // AL
	unsigned char D5 = data[5]; // AH

	unsigned char b8D5 = (D5 >= 0x80) ? 0 : 0x80; // nevyssi bit z d5 (AH) - NEGOVANY
	unsigned char b1D0 = (D0 & 0x01) << 7;               // nejnizsi bit z d0 (AL)

	D5 = (unsigned char)(D5 << 1); // rotace doleva, nejnizsi bit je 0

	D0 = D0 >> 1; // rotace doprava, nejvyssi bit je 0
	D0 += b8D5; // nejvyssi bit bude b8 z D5 (bud pricteme 0b0000000 nebo 0b10000000)

	D5 = D5 >> 1; // rotace doprava, nejvyssi bit je 0
	D5 += b1D0;   // nejvyssi bit bude b1 z D0 (bud pricteme 0b0000000 nebo 0b10000000)
	
	S[origLen + 1] = D0; // data zacinaji za origLen (jeste je tam 1B delka retezce)
	S[origLen + 2] = D5;

	S[origLen + 3] = data[4];
	S[origLen + 4] = data[3];

	S[origLen + 5] = data[2];
	S[origLen + 6] = data[1];

	if (descend) {
		negate_esdi(&S[origLen + 1], 6);
	}
}

void XString::StoreN(void* N, unsigned short len, bool descend)
{
	std::string inpStr((char*)N, len);
	pstring inpPStr = inpStr;

	if (descend) {
		negate_esdi(&inpPStr[1], inpPStr[0]);
	}
	
	S += inpPStr;
}

void XString::StoreF(void* F, unsigned short len, bool descend)
{
	unsigned char* data = (unsigned char*)F;
	unsigned char origLen = S[0];
	if (origLen + len < len) return; // proc to v ASM je? kvuli preteceni?
	S[0] = origLen + len;
	// in original ASM code there is the negation of highest bit for all bytes
	if (data[0] <= 0b01111111) {
		S[origLen + 1] = data[0] | 0b10000000; // 1. bit will be '1'
	}
	else {
		S[origLen + 1] = data[0] & 0b01111111; // 1. bit will be '0'
	}
	unsigned char newIndex = origLen + 2; // zacneme zapisovat do S za puvodni data
	// dokopirujeme zbytek dat (1. Byte uz mame, pokracujeme od 2.)
	for (size_t i = 1; i < len; i++) {
		S[newIndex++] = data[i];
	}

	if (descend) {
		negate_esdi(&S[origLen + 1], len);
	}
}

void XString::StoreA(void* A, unsigned short len, bool compLex, bool descend)
{
	int endSpaces = 0; // pocet mezer na konci retezce
	char* p = (char*)A;
	if (compLex) {
		std::string cplx = TranslateOrd(p);
		len = cplx.length();
		memcpy(p, cplx.c_str(), len);
	}
	// nahradi mezery na konci retezce za '0x1F'
	for (int i = len - 1; i >= 0; i--) {
		if (p[i] == ' ') {
			p[i] = 0x1F;
			endSpaces++;
			continue;
		}
		break;
	}
	// pokud je koncovych mezer vic, kopirujeme jen jednu
	if (endSpaces > 1) {
		len = len - (endSpaces - 1);
	}
	unsigned char oldLen = S[0];
	S[0] = oldLen + len;
	memcpy(&S[oldLen + 1], p, len);
	
	if (descend) {
		negate_esdi(&S[oldLen + 1], len);
	}
}

void XString::negate_esdi(void* data, size_t len)
{
	unsigned char* src = (unsigned char*)data;
	// neguj vsechny bity v datech
	for (size_t i = 0; i < len; i++) {
		src[i] = ~src[i];
	}
}
