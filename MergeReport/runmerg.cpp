#include "runmerg.h"
#include "shared.h"
#include "../cppfand/ChkD.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/legacy.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obaseww.h"
#include "../cppfand/runfrml.h"
#include "../Common/compare.h"
#include "../fandio/files.h"

int NRecsAll;

void RunMerge()
{
	short I = 0, MinIi = 0, res = 0, NEof = 0;      /*RunMerge - body*/
	bool EmptyGroup = false, b = false;
	//PushProcStk();
	OpenInpM();
	OpenOutp();
	MergOpGroup.Group = 1.0;
	RunMsgOn('M', NRecsAll);
	NRecsAll = 0;
	for (I = 1; I <= MaxIi; I++) ReadInpFileM(IDA[I]);
label1:
	MinIi = 0; NEof = 0;
	for (I = 1; I <= MaxIi; I++) /* !!! with IDA[I]^ do!!! */ {
		CFile = IDA[I]->Scan->FD;
		IDA[I]->IRec = IDA[I]->Scan->IRec;
		ZeroSumFlds(IDA[I]->Sum);
		if (IDA[I]->Scan->eof) NEof++;
		if (OldMFlds.empty()) {
			IDA[I]->Exist = !IDA[I]->Scan->eof;
			MinIi = 1;
		}
		else {
			CRecPtr = IDA[I]->ForwRecPtr;
			IDA[I]->Exist = false;
			IDA[I]->Count = 0.0;
			if (!IDA[I]->Scan->eof) {
				if (MinIi == 0) goto label2;
				res = CompMFlds(IDA[I]->MFld);
				if (res != _gt) {
					if (res == _lt)
					{
					label2:
						SetOldMFlds(IDA[I]->MFld);
						MinIi = I;
					}
					IDA[I]->Exist = true;
				}
			}
		}
	}
	for (I = 1; I <= MinIi - 1; I++) {
		IDA[I]->Exist = false;
	}
	if (NEof == MaxIi) {
		b = SaveCache(0, CFile->FF->Handle);
		RunMsgOff();
		if (!b) GoExit();
		CloseInpOutp();
		//PopProcStk();
		return;
	}
	EmptyGroup = false;
	if (Join) {
		JoinProc(1, EmptyGroup);
	}
	else {
		MergeProcM();
	}
	if (!EmptyGroup) {
		WriteOutp(OutpRDs);
		MergOpGroup.Group = MergOpGroup.Group + 1.0;
	}
	goto label1;
}

WORD CompMFlds(KeyFldD* M)
{
	XString x;
	x.PackKF(M);
	return CompStr(x.S, OldMXStr.S);
}

void SetOldMFlds(KeyFldD* M)
{
	//ConstListEl* C = nullptr;
	FieldDescr* F = nullptr;
	OldMXStr.Clear();
	//C = OldMFlds;
	for (auto& C : OldMFlds) { //while (C != nullptr) {
		F = M->FldD;
		switch (F->frml_type) {
		case 'S': {
			C.S = _ShortS(F);
			OldMXStr.StoreStr(C.S, M);
			break;
		}
		case 'R': {
			C.R = _R(F);
			OldMXStr.StoreReal(C.R, M);
			break;
		}
		default: {
			C.B = CFile->_B(F, CRecPtr);
			OldMXStr.StoreBool(C.B, M);
			break;
		}
		}
		//C = (ConstListEl*)C->pChain;
		M = (KeyFldD*)M->pChain;
	}
}

void SetMFlds(KeyFldD* M)
{
	FieldDescr* F = nullptr;
	std::vector<ConstListEl>::iterator it0 = OldMFlds.begin();
	while (M != nullptr)
	{
		F = M->FldD;
		switch (F->frml_type) {
		case 'S': { S_(CFile, F, it0->S); break; }
		case 'R': { CFile->R_(F, it0->R, CRecPtr); break; }
		default: { CFile->B_(F, it0->B, CRecPtr); break; }
		}
		M = (KeyFldD*)M->pChain;
		
		if (it0 != OldMFlds.end()) it0++;
	}
}

