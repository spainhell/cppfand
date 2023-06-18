#include "MergeReportBase.h"
#include "../Core/compile.h"
#include "../Core/FieldDescr.h"
#include "../Core/GlobalVariables.h"
#include "../Core/KeyFldD.h"

MergeReportBase::MergeReportBase()
{
	ChainSum = false;
	Ii = 0;
	Oi = 0;
	SumIi = 0;
	WhatToRd = '\0'; /*i=Oi output FDs;O=O outp.FDs*/
	NRecsAll = 0;
}

MergeReportBase::~MergeReportBase()
{
}

FileD* MergeReportBase::InpFD(WORD I)
{
	return IDA[I]->Scan->FD;
}

void MergeReportBase::TestNotSum()
{
	if (FrmlSumEl != nullptr) OldError(41);
}

void MergeReportBase::Err(char source, bool wasIiPrefix)
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

void MergeReportBase::SetIi_Merge(bool wasIiPrefix)
{
	if (!wasIiPrefix) {
		if (!Join && (WhatToRd == 'i')) Ii = Oi;
		else Ii = 1;
	}
}

void MergeReportBase::SetIi_Report(bool wasIiPrefix)
{
	if (!wasIiPrefix) {
		if (WhatToRd == 'i') Ii = Oi;
		else Ii = 1;
	}
}

bool MergeReportBase::RdIiPrefix()
{
	bool result;
	if ((ForwChar == '.') && (LexWord.length() == 2) && (LexWord[1] == 'I') &&
		(isdigit(LexWord[2]) && LexWord[2] != '0')) {
		Ii = LexWord[2] - '0';
		if ((Ii > MaxIi) || (WhatToRd == 'i') && (Ii > Oi)) {
			Error(9);
		}
		RdLex(); RdLex();
		result = true;
	}
	else {
		Ii = 0;
		result = false;
	}
	return result;
}

void MergeReportBase::CopyPrevMFlds()
{
	KeyFldD* M = IDA[Ii - 1]->MFld;
	//std::vector<std::string> vFieldNames;
	//std::vector<std::string>::iterator it;
	while (M != nullptr) {
		std::string fld_name = M->FldD->Name;
		// std::string LexWordString = LexWord;
		// toto je pridano navic, protoze se metoda cyklila
		// - zkontrolumeme, jestli uz uvedena polozka nebyla pridana
		// - je mozne, ze v nekterych sestavach bude nutne toto osetrit jinak
		//   (protoze se bude 1 polozka v sestave opakovat)
		// !!!! NAKONEC VYRESENO NASTAVENIM CHAIN NOVE POLOZKY NA NULL !!!!
		// !!!! O PAR RADKU NIZ !!!!
		/*it = std::find(vFieldNames.begin(), vFieldNames.end(), LexWordString);
		if (it != vFieldNames.end()) break;
		vFieldNames.push_back(LexWordString);*/

		FieldDescr* F = FindFldName(InpFD(Ii), fld_name);
		if (F == nullptr) {
			OldError(8);
		}
		if (!FldTypIdentity(M->FldD, F)) {
			OldError(12);
		}
		KeyFldD* MNew = new KeyFldD(); // (KeyFldD*)GetStore(sizeof(*MNew));
		MNew->pChain = nullptr; // M->pChain;
		MNew->CompLex = M->CompLex;
		MNew->Descend = M->Descend;
		MNew->FldD = F;

		if (IDA[Ii]->MFld == nullptr) IDA[Ii]->MFld = MNew;
		else ChainLast(IDA[Ii]->MFld, MNew);

		M = M->pChain;
	}
}

void MergeReportBase::CheckMFlds(KeyFldD* M1, KeyFldD* M2)
{
	while (M1 != nullptr) {
		if (M2 == nullptr) OldError(30);
		if (!FldTypIdentity(M1->FldD, M2->FldD)
			|| (M1->Descend != M2->Descend)
			|| (M1->CompLex != M2->CompLex))
			OldError(12);
		M1 = M1->pChain;
		M2 = M2->pChain;
	}
	if (M2 != nullptr) OldError(30);
}

void MergeReportBase::TestSetSumIi()
{
	if ((FrmlSumEl != nullptr) && (Ii != 0))
		if (FrstSumVar || (SumIi == 0)) SumIi = Ii;
		else if (SumIi != Ii) OldError(27);
}

void MergeReportBase::ZeroSumFlds(LvDescr* L)
{
	while (L != nullptr) {
		for (FrmlElemSum* el : L->ZeroLst) {
			el->R = 0;
		}
		L = L->ChainBack;
	}
}

void MergeReportBase::ZeroSumFlds(std::vector<FrmlElemSum*>* sum)
{
	if (sum != nullptr) {
		for (FrmlElemSum* el : *sum) {
			el->R = 0;
		}
	}
}

void MergeReportBase::SumUp(FileD* file_d, std::vector<FrmlElemSum*>* S, void* record)
{
	if (S == nullptr) return;
	for (size_t i = 0; i < S->size(); i++) {
		S->at(i)->R += RunReal(file_d, S->at(i)->Frml, record);
	}
}
