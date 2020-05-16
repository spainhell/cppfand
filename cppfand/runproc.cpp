#include "runproc.h"

#include "compile.h"
#include "drivers.h"
#include "expimp.h"
#include "genrprt.h"
#include "legacy.h"
#include "oaccess.h"
#include "obase.h"
#include "obaseww.h"
#include "olongstr.h"
#include "printtxt.h"
#include "rdfildcl.h"
#include "rdmerg.h"
#include "rdproc.h"
#include "rdrprt.h"
#include "runedi.h"
#include "runfrml.h"
#include "runmerg.h"
#include "runproj.h"
#include "runrprt.h"
#include "wwmenu.h"
#include "wwmix.h"


void UserHeadLine(pstring UserHeader)
{
	WORD n, l, maxlen; WParam* p;
	p = PushWParam(1, 1, TxtCols, 1, true);
	TextAttr = colors.fNorm;
	ClrEol();
	maxlen = TxtCols - 10;
	l = LenStyleStr(UserHeader);
	if (l >= maxlen) {
		UserHeader[0] = char(maxlen);
		l = LenStyleStr(UserHeader);
	}
	n = (TxtCols - l) / 2;
	if (n > 0) printf("%*c", n, ' ');
	WrStyleStr(UserHeader, colors.fNorm);
	screen.GotoXY(TxtCols - 10, 1);
	printf("%s", StrDate(Today(), "DD.MM.YYYY").c_str());
	PopWParam(p);
	ReleaseStore(p);
}

void ReportProc(RprtOpt* RO, bool save)
{
	void* p = nullptr; void* p2 = nullptr; char md; longint w; ExitRecord er;
	MarkBoth(p, p2); PrintView = false;
	/* !!! with RO^ do!!! */
	if (RO->Flds == nullptr) {
		SetInpTT(RO->RprtPos, true);
		if (RO->SyntxChk) {
			IsCompileErr = false;
			//NewExit(Ovr(), er);
			goto label1;
			ReadReport(RO);
			LastExitCode = 0;
		label1:
			RestoreExit(er); IsCompileErr = false; goto label2;
		}
		ReadReport(RO); RunReport(RO);
	}
	else {
		if (RO->WidthFrml != nullptr) RO->Width = RunInt(RO->WidthFrml);
		if (RO->Head != nullptr) RO->HeadTxt = RunLongStr(RO->Head);
		if (RO->UserSelFlds) PromptAutoRprt(RO);
		else RunAutoReport(RO);
	}
	if (RO->Edit) md = 'T'; else md = 'V';
	if (save) SaveFiles();
	if (PrintView) {
		w = PushW(1, 1, TxtCols, TxtRows);
		SetPrintTxtPath();
		pstring tmp;
		EditTxtFile(nullptr, md, tmp, nullptr, 0, 0, nullptr, 0, "", 0, nullptr);
		PopW(w);
	}
label2:
	ReleaseBoth(p, p2);
}

void PromptAutoRprt(RprtOpt* RO)
{
	wwmix ww;
	
	FieldList FL; FieldDPtr F; RprtOpt* RO2;
	RO2 = (RprtOpt*)GetStore(sizeof(*RO)); Move(RO, RO2, sizeof(*RO));
	FL = RO->Flds;
	while (FL != nullptr)
	{
		F = FL->FldD;
		if (F->Flg && f_Stored != 0) ww.PutSelect(F->Name);
		else
		{
			pstring tmpStr = SelMark;
			ww.PutSelect(tmpStr + F->Name);
		}
		FL = FL->Chain;
	}
	CFile = RO->FDL.FD; if (!ww.SelFieldList(36, true, RO2->Flds)) return;
	if ((RO->FDL.Cond == nullptr) &&
		!ww.PromptFilter("", RO2->FDL.Cond, RO2->CondTxt)) return;
	if (SelForAutoRprt(RO2)) RunAutoReport(RO2);
}

void AssignField(Instr* PD)
{
	longint N; LockMode md; WORD msg; FieldDPtr F;
	CFile = PD->FD; md = NewLMode(WrMode); F = PD->FldD;
	N = RunInt(PD->RecFrml);
	if ((N <= 0) || (N > CFile->NRecs)) { msg = 640; goto label1; }
	CRecPtr = GetRecSpace(); ReadRec(N);
	if (PD->Indexarg && !DeletedFlag()) {
		msg = 627;
	label1:
		Set2MsgPar(CFile->Name, F->Name); RunErrorM(md, msg);
	}
	AssgnFrml(F, PD->Frml, true, PD->Add); WriteRec(N);
	ReleaseStore(CRecPtr); OldLMode(md);
}

void AssignRecVar(LocVar* LV1, LocVar* LV2, AssignD* A)
{
	pstring EmptyStr(1);
	EmptyStr = "";
	FieldDPtr F; FrmlPtr Z; LongStr* S; pstring ss; bool b; double r;
	FileDPtr FD1, FD2; void* RP1; void* RP2;
	FD1 = LV1->FD; FD2 = LV2->FD; RP1 = LV1->RecPtr; RP2 = LV2->RecPtr;
	while (A != nullptr) {
		switch (A->Kind) {
		case _zero: {
			F = A->FldD;
			CFile = FD1;
			CRecPtr = RP1;
			switch (F->FrmlTyp) {
			case 'S': S_(F, EmptyStr); break;
			case 'R': R_(F, 0.0); break;
			default: B_(F, false); break;
			}
			break;
		}
		case _output: {
			CFile = FD1; CRecPtr = RP1; A->Frml->NewRP = RP2;
			AssgnFrml(A->OFldD, A->Frml, false, false);
			break;
		}
		}
		A = A->Chain;
	}
	CFile = FD1; CRecPtr = RP1;
	SetUpdFlag();
}

