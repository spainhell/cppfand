#include "runproc.h"

#include <fstream>

#include "Coding.h"
#include "../pascal/random.h"
#include "compile.h"
#include "OldDrivers.h"
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
#include "rdproc.h"
#include "runfrml.h"
#include "runproj.h"
#include "wwmenu.h"
#include "wwmix.h"
#include "../Prolog/RunProlog.h"
#include "../fandio/sort.h"
#include "../fandio/files.h"
#include "../fandio/FandXFile.h"
#include "../Editor/OldEditor.h"
#include "../Editor/EditorHelp.h"
#include "../Editor/runedi.h"
#include "../ExportImport/ExportImport.h"
#include "../MergeReport/genrprt.h"
#include "../MergeReport/rdmerg.h"
#include "../MergeReport/rdrprt.h"
#include "../MergeReport/runmerg.h"
#include "../MergeReport/runrprt.h"
#include "../Common/textfunc.h"


void UserHeadLine(std::string UserHeader)
{
	WParam* p = PushWParam(1, 1, TxtCols, 1, true);
	TextAttr = screen.colors.fNorm;
	ClrEol();
	WORD maxlen = TxtCols - 10;
	WORD l = GetLengthOfStyledString(UserHeader);
	if (l >= maxlen) {
		UserHeader = GetStyledStringOfLength(UserHeader, 0, maxlen);
	}
	WORD n = (TxtCols - l) / 2;

	if (n > 0) {
		//printf(); 
		char buf[MaxTxtCols]{ '\0' };
		size_t len = snprintf(buf, MaxTxtCols, "%*c", n, ' ');
		screen.ScrWrText(screen.WhereX(), screen.WhereY(), buf);
	}

	screen.WriteStyledStringToWindow(UserHeader, screen.colors.fNorm);
	screen.ScrWrText(TxtCols - 10, 1, CppToday().c_str());

	PopWParam(p);
	delete p;
}

void ReportProc(RprtOpt* RO, bool save)
{
	//void* p = nullptr;
	//void* p2 = nullptr;
	char md = '\0';
	int w = 0;
	//ExitRecord er;
	//MarkBoth(p, p2);
	PrintView = false;
	/* !!! with RO^ do!!! */
	if (RO->Flds.empty()) {
		SetInpTT(&RO->RprtPos, true);
		if (RO->SyntxChk) {
			IsCompileErr = false;
			//NewExit(Ovr(), er);
			//goto label1;
			ReadReport(RO);
			LastExitCode = 0;
			//label1:
				//RestoreExit(er);
			IsCompileErr = false;
			//goto label2;
			return;
		}
		ReadReport(RO);
		RunReport(RO);
	}
	else {
		if (RO->WidthFrml != nullptr) {
			RO->Width = RunInt(RO->WidthFrml);
		}
		if (RO->Head != nullptr) {
			RO->HeadTxt = RunStdStr(RO->Head);
		}
		if (RO->UserSelFlds) {
			PromptAutoRprt(RO);
		}
		else {
			RunAutoReport(RO);
		}
	}
	if (RO->Edit) md = 'T';
	else md = 'V';
	if (save) {
		SaveFiles();
	}
	if (PrintView) {
		w = PushW(1, 1, TxtCols, TxtRows);
		SetPrintTxtPath();
		std::string errMessage;
		std::vector<EdExitD*> emptyEdExit;
		EditTxtFile(nullptr, md, errMessage, emptyEdExit, 0, 0, nullptr, 0, "", 0, nullptr);
		PopW(w);
	}
	//label2:
		//ReleaseBoth(p, p2);
}

void PromptAutoRprt(RprtOpt* RO)
{
	wwmix ww;

	RprtOpt* RO2;
	RO2 = new RprtOpt();
	Move(RO, RO2, sizeof(*RO));
	//FieldList FL = RO->Flds;
	//while (FL != nullptr) {
	for (auto& f : RO->Flds) {
		//FieldDescr* F = FL->FldD;
		if ((f->Flg & f_Stored) != 0) ww.PutSelect(f->Name);
		else {
			pstring tmpStr = SelMark;
			ww.PutSelect(tmpStr + f->Name);
		}
		//FL = (FieldList)FL->pChain;
	}
	CFile = RO->FDL.FD;
	if (!ww.SelFieldList(36, true, RO2->Flds)) return;
	if ((RO->FDL.Cond == nullptr) &&
		!ww.PromptFilter("", &RO2->FDL.Cond, &RO2->CondTxt)) return;
	if (SelForAutoRprt(RO2)) {
		RunAutoReport(RO2);
	}
}

void AssignField(Instr_assign* PD)
{
	WORD msg = 0;
	CFile = PD->FD;
	LockMode md = CFile->NewLockMode(WrMode);
	FieldDescr* F = PD->FldD;
	int N = RunInt(PD->RecFrml);
	if ((N <= 0) || (N > CFile->FF->NRecs)) {
		msg = 640;
		goto label1;
	}
	CRecPtr = CFile->GetRecSpace();
	CFile->ReadRec(N, CRecPtr);
	if (PD->Indexarg && !CFile->DeletedFlag(CRecPtr)) {
		msg = 627;
	label1:
		SetMsgPar(CFile->Name, F->Name);
		CFile->RunErrorM(md);
		RunError(msg);
	}
	AssgnFrml(CFile, CRecPtr, F, PD->Frml, true, PD->Add);
	CFile->WriteRec(N, CRecPtr);
	ReleaseStore(CRecPtr);
	CFile->OldLockMode(md);
}

