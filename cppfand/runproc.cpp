#include "runproc.h"

#include <fstream>
#include "../pascal/random.h"
#include "compile.h"
#include "drivers.h"
#include "expimp.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "oaccess.h"
#include "obase.h"
#include "obaseww.h"
#include "olongstr.h"
#include "printtxt.h"
#include "rdfildcl.h"
#include "rdmerg.h"
#include "rdproc.h"
#include "runfrml.h"
#include "runmerg.h"
#include "runproj.h"
#include "wwmenu.h"
#include "wwmix.h"
#include "runprolg.h"
#include "sort.h"
#include "XFile.h"
#include "../Editor/Editor.h"
#include "../Editor/runedi.h"
#include "../Report/genrprt.h"
#include "../Report/rdrprt.h"
#include "../Report/runrprt.h"


void UserHeadLine(pstring UserHeader)
{
	WParam* p = PushWParam(1, 1, TxtCols, 1, true);
	TextAttr = screen.colors.fNorm;
	ClrEol();
	WORD maxlen = TxtCols - 10;
	WORD l = LenStyleStr(UserHeader);
	if (l >= maxlen) {
		UserHeader[0] = (BYTE)maxlen;
		l = LenStyleStr(UserHeader);
	}
	WORD n = (TxtCols - l) / 2;

	if (n > 0) {
		//printf(); 
		char buf[MaxTxtCols]{ '\0' };
		size_t len = snprintf(buf, MaxTxtCols, "%*c", n, ' ');
		screen.ScrWrText(screen.WhereX(), screen.WhereY(), buf);
	}

	//WrStyleStr(UserHeader, screen.colors.fNorm);
	screen.WriteStyledStringToWindow(UserHeader, ProcAttr);

	// screen.GotoXY(TxtCols - 10, 1);
	// printf("%s", StrDate(Today(), "DD.MM.YYYY").c_str());
	//screen.ScrWrText(TxtCols - 10, 1, StrDate(Today(), "DD.MM.YYYY").c_str());
	screen.ScrWrText(TxtCols - 10, 1, CppToday().c_str());

	PopWParam(p);
	delete p;
}

void ReportProc(RprtOpt* RO, bool save)
{
	void* p = nullptr; void* p2 = nullptr;
	char md = '\0'; longint w = 0; ExitRecord er;
	MarkBoth(p, p2);
	PrintView = false;
	/* !!! with RO^ do!!! */
	if (RO->Flds == nullptr) {
		SetInpTT(&RO->RprtPos, true);
		if (RO->SyntxChk) {
			IsCompileErr = false;
			//NewExit(Ovr(), er);
			//goto label1;
			ReadReport(RO);
			LastExitCode = 0;
		label1:
			RestoreExit(er);
			IsCompileErr = false;
			goto label2;
		}
		ReadReport(RO);
		RunReport(RO);
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

	FieldList FL; FieldDescr* F; RprtOpt* RO2;
	RO2 = (RprtOpt*)GetStore(sizeof(*RO)); Move(RO, RO2, sizeof(*RO));
	FL = RO->Flds;
	while (FL != nullptr)
	{
		F = FL->FldD;
		if ((F->Flg & f_Stored) != 0) ww.PutSelect(F->Name);
		else
		{
			pstring tmpStr = SelMark;
			ww.PutSelect(tmpStr + F->Name);
		}
		FL = (FieldList)FL->Chain;
	}
	CFile = RO->FDL.FD; if (!ww.SelFieldList(36, true, RO2->Flds)) return;
	if ((RO->FDL.Cond == nullptr) &&
		!ww.PromptFilter("", RO2->FDL.Cond, &RO2->CondTxt)) return;
	if (SelForAutoRprt(RO2)) RunAutoReport(RO2);
}

void AssignField(Instr_assign* PD)
{
	longint N = 0; LockMode md; WORD msg = 0; FieldDPtr F = nullptr;
	CFile = PD->FD; md = NewLMode(WrMode); F = PD->FldD;
	N = RunInt(PD->RecFrml);
	if ((N <= 0) || (N > CFile->NRecs)) { msg = 640; goto label1; }
	CRecPtr = GetRecSpace();
	ReadRec(CFile, N, CRecPtr);
	if (PD->Indexarg && !DeletedFlag()) {
		msg = 627;
	label1:
		SetMsgPar(CFile->Name, F->Name);
		RunErrorM(md, msg);
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
			CFile = FD1;
			CRecPtr = RP1;
			((FrmlElem8*)A->Frml)->NewRP = RP2;
			AssgnFrml(A->OFldD, A->Frml, false, false);
			break;
		}
		}
		A = (AssignD*)A->Chain;
	}
	CFile = FD1; CRecPtr = RP1;
	SetUpdFlag();
}

void AssignRecFld(Instr_assign* PD)
{
	FieldDescr* F = PD->RecFldD;
	CFile = PD->AssLV->FD; CRecPtr = PD->AssLV->RecPtr;
	SetUpdFlag();
	AssgnFrml(F, PD->Frml, HasTWorkFlag(), PD->Add);
}

void SortProc(FileD* FD, KeyFldD* SK)
{
	CFile = FD;
	LockMode md = NewLMode(ExclMode);
	SortAndSubst(SK);
	CFile = FD;
	OldLMode(md);
	SaveFiles();
}

void MergeProc(Instr_merge_display* PD)
{
	void* p = nullptr; void* p2 = nullptr;
	MarkBoth(p, p2);
	SetInpTT(&PD->Pos, true);
	ReadMerge();
	RunMerge();
	SaveFiles();
	ReleaseBoth(p, p2);
}