void AssignRecFld(Instr* PD)
{
	FieldDPtr F = PD->RecFldD;
	CFile = PD->AssLV->FD; CRecPtr = PD->AssLV->RecPtr;
	SetUpdFlag();
	AssgnFrml(F, PD->Frml, HasTWorkFlag(), PD->Add);
}

void SortProc(FileDPtr FD, KeyFldDPtr SK)
{
	LockMode md;
	CFile = FD;
	md = NewLMode(ExclMode);
	SortAndSubst(SK); CFile = FD; OldLMode(md); SaveFiles;
}

void MergeProc(Instr* PD)
{
	void* p = nullptr; void* p2 = nullptr;
	MarkBoth(p, p2); SetInpTT(PD->Pos, true);
	ReadMerge(); RunMerge(); SaveFiles(); ReleaseBoth(p, p2);
}

void WritelnProc(Instr* PD)
{
	LongStr* S; WORD i; char c; BYTE LF; WrLnD* W; pstring t, x; double r;
	W = &PD->WD; LF = PD->LF; t[0] = 0;
	TextAttr = ProcAttr;
	while (W != nullptr) {
		switch (W->Typ) {
		case 'S': {
			if (LF >= 2) t = t + RunShortStr(W->Frml);
			else {
				S = RunLongStr(W->Frml); WrLongStyleStr(S, ProcAttr); ReleaseStore(S);
			}
			goto label1; break;
		}
		case 'B': {
			if (RunBool(W->Frml)) x = AbbrYes;
			else x = AbbrNo;
			break;
		}
		case 'F': {
			r = RunReal(W->Frml);
			if (W->M == 255) str(r, W->N, x);
			else str(r, W->N, W->M, x);
			break;
		}
		case 'D': x = StrDate(RunReal(W->Frml), *W->Mask); break;
		}
		if (LF >= 2) t = t + x;
		else printf("%s", x.c_str());
	label1:
		W = W->Chain;
	}
label2:
	switch (LF) {
	case 1: printf("\n"); break;
	case 3: { F10SpecKey = _F1_; goto label3; break; }
	case 2: {
	label3:
		SetMsgPar(t); WrLLF10Msg(110);
		if (KbdChar == _F1_) {
			Help(PD->mHlpRdb, RunShortStr(PD->mHlpFrml), false); goto label2;
		}
		break;
	}
	}
}

void DisplayProc(RdbDPtr R, WORD IRec)
{
	LongStr* S = nullptr; void* p = nullptr; WORD i;
	MarkStore(p);
	if (IRec == 0) {
		S = GetHlpText(CRdb, RunShortStr(FrmlPtr(R)), true, i);
		if (S == nullptr) goto label1;
	}
	else {
		CFile = R->FD; CRecPtr = Chpt->RecPtr; ReadRec(IRec);
		S = CFile->TF->Read(1, _T(ChptTxt));
		if (R->Encrypted) CodingLongStr(S);
	}
	WrLongStyleStr(S, ProcAttr);
label1:
	ReleaseStore(p);
}

void ClrWwProc(Instr* PD)
{
	WRect v; WORD a; pstring s; char c;
	RunWFrml(PD->W, 0, v); a = RunWordImpl(PD->Attr, colors.uNorm); c = ' ';
	if (PD->FillC != nullptr) {
		s = RunShortStr(PD->FillC); if (s.length() > 0) c = s[1];
	}
	screen.ScrClr(v.C1 - 1, v.R1 - 1, v.C2 - v.C1 + 1, v.R2 - v.R1 + 1, c, a);
}

void ExecPgm(Instr* PD)
{
	pstring s; pstring Prog; WORD i; BYTE x = 0, y = 0; bool b = false;
	Wind wmin, wmax; longint w;
	wmin = WindMin;
	wmax = WindMax;
	TCrs crs = screen.CrsGet();
	w = PushW(1, 1, TxtCols, 1);
	WindMin = wmin;
	WindMax = wmax;
	screen.CrsSet(crs);
	s = RunShortStr(PD->Param); i = PD->ProgCatIRec; CVol = "";
	if (i != 0) Prog = RdCatField(i, CatPathName); else Prog = *PD->ProgPath;
	b = OSshell(Prog, s, PD->NoCancel, PD->FreeMm, PD->LdFont, PD->TextMd);
	/*asm mov ah, 3; mov bh, 0; push bp; int 10H; pop bp; mov x, dl; mov y, dh;*/
	PopW(w);
	screen.GotoXY(x - WindMin.X + 1, y - WindMin.Y + 1);
	if (!b) GoExit();
}

void CallRdbProc(Instr* PD)
{
	bool b; void* p = nullptr; ProcStkPtr bp = nullptr;
	MarkStore(p); bp = MyBP;
	b = EditExecRdb(PD->RdbNm, PD->ProcNm, PD->ProcCall);
	SetMyBP(bp); ReleaseStore(p); if (!b) GoExit();
}

void IndexfileProc(FileDPtr FD, bool Compress)
{
	FileDPtr FD2, cf; longint I; LockMode md;
	cf = CFile; CFile = FD; md = NewLMode(ExclMode);
	XFNotValid(); CRecPtr = GetRecSpace();
	if (Compress) {
		FD2 = OpenDuplF(false); for (I = 1; I < FD->NRecs; I++)
		{
			CFile = FD; ReadRec(I); if (!DeletedFlag())
			{
				CFile = FD2; PutRec();
			};
		}
		if (not SaveCache(0)) GoExit(); CFile = FD; SubstDuplF(FD2, false);
	}
	CFile->XF->NoCreate = false; TestXFExist();
	OldLMode(md); SaveFiles;
	ReleaseStore(CRecPtr); CFile = cf;
}