void AssignRecVar(LocVar* LV1, LocVar* LV2, AssignD* A)
{
	pstring EmptyStr(1);
	EmptyStr = "";
	pstring ss;
	FileD* FD1 = LV1->FD;
	FileD* FD2 = LV2->FD;
	void* RP1 = LV1->RecPtr;
	void* RP2 = LV2->RecPtr;

	while (A != nullptr) {
		switch (A->Kind) {
		case _zero: {
			FieldDescr* F = A->outputFldD;
			CFile = FD1;
			CRecPtr = RP1;
			switch (F->frml_type) {
			case 'S': { CFile->saveS(F, EmptyStr, CRecPtr); break; }
			case 'R': { CFile->saveR(F, 0.0, CRecPtr); break; }
			default: { CFile->saveB(F, false, CRecPtr); break; }
			}
			break;
		}
		case _output: {
			CFile = FD1;
			CRecPtr = RP1;
			((FrmlElem8*)A->Frml)->NewRP = RP2;
			AssgnFrml(CFile, CRecPtr, A->OFldD, A->Frml, false, false);
			break;
		}
		}
		A = A->pChain;
	}
	CFile = FD1; CRecPtr = RP1;
	CFile->SetUpdFlag(CRecPtr);
}

void AssignRecFld(Instr_assign* PD)
{
	FieldDescr* F = PD->RecFldD;
	FileD* FD = PD->AssLV->FD;
	void* record = PD->AssLV->RecPtr;

	CFile = PD->AssLV->FD; // TODO: odstranit
	CRecPtr = PD->AssLV->RecPtr; // TODO: odstranit

	FD->SetUpdFlag(record);
	AssgnFrml(CFile, CRecPtr, F, PD->Frml, FD->HasTWorkFlag(record), PD->Add);
}

void SortProc(FileD* FD, KeyFldD* SK)
{
	CFile = FD;
	LockMode md = CFile->NewLockMode(ExclMode);
	SortAndSubst(SK);
	CFile = FD;
	CFile->OldLockMode(md);
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
	WORD i = 0; char c = '\0';
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
			W = W->pChain;
			continue;
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
		if (LF == WriteType::message || LF == WriteType::msgAndHelp) {
			t = t + x;
		}
		else {
			printS += x;
		}

		W = W->pChain;
	}
	screen.WriteStyledStringToWindow(printS, ProcAttr);

	while (true) {
		switch (LF) {
		case WriteType::writeln: {
			screen.LF();
			break;
		}
		case WriteType::msgAndHelp: {
			F10SpecKey = __F1;
			SetMsgPar(t);
			WrLLF10Msg(110);
			if (Event.Pressed.KeyCombination() == __F1) {
				Help(PD->mHlpRdb, RunShortStr(PD->mHlpFrml), false);
				continue;
			}
			break;
		}
		case WriteType::message: {
			SetMsgPar(t);
			WrLLF10Msg(110);
			if (Event.Pressed.KeyCombination() == __F1) {
				Help(PD->mHlpRdb, RunShortStr(PD->mHlpFrml), false);
				continue;
			}
			break;
		}
		}
		break;
	}
}

void DisplayProc(RdbD* R, WORD IRec)
{
	std::string str;
	LongStr* S = nullptr;
	void* p = nullptr; WORD i = 0;
	if (IRec == 0) {
		str = GetHlpText(CRdb, RunShortStr((FrmlElem*)R), true, i);
		if (str.empty()) return;
	}
	else {
		CFile = R->FD;
		CRecPtr = Chpt->FF->RecPtr;
		CFile->ReadRec(IRec, CRecPtr);
		LongStr* S = CFile->FF->TF->Read(CFile->loadT(ChptTxt, CRecPtr));
		if (R->Encrypted) Coding::CodingLongStr(CFile, S);
		str = std::string(S->A, S->LL);
		delete S; S = nullptr;
	}
	screen.WriteStyledStringToWindow(str, ProcAttr);
}

void ClrWwProc(Instr_clrww* PD)
{
	WRect v;
	RunWFrml(PD->W2, 0, v);
	WORD a = RunWordImpl(PD->Attr2, screen.colors.uNorm);
	char c = ' ';
	if (PD->FillC != nullptr) {
		std::string s = RunShortStr(PD->FillC);
		if (s.length() > 0) {
			c = s[0];
		}
	}
	screen.ScrClr(v.C1, v.R1, v.C2 - v.C1 + 1, v.R2 - v.R1 + 1, c, a);
}

void ExecPgm(Instr_exec* PD)
{
	const Wind wmin = WindMin;
	const Wind wmax = WindMax;
	const TCrs crs = screen.CrsGet();
	const int w = PushW(1, 1, TxtCols, 1);
	WindMin = wmin;
	WindMax = wmax;
	screen.CrsSet(crs);
	std::string s = RunStdStr(PD->Param);
	WORD i = PD->ProgCatIRec;
	CVol = "";
	std::string prog;
	if (i != 0) {
		prog = CatFD->GetPathName(i);
	}
	else {
		prog = PD->ProgPath;
	}
	bool b = OSshell(prog, s, PD->NoCancel, PD->FreeMm, PD->LdFont, PD->TextMd);
	/*asm mov ah, 3; mov bh, 0; push bp; int 10H; pop bp; mov x, dl; mov y, dh;*/
	PopW(w);
	//screen.GotoXY(x - WindMin.X + 1, y - WindMin.Y + 1);
	screen.GotoXY(1, 1);
	if (!b) {
		GoExit();
	}
}

void CallRdbProc(Instr_call* PD)
{
	bool b = false;
	void* p = nullptr;
	wwmix ww;
	MarkStore(p);
	// TODO: tady se ma ulozit stav (MyBP - ProcStkD)
	b = EditExecRdb(PD->RdbNm, PD->ProcNm, PD->ProcCall, &ww);
	// TODO: tady se ma obnovit stav (MyBP - ProcStkD)
	ReleaseStore(p);
	if (!b) {
		GoExit();
	}
}

