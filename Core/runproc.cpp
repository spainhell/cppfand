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
#include "RunMessage.h"
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
#include "../fandio/XWKey.h"
#include "../Editor/OldEditor.h"
#include "../Editor/EditorHelp.h"
#include "../Editor/runedi.h"
#include "../ExportImport/ExportImport.h"
#include "../MergeReport/ReportGenerator.h"
#include "../MergeReport/Merge.h"
#include "../MergeReport/Report.h"
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
		const std::unique_ptr report = std::make_unique<Report>();
		if (RO->SyntxChk) {
			IsCompileErr = false;
			//NewExit(Ovr(), er);
			//goto label1;
			report->Read(RO);
			LastExitCode = 0;
			//label1:
				//RestoreExit(er);
			IsCompileErr = false;
			//goto label2;
			return;
		}
		report->Read(RO);
		report->Run(RO);
	}
	else {
		if (RO->WidthFrml != nullptr) {
			RO->Width = RunInt(CFile, RO->WidthFrml, CRecPtr);
		}
		if (RO->Head != nullptr) {
			RO->HeadTxt = RunStdStr(CFile, RO->Head, CRecPtr);
		}
		if (RO->UserSelFlds) {
			PromptAutoRprt(RO);
		}
		else {
			const std::unique_ptr auto_report = std::make_unique<ReportGenerator>();
			auto_report->RunAutoReport(RO);
		}
	}
	if (RO->Edit) md = 'T';
	else md = 'V';
	if (save) {
		SaveAndCloseAllFiles();
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

	const std::unique_ptr auto_report = std::make_unique<ReportGenerator>();
	if (auto_report->SelForAutoRprt(RO2)) {
		auto_report->RunAutoReport(RO2);
	}
}

void AssignField(Instr_assign* PD)
{
	WORD msg = 0;
	CFile = PD->FD;
	LockMode md = CFile->NewLockMode(WrMode);
	FieldDescr* F = PD->FldD;
	int N = RunInt(CFile, PD->RecFrml, CRecPtr);
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
	ReleaseStore(&CRecPtr);
	CFile->OldLockMode(md);
}

void AssignRecVar(LocVar* LV1, LocVar* LV2, AssignD* A)
{
	pstring EmptyStr(1);
	EmptyStr = "";
	pstring ss;
	FileD* FD1 = LV1->FD;  // destination record
	FileD* FD2 = LV2->FD;  // source record
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
	FieldDescr* field_d = PD->RecFldD;
	FileD* file_d = PD->AssLV->FD;
	void* record = PD->AssLV->RecPtr;

	file_d->SetUpdFlag(record);
	AssgnFrml(file_d, record, field_d, PD->Frml, file_d->HasTWorkFlag(record), PD->Add);
}

void SortProc(FileD* FD, KeyFldD* SK)
{
	LockMode md = FD->NewLockMode(ExclMode);
	FD->FF->SortAndSubst(SK);
	FD->OldLockMode(md);
	SaveAndCloseAllFiles();
}