void WritelnProc(Instr_writeln* PD)
{
	LongStr* S = nullptr; WORD i = 0; char c = '\0';
	WriteType LF = WriteType::write;
	WrLnD* W = nullptr;
	pstring t, x; double r = 0.0;
	W = &PD->WD;
	LF = PD->LF;
	t[0] = 0;
	TextAttr = ProcAttr;
	std::string printS;
	while (W != nullptr) {
		switch (W->Typ) {
		case 'S': {
			if (LF == WriteType::message || LF == WriteType::msgAndHelp) {
				t = t + RunShortStr(W->Frml);
			}
			else {
				std::string str = RunStdStr(W->Frml);
				printS += str;
			}
			goto label1;
			break;
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
		if (LF == WriteType::message || LF == WriteType::msgAndHelp) t = t + x;
		else printf("%s", x.c_str());
	label1:
		W = (WrLnD*)W->Chain;
	}
	screen.WriteStyledStringToWindow(printS, ProcAttr);
label2:
	switch (LF) {
	case WriteType::writeln: {
		//printf("\n");
		screen.LF();
		break;
	}
	case WriteType::msgAndHelp: {
		F10SpecKey = _F1_;
		goto label3;
		break; }
	case WriteType::message: {
	label3:
		SetMsgPar(t);
		WrLLF10Msg(110);
		if (KbdChar == _F1_) {
			Help(PD->mHlpRdb, RunShortStr(PD->mHlpFrml), false);
			goto label2;
		}
		break;
	}
	}
}

void DisplayProc(RdbD* R, WORD IRec)
{
	LongStr* S = nullptr; void* p = nullptr; WORD i = 0;
	std::string str;
	MarkStore(p);
	if (IRec == 0) {
		S = GetHlpText(CRdb, RunShortStr((FrmlElem*)R), true, i);
		if (S == nullptr) goto label1;
	}
	else {
		CFile = R->FD; CRecPtr = Chpt->RecPtr;
		ReadRec(CFile, IRec, CRecPtr);
		S = CFile->TF->Read(1, _T(ChptTxt));
		if (R->Encrypted) CodingLongStr(S);
	}
	//WrLongStyleStr(S, ProcAttr);
	str = std::string(S->A, S->LL);
	screen.WriteStyledStringToWindow(str, ProcAttr);
	ReleaseStore(S);

label1:
	ReleaseStore(p);
}

void ClrWwProc(Instr_clrww* PD)
{
	WRect v; WORD a = 0; pstring s; char c = '\0';
	RunWFrml(PD->W2, 0, v);
	a = RunWordImpl(PD->Attr2, screen.colors.uNorm);
	c = ' ';
	if (PD->FillC != nullptr) {
		s = RunShortStr(PD->FillC);
		if (s.length() > 0) c = s[1];
	}
	screen.ScrClr(v.C1, v.R1, v.C2 - v.C1 + 1, v.R2 - v.R1 + 1, c, a);
}

void ExecPgm(Instr_exec* PD)
{
	pstring s; pstring Prog; WORD i = 0; BYTE x = 0, y = 0; bool b = false;
	Wind wmin = WindMin;
	Wind wmax = WindMax;
	TCrs crs = screen.CrsGet();
	longint w = PushW(1, 1, TxtCols, 1);
	WindMin = wmin;
	WindMax = wmax;
	screen.CrsSet(crs);
	s = RunShortStr(PD->Param);
	i = PD->ProgCatIRec;
	CVol = "";
	if (i != 0) Prog = RdCatField(i, CatPathName);
	else Prog = *PD->ProgPath;
	b = OSshell(Prog, s, PD->NoCancel, PD->FreeMm, PD->LdFont, PD->TextMd);
	/*asm mov ah, 3; mov bh, 0; push bp; int 10H; pop bp; mov x, dl; mov y, dh;*/
	PopW(w);
	//screen.GotoXY(x - WindMin.X + 1, y - WindMin.Y + 1);
	screen.GotoXY(1, 1);
	if (!b) GoExit();
}

void CallRdbProc(Instr_call* PD)
{
	bool b = false; void* p = nullptr; ProcStkD* bp = nullptr;
	wwmix ww;
	MarkStore(p);
	bp = MyBP;
	b = EditExecRdb(PD->RdbNm, PD->ProcNm, PD->ProcCall, &ww);
	SetMyBP(bp);
	ReleaseStore(p);
	if (!b) GoExit();
}

void IndexfileProc(FileDPtr FD, bool Compress)
{
	FileD* FD2 = nullptr;
	LockMode md;
	FileD* cf = CFile;
	CFile = FD;
	md = NewLMode(ExclMode);
	XFNotValid();
	CRecPtr = GetRecSpace();
	if (Compress) {
		FD2 = OpenDuplF(false);
		for (longint I = 1; I < FD->NRecs; I++)
		{
			CFile = FD;
			ReadRec(CFile, I, CRecPtr);
			if (!DeletedFlag())
			{
				CFile = FD2;
				PutRec();
			}
		}
		if (!SaveCache(0, CFile->Handle)) GoExit();
		CFile = FD;
		SubstDuplF(FD2, false);
	}
	CFile->XF->NoCreate = false;
	TestXFExist();
	OldLMode(md);
	SaveFiles();
	ReleaseStore(CRecPtr);
	CFile = cf;
}

void MountProc(WORD CatIRec, bool NoCancel)
{
	ExitRecord er;
	//NewExit(Ovr, er);
	goto label1;
	SaveFiles();
	RdCatPathVol(CatIRec);
	TestMountVol(CPath[1]);
	LastExitCode = 0;
	RestoreExit(er);
	return;
label1:
	RestoreExit(er);
	if (NoCancel) LastExitCode = 1;
	else GoExit();
}

void EditProc(Instr_edit* PD)
{
	EdUpdated = false;
	SaveFiles();
	CFile = PD->EditFD;
	//EditOpt* EO = (EditOpt*)GetStore(sizeof(*EO));
	EditOpt* EO = new EditOpt();
	//Move(PD->EO, EO, sizeof(*EO));
	*EO = *PD->EO;
	if (!EO->UserSelFlds || SelFldsForEO(EO, nullptr)) EditDataFile(CFile, EO);
	SaveFiles();
	//ReleaseStore(EO);
	delete EO;
}

void EditTxtProc(Instr_edittxt* PD)
{
	longint i = 0; WRect v;
	WRect* pv = nullptr; BYTE a = 0; longint* lp = nullptr;
	MsgStr MsgS; void* p = nullptr; pstring msg;
	MarkStore(p);
	i = 1;
	if (PD->TxtPos != nullptr) i = RunInt(PD->TxtPos);
	EdUpdated = false;
	a = RunWordImpl(PD->Atr, 0);
	pv = nullptr;
	if (PD->Ww.C1 != nullptr) { RunWFrml(PD->Ww, PD->WFlags, v); pv = &v; }
	MsgS.Head = (PD->Head == nullptr) ? "" : *GetStr(PD->Head);
	MsgS.Last = (PD->Last == nullptr) ? "" : *GetStr(PD->Last);
	MsgS.CtrlLast = (PD->CtrlLast == nullptr) ? "" : *GetStr(PD->CtrlLast);
	MsgS.ShiftLast = (PD->ShiftLast == nullptr) ? "" : *GetStr(PD->ShiftLast);
	MsgS.AltLast = (PD->AltLast == nullptr) ? "" : *GetStr(PD->AltLast);

	if (PD->TxtLV != nullptr) lp = (longint*)(uintptr_t(MyBP) + PD->TxtLV->BPOfs);
	else {
		SetTxtPathVol(PD->TxtPath, PD->TxtCatIRec);
		lp = nullptr;
	}
	msg = "";
	if (PD->ErrMsg != nullptr) msg = RunShortStr(PD->ErrMsg);
	EditTxtFile(lp, PD->EdTxtMode, msg, PD->ExD, i, RunInt(PD->TxtXY), pv, a, RunShortStr(PD->Hd), PD->WFlags, &MsgS);
	ReleaseStore(p);
}

std::string* GetStr(FrmlElem* Z)
{
	if (Z == nullptr) return nullptr;
	std::string s = RunShortStr(Z);
	return StoreStr(s);
}

void PrintTxtProc(Instr_edittxt* PD)
{
	LongStr* s = nullptr;
	/* !!! with PD^ do!!! */
	if (PD->TxtLV != nullptr) {
		s = TWork.Read(1, *(longint*)(uintptr_t(MyBP) + PD->TxtLV->BPOfs));
		PrintArray(s->A, s->LL, false);
		ReleaseStore(s);
	}
	else {
		SetTxtPathVol(PD->TxtPath, PD->TxtCatIRec);
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
		cr = CRecPtr;
		CRecPtr = GetRecSpace();
		auto result = SearchKey(X, K, N);
		ReleaseStore(CRecPtr);
		CRecPtr = cr;
		return result;
	}
}

void DeleteRecProc(Instr_recs* PD)
{
	LockMode md; longint n; XString x;
	CFile = PD->RecFD;
	CRecPtr = GetRecSpace();
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
	ReadRec(CFile, n, CRecPtr);
	if (PD->AdUpd && !DeletedFlag()) LastExitCode = (!RunAddUpdte('-', nullptr, nullptr));
	if (CFile->Typ == 'X') {
		if (!DeletedFlag()) DeleteXRec(n, true);
	}
	else DeleteRec(n);
label1:
	OldLMode(md);
label2:
	ReleaseStore(CRecPtr);
}

void AppendRecProc()
{
	LockMode md;
	md = NewLMode(CrMode);
	CRecPtr = GetRecSpace();
	ZeroAllFlds();
	SetDeletedFlag();
	CreateRec(CFile->NRecs + 1);
	ReleaseStore(CRecPtr);
	OldLMode(md);
}

void UpdRec(void* CR, longint N, bool AdUpd)
{
	void* cr2 = GetRecSpace();
	CRecPtr = cr2;
	ReadRec(CFile, N, CRecPtr);
	bool del = DeletedFlag();
	CRecPtr = CR;
	if (AdUpd)
		if (del) LastExitCode = !RunAddUpdte('+', nullptr, nullptr);
		else LastExitCode = !RunAddUpdte('d', cr2, nullptr);
	if (CFile->Typ == 'X') OverWrXRec(N, cr2, CR);
	else WriteRec(N);
	if (!del) DelAllDifTFlds(cr2, nullptr);
	ReleaseStore(cr2);
}

void ReadWriteRecProc(bool IsRead, Instr_recs* PD)
{
	longint N = 0;
	bool app = false;
	XString x;
	WORD msg = 0;
	/* !!! with PD->LV^ do!!! */
	CFile = PD->LV->FD;
	CRecPtr = PD->LV->RecPtr;
	N = 1;
	KeyD* k = PD->Key;
	bool ad = PD->AdUpd;
	LockMode md = CFile->LMode;
	app = false;
	void* cr = GetRecSpace();
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
					DelTFlds();
					ZeroAllFlds();
					goto label4;
				}
				else {
				label1:
					NewLMode(CrMode);
					TestXFExist();
					IncNRecs(1);
					app = true;
				}
			N = CFile->NRecs;
		}
		else if (!SrchXKey(k, x, N)) {
		label2:
			if (IsRead) {
				DelTFlds();
				ZeroAllFlds();
				SetDeletedFlag();
				goto label4;
			}
			msg = 613;
			goto label3;
		}
	}
	else if ((N <= 0) || (N > CFile->NRecs)) {
		msg = 641;
	label3:
		SetMsgPar(PD->LV->Name);
		RunErrorM(md, msg);
	}
	if (IsRead) {
		CRecPtr = cr;
		ReadRec(CFile, N, CRecPtr);
		CRecPtr = PD->LV->RecPtr;
		DelTFlds();
		CopyRecWithT(cr, PD->LV->RecPtr);
	}
	else {
		CopyRecWithT(PD->LV->RecPtr, cr);
		if (app) {
			CRecPtr = cr;
			if (CFile->Typ == 'X') {
				RecallRec(N);
			}
			else WriteRec(N);
			if (ad) LastExitCode = !RunAddUpdte('+', nullptr, nullptr);
		}
		else UpdRec(cr, N, ad);
	}