void IndexfileProc(FileD* FD, bool Compress)
{
	FileD* cf = CFile;
	CFile = FD;
	LockMode md = CFile->NewLockMode(ExclMode);
	XFNotValid();
	CRecPtr = CFile->GetRecSpace();
	if (Compress) {
		FileD* FD2 = OpenDuplF(false);
		for (int I = 1; I <= FD->FF->NRecs; I++) {
			CFile = FD;
			CFile->ReadRec(I, CRecPtr);
			if (!CFile->DeletedFlag(CRecPtr)) {
				CFile = FD2;
				CFile->PutRec(CRecPtr);
			}
		}
		if (!SaveCache(0, CFile->FF->Handle)) {
			GoExit();
		}
		CFile = FD;
		SubstDuplF(FD2, false);
	}
	CFile->FF->XF->NoCreate = false;
	TestXFExist();
	CFile->OldLockMode(md);
	SaveFiles();
	ReleaseStore(CRecPtr);
	CFile = cf;
}

void MountProc(WORD CatIRec, bool NoCancel)
{
	try {
		SaveFiles();
		CVol = CatFD->GetVolume(CatIRec);
		CPath = FExpand(CatFD->GetPathName(CatIRec));
		FSplit(CPath, CDir, CName, CExt);
		TestMountVol(CPath[1]);
		LastExitCode = 0;
	}
	catch (std::exception& e) {
		if (NoCancel) LastExitCode = 1;
		else GoExit();
	}
}

void EditProc(Instr_edit* PD)
{
	EdUpdated = false;
	SaveFiles();
	CFile = PD->EditFD;

	// TODO: is needed to make copy of EditOptions before call edit?

	const bool selFlds = SelFldsForEO(&PD->EO, nullptr);
	if (!PD->EO.UserSelFlds || selFlds) {
		EditDataFile(CFile, &PD->EO);
	}
	SaveFiles();

	// TODO: and here delete copy?
}

std::string GetStr(FrmlElem* Z)
{
	std::string result;
	if (Z == nullptr) return result;
	result = RunShortStr(Z);
	return result;
}

void EditTxtProc(Instr_edittxt* PD)
{
	int i = 0; WRect v;
	WRect* pv = nullptr; BYTE a = 0;
	std::string* lp = nullptr;
	MsgStr MsgS; void* p = nullptr;
	MarkStore(p);
	i = 1;
	if (PD->TxtPos != nullptr) i = RunInt(PD->TxtPos);
	EdUpdated = false;
	a = RunWordImpl(PD->Atr, 0);
	pv = nullptr;
	if (PD->Ww.C1 != nullptr) {
		RunWFrml(PD->Ww, PD->WFlags, v);
		pv = &v;
	}
	MsgS.Head = GetStr(PD->Head);
	MsgS.Last = GetStr(PD->Last);
	MsgS.CtrlLast = GetStr(PD->CtrlLast);
	MsgS.ShiftLast = GetStr(PD->ShiftLast);
	MsgS.AltLast = GetStr(PD->AltLast);

	if (PD->TxtLV != nullptr) {
		lp = &PD->TxtLV->S;
	}
	else {
		SetTxtPathVol(PD->TxtPath, PD->TxtCatIRec);
		lp = nullptr;
	}
	std::string msg;
	if (PD->ErrMsg != nullptr) msg = RunStdStr(PD->ErrMsg);
	EditTxtFile(lp, PD->EdTxtMode, msg, PD->ExD, i, RunInt(PD->TxtXY), pv, a, RunShortStr(PD->Hd), PD->WFlags, &MsgS);
	ReleaseStore(p);
}

void PrintTxtProc(Instr_edittxt* PD)
{
	LongStr* s = nullptr;
	/* !!! with PD^ do!!! */
	if (PD->TxtLV != nullptr) {
		//s = TWork.Read(1, *(int*)(uintptr_t(MyBP) + PD->TxtLV->BPOfs));
		PrintArray(s->A, s->LL, false);
		ReleaseStore(s);
	}
	else {
		SetTxtPathVol(PD->TxtPath, PD->TxtCatIRec);
		PrintTxtFile(0);
	}
}

bool SrchXKey(XKey* K, XString& X, int& N)
{
	void* cr;
	if (CFile->FF->file_type == FileType::INDEX) {
		TestXFExist();
		return K->SearchInterval(X, false, N);
	}
	else {
		cr = CRecPtr;
		CRecPtr = CFile->GetRecSpace();
		auto result = SearchKey(X, K, N);
		ReleaseStore(CRecPtr);
		CRecPtr = cr;
		return result;
	}
}

void DeleteRecProc(Instr_recs* PD)
{
	int n;
	XString x;
	CFile = PD->RecFD;
	CRecPtr = PD->RecFD->GetRecSpace();
	if (PD->ByKey) {
		x.S = RunShortStr(PD->RecNr);
#ifdef FandSQL
		if (CFile->IsSQLFile) { Strm1->DeleteXRec(PD->Key, &x, PD->AdUpd); delete[] CRecPtr; return; }
#endif
	}
	LockMode md = CFile->NewLockMode(DelMode);
	if (PD->ByKey) {
		if (!SrchXKey(PD->Key, x, n)) {
			CFile->OldLockMode(md);
			delete[] CRecPtr;
			return;
		}
	}
	else {
		n = RunInt(PD->RecNr);
		if ((n <= 0) || (n > CFile->FF->NRecs)) {
			CFile->OldLockMode(md);
			delete[] CRecPtr;
			return;
		}
	}
	CFile->ReadRec(n, CRecPtr);
	if (PD->AdUpd && !CFile->DeletedFlag(CRecPtr)) {
		LastExitCode = (!RunAddUpdte('-', nullptr, nullptr));
	}
	if (CFile->FF->file_type == FileType::INDEX) {
		if (!CFile->DeletedFlag(CRecPtr)) DeleteXRec(n, true);
	}
	else {
		CFile->DeleteRec(n, CRecPtr);
	}
	CFile->OldLockMode(md);
	ReleaseStore(CRecPtr);
}