void MergeProc(Instr_merge_display* PD)
{
	void* p = nullptr; void* p2 = nullptr;
	MarkBoth(p, p2);
	SetInpTT(&PD->Pos, true);
	
	const std::unique_ptr merge = std::make_unique<Merge>();
	merge->Read();
	merge->Run();

	SaveAndCloseAllFiles();
	ReleaseStore(&p);
	ReleaseStore(&p2);
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
				t = t + RunShortStr(CFile, W->Frml, CRecPtr);
			}
			else {
				std::string str = RunStdStr(CFile, W->Frml, CRecPtr);
				printS += str;
			}
			W = W->pChain;
			continue;
			break;
		}
		case 'B': {
			if (RunBool(CFile, W->Frml, CRecPtr)) x = AbbrYes;
			else x = AbbrNo;
			break;
		}
		case 'F': {
			r = RunReal(CFile, W->Frml, CRecPtr);
			if (W->M == 255) str(r, W->N, x);
			else str(r, W->N, W->M, x);
			break;
		}
		case 'D': x = StrDate(RunReal(CFile, W->Frml, CRecPtr), *W->Mask); break;
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
				Help(PD->mHlpRdb, RunShortStr(CFile, PD->mHlpFrml, CRecPtr), false);
				continue;
			}
			break;
		}
		case WriteType::message: {
			SetMsgPar(t);
			WrLLF10Msg(110);
			if (Event.Pressed.KeyCombination() == __F1) {
				Help(PD->mHlpRdb, RunShortStr(CFile, PD->mHlpFrml, CRecPtr), false);
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
		str = GetHlpText(CRdb, RunShortStr(CFile, (FrmlElem*)R, CRecPtr), true, i);
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
	RunWFrml(CFile, PD->W2, 0, v, CRecPtr);
	WORD a = RunWordImpl(CFile, PD->Attr2, screen.colors.uNorm, CRecPtr);
	char c = ' ';
	if (PD->FillC != nullptr) {
		std::string s = RunShortStr(CFile, PD->FillC, CRecPtr);
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
	std::string s = RunStdStr(CFile, PD->Param, CRecPtr);
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
	ReleaseStore(&p);
	if (!b) {
		GoExit();
	}
}

void MountProc(WORD CatIRec, bool NoCancel)
{
	try {
		SaveAndCloseAllFiles();
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
	SaveAndCloseAllFiles();
	CFile = PD->EditFD;

	// TODO: is needed to make copy of EditOptions before call edit?

	const bool selFlds = SelFldsForEO(&PD->EO, nullptr);
	if (!PD->EO.UserSelFlds || selFlds) {
		EditDataFile(CFile, &PD->EO);
	}
	SaveAndCloseAllFiles();

	// TODO: and here delete copy?
}

std::string GetStr(FrmlElem* Z)
{
	std::string result;
	if (Z == nullptr) return result;
	result = RunShortStr(CFile, Z, CRecPtr);
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
	if (PD->TxtPos != nullptr) i = RunInt(CFile, PD->TxtPos, CRecPtr);
	EdUpdated = false;
	a = RunWordImpl(CFile, PD->Atr, 0, CRecPtr);
	pv = nullptr;
	if (PD->Ww.C1 != nullptr) {
		RunWFrml(CFile, PD->Ww, PD->WFlags, v, CRecPtr);
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
	if (PD->ErrMsg != nullptr) msg = RunStdStr(CFile, PD->ErrMsg, CRecPtr);
	EditTxtFile(lp, PD->EdTxtMode, msg, PD->ExD, i,
		RunInt(CFile, PD->TxtXY, CRecPtr), pv, a,
		RunShortStr(CFile, PD->Hd, CRecPtr), PD->WFlags, &MsgS);
	ReleaseStore(&p);
}

void PrintTxtProc(Instr_edittxt* PD)
{
	LongStr* s = nullptr;
	/* !!! with PD^ do!!! */
	if (PD->TxtLV != nullptr) {
		//s = TWork.Read(1, *(int*)(uintptr_t(MyBP) + PD->TxtLV->BPOfs));
		PrintArray(s->A, s->LL, false);
		delete s; s = nullptr;
	}
	else {
		SetTxtPathVol(PD->TxtPath, PD->TxtCatIRec);
		PrintTxtFile(0);
	}
}

bool SrchXKey(XKey* K, XString& X, int& N)
{
	if (CFile->FF->file_type == FileType::INDEX) {
		CFile->FF->TestXFExist();
		return K->SearchInterval(CFile, X, false, N);
	}
	else {
		BYTE* record = CFile->GetRecSpace();
		bool result = CFile->SearchKey(X, K, N, record);
		delete[] record; record = nullptr;
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
		x.S = RunShortStr(CFile, PD->RecNr, CRecPtr);
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
		n = RunInt(CFile, PD->RecNr, CRecPtr);
		if ((n <= 0) || (n > CFile->FF->NRecs)) {
			CFile->OldLockMode(md);
			delete[] CRecPtr;
			return;
		}
	}
	CFile->ReadRec(n, CRecPtr);
	if (PD->AdUpd && !CFile->DeletedFlag(CRecPtr)) {
		LastExitCode = (!RunAddUpdate(CFile, '-', nullptr, nullptr, CRecPtr));
	}
	if (CFile->FF->file_type == FileType::INDEX) {
		if (!CFile->DeletedFlag(CRecPtr)) CFile->FF->DeleteXRec(n, true, CRecPtr);
	}
	else {
		CFile->DeleteRec(n, CRecPtr);
	}
	CFile->OldLockMode(md);
	ReleaseStore(&CRecPtr);
}

void AppendRecProc(FileD* file_d)
{
	LockMode md = file_d->NewLockMode(CrMode);
	BYTE* record = file_d->GetRecSpace();
	file_d->ZeroAllFlds(record, false);
	file_d->SetDeletedFlag(record);
	file_d->CreateRec(file_d->FF->NRecs + 1, record);
	delete[] record; record = nullptr;
	file_d->OldLockMode(md);
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
			LastExitCode = !RunAddUpdate(CFile, '+', nullptr, nullptr, CRecPtr);
		}
		else {
			LastExitCode = !RunAddUpdate(CFile, 'd', cr2, nullptr, CRecPtr);
		}
	}
	if (CFile->FF->file_type == FileType::INDEX) {
		CFile->FF->OverWrXRec(N, cr2, CR, CRecPtr);
	}
	else {
		CFile->WriteRec(N, CRecPtr);
	}
	if (!del) {
		CFile->DelAllDifTFlds(cr2, nullptr);
	}
	ReleaseStore(&cr2);
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
		x.S = RunShortStr(CFile, PD->RecNr, CRecPtr);
#ifdef FandSQL
		if (CFile->IsSQLFile) {
			if (IsRead) if (Strm1->SelectXRec(k, @x, PD->CompOp, true)) goto label4; else goto label2;
			else if (Strm1->UpdateXRec(k, @x, ad)) goto label4; else goto label2;
		}
#endif
	}
	else N = RunInt(CFile, PD->RecNr, CRecPtr);
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
					//CFile->DelTFlds(CRecPtr);
					CFile->ZeroAllFlds(CRecPtr, true);
					ReleaseStore(&cr);
					CFile->OldLockMode(md);
					return;
				}
				else {
				label1:
					CFile->NewLockMode(CrMode);
					CFile->FF->TestXFExist();
					CFile->IncNRecs(1);
					app = true;
				}
			N = CFile->FF->NRecs;
		}
		else if (!SrchXKey(k, x, N)) {
			if (IsRead) {
				//CFile->DelTFlds(CRecPtr);
				CFile->ZeroAllFlds(CRecPtr, true);
				CFile->SetDeletedFlag(CRecPtr);
				ReleaseStore(&cr);
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
		CFile->ReadRec(N, cr);
		CRecPtr = PD->LV->RecPtr;
		//CFile->DelTFlds(PD->LV->RecPtr);
		CFile->CopyRecWithT(cr, PD->LV->RecPtr, true);
	}
	else {
		CFile->CopyRecWithT(PD->LV->RecPtr, cr, false);
		if (app) {
			CRecPtr = cr;
			if (CFile->FF->file_type == FileType::INDEX) {
				CFile->RecallRec(N, CRecPtr);
			}
			else {
				CFile->WriteRec(N, CRecPtr);
			}
			if (ad) {
				LastExitCode = !RunAddUpdate(CFile, '+', nullptr, nullptr, CRecPtr);
			}
		}
		else {
			UpdRec(cr, N, ad);
		}
	}

	ReleaseStore(&cr);
	CFile->OldLockMode(md);
}