void MountProc(WORD CatIRec, bool NoCancel)
{
	ExitRecord er;
	//NewExit(Ovr, er);
	goto label1;
	SaveFiles();
	RdCatPathVol(CatIRec); TestMountVol(CPath[1]);
	LastExitCode = 0; RestoreExit(er); return;
label1:
	RestoreExit(er);
	if (NoCancel) LastExitCode = 1;
	else GoExit();
}

void EditProc(Instr* PD)
{
	EditOpt* EO;
	EdUpdated = false; SaveFiles(); CFile = PD->EditFD;
	EO = (EditOpt*)GetStore(sizeof(*EO));
	Move(PD->EO, EO, sizeof(*EO));
	if (!EO->UserSelFlds || SelFldsForEO(EO, nullptr)) EditDataFile(CFile, EO);
	SaveFiles; ReleaseStore(EO);
}

void EditTxtProc(Instr* PD)
{
	longint i; WRect v; WRect* pv = nullptr; BYTE a; longint* lp;
	MsgStr MsgS; void* p = nullptr; pstring msg;
	MarkStore(p);
	i = 1; if (PD->TxtPos != nullptr) i = RunInt(PD->TxtPos); EdUpdated = false;
	a = RunWordImpl(PD->Atr, 0);
	pv = nullptr;
	if (PD->Ww.C1 != nullptr) { RunWFrml(PD->Ww, PD->WFlags, v); pv = &v; }
	MsgS.Head = GetStr(PD->Head); MsgS.Last = *GetStr(PD->Last);
	MsgS.CtrlLast = *GetStr(PD->CtrlLast); MsgS.ShiftLast = *GetStr(PD->ShiftLast);
	MsgS.AltLast = *GetStr(PD->AltLast);

	if (PD->TxtLV != nullptr) lp = (longint*)(uintptr_t(MyBP) + PD->TxtLV->BPOfs);
	else { SetTxtPathVol(*PD->TxtPath, PD->TxtCatIRec); lp = nullptr; }
	msg = ""; if (PD->ErrMsg != nullptr) msg = RunShortStr(PD->ErrMsg);
	EditTxtFile(lp, PD->EdTxtMode, msg, PD->ExD, i, RunInt(PD->TxtXY), pv, a, RunShortStr(PD->Hd), PD->WFlags, &MsgS);
	ReleaseStore(p);

}

pstring* GetStr(FrmlPtr Z)
{
	pstring s;
	if (Z == nullptr) return nullptr;
	s = RunShortStr(Z);
	return StoreStr(s);
}

void PrintTxtProc(Instr* PD)
{
	LongStr* s;
	/* !!! with PD^ do!!! */
	if (PD->TxtLV != nullptr) {
		s = TWork.Read(1, *(longint*)(uintptr_t(MyBP) + PD->TxtLV->BPOfs));
		PrintArray(s->A, s->LL, false);
		ReleaseStore(s);
	}
	else {
		SetTxtPathVol(*PD->TxtPath, PD->TxtCatIRec);
		PrintTxtFile(0);
	}
}

bool SrchXKey(KeyDPtr K, XString& X, longint& N)
{
	void* cr;
	if (CFile->Typ == 'X') {
		TestXFExist();
		return K->SearchIntvl(X, false, N);
	}
	else {
		cr = CRecPtr; CRecPtr = GetRecSpace();
		auto result = SearchKey(X, K, N);
		ReleaseStore(CRecPtr);
		CRecPtr = cr;
		return result;
	}
}

void DeleteRecProc(Instr* PD)
{
	LockMode md; longint n; XString x;
	CFile = PD->RecFD; CRecPtr = GetRecSpace();
	if (PD->ByKey) {
		x.S = RunShortStr(PD->RecNr);
#ifdef FandSQL
		if (CFile->IsSQLFile) { Strm1->DeleteXRec(PD->Key, &x, PD->AdUpd); goto label2; }
#endif
	}
	md = NewLMode(DelMode);
	if (PD->ByKey)
	{
		if (!SrchXKey(PD->Key, x, n)) goto label1;
	}
	else
	{
		n = RunInt(PD->RecNr);
		if ((n <= 0) || (n > CFile->NRecs)) goto label1;
	}
	ReadRec(n);
	if (PD->AdUpd && !DeletedFlag()) LastExitCode = (!RunAddUpdte('-', nullptr, nullptr));
	if (CFile->Typ == 'X') { if (!DeletedFlag()) DeleteXRec(n, true); }
	else DeleteRec(n);
label1:
	OldLMode(md);
label2:
	ReleaseStore(CRecPtr);
}

void AppendRecProc()
{
	LockMode md;
	md = NewLMode(CrMode); CRecPtr = GetRecSpace(); ZeroAllFlds();
	SetDeletedFlag(); CreateRec(CFile->NRecs + 1);
	ReleaseStore(CRecPtr); OldLMode(md);
}

void UpdRec(void* CR, longint N, bool AdUpd)
{
	void* cr2; bool del;
	cr2 = GetRecSpace(); CRecPtr = cr2; ReadRec(N);
	del = DeletedFlag(); CRecPtr = CR;
	if (AdUpd)
		if (del) LastExitCode = !RunAddUpdte('+', nullptr, nullptr);
		else LastExitCode = !RunAddUpdte('d', cr2, nullptr);
	if (CFile->Typ == 'X') OverWrXRec(N, cr2, CR); else WriteRec(N);
	if (!del) DelAllDifTFlds(cr2, nullptr);
	ReleaseStore(cr2);
}

