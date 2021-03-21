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

longint NRecsAll;

void RunMerge()
{
	integer I = 0, MinIi = 0, res = 0, NEof = 0;      /*RunMerge - body*/
	bool EmptyGroup = false, b = false;
	PushProcStk();
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
		if (OldMFlds == nullptr) {
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
		b = SaveCache(0, CFile->Handle);
		RunMsgOff();
		if (!b) GoExit();
		CloseInpOutp();
		PopProcStk();
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
	ConstListEl* C = nullptr;
	FieldDescr* F = nullptr;
	OldMXStr.Clear();
	*C = *OldMFlds;
	while (C != nullptr) {
		F = M->FldD;
		switch (F->FrmlTyp) {
		case 'S': {
			C->S = _ShortS(F);
			OldMXStr.StoreStr(C->S, M);
			break;
		}
		case 'R': {
			C->R = _R(F);
			OldMXStr.StoreReal(C->R, M);
			break;
		}
		default: {
			C->B = _B(F);
			OldMXStr.StoreBool(C->B, M);
			break;
		}
		}
		C = (ConstListEl*)C->Chain;
		M = (KeyFldD*)M->Chain;
	}
}

void SetMFlds(KeyFldD* M)
{
	FieldDescr* F = nullptr;
	ConstListEl* C = OldMFlds;
	while (M != nullptr)
	{
		F = M->FldD;
		switch (F->FrmlTyp) {
		case 'S': { S_(F, C->S); break; }
		case 'R': { R_(F, C->R); break; }
		default: { B_(F, C->B); break; }
		}
		M = (KeyFldD*)M->Chain;
		C = (ConstListEl*)C->Chain;
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

void RunAssign(AssignD* A)
{
	while (A != nullptr) {
		/* !!! with A^ do!!! */
		switch (A->Kind) {
		case _move: {
			if (A != nullptr && A->FromPtr != nullptr && A->ToPtr != nullptr) {
				Move(&A->FromPtr, &A->ToPtr, A->L);
			}
			break;
		}
		case _zero: {
			switch (A->FldD->FrmlTyp) {
			case 'S': S_(A->FldD, ""); break;
			case 'R': R_(A->FldD, 0); break;
			default: B_(A->FldD, false); break;
			}
			break;
		}
		case _output: AssgnFrml(A->OFldD, A->Frml, false, A->Add); break;
		case _locvar: LVAssignFrml(A->LV, MyBP, A->Add, A->Frml); break;
		case _parfile: AsgnParFldFrml(A->FD, A->PFldD, A->Frml, A->Add); break;
		case _ifthenelseM: {
			if (RunBool(A->Bool)) RunAssign(A->Instr);
			else RunAssign(A->ElseInstr);
			break;
		}
		}
		A = (AssignD*)A->Chain;
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
				ClearDeletedFlag();
				RunAssign(RD->Ass);
#ifdef FandSQL
				if (CFile->IsSQLFile) OD->Strm->PutRec;
				else
#endif
				{
					PutRec(CFile, CRecPtr);
					if (OD->Append && (CFile->Typ == 'X')) TryInsertAllIndexes(CFile->IRec);
				}
			}
		}
		RD = (OutpRD*)RD->Chain;
	}
}

void OpenInpM()
{
	NRecsAll = 0;
	for (integer I = 1; I <= MaxIi; I++)
		/* !!! with IDA[I]^ do!!! */ {
		CFile = IDA[I]->Scan->FD;
		if (IDA[I]->IsInplace) IDA[I]->Md = NewLMode(ExclMode);
		else IDA[I]->Md = NewLMode(RdMode);
		IDA[I]->Scan->ResetSort(IDA[I]->SK, IDA[I]->Bool, IDA[I]->Md, IDA[I]->SQLFilter);
		NRecsAll += IDA[I]->Scan->NRecs;
	}
}

void OpenOutp()
{
	OutpFD* OD = OutpFDRoot;
	while (OD != nullptr) {
		/* !!! with OD^ do!!! */
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
			OD = (OutpFD*)OD->Chain;
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
			else OldLMode(OD->Md);
		}
		OD = (OutpFD*)OD->Chain;
	}
	for (integer i = 1; i <= MaxIi; i++) /* !!! with IDA[i]^ do!!! */ {
		IDA[i]->Scan->Close();
		ClearRecSpace(IDA[i]->ForwRecPtr);
		OldLMode(IDA[i]->Md);
	}
}

void MoveForwToRecM(InpD* ID)
{
	/* !!! with ID^ do!!! */
	CFile = ID->Scan->FD;
	CRecPtr = CFile->RecPtr;
	Move(ID->ForwRecPtr, CRecPtr, CFile->RecLen + 1);
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
			C = (ChkD*)C->Chain;
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
					if (res == _lt) CFileError(607);
				}
			} while (res != _gt);
		else {
			CFile = ID->Scan->FD;
			CRecPtr = CFile->RecPtr;
			ZeroAllFlds();
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
					if (res == _lt) CFileError(607);
				}
			} while (res == _gt);
		}
		else {
			CFile = ID->Scan->FD;
			CRecPtr = CFile->RecPtr;
			EmptyGroup = true;
			ZeroAllFlds();
			SetMFlds(ID->MFld);
			JoinProc(Ii + 1, EmptyGroup);
		}
	}
}