void LinkRecProc(Instr_assign* PD)
{
	void* p = nullptr;
	void* r2 = nullptr;
	void* lr2 = nullptr;
	FileD* cf = nullptr;
	void* cr = nullptr;
	LinkD* ld = nullptr;

	int n = 0;
	cf = CFile;
	cr = CRecPtr;
	MarkStore(p);
	ld = PD->LinkLD;
	CRecPtr = PD->RecLV1->RecPtr;
	lr2 = PD->RecLV2->RecPtr;
	CFile = ld->ToFD;
	CFile->ClearRecSpace(lr2);
	CFile = ld->FromFD;
	BYTE* rec = nullptr;
	if (LinkUpw(CFile, ld, n, true, CRecPtr, &rec)) {
		LastExitCode = 0;
	}
	else {
		LastExitCode = 1;
	}
	//CFile->DelTFlds(lr2);
	CFile->CopyRecWithT(rec, lr2, true);
	delete[] rec; rec = nullptr;

	ReleaseStore(&p);
	CFile = cf;
	CRecPtr = cr;
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
	Bool = RunEvalFrml(CFile, PD->CBool, CRecPtr);
	lk = false;
#ifdef FandSQL
	if (PD->inSQL && !FD->IsSQLFile) return;
#endif
	if (LD != nullptr) {
		CFile = LD->ToFD; KF = LD->ToKey->KFlds;
		switch (PD->COwnerTyp) {
		case 'r': {
			CRecPtr = PD->CLV->RecPtr;
			xx.PackKF(CFile, KF, CRecPtr);
			break;
		}
		case 'F': {
			md = CFile->NewLockMode(RdMode);
			CRecPtr = CFile->GetRecSpace();
			CFile->ReadRec(RunInt(CFile, (FrmlElem*)PD->CLV, CRecPtr), CRecPtr);
			xx.PackKF(CFile, KF, CRecPtr);
			ReleaseStore(&p);
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
			xScan->Reset(Bool, PD->CSQLFilter, CRecPtr);
		}
#ifdef FandSQL
	if (!CFile->IsSQLFile)
#endif
		if (Key != nullptr) {
			if (PD->CWIdx) {
				CFile->FF->ScanSubstWIndex(xScan, Key->KFlds, 'W');
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
	xScan->GetRec(CRecPtr);
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
				CFile->ClearUpdFlag(lr);
				//CFile->DelTFlds(lr);
				CFile->CopyRecWithT(cr, lr, true);
			}
		//if (LVi != nullptr) *(double*)(LocVarAd(LVi)) = Scan->RecNr; // metoda LocVarAd byla odstranena z access.cpp
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
			OpenCreateF(CFile, CPath, Shared);
			if ((LVr != nullptr) && (LVi == nullptr) && CFile->HasUpdFlag(CRecPtr)) {
				md1 = CFile->NewLockMode(WrMode);
				CFile->CopyRecWithT(lr, cr, false);
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
	ReleaseStore(&p);
	BreakP = false;
}

void HeadLineProc(FrmlElem* Z)
{
	UserHeadLine(RunShortStr(CFile, Z, CRecPtr));
}

void SetKeyBufProc(FrmlElem* Z)
{
	//KbdBuffer = RunShortStr(Z);
	std::string keyBuf = RunShortStr(CFile, Z, CRecPtr);
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

	ProcAttr = RunWordImpl(CFile, PD->Attr, screen.colors.uNorm, CRecPtr); // nacte barvy do ProcAttr
	RunWFrml(CFile, PD->W, PD->WithWFlags, v, CRecPtr); // nacte rozmery okna
	auto top = RunShortStr(CFile, PD->Top, CRecPtr); // nacte nadpis
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
			ld->N = RunInt(CFile, ld->Frml, CRecPtr);
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
				OpenCreateF(CFile, CPath, Shared);
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
			SetPathAndVolume(CFile);
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
	Help(PD->HelpRdb0, RunShortStr(CFile, PD->Frml0, CRecPtr), true);
}

HANDLE OpenHForPutTxt(Instr_puttxt* PD)
{
	SetTxtPathVol(PD->TxtPath1, PD->TxtCatIRec1);
	TestMountVol(CPath[1]);
	FileOpenMode m = _isOverwriteFile;
	if (PD->App) m = _isOldNewFile;
	HANDLE h = OpenH(CPath, m, Exclusive);
	TestCPathError();
	if (PD->App) SeekH(h, FileSizeH(h));
	return h;
}

void PutTxt(Instr_puttxt* PD)
{
	HANDLE h = nullptr;
	FrmlElem* z = nullptr; pstring pth;
	z = PD->Txt;

	FileD* TFD02;
	FandTFile* TF02;
	int TF02Pos;

	// TODO: this causes problem, file is never saved
	// const bool canCopyT = false;
	const bool canCopyT = CanCopyT(CFile, nullptr, z, &TF02, &TFD02, TF02Pos, CRecPtr);

	if (canCopyT) {
		h = OpenHForPutTxt(PD);
		pth = CPath;
		CopyTFStringToH(CFile, h, TF02, TFD02, TF02Pos);
		CPath = pth;
	}
	else {
		std::string s = RunStdStr(CFile, z, CRecPtr);
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
	if (CFile != nullptr) CFile->CloseFile();
	std::string data = RunShortStr(CFile, PD->Frml3, CRecPtr);
	CatFD->SetField(PD->CatIRec, PD->CatFld, data);
}

void AssgnAccRight(Instr_assign* PD)
{
	AccRight = RunShortStr(CFile, PD->Frml, CRecPtr);
}

void AssgnUserName(Instr_assign* PD)
{
	UserName = RunShortStr(CFile, PD->Frml, CRecPtr);
}

void ReleaseDriveProc(FrmlElem* Z)
{
	SaveAndCloseAllFiles();
	pstring s = RunShortStr(CFile, Z, CRecPtr);
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
			CFile->CloseFile();
			CFile->CatIRec = CatFD->GetCatalogIRec(CFile->Name, CFile->FF->file_type == FileType::RDB);
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
	int N = RunInt(CFile, PD->RecNr, CRecPtr);
	CRecPtr = CFile->GetRecSpace();
	LockMode md = CFile->NewLockMode(CrMode);
	if ((N > 0) && (N <= CFile->FF->NRecs)) {
		CFile->ReadRec(N, CRecPtr);
		if (CFile->DeletedFlag(CRecPtr)) {
			CFile->RecallRec(N, CRecPtr);
			if (PD->AdUpd) {
				LastExitCode = !RunAddUpdate(CFile, '+', nullptr, nullptr, CRecPtr);
			}
		}
	}
	CFile->OldLockMode(md);
	ReleaseStore(&CRecPtr);
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
			Instr_loops* iPD = (Instr_loops*)PD;
			if (RunBool(CFile, iPD->Bool, CRecPtr)) {
				RunInstr(iPD->Instr1);
			}
			else {
				RunInstr(iPD->ElseInstr1);
			}
			break;
		}
		case _whiledo: {
			Instr_loops* iPD = (Instr_loops*)PD;
			while (!ExitP && !BreakP && RunBool(CFile, iPD->Bool, CRecPtr)) {
				RunInstr(iPD->Instr1);
			}
			BreakP = false;
			break;
		}
		case _repeatuntil: {
			Instr_loops* iPD = (Instr_loops*)PD;
			do {
				RunInstr(iPD->Instr1);
			} while (!(ExitP || BreakP || RunBool(CFile, iPD->Bool, CRecPtr)));
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
		case _save: {
			SaveAndCloseAllFiles();
			break;
		}
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
			WORD x = RunInt(CFile, iPD->GoX, CRecPtr);
			WORD y = RunInt(CFile, iPD->GoY, CRecPtr);
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
			LVAssignFrml(CFile, iPD->AssLV, iPD->Add, iPD->Frml, CRecPtr);
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
			CFile->AssignNRecs(iPD->Add, RunInt(CFile, iPD->Frml, CRecPtr));
			break;
		}
		case _appendrec: {
			FileD* rec_file = ((Instr_recs*)PD)->RecFD;
			AppendRecProc(rec_file);
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
			UserCode = RunInt(CFile, ((Instr_assign*)PD)->Frml, CRecPtr);
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
			userToday = RunReal(CFile, ((Instr_assign*)PD)->Frml, CRecPtr);
			break;
		}
		case _asgnclipbd: {
			LongStr* s = RunLongStr(CFile, ((Instr_assign*)PD)->Frml, CRecPtr);
			TWork.Delete(ClpBdPos);
			ClpBdPos = TWork.Store(s->A, s->LL);
			delete s; s = nullptr;
			break;
		}
		case _asgnedok: {
			EdOk = RunBool(CFile, ((Instr_assign*)PD)->Frml, CRecPtr);
			break;
		}
		case _turncat: {
			auto iPD = (Instr_turncat*)PD;
			CatFD->TurnCat(iPD->NextGenFD, iPD->FrstCatIRec, iPD->NCatIRecs, RunInt(CFile, iPD->TCFrml, CRecPtr));
			break;
		}
		case _releasedrive: {
			ReleaseDriveProc(((Instr_releasedrive*)PD)->Drive);
			break;
		}
		case _setprinter: {
			SetCurrPrinter(abs(RunInt(CFile, ((Instr_assign*)PD)->Frml, CRecPtr)));
			break;
		}
		case _indexfile: {
			auto iPD = (Instr_indexfile*)PD;
			iPD->IndexFD->FF->IndexFileProc(iPD->Compress);
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
			Delay((RunInt(CFile, ((Instr_assign*)PD)->Frml, CRecPtr) + 27) / 55);
			break;
		}
		case _sound: {
			Sound(RunInt(CFile, ((Instr_assign*)PD)->Frml, CRecPtr));
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
				ForAllFDs(ForAllFilesOperation::close_passive_fd);
			}
			else if (!CFile->FF->IsShared() || (CFile->FF->LMode == NullMode)) {
				CFile->CloseFile();
			}
			break;
		}
		case _backup: {
			auto iPD = (Instr_backup*)PD;
			BackUp(iPD->IsBackup, iPD->NoCompress, iPD->BrCatIRec, iPD->BrNoCancel);
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
			GetIndex((Instr_getindex*)PD);
			break;
		}
		case _setmouse: {
			auto iPD = (Instr_setmouse*)PD;
			SetMouse(RunInt(CFile, iPD->MouseX, CRecPtr),
				RunInt(CFile, iPD->MouseY, CRecPtr),
				RunBool(CFile, iPD->Show, CRecPtr));
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
			srand(RunInt(CFile, ((Instr_assign*)PD)->Frml, CRecPtr));
			break;
		}
		case _randomize: {
			Random();
			break;
		}
		case _asgnxnrecs: {
			((Instr_assign*)PD)->xnrIdx->Release(CFile);
			break;
		}
		case _portout: {
			auto iPD = (Instr_portout*)PD;
			PortOut(RunBool(CFile, iPD->IsWord, CRecPtr),
				(WORD)(RunInt(CFile, iPD->Port, CRecPtr)),
				(WORD)(RunInt(CFile, iPD->PortWhat, CRecPtr)));
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
	if (srcCode.find("PARAM1.nrecs>1: PARAM1.nrecs:=1; end; close(PARAM1);") != std::string::npos) {
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
				std::string code = RunStdStr(CFile, PD->TArg[i].TxtFrml, CRecPtr);
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
			LVAssignFrml(CFile, lv, false, PD->TArg[i].Frml, CRecPtr);
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
			CFile->ZeroAllFlds(CRecPtr, false);
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
			tmp->Open(CFile, hX->KFlds, true, false);
		}
		++it0;
	}
	ReleaseStore(&p2);

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
				CFile->ClearRecSpace((*it0)->RecPtr);
				break;
			}
			case 'i': {
				CFile = (*it0)->FD;
				((XWKey*)(*it0)->RecPtr)->Close(CFile);
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
		CFile->CloseFile();
		CFile = CFile->pChain;
	}
	lstFD->pChain = nullptr;
	ReleaseStore(&p1);
	ReleaseStore(&p2);
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
	delete PD; PD = nullptr;
	if (NewWw) screen.Window(1, 1, TxtCols, TxtRows);
}