void ReadWriteRecProc(bool IsRead, Instr* PD)
{
	longint N; bool app, ad; LockMode md; void* cr; XString x;
	KeyDPtr k; WORD msg;
	/* !!! with PD->LV^ do!!! */
	CFile = PD->LV->FD; CRecPtr = PD->LV->RecPtr; N = 1; k = PD->Key; ad = PD->AdUpd;
	md = CFile->LMode; app = false; cr = GetRecSpace();
	if (PD->ByKey) {
		x.S = RunShortStr(PD->RecNr);
#ifdef FandSQL
		if (CFile->IsSQLFile) {
			if (IsRead) if (Strm1->SelectXRec(k, @x, PD->CompOp, true)) goto label4; else goto label2;
			else if (Strm1->UpdateXRec(k, @x, ad)) goto label4; else goto label2;
		}
#endif
	}
	else N = RunInt(PD->RecNr);
	if (IsRead) {
		if (N == 0) goto label0;
		else NewLMode(RdMode);
	}
	else if (N == 0) {

#ifdef FandSQL
		if (CFile->IsSQLFile) { Strm1->InsertRec(ad, true); goto label4; }
#endif
		goto label1;
	}
	else NewLMode(WrMode);
	if (PD->ByKey) {
		if (k == nullptr/*IsParFile*/) {
			if (CFile->NRecs == 0)
				if (IsRead) {
				label0:
					DelTFlds(); ZeroAllFlds();
					goto label4;
				}
				else {
				label1:
					NewLMode(CrMode); TestXFExist(); IncNRecs(1); app = true;
				}
			N = CFile->NRecs;
		}
		else if (not SrchXKey(k, x, N)) {
		label2:
			if (IsRead) { DelTFlds(); ZeroAllFlds(); SetDeletedFlag(); goto label4; }
			msg = 613; goto label3;
		}
	}
	else if ((N <= 0) || (N > CFile->NRecs)) {
		msg = 641;
	label3:
		SetMsgPar(PD->LV->Name); RunErrorM(md, msg);
	}
	if (IsRead) {
		CRecPtr = cr; ReadRec(N);
		CRecPtr = PD->LV->RecPtr; DelTFlds(); CopyRecWithT(cr, PD->LV->RecPtr);
	}
	else {
		CopyRecWithT(PD->LV->RecPtr, cr);
		if (app) {
			CRecPtr = cr; if (CFile->Typ = 'X') RecallRec(N); else WriteRec(N);
			if (ad) LastExitCode = !RunAddUpdte('+', nullptr, nullptr);
		}
		else UpdRec(cr, N, ad);
	}
label4:
	ReleaseStore(cr); OldLMode(md);
}

void LinkRecProc(Instr* PD)
{
	void* p = nullptr; void* r2 = nullptr; void* lr2 = nullptr;
	FileDPtr cf = nullptr; void* cr = nullptr; LinkDPtr ld = nullptr; longint n;
	cf = CFile; cr = CRecPtr; MarkStore(p);
	ld = PD->LinkLD; CRecPtr = PD->RecLV1->RecPtr;
	lr2 = PD->RecLV2->RecPtr;
	CFile = ld->ToFD; ClearRecSpace(lr2); CFile = ld->FromFD;
	if (LinkUpw(ld, n, true)) LastExitCode = 0;
	else LastExitCode = 1;
	r2 = CRecPtr; CRecPtr = lr2; DelTFlds(); CopyRecWithT(r2, lr2);
	ReleaseStore(p); CFile = cf; CRecPtr = cr;
}

void ForAllProc(Instr* PD)
{
	FileDPtr FD = nullptr; KeyDPtr Key = nullptr, k = nullptr; FrmlPtr Bool = nullptr;
	LinkDPtr LD = nullptr; KeyInD* KI = nullptr;
	void* cr = nullptr; void* p = nullptr; void* lr = nullptr;
	XScan* Scan = nullptr; LockMode md, md1; XString xx;
	KeyFldDPtr KF = nullptr; LocVar* LVi = nullptr; LocVar* LVr = nullptr;
	bool lk, b;
#ifdef FandSQL
	bool sql;
#endif
	MarkStore(p); FD = PD->CFD; Key = PD->CKey; LVi = PD->CVar; LVr = &PD->CRecVar;
	LD = PD->CLD; KI = PD->CKIRoot; Bool = RunEvalFrml(PD->CBool); lk = false;
#ifdef FandSQL
	if (PD->inSQL && !FD->IsSQLFile) return;
#endif
	if (LD != nullptr) {
		CFile = LD->ToFD; KF = LD->ToKey->KFlds;
		switch (PD->COwnerTyp) {
		case 'r': { CRecPtr = PD->CLV->RecPtr; xx.PackKF(KF); break; }
		case 'F': { md = NewLMode(RdMode); CRecPtr = GetRecSpace();
			ReadRec(RunInt(FrmlPtr(PD->CLV))); xx.PackKF(KF);
			ReleaseStore(p); OldLMode(md);
			break;
		}
		}
	}
	CFile = FD;
#ifdef FandSQL
	sql = CFile->IsSQLFile;
#endif
	md = NewLMode(RdMode);
	cr = GetRecSpace(); CRecPtr = cr; lr = cr;
	//New(Scan, Init(CFile, Key, KI, true));
	Scan = new XScan(CFile, Key, KI, true);
#ifdef FandSQL
	if (PD->inSQL) Scan->ResetSQLTxt(Bool); else
#endif
		if (LD != nullptr)
			if (PD->COwnerTyp == 'i') Scan->ResetOwnerIndex(LD, PD->CLV, Bool);
			else Scan->ResetOwner(&xx, Bool);
		else Scan->Reset(Bool, PD->CSQLFilter);
#ifdef FandSQL
	if (!CFile->IsSQLFile)
#endif
		if (Key != nullptr)
			if (PD->CWIdx) ScanSubstWIndex(Scan, Key->KFlds, 'W');
			else { CFile->XF->UpdLockCnt++; lk = true; }
	if (LVr != nullptr) lr = LVr->RecPtr; k = CFile->Keys;
	b = PD->CProcent; if (b) RunMsgOn('F', Scan->NRecs);
label1:
#ifdef FandSQL
	if (sql) CRecPtr = lr else
#endif
		CRecPtr = cr;
	Scan->GetRec(); if (b) RunMsgN(Scan->IRec);
	if (!Scan->eof) {
#ifdef FandSQL

		if (sql) { ClearUpdFlag; if (k != nullptr) xx.PackKF(k->KFlds) }
		else
#endif
			if (LVr != nullptr) {
				CRecPtr = lr; ClearUpdFlag(); DelTFlds(); CopyRecWithT(cr, lr);
			}
		if (LVi != nullptr) *(double*)(LocVarAd(LVi)) = Scan->RecNr;
		RunInstr(PD->CInstr); CFile = FD; CRecPtr = lr;
#ifdef FandSQL
		if (sql) {
			if (HasUpdFlag && !PD->inSQL) {
				if (k = nullptr) CFileError(650); Strm1->UpdateXRec(k, @xx, CFile->Add != nullptr);
			}
		}
		else
#endif
		{
			OpenCreateF(Shared);
			if ((LVr != nullptr) && (LVi == nullptr) && HasUpdFlag()) {
				md1 = NewLMode(WrMode); CopyRecWithT(lr, cr);
				UpdRec(cr, Scan->RecNr, true); OldLMode(md1);
			};
		}
		if (not (ExitP || BreakP)) {
			if (
#ifdef FandSQL
				!sql &&
#endif 
				(Key = nullptr) && (Scan->NRecs > CFile->NRecs)) {
				Scan->IRec--; Scan->NRecs--;
			}
			goto label1;
		}
	}
	if (lk) CFile->XF->UpdLockCnt--;
	Scan->Close(); OldLMode(md);
	if (b) RunMsgOff();
	ReleaseStore(p);
	BreakP = false;
}

