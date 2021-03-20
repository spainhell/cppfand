#include "shared.h"


#include "rdmerg.h"
#include "rdrprt.h"
#include "../cppfand/compile.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"

void TestNotSum()
{
	if (FrmlSumEl != nullptr) OldError(41);
}

void Err(char source, bool wasIiPrefix)
{
	TestNotSum();
	switch (source) {
	case 'r': SetIi_Report(wasIiPrefix); break;
	case 'm': SetIi_Merge(wasIiPrefix); break;
	}
	if (IDA[Ii]->ErrTxtFrml == nullptr)
	{
		IDA[Ii]->ErrTxtFrml = new FrmlElem4(_const, 0); // GetOp(_const, 256);
	}
}

void SetIi_Merge(bool wasIiPrefix)
{
	if (!wasIiPrefix) {
		if (!Join && (WhatToRd == 'i')) Ii = Oi;
		else Ii = 1;
	}
}

void SetIi_Report(bool wasIiPrefix)
{
	if (!wasIiPrefix) {
		if (WhatToRd == 'i') Ii = Oi;
		else Ii = 1;
	}
}

bool RdIiPrefix()
{
	auto result = false;
	if ((ForwChar == '.') && (LexWord.length() == 2) && (LexWord[1] == 'I') &&
		(isdigit(LexWord[2]) && LexWord[2] != '0')) {
		Ii = LexWord[2] - '0';
		if ((Ii > MaxIi) || (WhatToRd == 'i') && (Ii > Oi)) Error(9);
		RdLex(); RdLex(); result = true;
	}
	else {
		Ii = 0; result = false;
	}
	return result;
}

void CopyPrevMFlds()
{
	KeyFldD* MNew = nullptr; FieldDescr* F = nullptr;
	pstring s = LexWord;
	KeyFldD* M = IDA[Ii - 1]->MFld;
	//std::vector<std::string> vFieldNames;
	//std::vector<std::string>::iterator it;
	while (M != nullptr) {
		LexWord = M->FldD->Name;
		std::string LexWordString = LexWord;
		// toto je pridano navic, protoze se metoda cyklila
		// - zkontrolumeme, jestli uz uvedena polozka nebyla pridana
		// - je mozne, ze v nekterych sestavach bude nutne toto osetrit jinak
		//   (protoze se bude 1 polozka v sestave opakovat)
		// !!!! NAKONEC VYRESENO NASTAVENIM CHAIN NOVE POLOZKY NA NULL !!!!
		// !!!! O PAR RADKU NIZ !!!!
		/*it = std::find(vFieldNames.begin(), vFieldNames.end(), LexWordString);
		if (it != vFieldNames.end()) break;
		vFieldNames.push_back(LexWordString);*/

		F = FindFldName(InpFD(Ii));
		if (F == nullptr) OldError(8);
		if (!FldTypIdentity(M->FldD, F)) OldError(12);
		MNew = new KeyFldD(); // (KeyFldD*)GetStore(sizeof(*MNew));
		MNew->Chain = nullptr; // M->Chain;
		MNew->CompLex = M->CompLex;
		MNew->Descend = M->Descend;
		MNew->FldD = F;

		if (IDA[Ii]->MFld == nullptr) IDA[Ii]->MFld = MNew;
		else ChainLast(IDA[Ii]->MFld, MNew);

		M = (KeyFldD*)M->Chain;
	}
	LexWord = s;
}