void ReadInpFileM(InpD* ID)
{
	/* !!! with ID^ do!!! */
	CRecPtr = ID->ForwRecPtr;
label1:
	ID->Scan->GetRec();
	if (ID->Scan->eof) return;
	NRecsAll++;
	RunMsgN(NRecsAll);
	if (!RunBool(ID->Bool)) goto label1;
}

void RunAssign(std::vector<AssignD*> Assigns)
{
	for (auto* A : Assigns) {
		/* !!! with A^ do!!! */
		switch (A->Kind) {
		case _move: {
			if (A != nullptr && A->FromPtr != nullptr && A->ToPtr != nullptr) {
				memcpy(A->ToPtr, A->FromPtr, A->L);
			}
			break;
		}
		case _zero: {
			switch (A->outputFldD->frml_type) {
			case 'S': { S_(CFile, A->outputFldD, ""); break; }
			case 'R': { CFile->R_(A->outputFldD, 0, CRecPtr); break; }
			default: { CFile->B_(A->outputFldD, false, CRecPtr); break; }
			}
			break;
		}
		case _output: {
			AssgnFrml(CFile, CRecPtr, A->OFldD, A->Frml, false, A->Add);
			break;
		}
		case _locvar: {
			LVAssignFrml(A->LV, A->Add, A->Frml);
			break;
		}
		case _parfile: {
			AsgnParFldFrml(A->FD, A->PFldD, A->Frml, A->Add);
			break;
		}
		case _ifthenelseM: {
			if (RunBool(A->Bool)) {
				RunAssign(A->Instr);
			}
			else {
				RunAssign(A->ElseInstr);
			}
			break;
		}
		}
	}
}

void WriteOutp(OutpRD* RD)
{
	OutpFD* OD;
	while (RD != nullptr) {
		if (RunBool(RD->Bool)) {
			OD = RD->OD;
			if (OD == nullptr /*dummy */) RunAssign(RD->Ass);
			else {
				CFile = OD->FD;
				CRecPtr = OD->RecPtr;
				CFile->ClearDeletedFlag(CRecPtr);
				RunAssign(RD->Ass);
#ifdef FandSQL
				if (CFile->IsSQLFile) OD->Strm->PutRec;
				else
#endif
				{
					CFile->PutRec(CRecPtr);
					if (OD->Append && (CFile->FF->file_type == FileType::INDEX)) TryInsertAllIndexes(CFile->IRec);
				}
			}
}
		RD = RD->pChain;
	}
}

void OpenInpM()
{
	NRecsAll = 0;
	for (short I = 1; I <= MaxIi; I++)
		/* !!! with IDA[I]^ do!!! */ {
		CFile = IDA[I]->Scan->FD;
		if (IDA[I]->IsInplace) IDA[I]->Md = CFile->NewLockMode(ExclMode);
		else IDA[I]->Md = CFile->NewLockMode(RdMode);
		IDA[I]->Scan->ResetSort(IDA[I]->SK, IDA[I]->Bool, IDA[I]->Md, IDA[I]->SQLFilter);
		NRecsAll += IDA[I]->Scan->NRecs;
	}
}

void OpenOutp()
{
	OutpFD* OD = OutpFDRoot;
	while (OD != nullptr) {
		CFile = OD->FD;
#ifdef FandSQL
		if (CFile->IsSQLFile) {
			New(Strm, Init);
			Strm->OutpRewrite(OD->Append);
			CRecPtr = OD->RecPtr;
			SetTWorkFlag();
		}
		else
#endif
		{
			if (OD->InplFD != nullptr) OD->FD = OpenDuplF(true);
			else OD->Md = RewriteF(OD->Append);
			OD = OD->pChain;
		}
	}
}