void HeadLineProc(FrmlPtr Z)
{
	UserHeadLine(RunShortStr(Z));
}

void SetKeyBufProc(FrmlPtr Z)
{
	KbdBuffer = RunShortStr(Z);
}

void SetWwViewPort()
{
	WORD x1, x2, y1, y2;
#ifdef FandGraph
	if (IsGraphMode) {
		RectToPixel(WindMin.X, WindMin.Y, WindMax.X, WindMax.Y, x1, y1, x2, y2);
		SetViewPort(x1, y1, x2, y2, true);
	}
#endif
}

void WithWindowProc(Instr* PD)
{
	BYTE PAttr; longint w1; WRect v;
	PAttr = ProcAttr;
	/* !!! with PD^ do!!! */
	ProcAttr = RunWordImpl(PD->Attr, colors.uNorm);
	RunWFrml(PD->W, PD->WithWFlags, v);
	w1 = PushWFramed(v.C1, v.R1, v.C2, v.R2, ProcAttr, RunShortStr(PD->Top), "", PD->WithWFlags);
	if ((PD->WithWFlags & WNoClrScr) == 0) ClrScr();
	SetWwViewPort();
	RunInstr(PD->WwInstr);
	PopW2(w1, (PD->WithWFlags & WNoPop) == 0);
	SetWwViewPort();
	ProcAttr = PAttr;
}

void WithLockedProc(Instr* PD)
{
	PInstrCode op; LockD* ld; longint w, w1;
	WORD msg; pstring ntxt(10); LockMode md;
	op = PD->Kind;
	if (op == _withlocked) {
		ld = &PD->WLD;
		while (ld != nullptr) {
			ld->N = RunInt(ld->Frml); ld = ld->Chain;
		}
	}
	w = 0;
label1:
	ld = &PD->WLD;
	while (ld != nullptr) {
		CFile = ld->FD;
		if (CFile->Handle == nullptr)
			if (OpenF1(Shared))
				if (TryLMode(RdMode, md, 2)) { OpenF2(); OldLMode(NullMode); }
				else { CloseClearHCFile(); goto label2; }
			else OpenCreateF(Shared);
		if (CFile->IsShared()) {
			if (op == _withlocked) { if (TryLockN(ld->N, 2)) goto label3; }
			else { if (TryLMode(ld->Md, ld->OldMd, 2)) goto label3; }
		label2:
			UnLck(PD, ld, op);
			if (PD->WasElse) { RunInstr(PD->WElseInstr); return; }
			CFile = ld->FD; SetCPathVol();
			if (op == _withlocked) {
				msg = 839; str(ld->N, ntxt); Set2MsgPar(ntxt, CPath);
			}
			else { msg = 825; Set2MsgPar(CPath, LockModeTxt[ld->Md]); }
			w1 = PushWrLLMsg(msg, false);
			if (w == 0) w = w1;
			else TWork.Delete(w1);
			beep(); KbdTimer(spec.NetDelay, 0); goto label1;
		}
	label3:
		ld = ld->Chain;
	}
	if (w != 0) PopW(w);
	RunInstr(PD->WDoInstr);
	UnLck(PD, nullptr, op);
}

void HelpProc(Instr* PD)
{
	Help(PD->HelpRdb, RunShortStr(PD->Frml), true);
}

FILE* OpenHForPutTxt(Instr* PD)
{
	FileOpenMode m; FILE* h;
	SetTxtPathVol(*PD->TxtPath, PD->TxtCatIRec); TestMountVol(CPath[1]);
	m = _isoverwritefile; if (PD->App) m = _isoldnewfile;
	h = OpenH(m, Exclusive); TestCPathError();
	if (PD->App) SeekH(h, FileSizeH(h));
	return h;
}