label4:
	ReleaseStore(cr);
	OldLMode(md);
}

void LinkRecProc(Instr_assign* PD)
{
	void* p = nullptr; void* r2 = nullptr; void* lr2 = nullptr;
	FileD* cf = nullptr; void* cr = nullptr; LinkD* ld = nullptr; longint n = 0;
	cf = CFile; cr = CRecPtr; MarkStore(p);
	ld = PD->LinkLD; CRecPtr = PD->RecLV1->RecPtr;
	lr2 = PD->RecLV2->RecPtr;
	CFile = ld->ToFD; ClearRecSpace(lr2); CFile = ld->FromFD;
	if (LinkUpw(ld, n, true)) LastExitCode = 0;
	else LastExitCode = 1;
	r2 = CRecPtr; CRecPtr = lr2; DelTFlds(); CopyRecWithT(r2, lr2);
	ReleaseStore(p); CFile = cf; CRecPtr = cr;
}

void ForAllProc(Instr_forall* PD)
{
	FileD* FD = nullptr; KeyD* Key = nullptr; KeyD* k = nullptr; FrmlElem* Bool = nullptr;
	LinkD* LD = nullptr; KeyInD* KI = nullptr;
	void* cr = nullptr; void* p = nullptr; void* lr = nullptr;
	XScan* Scan = nullptr; LockMode md, md1; XString xx;
	KeyFldD* KF = nullptr; LocVar* LVi = nullptr; LocVar* LVr = nullptr;
	bool lk = false, b = false;
#ifdef FandSQL
	bool sql;
#endif
	MarkStore(p);
	FD = PD->CFD; Key = PD->CKey;
	LVi = PD->CVar; LVr = PD->CRecVar;
	LD = PD->CLD; KI = PD->CKIRoot;
	Bool = RunEvalFrml(PD->CBool);
	lk = false;
#ifdef FandSQL
	if (PD->inSQL && !FD->IsSQLFile) return;
#endif
	if (LD != nullptr) {
		CFile = LD->ToFD; KF = LD->ToKey->KFlds;
		switch (PD->COwnerTyp) {
		case 'r': {
			CRecPtr = PD->CLV->RecPtr;
			xx.PackKF(KF);
			break;
		}
		case 'F': {
			md = NewLMode(RdMode);
			CRecPtr = GetRecSpace();
			ReadRec(CFile, RunInt(FrmlPtr(PD->CLV)), CRecPtr);
			xx.PackKF(KF);
			ReleaseStore(p);
			OldLMode(md);
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
	Scan = new XScan(CFile, Key, KI, true); //New(Scan, Init(CFile, Key, KI, true));
#ifdef FandSQL
	if (PD->inSQL) Scan->ResetSQLTxt(Bool); else
#endif
		if (LD != nullptr) {
			if (PD->COwnerTyp == 'i') Scan->ResetOwnerIndex(LD, PD->CLV, Bool);
			else Scan->ResetOwner(&xx, Bool);
		}
		else Scan->Reset(Bool, PD->CSQLFilter);
#ifdef FandSQL
	if (!CFile->IsSQLFile)
#endif
		if (Key != nullptr)
			if (PD->CWIdx) ScanSubstWIndex(Scan, Key->KFlds, 'W');
			else { CFile->XF->UpdLockCnt++; lk = true; }
	if (LVr != nullptr) lr = LVr->RecPtr;
	k = CFile->Keys;
	b = PD->CProcent;
	if (b) RunMsgOn('F', Scan->NRecs);
label1:
#ifdef FandSQL
	if (sql) CRecPtr = lr;
	else
#endif
		CRecPtr = cr;
	Scan->GetRec();
	if (b) RunMsgN(Scan->IRec);
	if (!Scan->eof) {
#ifdef FandSQL

		if (sql) { ClearUpdFlag; if (k != nullptr) xx.PackKF(k->KFlds) }
		else
#endif
			if (LVr != nullptr) {
				CRecPtr = lr;
				ClearUpdFlag();
				DelTFlds();
				CopyRecWithT(cr, lr);
			}
		//if (LVi != nullptr) *(double*)(LocVarAd(LVi)) = Scan->RecNr;
		if (LVi != nullptr) {
			LVi->R = Scan->RecNr;
		}
		RunInstr(PD->CInstr);
		CFile = FD;
		CRecPtr = lr;
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
				md1 = NewLMode(WrMode);
				CopyRecWithT(lr, cr);
				UpdRec(cr, Scan->RecNr, true);
				OldLMode(md1);
			}
		}
		if (!(ExitP || BreakP)) {
			if (
#ifdef FandSQL
				!sql &&
#endif 
				(Key == nullptr) && (Scan->NRecs > CFile->NRecs)) {
				Scan->IRec--;
				Scan->NRecs--;
			}
			goto label1;
		}
	}
	if (lk) CFile->XF->UpdLockCnt--;
	Scan->Close();
	OldLMode(md);
	if (b) RunMsgOff();
	ReleaseStore(p);
	BreakP = false;
}

void HeadLineProc(FrmlElem* Z)
{
	UserHeadLine(RunShortStr(Z));
}

void SetKeyBufProc(FrmlElem* Z)
{
	//KbdBuffer = RunShortStr(Z);
	std::string keyBuf = RunShortStr(Z);
	keyboard.SetKeyBuf(keyBuf);
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

void WithWindowProc(Instr_window* PD)
{
	BYTE PAttr = ProcAttr;
	longint w1 = 0;
	WRect v;

	/* !!! with PD^ do!!! */
	ProcAttr = RunWordImpl(PD->Attr, screen.colors.uNorm); // nacte barvy do ProcAttr
	RunWFrml(PD->W, PD->WithWFlags, v); // nacte rozmery okna
	auto top = RunShortStr(PD->Top); // nacte nadpis
	w1 = PushWFramed(v.C1, v.R1, v.C2, v.R2, ProcAttr, top, "", PD->WithWFlags); // vykresli oramovane okno s nadpisem
	if ((PD->WithWFlags & WNoClrScr) == 0) ClrScr();
	SetWwViewPort();
	RunInstr(PD->WwInstr);
	PopW2(w1, (PD->WithWFlags & WNoPop) == 0);
	SetWwViewPort();
	ProcAttr = PAttr;
}

void WithLockedProc(Instr_withshared* PD)
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
				if (TryLMode(RdMode, md, 2)) {
					OpenF2();
					OldLMode(NullMode);
				}
				else {
					CloseClearHCFile();
					goto label2;
				}
			else OpenCreateF(Shared);
		if (CFile->IsShared()) {
			if (op == _withlocked) {
				if (TryLockN(ld->N, 2)) goto label3;
			}
			else {
				if (TryLMode(ld->Md, ld->OldMd, 2)) goto label3;
			}
		label2:
			UnLck(PD, ld, op);
			if (PD->WasElse) {
				RunInstr(PD->WElseInstr);
				return;
			}
			CFile = ld->FD;
			SetCPathVol();
			if (op == _withlocked) {
				msg = 839;
				str(ld->N, ntxt);
				SetMsgPar(ntxt, CPath);
			}
			else {
				msg = 825;
				SetMsgPar(CPath, LockModeTxt[ld->Md]);
			}
			w1 = PushWrLLMsg(msg, false);
			if (w == 0) w = w1;
			else TWork.Delete(w1);
			beep();
			KbdTimer(spec.NetDelay, 0);
			goto label1;
		}
	label3:
		ld = ld->Chain;
	}
	if (w != 0) PopW(w);
	RunInstr(PD->WDoInstr);
	UnLck(PD, nullptr, op);
}