void AppendRecProc()
{
	LockMode md = CFile->NewLockMode(CrMode);
	CRecPtr = CFile->GetRecSpace();
	ZeroAllFlds(CFile, CRecPtr);
	CFile->SetDeletedFlag(CRecPtr);
	CFile->CreateRec(CFile->FF->NRecs + 1, CRecPtr);
	ReleaseStore(CRecPtr);
	CFile->OldLockMode(md);
}

void UpdRec(void* CR, int N, bool AdUpd)
{
	void* cr2 = CFile->GetRecSpace();
	CRecPtr = cr2;
	CFile->ReadRec(N, CRecPtr);
	bool del = CFile->DeletedFlag(CRecPtr);
	CRecPtr = CR;
	if (AdUpd) {
		if (del) {
			LastExitCode = !RunAddUpdte('+', nullptr, nullptr);
		}
		else {
			LastExitCode = !RunAddUpdte('d', cr2, nullptr);
		}
	}
	if (CFile->FF->file_type == FileType::INDEX) {
		OverWrXRec(N, cr2, CR);
	}
	else {
		CFile->WriteRec(N, CRecPtr);
	}
	if (!del) {
		CFile->DelAllDifTFlds(cr2, nullptr);
	}
	ReleaseStore(cr2);
}

void ReadWriteRecProc(bool IsRead, Instr_recs* PD)
{
	int N = 0;
	bool app = false;
	XString x;
	WORD msg = 0;
	/* !!! with PD->LV^ do!!! */
	CFile = PD->LV->FD;
	CRecPtr = PD->LV->RecPtr;
	N = 1;
	XKey* k = PD->Key;
	bool ad = PD->AdUpd;
	LockMode md = CFile->FF->LMode;
	app = false;
	void* cr = CFile->GetRecSpace();
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
		else CFile->NewLockMode(RdMode);
	}
	else if (N == 0) {

#ifdef FandSQL
		if (CFile->IsSQLFile) { Strm1->InsertRec(ad, true); ReleaseStore(cr); CFile->OldLockMode(md); return; }
#endif
		goto label1;
	}
	else CFile->NewLockMode(WrMode);
	if (PD->ByKey) {
		if (k == nullptr/*IsParFile*/) {
			if (CFile->FF->NRecs == 0)
				if (IsRead) {
				label0:
					DelTFlds();
					ZeroAllFlds(CFile, CRecPtr);
					ReleaseStore(cr);
					CFile->OldLockMode(md);
					return;
				}
				else {
				label1:
					CFile->NewLockMode(CrMode);
					TestXFExist();
					CFile->IncNRecs(1);
					app = true;
				}
			N = CFile->FF->NRecs;
		}
		else if (!SrchXKey(k, x, N)) {
			if (IsRead) {
				DelTFlds();
				ZeroAllFlds(CFile, CRecPtr);
				CFile->SetDeletedFlag(CRecPtr);
				ReleaseStore(cr);
				CFile->OldLockMode(md);
				return;
			}
			msg = 613;
			SetMsgPar(PD->LV->Name);
			CFile->RunErrorM(md);
			RunError(msg);
		}
	}
	else {
		if ((N <= 0) || (N > CFile->FF->NRecs)) {
			msg = 641;
			SetMsgPar(PD->LV->Name);
			CFile->RunErrorM(md);
			RunError(msg);
		}
	}
	if (IsRead) {
		CRecPtr = cr;
		CFile->ReadRec(N, CRecPtr);
		CRecPtr = PD->LV->RecPtr;
		DelTFlds();
		CopyRecWithT(cr, PD->LV->RecPtr);
	}
	else {
		CopyRecWithT(PD->LV->RecPtr, cr);
		if (app) {
			CRecPtr = cr;
			if (CFile->FF->file_type == FileType::INDEX) {
				CFile->RecallRec(N, CRecPtr);
			}
			else {
				CFile->WriteRec(N, CRecPtr);
			}
			if (ad) {
				LastExitCode = !RunAddUpdte('+', nullptr, nullptr);
			}
		}
		else {
			UpdRec(cr, N, ad);
		}
	}

	ReleaseStore(cr);
	CFile->OldLockMode(md);
}

void LinkRecProc(Instr_assign* PD)
{
	void* p = nullptr; void* r2 = nullptr; void* lr2 = nullptr;
	FileD* cf = nullptr; void* cr = nullptr; LinkD* ld = nullptr; int n = 0;
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
	FileD* FD = nullptr; XKey* Key = nullptr; XKey* k = nullptr; FrmlElem* Bool = nullptr;
	LinkD* LD = nullptr; KeyInD* KI = nullptr;
	void* cr = nullptr; void* p = nullptr; void* lr = nullptr;
	XScan* xScan = nullptr; LockMode md, md1; XString xx;
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
			md = CFile->NewLockMode(RdMode);
			CRecPtr = CFile->GetRecSpace();
			CFile->ReadRec(RunInt((FrmlElem*)PD->CLV), CRecPtr);
			xx.PackKF(KF);
			ReleaseStore(p);
			CFile->OldLockMode(md);
			break;
		}
		}
	}
	CFile = FD;
#ifdef FandSQL
	sql = CFile->IsSQLFile;
#endif
	md = CFile->NewLockMode(RdMode);
	cr = CFile->GetRecSpace();
	CRecPtr = cr; lr = cr;
	xScan = new XScan(CFile, Key, KI, true);
#ifdef FandSQL
	if (PD->inSQL) Scan->ResetSQLTxt(Bool); else
#endif
		if (LD != nullptr) {
			if (PD->COwnerTyp == 'i') {
				xScan->ResetOwnerIndex(LD, PD->CLV, Bool);
			}
			else {
				xScan->ResetOwner(&xx, Bool);
			}
		}
		else {
			xScan->Reset(Bool, PD->CSQLFilter);
		}