void PutTxt(Instr* PD)
{
	FILE* h; LongStr* s; FrmlPtr z; pstring pth;
	z = PD->Txt; if (CanCopyT(nullptr, z)) {
		h = OpenHForPutTxt(PD); pth = CPath; CopyTFStringToH(h); CPath = pth;
	}
	else {
		s = RunLongStr(z); h = OpenHForPutTxt(PD);
		WriteH(h, s->LL, s->A); ReleaseStore(s);
	}
	CPath = pth; TestCPathError(); WriteH(h, 0, h)/*trunc*/; CloseH(h);
}

void AssgnCatFld(Instr* PD)
{
	/* !!! with PD^ do!!! */
	CFile = PD->FD3;
	if (CFile != nullptr) CloseFile();
	WrCatField(PD->CatIRec, PD->CatFld, RunShortStr(PD->Frml3));
}

void AssgnAccRight(Instr* PD)
{
	AccRight = RunShortStr(PD->Frml);
}

void AssgnUserName(Instr* PD)
{
	UserName = RunShortStr(PD->Frml);
}

void ReleaseDriveProc(FrmlPtr Z)
{
	pstring s;
	char c;
	SaveFiles;
	s = RunShortStr(Z);
	c = toupper((char)s[1]);
	if (c == spec.CPMdrive) ReleaseDrive(FloppyDrives); else
		if ((c == 'A') || (c == 'B')) ReleaseDrive(c - '@');
}

void WithGraphicsProc(Instr* PD)
{
	void* p = nullptr;
	MarkStore(p);
	if (IsGraphMode) RunInstr(PD);
	else {
		ScrGraphMode(true, 0); SetWwViewPort();
		RunInstr(PD); ScrTextMode(true, false);
	}
	ReleaseStore(p);
}

void ResetCatalog()
{
	FileDPtr cf; RdbDPtr r;
	cf = CFile; r = CRdb;
	while (CRdb != nullptr) {
		CFile = CRdb->FD->Chain;
		while (CFile != nullptr) {
			CloseFile();
			CFile->CatIRec = GetCatIRec(CFile->Name, CFile->Typ = '0');
#ifdef FandSQL
			SetIsSQLFile();
#endif
			CFile = CFile->Chain;
		}
		CRdb = CRdb->ChainBack;
	}
	CFile = cf; CRdb = r;
}

void PortOut(bool IsWord, WORD Port, WORD What)
{
}

void RecallRecProc(Instr* PD)
{
	LockMode md; longint N;
	CFile = PD->RecFD;
	if (CFile->Typ != 'X') return;
	N = RunInt(PD->RecNr);
	CRecPtr = GetRecSpace();
	md = NewLMode(CrMode);
	if ((N > 0) && (N <= CFile->NRecs)) {
		ReadRec(N);
		if (DeletedFlag()) {
			RecallRec(N);
			if (PD->AdUpd) LastExitCode = !RunAddUpdte('+', nullptr, nullptr);
		}
	}
	OldLMode(md); ReleaseStore(CRecPtr);
}

void UnLck(Instr* PD, LockD* Ld1, PInstrCode Op)
{
	LockD* ld;
	ld = &PD->WLD;
	while (ld != Ld1) {
		CFile = ld->FD;
		if (CFile->IsShared()) {
			if (Op == _withlocked) UnLockN(ld->N);
			OldLMode(ld->OldMd);
		}
		ld = ld->Chain;
	}
}

void WaitProc() // ø. 604
{
	WORD w;
	do
	{
		GetEvent();
		w = Event.What;
		ClrEvent();
	} while (w != evKeyDown && w != evMouseDown);
}

void MemDiagProc()
{
}