void HelpProc(Instr_help* PD)
{
	Help(PD->HelpRdb0, RunShortStr(PD->Frml0), true);
}

FILE* OpenHForPutTxt(Instr_puttxt* PD)
{
	FileOpenMode m; FILE* h;
	SetTxtPathVol(PD->TxtPath1, PD->TxtCatIRec1);
	TestMountVol(CPath[1]);
	m = _isoverwritefile;
	if (PD->App) m = _isoldnewfile;
	h = OpenH(m, Exclusive);
	TestCPathError();
	if (PD->App) SeekH(h, FileSizeH(h));
	return h;
}

void PutTxt(Instr_puttxt* PD)
{
	FILE* h = nullptr; LongStr* s = nullptr;
	FrmlElem* z = nullptr; pstring pth;
	z = PD->Txt;
	if (CanCopyT(nullptr, z)) {
		h = OpenHForPutTxt(PD);
		pth = CPath;
		CopyTFStringToH(h);
		CPath = pth;
	}
	else {
		s = RunLongStr(z);
		h = OpenHForPutTxt(PD);
		WriteH(h, s->LL, s->A);
		ReleaseStore(s);
	}
	CPath = pth;
	TestCPathError();
	WriteH(h, 0, h)/*trunc*/;
	CloseH(&h);
}

