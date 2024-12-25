#include "MergeReportBase.h"

#include "InpD.h"
#include "../Core/Compiler.h"
#include "../Core/FieldDescr.h"
#include "../Core/GlobalVariables.h"
#include "../Core/KeyFldD.h"
#include "../Core/RunFrml.h"

void MergeReportBase::SetInput(std::string& input)
{
	base_compiler->input_string = input;
}

void MergeReportBase::SetInput(std::string& s, bool ShowErr)
{
	base_compiler->SetInpStdStr(s, ShowErr);
}

void MergeReportBase::SetInput(RdbPos* rdb_pos, bool FromTxt)
{
	base_compiler->SetInpTT(rdb_pos, FromTxt);
}

void MergeReportBase::SetInput(FileD* file_d, int Pos, bool Decode)
{
	base_compiler->SetInpTTPos(file_d, Pos, Decode);
}

MergeReportBase::MergeReportBase()
{
	base_compiler = new Compiler();
	ChainSum = false;
	Ii = 0;
	Oi = 0;
	SumIi = 0;
	WhatToRd = '\0'; /*i=Oi output FDs;O=O outp.FDs*/
	NRecsAll = 0;
}

MergeReportBase::~MergeReportBase()
{
	delete base_compiler;
}

FileD* MergeReportBase::InpFD(WORD I)
{
	return IDA[I]->Scan->FD;
}

void MergeReportBase::TestNotSum()
{
	if (FrmlSumEl != nullptr) base_compiler->OldError(41);
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
	if ((base_compiler->ForwChar == '.') && (base_compiler->LexWord.length() == 2) && (base_compiler->LexWord[1] == 'I') &&
		(isdigit(base_compiler->LexWord[2]) && base_compiler->LexWord[2] != '0')) {
		Ii = base_compiler->LexWord[2] - '0';
		if ((Ii > MaxIi) || (WhatToRd == 'i') && (Ii > Oi)) {
			base_compiler->Error(9);
		}
		base_compiler->RdLex(); base_compiler->RdLex();
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

		FieldDescr* F = base_compiler->FindFldName(InpFD(Ii), fld_name);
		if (F == nullptr) {
			base_compiler->OldError(8);
		}
		if (!base_compiler->FldTypIdentity(M->FldD, F)) {
			base_compiler->OldError(12);
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
		base_compiler->OldError(30);
	}

	for (size_t i = 0; i < M1.size(); i++) {
		KeyFldD* m1 = M1[i];
		KeyFldD* m2 = M2[i];
		if (!base_compiler->FldTypIdentity(m1->FldD, m2->FldD)
			|| (m1->Descend != m2->Descend)
			|| (m1->CompLex != m2->CompLex))
			base_compiler->OldError(12);
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
		else if (SumIi != Ii) base_compiler->OldError(27);
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