void RunInstr(Instr* PD)
{
	LongStr* s;
	while (!ExitP && !BreakP && (PD != nullptr)) {
		switch (PD->Kind) {
		case _ifthenelseP: {
			/* !!! with PD^ do!!! */
			if (RunBool(PD->Bool)) RunInstr(PD->Instr1);
			else RunInstr(PD->ElseInstr1);
			break; }
		case _whiledo: {
			/* !!! with PD^ do!!! */
			while (!ExitP && !BreakP && RunBool(PD->Bool)) RunInstr(PD->Instr1);
			BreakP = false;
			break; }
		case _repeatuntil: {
			/* !!! with PD^ do!!! */
			do { RunInstr(PD->Instr1); } while (!(ExitP || BreakP || RunBool(PD->Bool)));
			BreakP = false;
			break; }
		case _menubox: { MenuBoxProc(PD); break; }
		case _menubar: { MenuBarProc(PD); break; }
		case _forall: ForAllProc(PD); break;
		case _window: WithWindowProc(PD); break;
		case _break: BreakP = true; break;
		case _exitP: ExitP = true; break;
		case _cancel: GoExit(); break;
		case _save: SaveFiles(); break;
		case _clrscr: { TextAttr = ProcAttr; ClrScr(); break; }
		case _clrww: ClrWwProc(PD); break;
		case _clreol: { TextAttr = ProcAttr; ClrEol(); break; }
		case _exec: ExecPgm(PD); break;
		case _proc: CallProcedure(PD); break;
		case _call: CallRdbProc(PD); break;
		case _copyfile: CopyFile(PD->CD); break;
		case _headline: HeadLineProc(PD->Frml); break;
		case _setkeybuf: SetKeyBufProc(PD->Frml); break;
		case _writeln: WritelnProc(PD); break;
		case _gotoxy: screen.GotoXY(RunInt(PD->GoX), RunInt(PD->GoY));
		case _merge: MergeProc(PD); break;
#ifdef FandProlog
		case _lproc: RunProlog(PD->lpPos, PD->lpName); break;
#endif
		case _report: ReportProc(PD->RO, true); break;
		case _sort: SortProc(PD->SortFD, PD->SK); break;
		case _edit: EditProc(PD); break;
		case _asgnloc:/* !!! with PD^ do!!! */ LVAssignFrml(PD->LV, MyBP, PD->Add, PD->Frml); break;
		case _asgnrecfld: AssignRecFld(PD); break;
		case _asgnrecvar:/* !!! with PD^ do!!! */ AssignRecVar(PD->RecLV1, PD->RecLV2, PD->Ass); break;
		case _asgnpar:/* !!! with PD^ do!!! */ AsgnParFldFrml(PD->FD, PD->FldD, PD->Frml, PD->Add); break;
		case _asgnfield: AssignField(PD); break;
		case _asgnnrecs:/* !!! with PD^ do!!! */ { CFile = PD->FD; AssignNRecs(PD->Add, RunInt(PD->Frml)); break; }
		case _appendrec: { CFile = PD->RecFD; AppendRecProc(); break; }
		case _deleterec: DeleteRecProc(PD); break;
		case _recallrec: RecallRecProc(PD); break;
		case _readrec: ReadWriteRecProc(true, PD); break;
		case _writerec: ReadWriteRecProc(false, PD); break;
		case _linkrec: LinkRecProc(PD); break;
		case _withshared:
		case _withlocked: WithLockedProc(PD); break;
		case _edittxt: EditTxtProc(PD); break;
		case _printtxt: PrintTxtProc(PD); break;
		case _puttxt: PutTxt(PD); break;
		case _asgncatfield: AssgnCatFld(PD); break;
		case _asgnusercode: { UserCode = RunInt(PD->Frml); AccRight[0] = 0x01; AccRight[1] = char(UserCode); break; }
		case _asgnaccright: AssgnAccRight(PD); break;
		case _asgnusername: AssgnUserName(PD); break;
		case _asgnusertoday: userToday = RunReal(PD->Frml); break;
		case _asgnclipbd: {
			s = RunLongStr(PD->Frml);
			TWork.Delete(ClpBdPos);
			ClpBdPos = TWork.Store(s);
			ReleaseStore(s);
			break; }
		case _asgnedok: EdOk = RunBool(PD->Frml);
		case _turncat:/* !!! with PD^ do!!! */ {
			CFile = PD->NextGenFD;
			TurnCat(PD->FrstCatIRec, PD->NCatIRecs, RunInt(PD->TCFrml));
			break;
		}
		case _releasedrive: ReleaseDriveProc(PD->Drive); break;
		case _setprinter: SetCurrPrinter(abs(RunInt(PD->Frml))); break;
		case _indexfile:/* !!! with PD^ do!!! */ IndexfileProc(PD->IndexFD, PD->Compress); break;
		case _display:/* !!! with PD^ do!!! */ DisplayProc(PD->Pos.R, PD->Pos.IRec); break;
		case _mount:/* !!! with PD^ do!!! */ MountProc(PD->MountCatIRec, PD->MountNoCancel); break;
		case _clearkeybuf: ClearKbdBuf(); break;
		case _help: HelpProc(PD); break;
		case _wait: WaitProc(); break;
		case _beepP: beep(); break;
		case _delay: Delay((RunInt(PD->Frml) + 27) / 55); break;
		case _sound: Sound(RunInt(PD->Frml)); break;
		case _nosound: NoSound(); break;
#ifdef FandGraph
		case _graph: RunBGraph(PD->GD, false); break;
		case _putpixel: case _line: case _rectangle: case _ellipse:
		case _floodfill: {DrawProc outtextxy(PD); break; }
#endif
		case _withgraphics: WithGraphicsProc(PD->WDoInstr);
#ifndef FandRunV
		case _memdiag: MemDiagProc();
#endif 
		case _closefds: {
			CFile = PD->clFD;
			if (CFile == nullptr) ForAllFDs(ClosePassiveFD);
			else if (!CFile->IsShared() || (CFile->LMode == NullMode)) CloseFile();
			break;
		}
		case _backup: Backup(PD->IsBackup, PD->NoCompress, PD->BrCatIRec, PD->BrNoCancel); break;
		case _backupm: BackupM(PD); break;
		case _resetcat: ResetCatalog(); break;
		case _setedittxt: { SetEditTxt(PD); break; }
		//case _getindex: { GetIndex(); break; }
		case _setmouse: SetMouse(RunInt(PD->MouseX), RunInt(PD->MouseY), RunBool(PD->Show)); break;
		case _checkfile: { SetTxtPathVol(*PD->cfPath, PD->cfCatIRec); CheckFile(PD->cfFD); break; }
#ifdef FandSQL
		case _sql: SQLProc(PD->Frml); break;
		case _login: /* !!! with PD^ do!!! */ StartLogIn(liName, liPassWord); break;
		case _sqlrdwrtxt: SQLRdWrTxt(PD); break;
#endif

		case _asgnrand: srand(RunInt(PD->Frml)); break;
		case _randomize: Random(); break;
		case _asgnxnrecs: PD->xnrIdx->Release(); break;
		case _portout: PortOut(RunBool(PD->IsWord), WORD(RunInt(PD->Port)), WORD(RunInt(PD->PortWhat))); break;
		}
		PD = PD->Chain;
	}
}

void RunProcedure(void* PDRoot)
{
	bool ExP, BrkP;
	ExP = ExitP; BrkP = BreakP; ExitP = false; BreakP = false;
	RunInstr((Instr*)PDRoot);
	ExitP = ExP; BreakP = BrkP;
}