#ifdef FandSQL
	if (!CFile->IsSQLFile)
#endif
		if (Key != nullptr) {
			if (PD->CWIdx) {
				ScanSubstWIndex(xScan, Key->KFlds, 'W');
			}
			else {
				CFile->FF->XF->UpdLockCnt++;
				lk = true;
			}
		}
	if (LVr != nullptr) {
		lr = LVr->RecPtr;
	}
	k = CFile->Keys.empty() ? nullptr : CFile->Keys[0];
	b = PD->CProcent;
	if (b) {
		RunMsgOn('F', xScan->NRecs);
	}
label1:
#ifdef FandSQL
	if (sql) CRecPtr = lr;
	else
#endif
		CRecPtr = cr;
	xScan->GetRec();
	if (b) {
		RunMsgN(xScan->IRec);
	}
	if (!xScan->eof) {
#ifdef FandSQL

		if (sql) { ClearUpdFlag; if (k != nullptr) xx.PackKF(k->KFlds) }
		else
#endif
			if (LVr != nullptr) {
				CRecPtr = lr;
				CFile->ClearUpdFlag(CRecPtr);
				DelTFlds();
				CopyRecWithT(cr, lr);
			}
		//if (LVi != nullptr) *(double*)(LocVarAd(LVi)) = Scan->RecNr;
		if (LVi != nullptr) {
			LVi->R = xScan->RecNr;
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
			OpenCreateF(CFile, Shared);
			if ((LVr != nullptr) && (LVi == nullptr) && CFile->HasUpdFlag(CRecPtr)) {
				md1 = CFile->NewLockMode(WrMode);
				CopyRecWithT(lr, cr);
				UpdRec(cr, xScan->RecNr, true);
				CFile->OldLockMode(md1);
			}
		}
		if (!(ExitP || BreakP)) {
			if (
#ifdef FandSQL
				!sql &&
#endif 
				(Key == nullptr) && (xScan->NRecs > CFile->FF->NRecs)) {
				xScan->IRec--;
				xScan->NRecs--;
			}
			goto label1;
		}
	}
	if (lk) {
		CFile->FF->XF->UpdLockCnt--;
	}
	xScan->Close();
	CFile->OldLockMode(md);
	if (b) {
		RunMsgOff();
	}
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
	int w1 = 0;
	WRect v;

	ProcAttr = RunWordImpl(PD->Attr, screen.colors.uNorm); // nacte barvy do ProcAttr
	RunWFrml(PD->W, PD->WithWFlags, v); // nacte rozmery okna
	auto top = RunShortStr(PD->Top); // nacte nadpis
	w1 = PushWFramed(v.C1, v.R1, v.C2, v.R2, ProcAttr, top, "", PD->WithWFlags); // vykresli oramovane okno s nadpisem
	if ((PD->WithWFlags & WNoClrScr) == 0) {
		ClrScr();
	}
	SetWwViewPort();
	RunInstr(PD->WwInstr);
	PopW(w1, (PD->WithWFlags & WNoPop) == 0);
	SetWwViewPort();
	ProcAttr = PAttr;
}