// ulozi do souboru hodnotu promenne
void AssgnCatFld(Instr_assign* PD)
{
	CFile = PD->FD3;
	if (CFile != nullptr) CloseFile();
	std::string data = RunShortStr(PD->Frml3);
	WrCatField(CatFD, PD->CatIRec, PD->CatFld, data);
}

void AssgnAccRight(Instr_assign* PD)
{
	AccRight = RunShortStr(PD->Frml);
}

void AssgnUserName(Instr_assign* PD)
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
	if (c == spec.CPMdrive) ReleaseDrive(FloppyDrives);
	else
		if ((c == 'A') || (c == 'B')) ReleaseDrive(c - '@');
}

void WithGraphicsProc(Instr* PD)
{
	throw std::exception("WithGraphicsProc() not implemented!");
	//void* p = nullptr;
	//MarkStore(p);
	//if (IsGraphMode) RunInstr(PD);
	//else {
	//	ScrGraphMode(true, 0);
	//	SetWwViewPort();
	//	RunInstr(PD);
	//	ScrTextMode(true, false);
	//}
	//ReleaseStore(p);
}

#ifdef FandGraph
void DrawProc(Instr_graph* PD)
{
}
#endif


void ResetCatalog()
{
	FileD* cf = CFile;
	RdbD* r = CRdb;
	while (CRdb != nullptr) {
		CFile = (FileD*)CRdb->FD->Chain;
		while (CFile != nullptr) {
			CloseFile();
			CFile->CatIRec = GetCatIRec(CFile->Name, CFile->Typ == '0');
#ifdef FandSQL
			SetIsSQLFile();
#endif
			CFile = (FileD*)CFile->Chain;
		}
		CRdb = CRdb->ChainBack;
	}
	CFile = cf;
	CRdb = r;
}

void PortOut(bool IsWord, WORD Port, WORD What)
{
}

void RecallRecProc(Instr_recs* PD)
{
	LockMode md; longint N;
	CFile = PD->RecFD;
	if (CFile->Typ != 'X') return;
	N = RunInt(PD->RecNr);
	CRecPtr = GetRecSpace();
	md = NewLMode(CrMode);
	if ((N > 0) && (N <= CFile->NRecs)) {
		ReadRec(CFile, N, CRecPtr);
		if (DeletedFlag()) {
			RecallRec(N);
			if (PD->AdUpd) LastExitCode = !RunAddUpdte('+', nullptr, nullptr);
		}
	}
	OldLMode(md); ReleaseStore(CRecPtr);
}