void CallProcedure(Instr* PD)
{
	void* p = nullptr; void* p1 = nullptr; void* p2 = nullptr;
	void* oldbp = nullptr; void* oldprocbp = nullptr;
	LocVar* lv = nullptr; LocVar* lv1 = nullptr; LocVar* lvroot = nullptr;
	WORD i, j, n;
	FrmlPtr z = nullptr; longint l; Instr* pd1 = nullptr;
	LinkDPtr ld = nullptr; FileDPtr lstFD = nullptr;
	KeyFldDPtr kf1 = nullptr, kf2 = nullptr;

	if (PD == nullptr) return;
	MarkBoth(p1, p2); oldprocbp = ProcMyBP;
	ld = LinkDRoot;
	lstFD = (FileD*)LastInChain(FileDRoot);
	SetInpTT(PD->Pos, true);
	ReadProcHead();
	n = LVBD.NParam; lvroot = LVBD.Root; oldbp = MyBP; PushProcStk();
	if ((n != PD->N) && !((n == PD->N - 1) && PD->ExPar)) {
	label1:
		CurrPos = 0; Error(119);
	}
	lv = lvroot;
	for (i = 1; i < n; i++) /* !!! with PD->TArg[i] do!!! */
	{
		if (PD->TArg[i].FTyp != lv->FTyp) goto label1;
		switch (PD->TArg[i].FTyp) {
		case 'r':
		case 'i': { if (lv->FD != PD->TArg[i].FD) goto label1; lv->RecPtr = PD->TArg[i].RecPtr; break; }
		case 'f': {
			if (PD->TArg[i].RecPtr != nullptr) {
				p = SaveCompState(); SetInpLongStr(RunLongStr(PD->TArg[i].TxtFrml), true);
				RdFileD(PD->TArg[i].Name, '6', '$'); RestoreCompState(p);
			}
			else CFile = PD->TArg[i].FD;
			lv1 = lv; while (lv1 != nullptr) {
				if ((lv1->FTyp == 'i' || lv1->FTyp == 'r') && (lv1->FD == lv->FD)) lv1->FD = CFile;
				lv1 = lv1->Chain;
			}
			lv->FD = CFile; FDLocVarAllowed = true;
			break;
		}
		default: {
			z = PD->TArg[i].Frml;
			if (lv->IsRetPar && (z->Op != _getlocvar)
				|| PD->TArg[i].FromProlog
				&& (PD->TArg[i].IsRetPar != lv->IsRetPar)) goto label1;
			LVAssignFrml(lv, oldbp, false, PD->TArg[i].Frml);
			break;
		}
		}
		lv = lv->Chain;
	}
	lv1 = lv; while (lv != nullptr) {
		if (lv->FTyp == 'r') {
			CFile = lv->FD; CRecPtr = GetRecSpace();
			SetTWorkFlag(); ZeroAllFlds(); ClearDeletedFlag(); lv->RecPtr = CRecPtr;
		}
		lv = lv->Chain;
	}
	ProcMyBP = MyBP; pd1 = ReadProcBody(); FDLocVarAllowed = false;
	lv = lv1; while (lv != nullptr) {
		if (lv->FTyp == 'i') /* !!! with WKeyDPtr(lv->RecPtr)^ do!!! */ {
			auto hX = WKeyDPtr(lv->RecPtr);
			if (hX->KFlds == nullptr) hX->KFlds = lv->FD->Keys->KFlds;
			auto tmp = (XWKey*)lv->RecPtr;
			tmp->Open(hX->KFlds, true, false);
		}
		lv = lv->Chain;
	}
	ReleaseStore2(p2);
	RunProcedure(pd1);
	lv = lvroot; i = 1; while (lv != nullptr) {
		if (lv->IsRetPar) {
			z = PD->TArg[i].Frml;
			switch (lv->FTyp) {
			case 'R': /*FloatPtr(Ptr(Seg(oldbp^), Ofs(oldbp^) + z->BPOfs)) =
				FloatPtr(Ptr(Seg(MyBP^), Ofs(MyBP^) + lv->BPOfs))*;*/
				break;
			case 'S': { /*l = LongintPtr(Ptr(Seg(oldbp^), Ofs(oldbp^) + z->BPOfs))*;
			LongintPtr(Ptr(Seg(oldbp^), Ofs(oldbp^) + z->BPOfs)) ^ =
				LongintPtr(Ptr(Seg(MyBP^), Ofs(MyBP^) + lv->BPOfs))*;
			LongintPtr(Ptr(Seg(MyBP^), Ofs(MyBP^) + lv->BPOfs)) ^ = l;*/
				break; }
			case 'B': /*BooleanPtr(Ptr(Seg(oldbp^), Ofs(oldbp^) + z->BPOfs)) ^ =
				BooleanPtr(Ptr(Seg(MyBP^), Ofs(MyBP^) + lv->BPOfs))^*/
				break;
			}
		}
		if (i > n) switch (lv->FTyp) {
		case 'r': { CFile = lv->FD; ClearRecSpace(lv->RecPtr); break; }
		case 'i': { CFile = lv->FD; WKeyDPtr(lv->RecPtr)->Close(); break; };
		}
		i++; lv = lv->Chain;
	}
	PopProcStk();
	ProcMyBP = (ProcStkD*)oldprocbp;
	LinkDRoot = ld;
	CFile = lstFD->Chain;
	while (CFile != nullptr) { CloseFile(); CFile = CFile->Chain; }
	lstFD->Chain = nullptr;
	ReleaseBoth(p1, p2);
}

void RunMainProc(RdbPos RP, bool NewWw)
{
	Instr* PD; void* p1; void* p2; LocVar* lv;
	if (NewWw) {
		ProcAttr = colors.uNorm; screen.Window(1, 2, TxtCols, TxtRows);
		TextAttr = ProcAttr; ClrScr(); UserHeadLine(""); MenuX = 1; MenuY = 2;
	}
	PD = GetPInstr(_proc, sizeof(RdbPos) + 2); PD->Pos = RP;
	CallProcedure(PD);
	ReleaseStore(PD); if (NewWw) screen.Window(1, 1, TxtCols, TxtRows);
}