void CloseInpOutp()
{
	OutpFD* OD = OutpFDRoot;
	while (OD != nullptr) /* !!! with OD^ do!!! */ {
		CFile = OD->FD;
		ClearRecSpace(OD->RecPtr);
#ifdef FandSQL
		if (CFile->IsSQLFile) /* !!! with Strm^ do!!! */ {
			OutpClose(); Done();
		}
		else
#endif
		{
			if (OD->InplFD != nullptr) {
				CFile = OD->InplFD;
				SubstDuplF(OD->FD, true);
			}
			else CFile->OldLockMode(OD->Md);
		}
		OD = (OutpFD*)OD->pChain;
	}
	for (short i = 1; i <= MaxIi; i++) /* !!! with IDA[i]^ do!!! */ {
		IDA[i]->Scan->Close();
		ClearRecSpace(IDA[i]->ForwRecPtr);
		CFile->OldLockMode(IDA[i]->Md);
	}
	}

void MoveForwToRecM(InpD* ID)
{
	/* !!! with ID^ do!!! */
	CFile = ID->Scan->FD;
	CRecPtr = CFile->FF->RecPtr;
	Move(ID->ForwRecPtr, CRecPtr, CFile->FF->RecLen + 1);
	ID->Count = ID->Count + 1;
	ChkD* C = ID->Chk;
	if (C != nullptr) {
		ID->Error = false;
		ID->Warning = false;
		ID->ErrTxtFrml->S[0] = 0;
		while (C != nullptr) {
			if (!RunBool(C->Bool)) {
				ID->Warning = true;
				ID->ErrTxtFrml->S = RunShortStr(C->TxtZ);
				if (!C->Warning) { ID->Error = true; return; }
			}
			C = (ChkD*)C->pChain;
		}
	}
}

void MergeProcM()
{
	WORD res = 0;
	for (WORD i = 1; i <= MaxIi; i++) {
		InpD* ID = IDA[i];
		/* !!! with ID^ do!!! */
		if (ID->Exist)
			do {
				MoveForwToRecM(ID);
				SumUp(ID->Sum);
				WriteOutp(ID->RD);
				ReadInpFileM(ID);
				if (ID->Scan->eof) res = _gt;
				else {
					res = CompMFlds(ID->MFld);
					if (res == _lt) CFileError(CFile, 607);
				}
			} while (res != _gt);
		else {
			CFile = ID->Scan->FD;
			CRecPtr = CFile->FF->RecPtr;
			ZeroAllFlds(CFile, CRecPtr);
			SetMFlds(ID->MFld);
		}
	}
}

void JoinProc(WORD Ii, bool& EmptyGroup)
{
	if (Ii > MaxIi) {
		if (!EmptyGroup) {
			for (WORD I = 1; I <= MaxIi; I++) {
				SumUp(IDA[I]->Sum);
			}
			WriteOutp(IDA[MaxIi]->RD);
		}
	}
	else {
		InpD* ID = IDA[Ii]; /* !!! with ID^ do!!! */
		if (ID->Exist) {
			ID->Scan->SeekRec(ID->IRec - 1);
			ID->Count = 0.0;
			CRecPtr = ID->ForwRecPtr;
			ID->Scan->GetRec();
			WORD res;
			do {
				MoveForwToRecM(ID);
				JoinProc(Ii + 1, EmptyGroup);
				ReadInpFileM(ID);
				if (ID->Scan->eof) {
					res = _gt;
				}
				else {
					res = CompMFlds(ID->MFld);
					if (res == _lt) CFileError(CFile, 607);
				}
			} while (res == _gt);
		}
		else {
			CFile = ID->Scan->FD;
			CRecPtr = CFile->FF->RecPtr;
			EmptyGroup = true;
			ZeroAllFlds(CFile, CRecPtr);
			SetMFlds(ID->MFld);
			JoinProc(Ii + 1, EmptyGroup);
		}
	}
}