void UnLck(Instr_withshared* PD, LockD* Ld1, PInstrCode Op)
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
	LongStr* s = nullptr;
	while (!ExitP && !BreakP && (PD != nullptr)) {
		switch (PD->Kind) {
		case _ifthenelseP: {
			/* !!! with PD^ do!!! */
			auto iPD = (Instr_loops*)PD;
			if (RunBool(iPD->Bool)) {
				RunInstr(iPD->Instr1);
			}
			else {
				RunInstr(iPD->ElseInstr1);
			}
			break;
		}
		case _whiledo: {
			/* !!! with PD^ do!!! */
			auto iPD = (Instr_loops*)PD;
			while (!ExitP && !BreakP && RunBool(iPD->Bool)) {
				RunInstr(iPD->Instr1);
			}
			BreakP = false;
			break;
		}
		case _repeatuntil: {
			/* !!! with PD^ do!!! */
			auto iPD = (Instr_loops*)PD;
			do {
				RunInstr(iPD->Instr1);
			} while (!(ExitP || BreakP || RunBool(iPD->Bool)));
			BreakP = false;
			break;
		}
		case _menubox: { MenuBoxProc((Instr_menu*)PD); break; }
		case _menubar: { MenuBarProc((Instr_menu*)PD); break; }
		case _forall: ForAllProc((Instr_forall*)PD); break;
		case _window: WithWindowProc((Instr_window*)PD); break;
		case _break: BreakP = true; break;
		case _exitP: ExitP = true; break;
		case _cancel: GoExit(); break;
		case _save: SaveFiles(); break;
		case _clrscr: { TextAttr = ProcAttr; ClrScr(); break; }
		case _clrww: ClrWwProc((Instr_clrww*)PD); break;
		case _clreol: { TextAttr = ProcAttr; ClrEol(); break; }
		case _exec: ExecPgm((Instr_exec*)PD); break;
		case _proc: CallProcedure((Instr_proc*)PD); break;
		case _call: CallRdbProc((Instr_call*)PD); break;
		case _copyfile: CopyFileE(((Instr_copyfile*)PD)->CD); break;
		case _headline: HeadLineProc(((Instr_assign*)PD)->Frml); break;
		case _setkeybuf: SetKeyBufProc(((Instr_assign*)PD)->Frml); break;
		case _writeln: WritelnProc((Instr_writeln*)PD); break;
		case _gotoxy: {
			auto iPD = (Instr_gotoxy*)PD;
			WORD x = RunInt(iPD->GoX);
			WORD y = RunInt(iPD->GoX);
			screen.Window(1, 1, TxtCols, TxtRows);
			screen.GotoXY(x, y, absolute);
			break;
		}
		case _merge: MergeProc((Instr_merge_display*)PD); break;
#ifdef FandProlog
		case _lproc:
		{
			auto iPD = (Instr_lproc*)PD;
			RunProlog(&iPD->lpPos, iPD->lpName);
			break;
		}
#endif
		case _report: {
			ReportProc(((Instr_report*)PD)->RO, true);
			break;
		}
		case _sort: {
			auto iPD = (Instr_sort*)PD;
			SortProc(iPD->SortFD, iPD->SK);
			break;
		}
		case _edit: {
			EditProc((Instr_edit*)PD);
			break;
		}
		case _asgnloc:/* !!! with PD^ do!!! */ {
			auto iPD = (Instr_assign*)PD;
			LVAssignFrml(iPD->AssLV, MyBP, iPD->Add, iPD->Frml);
			break;
		}
		case _asgnrecfld: AssignRecFld(((Instr_assign*)PD)); break;
		case _asgnrecvar:/* !!! with PD^ do!!! */ {
			auto iPD = (Instr_assign*)PD;
			AssignRecVar(iPD->RecLV1, iPD->RecLV2, iPD->Ass);
			break;
		}
		case _asgnpar: /* !!! with PD^ do!!! */ {
			// ulozi globalni parametr - do souboru
			auto iPD = (Instr_assign*)PD;
			AsgnParFldFrml(iPD->FD, iPD->FldD, iPD->Frml, iPD->Add);
			break;
		}
		case _asgnfield: AssignField(((Instr_assign*)PD)); break;
		case _asgnnrecs: /* !!! with PD^ do!!! */ {
			auto iPD = (Instr_assign*)PD;
			CFile = iPD->FD;
			AssignNRecs(iPD->Add, RunInt(iPD->Frml));
			break;
		}
		case _appendrec: {
			CFile = ((Instr_recs*)PD)->RecFD;
			AppendRecProc();
			break;
		}
		case _deleterec: DeleteRecProc((Instr_recs*)PD); break;
		case _recallrec: RecallRecProc((Instr_recs*)PD); break;
		case _readrec: ReadWriteRecProc(true, (Instr_recs*)PD); break;
		case _writerec: ReadWriteRecProc(false, (Instr_recs*)PD); break;
		case _linkrec: LinkRecProc((Instr_assign*)PD); break;
		case _withshared:
		case _withlocked: WithLockedProc((Instr_withshared*)PD); break;
		case _edittxt: EditTxtProc((Instr_edittxt*)PD); break;
		case _printtxt: PrintTxtProc((Instr_edittxt*)PD); break;
		case _puttxt: PutTxt((Instr_puttxt*)PD); break;
		case _asgncatfield: AssgnCatFld((Instr_assign*)PD); break;
		case _asgnusercode: {
			UserCode = RunInt(((Instr_assign*)PD)->Frml);
			AccRight[0] = 0x01;
			AccRight[1] = char(UserCode);
			break;
		}
		case _asgnaccright: AssgnAccRight((Instr_assign*)PD); break;
		case _asgnusername: AssgnUserName((Instr_assign*)PD); break;
		case _asgnusertoday: userToday = RunReal(((Instr_assign*)PD)->Frml); break;
		case _asgnclipbd: {
			s = RunLongStr(((Instr_assign*)PD)->Frml);
			TWork.Delete(ClpBdPos);
			ClpBdPos = TWork.Store(s);
			ReleaseStore(s);
			break;
		}
		case _asgnedok: EdOk = RunBool(((Instr_assign*)PD)->Frml); break;
		case _turncat:/* !!! with PD^ do!!! */ {
			auto iPD = (Instr_turncat*)PD;
			CFile = iPD->NextGenFD;
			TurnCat(iPD->FrstCatIRec, iPD->NCatIRecs, RunInt(iPD->TCFrml));
			break;
		}
		case _releasedrive: ReleaseDriveProc(((Instr_releasedrive*)PD)->Drive); break;
		case _setprinter: SetCurrPrinter(abs(RunInt(((Instr_assign*)PD)->Frml))); break;
		case _indexfile: /* !!! with PD^ do!!! */ {
			auto iPD = (Instr_indexfile*)PD;
			IndexfileProc(iPD->IndexFD, iPD->Compress);
			break;
		}
		case _display: {
			/* !!! with PD^ do!!! */
			auto iPD = (Instr_merge_display*)PD;
			DisplayProc(iPD->Pos.R, iPD->Pos.IRec);
			break;
		}
		case _mount: /* !!! with PD^ do!!! */ {
			auto iPD = (Instr_mount*)PD;
			MountProc(iPD->MountCatIRec, iPD->MountNoCancel);
			break;
		}
		case _clearkeybuf: ClearKbdBuf(); break;
		case _help: HelpProc((Instr_help*)PD); break;
		case _wait: WaitProc(); break;
		case _beepP: beep(); break;
		case _delay: { Delay((RunInt(((Instr_assign*)PD)->Frml) + 27) / 55); break; }
		case _sound: { Sound(RunInt(((Instr_assign*)PD)->Frml)); break; }
		case _nosound: { NoSound(); break; }
#ifdef FandGraph
		case _graph: { RunBGraph(((Instr_graph*)PD)->GD, false); break; }
		case _putpixel:
		case _line:
		case _rectangle:
		case _ellipse:
		case _floodfill:
		case _outtextxy: { DrawProc((Instr_graph*)PD); break; }
#endif
		case _withgraphics: { WithGraphicsProc(((Instr_withshared*)PD)->WDoInstr); break; }
#ifndef FandRunV
		case _memdiag: { MemDiagProc(); break; }
#endif 
		case _closefds: {
			// zavre soubor
			CFile = ((Instr_closefds*)PD)->clFD;
			if (CFile == nullptr) ForAllFDs(ClosePassiveFD);
			else if (!CFile->IsShared() || (CFile->LMode == NullMode)) CloseFile();
			break;
		}
		case _backup: {
			auto iPD = (Instr_backup*)PD;
			Backup(iPD->IsBackup, iPD->NoCompress, iPD->BrCatIRec, iPD->BrNoCancel);
			break;
		}
		case _backupm: BackupM((Instr_backup*)PD); break;
		case _resetcat: ResetCatalog(); break;
		case _setedittxt: { SetEditTxt((Instr_setedittxt*)PD); break; }
		case _getindex: {
			GetIndexSort((Instr_getindex*)PD);
			break;
		}
		case _setmouse: {
			auto iPD = (Instr_setmouse*)PD;
			SetMouse(RunInt(iPD->MouseX), RunInt(iPD->MouseY), RunBool(iPD->Show));
			break;
		}
		case _checkfile: {
			auto iPD = (Instr_checkfile*)PD;
			SetTxtPathVol(iPD->cfPath, iPD->cfCatIRec);
			CheckFile(iPD->cfFD);
			break;
		}
#ifdef FandSQL
		case _sql: SQLProc(PD->Frml); break;
		case _login: /* !!! with PD^ do!!! */ StartLogIn(liName, liPassWord); break;
		case _sqlrdwrtxt: SQLRdWrTxt(PD); break;
#endif
		case _asgnrand: srand(RunInt(((Instr_assign*)PD)->Frml)); break;
		case _randomize: Random(); break;
		case _asgnxnrecs: ((Instr_assign*)PD)->xnrIdx->Release(); break;
		case _portout: {
			auto iPD = (Instr_portout*)PD;
			PortOut(RunBool(iPD->IsWord),
				WORD(RunInt(iPD->Port)),
				WORD(RunInt(iPD->PortWhat)));
			break;
		}
		}
		PD = (Instr*)PD->Chain;
	}
}

