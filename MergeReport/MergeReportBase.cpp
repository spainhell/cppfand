#include "MergeReportBase.h"

#include "InpD.h"
#include "../Core/Compiler.h"
#include "../Core/FieldDescr.h"
#include "../Core/GlobalVariables.h"
#include "../Core/KeyFldD.h"
#include "../Core/RunFrml.h"

MergeReportBase::MergeReportBase()
{
	//base_compiler = new Compiler(input);
	ChainSum = false;
	Ii = 0;
	Oi = 0;
	SumIi = 0;
	WhatToRd = '\0'; /*i=Oi output FDs;O=O outp.FDs*/
	NRecsAll = 0;
}

MergeReportBase::~MergeReportBase()
{
	//delete base_compiler;
}

FileD* MergeReportBase::InpFD(WORD I)
{
	return IDA[I]->Scan->FD;
}

void MergeReportBase::TestNotSum()
{
	if (FrmlSumEl != nullptr) gc->OldError(41);
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
		IDA[Ii]->ErrTxtFrml = new FrmlElemString(_const, 0); // GetOp(_const, 256);
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
	if ((gc->ForwChar == '.') && (gc->LexWord.length() == 2) && (gc->LexWord[1] == 'I') &&
		(isdigit(gc->LexWord[2]) && gc->LexWord[2] != '0')) {
		Ii = gc->LexWord[2] - '0';
		if ((Ii > MaxIi) || (WhatToRd == 'i') && (Ii > Oi)) {
			gc->Error(9);
		}
		gc->RdLex(); gc->RdLex();
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
	//KeyFldD* M = IDA[Ii - 1]->MFld;
	//std::vector<std::string> vFieldNames;
	//std::vector<std::string>::iterator it;
	//while (M != nullptr) {
	for (KeyFldD* M : IDA[Ii - 1]->MFld) {
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

		FieldDescr* F = gc->FindFldName(InpFD(Ii), fld_name);
		if (F == nullptr) {
			gc->OldError(8);
		}
		if (!gc->FldTypIdentity(M->FldD, F)) {
			gc->OldError(12);
		}
		KeyFldD* MNew = new KeyFldD();
		//MNew->pChain = nullptr; // M->pChain;
		MNew->CompLex = M->CompLex;
		MNew->Descend = M->Descend;
		MNew->FldD = F;

		IDA[Ii]->MFld.push_back(MNew);
		

		//M = M->pChain;
	}
}

void MergeReportBase::CheckMFlds(std::vector<KeyFldD*>& M1, std::vector<KeyFldD*>& M2)
{
	if (M1.size() != M2.size()) {
		gc->OldError(30);
	}

	for (size_t i = 0; i < M1.size(); i++) {
		KeyFldD* m1 = M1[i];
		KeyFldD* m2 = M2[i];
		if (!gc->FldTypIdentity(m1->FldD, m2->FldD)
			|| (m1->Descend != m2->Descend)
			|| (m1->CompLex != m2->CompLex))
			gc->OldError(12);
	}

	//while (M1 != nullptr) {
	//	//if (M2 == nullptr) g_compiler->OldError(30);
	//	if (!g_compiler->FldTypIdentity(M1->FldD, M2->FldD)
	//		|| (M1->Descend != M2->Descend)
	//		|| (M1->CompLex != M2->CompLex))
	//		g_compiler->OldError(12);
	//	M1 = M1->pChain;
	//	M2 = M2->pChain;
	//}

	//if (M2 != nullptr) {
	//	g_compiler->OldError(30);
	//}
}

void MergeReportBase::TestSetSumIi()
{
	if ((FrmlSumEl != nullptr) && (Ii != 0))
		if (FrstSumVar || (SumIi == 0)) SumIi = Ii;
		else if (SumIi != Ii) gc->OldError(27);
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