void WithLockedProc(Instr_withshared* PD)
{
	LockD* ld;
	int w1;
	WORD msg;
	pstring ntxt(10);
	LockMode md;
	PInstrCode op = PD->Kind;
	if (op == _withlocked) {
		ld = &PD->WLD;
		while (ld != nullptr) {
			ld->N = RunInt(ld->Frml);
			ld = ld->Chain;
		}
	}
	int w = 0;
label1:
	ld = &PD->WLD;
	while (ld != nullptr) {
		CFile = ld->FD;
		if (CFile->FF->Handle == nullptr)
			if (OpenF1(CFile, CPath, Shared)) {
				if (CFile->TryLockMode(RdMode, md, 2)) {
					OpenF2(CFile, CPath);
					CFile->OldLockMode(NullMode);
				}
				else {
					CloseClearHCFile(CFile->FF);
					goto label2;
				}
			}
			else {
				OpenCreateF(CFile, Shared);
			}
		if (CFile->FF->IsShared()) {
			if (op == _withlocked) {
				if (CFile->Lock(ld->N, 2)) {
					goto label3;
				}
			}
			else {
				if (CFile->TryLockMode(ld->Md, ld->OldMd, 2)) {
					goto label3;
				}
			}
		label2:
			UnLck(PD, ld, op);
			if (PD->WasElse) {
				RunInstr(PD->WElseInstr);
				return;
			}
			CFile = ld->FD;
			SetCPathVol(CFile);
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
	SetTxtPathVol(PD->TxtPath1, PD->TxtCatIRec1);
	TestMountVol(CPath[1]);
	FileOpenMode m = _isoverwritefile;
	if (PD->App) m = _isoldnewfile;
	FILE* h = OpenH(CPath, m, Exclusive);
	TestCPathError();
	if (PD->App) SeekH(h, FileSizeH(h));
	return h;
}

void PutTxt(Instr_puttxt* PD)
{
	FILE* h = nullptr;
	FrmlElem* z = nullptr; pstring pth;
	z = PD->Txt;

	FileD* TFD02;
	FandTFile* TF02;
	int TF02Pos;

	// TODO: this causes problem, file is never saved
	// const bool canCopyT = false;
	const bool canCopyT = CanCopyT(nullptr, z, &TF02, &TFD02, TF02Pos);

	if (canCopyT) {
		h = OpenHForPutTxt(PD);
		pth = CPath;
		CopyTFStringToH(h, TF02, TFD02, TF02Pos);
		CPath = pth;
	}
	else {
		std::string s = RunStdStr(z);
		h = OpenHForPutTxt(PD);
		WriteH(h, s.length(), (void*)s.c_str());
	}
	CPath = pth;
	TestCPathError();
	WriteH(h, 0, h)/*trunc*/;
	CloseH(&h);
}

// ulozi do katalogu hodnotu promenne
void AssgnCatFld(Instr_assign* PD)
{
	CFile = PD->FD3;
	if (CFile != nullptr) CloseFile(CFile);
	std::string data = RunShortStr(PD->Frml3);
	CatFD->SetField(PD->CatIRec, PD->CatFld, data);
}

void AssgnAccRight(Instr_assign* PD)
{
	AccRight = RunShortStr(PD->Frml);
}

void AssgnUserName(Instr_assign* PD)
{
	UserName = RunShortStr(PD->Frml);
}

void ReleaseDriveProc(FrmlElem* Z)
{
	SaveFiles();
	pstring s = RunShortStr(Z);
	char c = (char)toupper((char)s[1]);
	if (c == spec.CPMdrive) ReleaseDrive(FloppyDrives);
	else
		if ((c == 'A') || (c == 'B')) ReleaseDrive(c - '@');
}

void WithGraphicsProc(Instr* PD)
{
	throw std::exception("WithGraphicsProc() not implemented!");
	//if (IsGraphMode) RunInstr(PD);
	//else {
	//	ScrGraphMode(true, 0);
	//	SetWwViewPort();
	//	RunInstr(PD);
	//	ScrTextMode(true, false);
	//}
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
		CFile = CRdb->FD->pChain;
		while (CFile != nullptr) {
			CloseFile(CFile);
			CFile->CatIRec = GetCatalogIRec(CFile->Name, CFile->FF->file_type == FileType::RDB);
#ifdef FandSQL
			SetIsSQLFile();
#endif
			CFile = CFile->pChain;
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
	CFile = PD->RecFD;
	if (CFile->FF->file_type != FileType::INDEX) return;
	int N = RunInt(PD->RecNr);
	CRecPtr = CFile->GetRecSpace();
	LockMode md = CFile->NewLockMode(CrMode);
	if ((N > 0) && (N <= CFile->FF->NRecs)) {
		CFile->ReadRec(N, CRecPtr);
		if (CFile->DeletedFlag(CRecPtr)) {
			CFile->RecallRec(N, CRecPtr);
			if (PD->AdUpd) {
				LastExitCode = !RunAddUpdte('+', nullptr, nullptr);
			}
		}
	}
	CFile->OldLockMode(md);
	ReleaseStore(CRecPtr);
}

void UnLck(Instr_withshared* PD, LockD* Ld1, PInstrCode Op)
{
	LockD* ld = &PD->WLD;
	while (ld != Ld1) {
		CFile = ld->FD;
		if (CFile->FF->IsShared()) {
			if (Op == _withlocked) CFile->Unlock(ld->N);
			CFile->OldLockMode(ld->OldMd);
		}
		ld = ld->Chain;
	}
}

void WaitProc() // r. 604
{
	WORD w;
	do {
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
	while (!ExitP && !BreakP && (PD != nullptr)) {
		switch (PD->Kind) {
		case _ifthenelseP: {
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
			auto iPD = (Instr_loops*)PD;
			while (!ExitP && !BreakP && RunBool(iPD->Bool)) {
				RunInstr(iPD->Instr1);
			}
			BreakP = false;
			break;
		}
		case _repeatuntil: {
			auto iPD = (Instr_loops*)PD;
			do {
				RunInstr(iPD->Instr1);
			} while (!(ExitP || BreakP || RunBool(iPD->Bool)));
			BreakP = false;
			break;
		}
		case _menubox: {
			auto menu = std::make_unique<TMenuBoxP>(0, 0, nullptr, (Instr_menu*)PD);
			menu->call();
			break;
		}
		case _menubar: {
			MenuBarProc((Instr_menu*)PD);
			break;
		}
		case _forall: {
			ForAllProc((Instr_forall*)PD);
			break;
		}
		case _window: {
			WithWindowProc((Instr_window*)PD);
			break;
		}
		case _break: { BreakP = true; break; }
		case _exitP: { ExitP = true; break; }
		case _cancel: {
			// TODO: procedure GoExit; assembler; - was called here
			BreakP = true; // from ExitBuf.BrkP
			ExitP = true; // from ExitBuf.ExP
			//GoExit();
			break;
		}
		case _save: { SaveFiles(); break; }
		case _clrscr: {
			TextAttr = ProcAttr;
			ClrScr();
			break;
		}
		case _clrww: {
			ClrWwProc((Instr_clrww*)PD);
			break;
		}
		case _clreol: {
			TextAttr = ProcAttr;
			ClrEol();
			break;
		}
		case _exec: {
			ExecPgm((Instr_exec*)PD);
			break;
		}
		case _proc: {
			CallProcedure((Instr_proc*)PD);
			break;
		}
		case _call: {
			CallRdbProc((Instr_call*)PD);
			break;
		}
		case _copyfile: {
			FileCopy(((Instr_copyfile*)PD)->CD);
			break;
		}
		case _headline: {
			HeadLineProc(((Instr_assign*)PD)->Frml);
			break;
		}
		case _setkeybuf: {
			SetKeyBufProc(((Instr_assign*)PD)->Frml);
			break;
		}
		case _writeln: {
			WritelnProc((Instr_writeln*)PD);
			break;
		}
		case _gotoxy: {
			auto iPD = (Instr_gotoxy*)PD;
			WORD x = RunInt(iPD->GoX);
			WORD y = RunInt(iPD->GoY);
			screen.GotoXY(x + WindMin.X - 1, y + WindMin.Y - 1, absolute);
			break;
		}
		case _merge: {
			MergeProc((Instr_merge_display*)PD);
			break;
		}
		case _lproc: {
			auto iPD = (Instr_lproc*)PD;
			RunProlog(&iPD->lpPos, iPD->lpName);
			break;
		}
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
			LVAssignFrml(iPD->AssLV, iPD->Add, iPD->Frml);
			break;
		}
		case _asgnrecfld: {
			AssignRecFld((Instr_assign*)PD);
			break;
		}
		case _asgnrecvar: {
			auto iPD = (Instr_assign*)PD;
			AssignRecVar(iPD->RecLV1, iPD->RecLV2, iPD->Ass);
			break;
		}
		case _asgnpar: {
			// ulozi globalni parametr - do souboru
			auto iPD = (Instr_assign*)PD;
			AsgnParFldFrml(iPD->FD, iPD->FldD, iPD->Frml, iPD->Add);
			break;
		}
		case _asgnfield: {
			AssignField((Instr_assign*)PD);
			break;
		}
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
		case _deleterec: { DeleteRecProc((Instr_recs*)PD); break; }
		case _recallrec: { RecallRecProc((Instr_recs*)PD); break; }
		case _readrec: { ReadWriteRecProc(true, (Instr_recs*)PD); break; }
		case _writerec: { ReadWriteRecProc(false, (Instr_recs*)PD); break; }
		case _linkrec: { LinkRecProc((Instr_assign*)PD); break; }
		case _withshared:
		case _withlocked: { WithLockedProc((Instr_withshared*)PD); break; }
		case _edittxt: { EditTxtProc((Instr_edittxt*)PD); break; }
		case _printtxt: { PrintTxtProc((Instr_edittxt*)PD); break; }
		case _puttxt: { PutTxt((Instr_puttxt*)PD); break; }
		case _asgncatfield: { AssgnCatFld((Instr_assign*)PD); break; }
		case _asgnusercode: {
			UserCode = RunInt(((Instr_assign*)PD)->Frml);
			AccRight[0] = 0x01;
			AccRight[1] = (char)UserCode;
			break;
		}
		case _asgnaccright: {
			AssgnAccRight((Instr_assign*)PD);
			break;
		}
		case _asgnusername: {
			AssgnUserName((Instr_assign*)PD);
			break;
		}
		case _asgnusertoday: {
			userToday = RunReal(((Instr_assign*)PD)->Frml);
			break;
		}
		case _asgnclipbd: {
			LongStr* s = RunLongStr(((Instr_assign*)PD)->Frml);
			TWork.Delete(ClpBdPos);
			ClpBdPos = TWork.Store(s->A, s->LL);
			ReleaseStore(s);
			break;
		}
		case _asgnedok: {
			EdOk = RunBool(((Instr_assign*)PD)->Frml);
			break;
		}
		case _turncat: {
			auto iPD = (Instr_turncat*)PD;
			CFile = iPD->NextGenFD;
			TurnCat(iPD->FrstCatIRec, iPD->NCatIRecs, RunInt(iPD->TCFrml));
			break;
		}
		case _releasedrive: {
			ReleaseDriveProc(((Instr_releasedrive*)PD)->Drive);
			break;
		}
		case _setprinter: {
			SetCurrPrinter(abs(RunInt(((Instr_assign*)PD)->Frml)));
			break;
		}
		case _indexfile: {
			auto iPD = (Instr_indexfile*)PD;
			IndexfileProc(iPD->IndexFD, iPD->Compress);
			break;
		}
		case _display: {
			auto iPD = (Instr_merge_display*)PD;
			DisplayProc(iPD->Pos.R, iPD->Pos.IRec);
			break;
		}
		case _mount: {
			auto iPD = (Instr_mount*)PD;
			MountProc(iPD->MountCatIRec, iPD->MountNoCancel);
			break;
		}
		case _clearkeybuf: {
			ClearKbdBuf();
			break;
		}
		case _help: {
			HelpProc((Instr_help*)PD);
			break;
		}
		case _wait: {
			WaitProc();
			break;
		}
		case _beepP: {
			beep();
			break;
		}
		case _delay: {
			Delay((RunInt(((Instr_assign*)PD)->Frml) + 27) / 55);
			break;
		}
		case _sound: {
			Sound(RunInt(((Instr_assign*)PD)->Frml));
			break;
		}
		case _nosound: {
			NoSound();
			break;
		}
#ifdef FandGraph
		case _graph: {
			RunBGraph(((Instr_graph*)PD)->GD, false);
			break;
		}
		case _putpixel:
		case _line:
		case _rectangle:
		case _ellipse:
		case _floodfill:
		case _outtextxy: {
			DrawProc((Instr_graph*)PD);
			break;
		}
#endif
		case _withgraphics: {
			WithGraphicsProc(((Instr_withshared*)PD)->WDoInstr);
			break;
		}
#ifndef FandRunV
		case _memdiag: {
			MemDiagProc();
			break;
		}
#endif 
		case _closefds: {
			// zavre soubor
			CFile = ((Instr_closefds*)PD)->clFD;
			if (CFile == nullptr) {
				ForAllFDs(ClosePassiveFD);
			}
			else if (!CFile->FF->IsShared() || (CFile->FF->LMode == NullMode)) {
				CloseFile(CFile);
			}
			break;
		}
		case _backup: {
			auto iPD = (Instr_backup*)PD;
			Backup(iPD->IsBackup, iPD->NoCompress, iPD->BrCatIRec, iPD->BrNoCancel);
			break;
		}
		case _backupm: {
			BackupM((Instr_backup*)PD);
			break;
		}
		case _resetcat: {
			ResetCatalog();
			break;
		}
		case _setedittxt: {
			SetEditTxt((Instr_setedittxt*)PD);
			break;
		}
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
		case _asgnrand: {
			srand(RunInt(((Instr_assign*)PD)->Frml));
			break;
		}
		case _randomize: {
			Random();
			break;
		}
		case _asgnxnrecs: {
			((Instr_assign*)PD)->xnrIdx->Release();
			break;
		}
		case _portout: {
			auto iPD = (Instr_portout*)PD;
			PortOut(RunBool(iPD->IsWord), (WORD)(RunInt(iPD->Port)), (WORD)(RunInt(iPD->PortWhat)));
			break;
		}
		}
		PD = PD->Chain;
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
	void* p1 = nullptr;
	void* p2 = nullptr;

	std::_Vector_iterator<std::_Vector_val<std::_Simple_types<LocVar*>>> it0;
	std::_Vector_iterator<std::_Vector_val<std::_Simple_types<LocVar*>>> it1;

	WORD i = 0, j = 0, n = 0;
	int l = 0; Instr* pd1 = nullptr;
	FileD* lstFD = nullptr;
	KeyFldD* kf1 = nullptr; KeyFldD* kf2 = nullptr;

	if (PD == nullptr) return;
	MarkBoth(p1, p2);
	//oldprocbp = ProcMyBP;
	std::deque<LinkD*> ld = LinkDRoot;
	lstFD = (FileD*)LastInChain(FileDRoot);
	SetInpTT(&PD->PPos, true);

#ifdef _DEBUG
	std::string srcCode = std::string((char*)InpArrPtr, InpArrLen);
	if (srcCode.find("FILE.Path:=ADR01.Path+'{GLOB}") != std::string::npos) {
		printf("");
	}
#endif

	// save LVBD
	LocVarBlkD oldLVDB = LVBD;

	ReadProcHead("");
	PD->variables = LVBD;
	n = PD->variables.NParam;
	LocVar* lvroot = PD->variables.GetRoot();
	//oldbp = MyBP;
	//PushProcStk();
	if ((n != PD->N) && !((n == PD->N - 1) && PD->ExPar)) {
		CurrPos = 0;
		Error(119);
	}

	it0 = PD->variables.vLocVar.begin();
	// projdeme vstupni parametry funkce
	for (i = 0; i < n; i++) {
		if (PD->TArg[i].FTyp != (*it0)->FTyp) {
			CurrPos = 0;
			Error(119);
		}
		switch (PD->TArg[i].FTyp) {
		case 'r':
		case 'i': {
			if ((*it0)->FD != PD->TArg[i].FD) {
				CurrPos = 0;
				Error(119);
			}
			(*it0)->RecPtr = PD->TArg[i].RecPtr;
			break;
		}
		case 'f': {
			if (PD->TArg[i].RecPtr != nullptr) {
				const auto state = SaveCompState();
				std::string code = RunStdStr(PD->TArg[i].TxtFrml);
				SetInpStdStr(code, true);
				RdFileD(PD->TArg[i].Name, FileType::FAND16, "$");
				RestoreCompState(state);
			}
			else {
				CFile = PD->TArg[i].FD;
			}
			it1 = it0;
			while (it1 != PD->variables.vLocVar.end()) {
				if (((*it1)->FTyp == 'i' || (*it1)->FTyp == 'r')
					&& ((*it1)->FD == (*it0)->FD)) (*it1)->FD = CFile;
				++it1;
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
				&& (PD->TArg[i].IsRetPar != lv->IsRetPar)) {
				CurrPos = 0;
				Error(119);
			}
			LVAssignFrml(lv, false, PD->TArg[i].Frml);
			break;
		}
		}
		++it0;
	}
	it1 = it0;
	while (it0 != PD->variables.vLocVar.end()) {
		if ((*it0)->FTyp == 'r') {
			CFile = (*it0)->FD;
			CRecPtr = CFile->GetRecSpace();
			CFile->SetTWorkFlag(CRecPtr);
			ZeroAllFlds(CFile, CRecPtr);
			CFile->ClearDeletedFlag(CRecPtr);
			(*it0)->RecPtr = CRecPtr;
		}
		else if ((*it0)->FTyp == 'f') {
			// dynamic file definition
			//printf("");
		}
		++it0;
	}
	//ProcMyBP = MyBP;
	pd1 = ReadProcBody();

#ifdef _DEBUG
	// vytvorime vektor instrukci pro snadny prehled
	std::vector<Instr*> vI;
	Instr* next = pd1;
	while (next != nullptr) {
		vI.push_back(next);
		next = next->Chain;
	}
#endif

	FDLocVarAllowed = false;
	it0 = it1;
	while (it0 != PD->variables.vLocVar.end()) {
		if ((*it0)->FTyp == 'i') {
			auto hX = (XWKey*)(*it0)->RecPtr;
			if (hX->KFlds == nullptr) hX->KFlds = (*it0)->FD->Keys[0]->KFlds;
			auto tmp = (XWKey*)(*it0)->RecPtr;
			tmp->Open(hX->KFlds, true, false);
		}
		++it0;
	}
	ReleaseStore(p2);

	// **** RUN PROCEDURE **** //
	RunProcedure(pd1);
	// *********************** //

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
				break;
			}
			case 'S': {
				z18->locvar->S = (*it0)->S;
				break;
			}
			case 'B': {
				z18->locvar->B = (*it0)->B;
				break;
			}
			}
		}
		if (i > n) {
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
		}
		i++;
		++it0;
	}
	//PopProcStk();
	//ProcMyBP = (ProcStkD*)oldprocbp;

	LVBD = oldLVDB;
	LinkDRoot = ld;

	CFile = lstFD->pChain;
	while (CFile != nullptr) {
		CloseFile(CFile);
		CFile = CFile->pChain;
	}
	lstFD->pChain = nullptr;
	ReleaseBoth(p1, p2);
}

void RunMainProc(RdbPos RP, bool NewWw)
{
	if (NewWw) {
		ProcAttr = screen.colors.uNorm;
		screen.Window(1, 2, TxtCols, TxtRows);
		TextAttr = ProcAttr;
		ClrScr();
		UserHeadLine("");
		MenuX = 1; MenuY = 2;
	}
	auto PD = new Instr_proc(0);
	PD->PPos = RP;
	CallProcedure(PD);
	ReleaseStore(PD);
	if (NewWw) screen.Window(1, 1, TxtCols, TxtRows);
}