void RunProcedure(Instr* PDRoot)
{
	bool ExP = ExitP; bool BrkP = BreakP;
	ExitP = false; BreakP = false;
	RunInstr(PDRoot);
	ExitP = ExP; BreakP = BrkP;
}

void CallProcedure(Instr_proc* PD)
{
	stSaveState* p = nullptr;
	void* p1 = nullptr; void* p2 = nullptr;
	void* oldbp = nullptr; void* oldprocbp = nullptr;
	//LocVar* lv0 = nullptr;
	std::_Vector_iterator<std::_Vector_val<std::_Simple_types<LocVar*>>> it0;
	//LocVar* lv1 = nullptr;
	std::_Vector_iterator<std::_Vector_val<std::_Simple_types<LocVar*>>> it1;
	LocVar* lvroot = nullptr;
	WORD i = 0, j = 0, n = 0;
	longint l = 0; Instr* pd1 = nullptr;
	LinkD* ld = nullptr; FileD* lstFD = nullptr;
	KeyFldD* kf1 = nullptr; KeyFldD* kf2 = nullptr;

	if (PD == nullptr) return;
	MarkBoth(p1, p2);
	oldprocbp = ProcMyBP;
	ld = LinkDRoot;
	lstFD = (FileD*)LastInChain(FileDRoot);
	SetInpTT(&PD->PPos, true);

#ifdef _DEBUG
	std::string srcCode = std::string((char*)InpArrPtr, InpArrLen);
	if (srcCode.find("(copy(Path,length(Path)-5,2)); sestp:=copy(Path,1,length(Path)-6)+'??.TXT'; end; h:=PARAM3.TxtHd; ww:=PARAM3.TxtWw; PARAM3.InTxt:=true; repeat if PARAM3.TxtLl=~'' then if PARAM3.Prohl then lp:='") != std::string::npos) {
		printf("");
	}
#endif

	ReadProcHead("");
	PD->variables = LVBD;
	n = PD->variables.NParam;
	lvroot = PD->variables.GetRoot();
	oldbp = MyBP;
	//PushProcStk();
	if ((n != PD->N) && !((n == PD->N - 1) && PD->ExPar)) {
	label1:
		CurrPos = 0;
		Error(119);
	}
	//lv0 = lvroot;
	it0 = PD->variables.vLocVar.begin();
	// projdeme vstupni parametry funkce
	for (i = 0; i < n; i++) /* !!! with PD->TArg[i] do!!! */
	{
		if (PD->TArg[i].FTyp != (*it0)->FTyp) goto label1;
		switch (PD->TArg[i].FTyp) {
		case 'r':
		case 'i': {
			if ((*it0)->FD != PD->TArg[i].FD) goto label1;
			(*it0)->RecPtr = PD->TArg[i].RecPtr;
			break;
		}
		case 'f': {
			if (PD->TArg[i].RecPtr != nullptr) {
				p = SaveCompState();
				SetInpLongStr(RunLongStr(PD->TArg[i].TxtFrml), true);
				RdFileD(PD->TArg[i].Name, '6', "$");
				RestoreCompState(p);
			}
			else CFile = PD->TArg[i].FD;
			it1 = it0;
			while (it1 != PD->variables.vLocVar.end()) {
				if (((*it1)->FTyp == 'i' || (*it1)->FTyp == 'r')
					&& ((*it1)->FD == (*it0)->FD)) (*it1)->FD = CFile;
				it1++; // (LocVar*)lv1->Chain;
			}
			(*it0)->FD = CFile;
			FDLocVarAllowed = true;
			break;
		}
		default: {
			FrmlElem* z = PD->TArg[i].Frml;
			auto lv = *it0;
			if (lv->IsRetPar && (z->Op != _getlocvar)
				|| PD->TArg[i].FromProlog
				&& (PD->TArg[i].IsRetPar != lv->IsRetPar)) goto label1;
			LVAssignFrml(lv, oldbp, false, PD->TArg[i].Frml);
			break;
		}
		}
		/*if (it0 != LVBD.vLocVar.end())*/
		++it0;
		// (LocVar*)lv0->Chain;
	}
	//lv1 = lv0;
	it1 = it0;
	while (it0 != PD->variables.vLocVar.end()) {
		if ((*it0)->FTyp == 'r') {
			CFile = (*it0)->FD;
			CRecPtr = GetRecSpace();
			SetTWorkFlag();
			ZeroAllFlds();
			ClearDeletedFlag();
			(*it0)->RecPtr = CRecPtr;
		}
		++it0; // (LocVar*)lv0->Chain;
	}
	ProcMyBP = MyBP;
	pd1 = ReadProcBody();

	// vytvorime vektor instrukci pro snadny prehled
//#ifdef _DEBUG
	std::vector<Instr*> vI;
	Instr* next = pd1;
	while (next != nullptr) {
		vI.push_back(next);
		next = (Instr*)next->Chain;
	}
	//#endif
	FDLocVarAllowed = false;
	//lv0 = lv1;
	it0 = it1;
	while (it0 != PD->variables.vLocVar.end()/*lv0 != nullptr*/) {
		if ((*it0)->FTyp == 'i') /* !!! with WKeyDPtr(lv->RecPtr)^ do!!! */
		{
			auto hX = (XWKey*)(*it0)->RecPtr;
			if (hX->KFlds == nullptr) hX->KFlds = (*it0)->FD->Keys->KFlds;
			auto tmp = (XWKey*)(*it0)->RecPtr;
			tmp->Open(hX->KFlds, true, false);
		}
		++it0; // (LocVar*)lv0->Chain;
	}
	ReleaseStore2(p2);
	RunProcedure(pd1);
	//lv0 = lvroot;
	it0 = PD->variables.vLocVar.begin();
	i = 0;
	while (it0 != PD->variables.vLocVar.end()) {
		// projdeme navratove hodnoty (navratova hodnota funkce + VAR parametry)
		// a tyto navratove hodnoty ulozime zpet do patricneho FrmlElem
		if ((*it0)->IsRetPar) {
			auto z18 = (FrmlElem18*)PD->TArg[i].Frml;
			switch ((*it0)->FTyp) {
			case 'R': {
				z18->locvar->R = (*it0)->R;
				/*FloatPtr(Ptr(Seg(oldbp^), Ofs(oldbp^) + z->BPOfs)) =
				FloatPtr(Ptr(Seg(MyBP^), Ofs(MyBP^) + lv->BPOfs))*;*/
				break;
			}
			case 'S': {
				z18->locvar->S = (*it0)->S;
				/*l = LongintPtr(Ptr(Seg(oldbp^), Ofs(oldbp^) + z->BPOfs))*;
				LongintPtr(Ptr(Seg(oldbp^), Ofs(oldbp^) + z->BPOfs)) ^ =
				LongintPtr(Ptr(Seg(MyBP^), Ofs(MyBP^) + lv->BPOfs))*;
				LongintPtr(Ptr(Seg(MyBP^), Ofs(MyBP^) + lv->BPOfs)) ^ = l;*/
				break; }
			case 'B': {
				z18->locvar->B = (*it0)->B;
				/*BooleanPtr(Ptr(Seg(oldbp^), Ofs(oldbp^) + z->BPOfs)) ^ =
				BooleanPtr(Ptr(Seg(MyBP^), Ofs(MyBP^) + lv->BPOfs))^*/
				break;
			}
			}
		}
		if (i > n)
			switch ((*it0)->FTyp) {
			case 'r': {
				CFile = (*it0)->FD;
				ClearRecSpace((*it0)->RecPtr);
				break;
			}
			case 'i': {
				CFile = (*it0)->FD;
				((XWKey*)(*it0)->RecPtr)->Close();
				break;
			}
			}
		i++;
		it0++; // (LocVar*)lv0->Chain;
	}
	PopProcStk();
	ProcMyBP = (ProcStkD*)oldprocbp;
	LinkDRoot = ld;
	CFile = (FileD*)lstFD->Chain;
	while (CFile != nullptr)
	{
		CloseFile();
		CFile = (FileD*)CFile->Chain;
	}
	lstFD->Chain = nullptr;
	ReleaseBoth(p1, p2);
}

void RunMainProc(RdbPos RP, bool NewWw)
{
	void* p1 = nullptr; void* p2 = nullptr;
	LocVar* lv = nullptr;
	if (NewWw) {
		ProcAttr = screen.colors.uNorm;
		screen.Window(1, 2, TxtCols, TxtRows);
		TextAttr = ProcAttr;
		ClrScr();
		UserHeadLine("");
		MenuX = 1; MenuY = 2;
	}
	auto PD = new Instr_proc(0); // GetPInstr(_proc, sizeof(RdbPos) + 2);
	PD->PPos = RP;
	CallProcedure(PD);
	ReleaseStore(PD);
	if (NewWw) screen.Window(1, 1, TxtCols, TxtRows);
}
