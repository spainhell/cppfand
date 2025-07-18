#include "runproc.h"

#include <fstream>

#include "Coding.h"
#include "../pascal/random.h"
#include "Compiler.h"
#include "DateTime.h"
#include "OldDrivers.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "LinkD.h"
#include "oaccess.h"
#include "obase.h"
#include "obaseww.h"
#include "RunMessage.h"
#include "printtxt.h"
#include "rdfildcl.h"
#include "rdproc.h"
#include "runfrml.h"
#include "runproj.h"
#include "wwmenu.h"
#include "wwmix.h"
#include "../Prolog/RunProlog.h"
#include "../fandio/sort.h"
#include "../fandio/FandXFile.h"
#include "../fandio/XWKey.h"
#include "../TextEditor/TextEditor.h"
#include "../TextEditor/EditorHelp.h"
#include "../DataEditor/DataEditor.h"
#include "../ExportImport/ExportImport.h"
#include "../MergeReport/ReportGenerator.h"
#include "../MergeReport/Merge.h"
#include "../MergeReport/Report.h"
#include "../Common/textfunc.h"
#include "../Drivers/constants.h"


void UserHeadLine(std::string UserHeader)
{
	WParam* p = PushWParam(1, 1, TxtCols, 1, true);
	TextAttr = screen.colors.fNorm;
	ClrEol(TextAttr);
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
	EditorMode md = EditorMode::Unknown;
	int w = 0;
	//ExitRecord er;
	//MarkBoth(p, p2);
	PrintView = false;
	if (RO->Flds.empty()) {
		const std::unique_ptr report = std::make_unique<Report>();
		report->SetInput(&RO->RprtPos, true);

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
			RO->HeadTxt = RunString(CFile, RO->Head, CRecPtr);
		}
		if (RO->UserSelFlds) {
			PromptAutoRprt(RO);
		}
		else {
			const std::unique_ptr auto_report = std::make_unique<ReportGenerator>();
			auto_report->RunAutoReport(RO);
		}
	}
	if (RO->Edit) md = EditorMode::Text;
	else md = EditorMode::View;
	if (save) {
		SaveFiles();
	}
	if (PrintView) {
		w = PushW(1, 1, TxtCols, TxtRows);
		SetPrintTxtPath();
		std::string errMessage;
		std::vector<EdExitD*> emptyEdExit;
		std::unique_ptr<TextEditor> editor = std::make_unique<TextEditor>(md, TextType::Unknown);
		editor->EditTxtFile(nullptr, md, errMessage, emptyEdExit, 0, 0, nullptr, 0, "", 0, nullptr);
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
	for (FieldDescr* f : RO->Flds) {
		//FieldDescr* F = FL->FldD;
		if ((f->Flg & f_Stored) != 0) ww.PutSelect(f->Name);
		else {
			pstring tmpStr = SelMark;
			ww.PutSelect(tmpStr + f->Name);
		}
		//FL = (FieldList)FL->pChain;
	}
	CFile = RO->FDL[0]->FD;
	if (!ww.SelFieldList(CFile, 36, true, RO2->Flds)) return;
	if ((RO->FDL[0]->Cond == nullptr) &&
		!ww.PromptFilter("", &RO2->FDL[0]->Cond, &RO2->CondTxt)) return;

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

void AssignRecVar(LocVar* LV1, LocVar* LV2, std::vector<AssignD*>& A)
{
	FileD* FD1 = LV1->FD;  // destination record
	FileD* FD2 = LV2->FD;  // source record
	void* RP1 = LV1->record;
	void* RP2 = LV2->record;

	//while (A != nullptr) {
	for (AssignD* a : A) {
		switch (a->Kind) {
		case MInstrCode::_zero: {
			FieldDescr* F = a->outputFldD;
			CFile = FD1;
			CRecPtr = RP1;
			switch (F->frml_type) {
			case 'S': { CFile->saveS(F, "", CRecPtr); break; }
			case 'R': { CFile->saveR(F, 0.0, CRecPtr); break; }
			default: { CFile->saveB(F, false, CRecPtr); break; }
			}
			break;
		}
		case MInstrCode::_output: {
			CFile = FD1;
			CRecPtr = RP1;
			((FrmlElemNewFile*)a->Frml)->NewRP = RP2;
			AssgnFrml(CFile, CRecPtr, a->OFldD, a->Frml, false, false);
			break;
		}
		}
		//A = A->pChain;
	}
	CFile = FD1; CRecPtr = RP1;
	CFile->SetRecordUpdateFlag(CRecPtr);
}

void AssignRecFld(Instr_assign* PD)
{
	FieldDescr* field_d = PD->RecFldD;
	FileD* file_d = PD->AssLV->FD;
	void* record = PD->AssLV->record;

	file_d->SetRecordUpdateFlag(record);
	AssgnFrml(file_d, record, field_d, PD->Frml, file_d->HasTWorkFlag(record), PD->Add);
}

void SortProc(FileD* FD, std::vector<KeyFldD*>& SK)
{
	LockMode md = FD->NewLockMode(ExclMode);
	FD->SortByKey(SK, RunMsgOn, RunMsgN, RunMsgOff);
	FD->OldLockMode(md);
	SaveFiles();
}

void MergeProc(Instr_merge_display* PD)
{
	void* p = nullptr; void* p2 = nullptr;
	MarkBoth(p, p2);

	const std::unique_ptr merge = std::make_unique<Merge>();
	merge->SetInput(&PD->Pos, true);
	merge->Read();
	merge->Run();

	SaveFiles();

	ReleaseStore(&p);
	ReleaseStore(&p2);
}

void WritelnProc(Instr_writeln* PD)
{
	std::string t;
	std::string x;

	//WrLnD* W = &PD->WD;
	WriteType LF = PD->LF;
	TextAttr = ProcAttr;
	std::string printS;

	//while (W != nullptr) {
	for (WrLnD* W : PD->WD) {
		switch (W->Typ) {
		case 'S': {
			if (LF == WriteType::message || LF == WriteType::msgAndHelp) {
				t += RunString(CFile, W->Frml, CRecPtr);
			}
			else {
				printS += RunString(CFile, W->Frml, CRecPtr);
			}
			//W = W->pChain;
			continue;
			break;
		}
		case 'B': {
			if (RunBool(CFile, W->Frml, CRecPtr)) x = AbbrYes;
			else x = AbbrNo;
			break;
		}
		case 'F': {
			const double r = RunReal(CFile, W->Frml, CRecPtr);
			if (W->M == 255) str(r, W->N, x);
			else str(r, W->N, W->M, x);
			break;
		}
		case 'D':
			x = StrDate(RunReal(CFile, W->Frml, CRecPtr), W->Mask);
			break;
		}

		if (LF == WriteType::message || LF == WriteType::msgAndHelp) {
			t += x;
		}
		else {
			printS += x;
		}

		//W = W->pChain;
	} // for

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
				Help(PD->mHlpRdb, RunString(CFile, PD->mHlpFrml, CRecPtr), false);
				continue;
			}
			break;
		}
		case WriteType::message: {
			SetMsgPar(t);
			WrLLF10Msg(110);
			if (Event.Pressed.KeyCombination() == __F1) {
				Help(PD->mHlpRdb, RunString(CFile, PD->mHlpFrml, CRecPtr), false);
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

	if (IRec == 0) {
		WORD i = 0;
		str = GetHlpText(CRdb, RunString(CFile, (FrmlElem*)R, CRecPtr), true, i);
		if (str.empty()) return;
	}
	else {
		CFile = R->v_files[0];
		CRecPtr = Chpt->FF->RecPtr;
		CFile->ReadRec(IRec, CRecPtr);
		int pos = CFile->loadT(ChptTxt, CRecPtr);
		str = CFile->FF->TF->Read(pos);
		if (R->Encrypted) {
			str = Coding::CodingString(CFile, str);
		}
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
		std::string s = RunString(CFile, PD->FillC, CRecPtr);
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
	std::string s = RunString(CFile, PD->Param, CRecPtr);
	WORD i = PD->ProgCatIRec;
	CVol = "";
	std::string prog;
	if (i != 0) {
		prog = catalog->GetPathName(i);
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
		GoExit(MsgLine);
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
		GoExit(MsgLine);
	}
}

void MountProc(WORD CatIRec, bool NoCancel)
{
	try {
		SaveFiles();
		CVol = catalog->GetVolume(CatIRec);
		CPath = FExpand(catalog->GetPathName(CatIRec));
		FSplit(CPath, CDir, CName, CExt);
		TestMountVol(CPath[1]);
		LastExitCode = 0;
	}
	catch (std::exception& e) {
		if (NoCancel) LastExitCode = 1;
		else GoExit(MsgLine);
	}
}

void EditProc(Instr_edit* PD)
{
	EdUpdated = false;
	SaveFiles();

	CFile = PD->EditFD; // TODO: to be certain

	// TODO: is needed to make copy of EditOptions before call edit?
	std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>(PD->EditFD);
	const bool selFlds = data_editor->SelFldsForEO(&PD->options, nullptr);
	if (!PD->options.UserSelFlds || selFlds) {
		data_editor->EditDataFile(PD->EditFD, &PD->options);
	}
	SaveFiles();

	// TODO: and here delete copy?
}

std::string GetStr(FrmlElem* Z)
{
	std::string result;
	if (Z == nullptr) {

	}
	else {
		//result = RunShortStr(CFile, Z, CRecPtr);
		result = RunString(nullptr, Z, nullptr);
	}
	return result;
}

void EditTxtProc(Instr_edittxt* PD)
{
	int i = 0;
	WRect v;
	WRect* pv = nullptr;
	BYTE a = 0;
	std::string* lp = nullptr;
	MsgStr MsgS;
	void* p = nullptr;
	MarkStore(p);
	i = 1;
	if (PD->TxtPos != nullptr) {
		i = RunInt(nullptr, PD->TxtPos, nullptr); // RunInt(CFile, PD->TxtPos, CRecPtr)
	}
	EdUpdated = false;
	a = RunWordImpl(nullptr, PD->Atr, 0, nullptr);  //RunWordImpl(CFile, PD->Atr, 0, CRecPtr)
	pv = nullptr;
	if (PD->Ww.C1 != nullptr) {
		//RunWFrml(CFile, PD->Ww, PD->WFlags, v, CRecPtr);
		RunWFrml(nullptr, PD->Ww, PD->WFlags, v, nullptr);
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
	if (PD->ErrMsg != nullptr) {
		msg = RunString(nullptr, PD->ErrMsg, nullptr);  //RunString(CFile, PD->ErrMsg, CRecPtr)
	}

	std::unique_ptr<TextEditor> editor = std::make_unique<TextEditor>(PD->EdTxtMode, TextType::Unknown);
	editor->EditTxtFile(
		lp, PD->EdTxtMode, msg, PD->ExD, i,
		RunInt(nullptr, PD->TxtXY, nullptr), /*RunInt(CFile, PD->TxtXY, CRecPtr),*/
		pv, a,
		RunString(nullptr, PD->Hd, nullptr), /*RunShortStr(CFile, PD->Hd, CRecPtr),*/
		PD->WFlags, &MsgS);

	ReleaseStore(&p);
}

void PrintTxtProc(Instr_edittxt* PD)
{
	LongStr* s = nullptr;
	/* !!! with PD^ do!!! */
	if (PD->TxtLV != nullptr) {
		//s = TWork.ReadLongStr(1, *(int*)(uintptr_t(MyBP) + PD->TxtLV->BPOfs));
		PrintArray(s->A, s->LL, false);
		delete s; s = nullptr;
	}
	else {
		SetTxtPathVol(PD->TxtPath, PD->TxtCatIRec);
		PrintTxtFile(0);
	}
}

void DeleteRecProc(Instr_recs* PD)
{
	int n;
	XString x;
	CFile = PD->RecFD;
	CRecPtr = PD->RecFD->GetRecSpace();
	if (PD->ByKey) {
		x.S = RunString(CFile, PD->RecNr, CRecPtr);
#ifdef FandSQL
		if (CFile->IsSQLFile) { Strm1->DeleteXRec(PD->Key, &x, PD->AdUpd); delete[] CRecPtr; return; }
#endif
	}
	LockMode md = CFile->NewLockMode(DelMode);
	if (PD->ByKey) {
		if (!CFile->SearchXKey(PD->Key, x, n)) {
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
	if (CFile->FF->file_type == FandFileType::INDEX) {
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

void UpdRec(FileD* file_d, int rec_nr, bool ad_upd, void* new_data)
{
	uint8_t* old_data = file_d->GetRecSpace();
	file_d->ReadRec(rec_nr, old_data);

	const bool deleted = file_d->DeletedFlag(old_data);

	if (ad_upd) {
		if (deleted) {
			LastExitCode = !RunAddUpdate(file_d, '+', nullptr, nullptr, new_data);
		}
		else {
			LastExitCode = !RunAddUpdate(file_d, 'd', old_data, nullptr, new_data);
		}
	}

	if (file_d->FF->file_type == FandFileType::INDEX) {
		file_d->FF->OverWrXRec(rec_nr, old_data, new_data, new_data);
	}
	else {
		file_d->WriteRec(rec_nr, new_data);
	}

	if (!deleted) {
		file_d->DelAllDifTFlds(old_data, nullptr);
	}

	delete[] old_data; old_data = nullptr;
}

void ReadWriteRecProc(bool IsRead, Instr_recs* PD)
{
	// PD->LV is a local variable (record of)
	LocVar* lv = PD->LV;

	XString x;

	int N = 1;
	XKey* k = PD->Key;
	bool ad = PD->AdUpd;
	LockMode md = lv->FD->GetLMode();
	bool app = false;
	BYTE* record1 = lv->FD->GetRecSpace();
	if (PD->ByKey) {
		x.S = RunString(lv->FD, PD->RecNr, lv->record);
#ifdef FandSQL
		if (CFile->IsSQLFile) {
			if (IsRead) if (Strm1->SelectXRec(k, @x, PD->CompOp, true)) goto label4; else goto label2;
			else if (Strm1->UpdateXRec(k, @x, ad)) goto label4; else goto label2;
		}
#endif
	}
	else {
		N = RunInt(lv->FD, PD->RecNr, lv->record);
	}

	if (IsRead) {
		if (N == 0) {
			goto label0;
		}
		else {
			lv->FD->NewLockMode(RdMode);
		}
	}
	else if (N == 0) {
#ifdef FandSQL
		if (CFile->IsSQLFile) { Strm1->InsertRec(ad, true); ReleaseStore(cr); CFile->OldLockMode(md); return; }
#endif
		goto label1;
	}
	else {
		lv->FD->NewLockMode(WrMode);
	}

	if (PD->ByKey) {
		if (k == nullptr/*IsParFile*/) {
			if (lv->FD->GetNRecs() == 0) {
				if (IsRead) {
				label0:
					//CFile->DelTFlds(CRecPtr);
					lv->FD->ZeroAllFlds(lv->record, true);
					delete[] record1; record1 = nullptr;
					lv->FD->OldLockMode(md);
					return;
				}
				else {
				label1:
					lv->FD->NewLockMode(CrMode);
					if (lv->FD->FileType == DataFileType::FandFile) {
						lv->FD->FF->TestXFExist();
					}
					lv->FD->IncNRecs(1);
					app = true;
				}
			}
			N = lv->FD->GetNRecs();
		}
		else if (!lv->FD->SearchXKey(k, x, N)) {
			if (IsRead) {
				//CFile->DelTFlds(CRecPtr);
				lv->FD->ZeroAllFlds(lv->record, true);
				lv->FD->SetDeletedFlag(lv->record);
				delete[] record1; record1 = nullptr;
				lv->FD->OldLockMode(md);
				return;
			}
			WORD msg = 613;
			SetMsgPar(lv->name);
			lv->FD->RunErrorM(md);
			RunError(msg);
		}
	}
	else {
		if ((N <= 0) || (N > lv->FD->GetNRecs())) {
			WORD msg = 641;
			SetMsgPar(lv->name);
			lv->FD->RunErrorM(md);
			RunError(msg);
		}
	}

	if (IsRead) {
		lv->FD->ReadRec(N, record1);
		lv->FD->CopyRec(record1, lv->record, true);
	}
	else {
		lv->FD->CopyRec(lv->record, record1, false);
		if (app) {
			if (lv->FD->FileType == DataFileType::FandFile && lv->FD->FF->file_type == FandFileType::INDEX) {
				lv->FD->RecallRec(N, record1);
			}
			else {
				lv->FD->WriteRec(N, record1);
			}

			if (ad) {
				LastExitCode = !RunAddUpdate(lv->FD, '+', nullptr, nullptr, record1);
			}
		}
		else {
			UpdRec(lv->FD, N, ad, record1);
		}
	}

	delete[] record1; record1 = nullptr;
	lv->FD->OldLockMode(md);
}

void LinkRecProc(Instr_assign* assign_instr)
{
	int n = 0;
	BYTE* rec = nullptr;
	LinkD* ld = assign_instr->LinkLD;
	uint8_t* lr2 = assign_instr->RecLV2->record;

	ld->ToFD->ClearRecSpace(lr2);

	if (LinkUpw(ld, n, true, assign_instr->RecLV1->record, &rec)) {
		LastExitCode = 0;
	}
	else {
		LastExitCode = 1;
	}

	ld->ToFD->CopyRec(rec, lr2, true);

	delete[] rec; rec = nullptr;
}

void ForAllProc(Instr_forall* PD)
{
	FileD* FD = nullptr; XKey* Key = nullptr; XKey* k = nullptr; FrmlElem* Bool = nullptr;
	LinkD* LD = nullptr;
	//KeyInD* KI = nullptr;
	void* cr = nullptr; void* p = nullptr; void* lr = nullptr;
	XScan* xScan = nullptr; LockMode md, md1; XString xx;
	//KeyFldD* KF = nullptr;
	LocVar* LVi = nullptr; LocVar* LVr = nullptr;
	bool lk = false, b = false;
#ifdef FandSQL
	bool sql;
#endif
	MarkStore(p);
	FD = PD->CFD; Key = PD->CKey;
	LVi = PD->CVar; LVr = PD->CRecVar;
	LD = PD->CLD;
	//KI = PD->CKIRoot;
	Bool = RunEvalFrml(CFile, PD->CBool, CRecPtr);
	lk = false;
#ifdef FandSQL
	if (PD->inSQL && !v_files->IsSQLFile) return;
#endif
	if (LD != nullptr) {
		CFile = LD->ToFD;
		//KF = LD->ToKey->KFlds;
		switch (PD->COwnerTyp) {
		case 'r': {
			CRecPtr = PD->CLV->record;
			xx.PackKF(CFile, LD->ToKey->KFlds, CRecPtr);
			break;
		}
		case 'F': {
			md = CFile->NewLockMode(RdMode);
			CRecPtr = CFile->GetRecSpace();
			CFile->ReadRec(RunInt(CFile, (FrmlElem*)PD->CLV, CRecPtr), CRecPtr);
			xx.PackKF(CFile, LD->ToKey->KFlds, CRecPtr);
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
	xScan = new XScan(CFile, Key, PD->CKIRoot, true);
#ifdef FandSQL
	if (PD->inSQL) Scan->ResetSQLTxt(Bool); else
#endif
		if (LD != nullptr) {
			if (PD->COwnerTyp == 'i') {
				int32_t err_no = xScan->ResetOwnerIndex(LD, PD->CLV, Bool);
				if (err_no != 0) {
					RunError(err_no);
				}
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
		lr = LVr->record;
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
				CFile->ClearRecordUpdateFlag(lr);
				//CFile->DelTFlds(lr);
				CFile->CopyRec((uint8_t*)cr, (uint8_t*)lr, true);
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
			CFile->OpenCreateF(CPath, Shared);
			if ((LVr != nullptr) && (LVi == nullptr) && CFile->HasRecordUpdateFlag(CRecPtr)) {
				md1 = CFile->NewLockMode(WrMode);
				CFile->CopyRec((uint8_t*)lr, (uint8_t*)cr, false);
				UpdRec(CFile, xScan->RecNr, true, cr);
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
	UserHeadLine(RunString(CFile, Z, CRecPtr));
}

void SetKeyBufProc(FrmlElem* Z)
{
	//KbdBuffer = RunShortStr(Z);
	std::string keyBuf = RunString(CFile, Z, CRecPtr);
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
	auto top = RunString(CFile, PD->Top, CRecPtr); // nacte nadpis
	w1 = PushWFramed(v.C1, v.R1, v.C2, v.R2, ProcAttr, top, "", PD->WithWFlags); // vykresli oramovane okno s nadpisem
	if ((PD->WithWFlags & WNoClrScr) == 0) {
		ClrScr(TextAttr);
	}
	SetWwViewPort();
	RunInstr(PD->v_ww_instr);
	PopW(w1, (PD->WithWFlags & WNoPop) == 0);
	SetWwViewPort();
	ProcAttr = PAttr;
}

void WithLockedProc(Instr_withshared* PD)
{
	WORD msg;
	pstring ntxt(10);
	LockMode md;
	PInstrCode op = PD->Kind;
	if (op == PInstrCode::_withlocked) {
		for (LockD* ld : PD->WLD) {
			ld->N = RunInt(CFile, ld->Frml, CRecPtr);
		}
	}
	int w = 0;

	std::vector<LockD*>::iterator it = PD->WLD.begin();
	while (it != PD->WLD.end()) {
		CFile = (*it)->FD;
		if (CFile->FF->Handle == nullptr) {
			if (CFile->OpenF1(CPath, Shared)) {
				if (CFile->TryLockMode(RdMode, md, 2)) {
					CFile->OpenF2(CPath);
					CFile->OldLockMode(NullMode);
				}
				else {
					CFile->FF->Close(); //CloseClearH(CFile->FF);
					goto label2;
				}
			}
			else {
				CFile->OpenCreateF(CPath, Shared);
			}
		}
		if (CFile->FF->IsShared()) {
			if (op == PInstrCode::_withlocked) {
				if (CFile->Lock((*it)->N, 2)) {
					++it;
					continue;
				}
			}
			else {
				if (CFile->TryLockMode((*it)->Md, (*it)->OldMd, 2)) {
					++it;
					continue;
				}
			}
		label2:
			UnLck(PD, *it, op);
			if (PD->WasElse) {
				RunInstr(PD->WElseInstr);
				return;
			}
			CFile = (*it)->FD;
			CFile->SetPathAndVolume();
			if (op == PInstrCode::_withlocked) {
				msg = 839;
				str((*it)->N, ntxt);
				SetMsgPar(ntxt, CPath);
			}
			else {
				msg = 825;
				SetMsgPar(CPath, LockModeTxt[(*it)->Md]);
			}
			int w1 = PushWrLLMsg(msg, false);
			if (w == 0) {
				w = w1;
			}
			else {
				TWork.Delete(w1);
			}
			Beep();
			KbdTimer(spec.NetDelay, 0);
			it = PD->WLD.begin();
			continue;
		}
		++it;
	}


	if (w != 0) {
		PopW(w);
	}
	RunInstr(PD->WDoInstr);
	UnLck(PD, nullptr, op);
}

void HelpProc(Instr_help* PD)
{
	Help(PD->HelpRdb0, RunString(CFile, PD->Frml0, CRecPtr), true);
}

HANDLE OpenHForPutTxt(Instr_puttxt* PD)
{
	SetTxtPathVol(PD->TxtPath1, PD->TxtCatIRec1);
	TestMountVol(CPath[1]);
	FileOpenMode m = _isOverwriteFile;
	if (PD->App) {
		m = _isOldNewFile;
	}
	HANDLE h = OpenH(CPath, m, Exclusive);
	TestCPathError();
	if (PD->App) {
		SeekH(h, FileSizeH(h));
	}
	return h;
}

void PutTxt(Instr_puttxt* PD)
{
	HANDLE h = nullptr;
	std::string path;
	FrmlElem* z = PD->Txt;

	FileD* TFD02;
	FandTFile* TF02;
	int TF02Pos;

	if (CFile->FileType != DataFileType::FandFile) {
		throw std::exception("runproc.cpp PutTxt() not implemented for non-FandFile");
	}

	const bool canCopyT = CanCopyT(CFile, nullptr, z, &TF02, &TFD02, TF02Pos, CRecPtr);

	if (canCopyT) {
		h = OpenHForPutTxt(PD);
		path = CPath;
		Fand0File::CopyTFStringToH(CFile, h, TF02, TFD02, TF02Pos);
		CPath = path;
	}
	else {
		std::string s = RunString(CFile, z, CRecPtr);
		h = OpenHForPutTxt(PD);
		WriteH(h, s.length(), (void*)s.c_str());
	}

	CPath = path;
	TestCPathError();
	WriteH(h, 0, h); /*trunc*/
	CloseH(&h);
}

// ulozi do katalogu hodnotu promenne
void AssgnCatFld(Instr_assign* PD, void* record)
{
	if (PD->FD3 != nullptr) {
		PD->FD3->CloseFile();
	}
	std::string data = RunString(PD->FD3, PD->Frml3, record);
	catalog->SetField(PD->CatIRec, PD->CatFld, data);
}

void AssgnAccRight(Instr_assign* PD)
{
	user->set_acc_rights(RunString(CFile, PD->Frml, CRecPtr));
}

void AssgnUserName(Instr_assign* PD)
{
	user->set_user_name(RunString(CFile, PD->Frml, CRecPtr));
}

void ReleaseDriveProc(FrmlElem* Z)
{
	SaveFiles();
	std::string s = RunString(CFile, Z, CRecPtr);

	if (!s.empty()) {
		char c = toupper(s[0]);

		if (c == spec.CPMdrive) {
			ReleaseDrive(FloppyDrives);
		}
		else if ((c == 'A') || (c == 'B')) {
			ReleaseDrive(c - '@');
		}
	}
	else {
	}
}

void WithGraphicsProc(std::vector<Instr*>& PD)
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
		//CFile = CRdb->v_files->pChain;
		//while (CFile != nullptr) {
		for (size_t i = 1; i < CRdb->v_files.size(); i++) {
			FileD* f = CRdb->v_files[i];
			f->CloseFile();
			f->CatIRec = catalog->GetCatalogIRec(f->Name, f->FF->file_type == FandFileType::RDB);
#ifdef FandSQL
			SetIsSQLFile();
#endif
			//CFile = CFile->pChain;
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
	if (CFile->FF->file_type != FandFileType::INDEX) return;
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
	//LockD* ld = &PD->WLD;
	//while (ld != Ld1) {
	for (LockD* ld : PD->WLD) {
		CFile = ld->FD;
		if (ld->FD->FF->IsShared()) {
			if (Op == PInstrCode::_withlocked) {
				ld->FD->Unlock(ld->N);
			}
			ld->FD->OldLockMode(ld->OldMd);
		}
		//ld = ld->Chain;
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

void RunInstr(const std::vector<Instr*>& instructions)
{
	for (size_t i = 0; i < instructions.size(); i++) {
		Instr* instr = instructions[i];

		if (ExitP || BreakP) break;

		switch (instr->Kind) {
		case PInstrCode::_ifthenelseP: {
			Instr_loops* iPD = (Instr_loops*)instr;
			if (RunBool(CFile, iPD->Bool, CRecPtr)) {
				RunInstr(iPD->v_instr);
			}
			else {
				RunInstr(iPD->v_else_instr);
			}
			break;
		}
		case PInstrCode::_whiledo: {
			Instr_loops* iPD = (Instr_loops*)instr;
			while (!ExitP && !BreakP && RunBool(CFile, iPD->Bool, CRecPtr)) {
				RunInstr(iPD->v_instr);
			}
			BreakP = false;
			break;
		}
		case PInstrCode::_repeatuntil: {
			Instr_loops* iPD = (Instr_loops*)instr;
			do {
				RunInstr(iPD->v_instr);
			} while (!(ExitP || BreakP || RunBool(CFile, iPD->Bool, CRecPtr)));
			BreakP = false;
			break;
		}
		case PInstrCode::_menubox: {
			std::unique_ptr<TMenuBoxP> menu = std::make_unique<TMenuBoxP>(0, 0, nullptr, (Instr_menu*)instr);
			menu->call();
			break;
		}
		case PInstrCode::_menubar: {
			MenuBarProc((Instr_menu*)instr);
			break;
		}
		case PInstrCode::_forall: {
			ForAllProc((Instr_forall*)instr);
			break;
		}
		case PInstrCode::_window: {
			WithWindowProc((Instr_window*)instr);
			break;
		}
		case PInstrCode::_break: {
			BreakP = true;
			break;
		}
		case PInstrCode::_exitP: {
			ExitP = true;
			break;
		}
		case PInstrCode::_cancel: {
			// ukonceni ulohy - navrat do OS nebo do hlavniho menu
			throw std::exception("CANCEL");
			break;
		}
		case PInstrCode::_save: {
			SaveFiles();
			break;
		}
		case PInstrCode::_clrscr: {
			TextAttr = ProcAttr;
			ClrScr(TextAttr);
			break;
		}
		case PInstrCode::_clrww: {
			ClrWwProc((Instr_clrww*)instr);
			break;
		}
		case PInstrCode::_clreol: {
			TextAttr = ProcAttr;
			ClrEol(TextAttr);
			break;
		}
		case PInstrCode::_exec: {
			ExecPgm((Instr_exec*)instr);
			break;
		}
		case PInstrCode::_proc: {
			CallProcedure((Instr_proc*)instr);
			break;
		}
		case PInstrCode::_call: {
			CallRdbProc((Instr_call*)instr);
			break;
		}
		case PInstrCode::_copyfile: {
			FileCopy(((Instr_copyfile*)instr)->CD);
			break;
		}
		case PInstrCode::_headline: {
			HeadLineProc(((Instr_assign*)instr)->Frml);
			break;
		}
		case PInstrCode::_setkeybuf: {
			SetKeyBufProc(((Instr_assign*)instr)->Frml);
			break;
		}
		case PInstrCode::_writeln: {
			WritelnProc((Instr_writeln*)instr);
			break;
		}
		case PInstrCode::_gotoxy: {
			Instr_gotoxy* iPD = (Instr_gotoxy*)instr;
			WORD x = static_cast<uint16_t>(RunInt(CFile, iPD->GoX, CRecPtr));
			WORD y = static_cast<uint16_t>(RunInt(CFile, iPD->GoY, CRecPtr));
			screen.GotoXY(x + WindMin.X - 1, y + WindMin.Y - 1, absolute);
			break;
		}
		case PInstrCode::_merge: {
			MergeProc((Instr_merge_display*)instr);
			break;
		}
		case PInstrCode::_lproc: {
			Instr_lproc* iPD = (Instr_lproc*)instr;
			std::unique_ptr<RunProlog> prolog = std::make_unique<RunProlog>(iPD);
			prolog->Run();
			break;
		}
		case PInstrCode::_report: {
			ReportProc(((Instr_report*)instr)->RO, true);
			break;
		}
		case PInstrCode::_sort: {
			Instr_sort* iPD = (Instr_sort*)instr;
			SortProc(iPD->SortFD, iPD->SK);
			break;
		}
		case PInstrCode::_edit: {
			EditProc((Instr_edit*)instr);
			break;
		}
		case PInstrCode::_asgnloc: {
			Instr_assign* iPD = (Instr_assign*)instr;
			LVAssignFrml(CFile, iPD->AssLV, iPD->Add, iPD->Frml, CRecPtr);
			break;
		}
		case PInstrCode::_asgnrecfld: {
			AssignRecFld((Instr_assign*)instr);
			break;
		}
		case PInstrCode::_asgnrecvar: {
			Instr_assign* iPD = (Instr_assign*)instr;
			AssignRecVar(iPD->RecLV1, iPD->RecLV2, iPD->Ass);
			break;
		}
		case PInstrCode::_asgnpar: {
			// ulozi globalni parametr - do souboru
			Instr_assign* iPD = (Instr_assign*)instr;
			AsgnParFldFrml(iPD->FD, iPD->FldD, iPD->Frml, iPD->Add);
			break;
		}
		case PInstrCode::_asgnField: {
			AssignField((Instr_assign*)instr);
			break;
		}
		case PInstrCode::_asgnnrecs: {
			Instr_assign* iPD = (Instr_assign*)instr;
			CFile = iPD->FD;
			iPD->FD->AssignNRecs(iPD->Add, RunInt(iPD->FD, iPD->Frml, CRecPtr));
			break;
		}
		case PInstrCode::_appendRec: {
			FileD* rec_file = ((Instr_recs*)instr)->RecFD;
			AppendRecProc(rec_file);
			break;
		}
		case PInstrCode::_deleterec: {
			DeleteRecProc((Instr_recs*)instr);
			break;
		}
		case PInstrCode::_recallrec: {
			RecallRecProc((Instr_recs*)instr);
			break;
		}
		case PInstrCode::_readrec: {
			ReadWriteRecProc(true, (Instr_recs*)instr);
			break;
		}
		case PInstrCode::_writerec: {
			ReadWriteRecProc(false, (Instr_recs*)instr);
			break;
		}
		case PInstrCode::_linkrec: {
			LinkRecProc((Instr_assign*)instr);
			break;
		}
		case PInstrCode::_withshared:
		case PInstrCode::_withlocked: {
			WithLockedProc(static_cast<Instr_withshared*>(instr));
			break;
		}
		case PInstrCode::_edittxt: {
			EditTxtProc(static_cast<Instr_edittxt*>(instr));
			break;
		}
		case PInstrCode::_printtxt: {
			PrintTxtProc(static_cast<Instr_edittxt*>(instr));
			break;
		}
		case PInstrCode::_puttxt: {
			PutTxt(static_cast<Instr_puttxt*>(instr));
			break;
		}
		case PInstrCode::_asgnCatField: {
			AssgnCatFld(static_cast<Instr_assign*>(instr), CRecPtr);
			break;
		}
		case PInstrCode::_asgnusercode: {
			//UserCode = RunInt(CFile, ((Instr_assign*)PD)->Frml, CRecPtr);
			//AccRight[0] = 0x01;
			//AccRight[1] = (char)UserCode;
			uint32_t userCode = RunInt(CFile, ((Instr_assign*)instr)->Frml, CRecPtr);
			user->set_user_code(userCode);
			user->set_acc_right(static_cast<uint16_t>(userCode));
			break;
		}
		case PInstrCode::_asgnAccRight: {
			AssgnAccRight((Instr_assign*)instr);
			break;
		}
		case PInstrCode::_asgnusername: {
			AssgnUserName((Instr_assign*)instr);
			break;
		}
		case PInstrCode::_asgnusertoday: {
			userToday = RunReal(CFile, ((Instr_assign*)instr)->Frml, CRecPtr);
			break;
		}
		case PInstrCode::_asgnClipbd: {
			std::string s = RunString(CFile, ((Instr_assign*)instr)->Frml, CRecPtr);
			TWork.Delete(ClpBdPos);
			ClpBdPos = TWork.Store(s);
			break;
		}
		case PInstrCode::_asgnEdOk: {
			EdOk = RunBool(CFile, ((Instr_assign*)instr)->Frml, CRecPtr);
			break;
		}
		case PInstrCode::_turncat: {
			auto iPD = (Instr_turncat*)instr;
			catalog->TurnCat(iPD->NextGenFD, iPD->FrstCatIRec, iPD->NCatIRecs, RunInt(CFile, iPD->TCFrml, CRecPtr));
			break;
		}
		case PInstrCode::_releasedrive: {
			ReleaseDriveProc(((Instr_releasedrive*)instr)->Drive);
			break;
		}
		case PInstrCode::_setprinter: {
			SetCurrPrinter(abs(RunInt(CFile, ((Instr_assign*)instr)->Frml, CRecPtr)));
			break;
		}
		case PInstrCode::_indexfile: {
			Instr_indexfile* iPD = (Instr_indexfile*)instr;
			iPD->IndexFD->IndexesMaintenance(iPD->Compress);
			break;
		}
		case PInstrCode::_display: {
			Instr_merge_display* iPD = (Instr_merge_display*)instr;
			DisplayProc(iPD->Pos.rdb, iPD->Pos.i_rec);
			break;
		}
		case PInstrCode::_mount: {
			Instr_mount* iPD = (Instr_mount*)instr;
			MountProc(iPD->MountCatIRec, iPD->MountNoCancel);
			break;
		}
		case PInstrCode::_clearkeybuf: {
			ClearKbdBuf();
			break;
		}
		case PInstrCode::_help: {
			HelpProc((Instr_help*)instr);
			break;
		}
		case PInstrCode::_wait: {
			WaitProc();
			break;
		}
		case PInstrCode::_beepP: {
			Beep();
			break;
		}
		case PInstrCode::_delay: {
			const int value = RunInt(CFile, ((Instr_assign*)instr)->Frml, CRecPtr);
			Delay((value + 27) / 55);
			break;
		}
		case PInstrCode::_sound: {
			Sound(RunInt(CFile, ((Instr_assign*)instr)->Frml, CRecPtr));
			break;
		}
		case PInstrCode::_nosound: {
			NoSound();
			break;
		}
#ifdef FandGraph
		case PInstrCode::_graph: {
			RunBGraph(((Instr_graph*)instr)->GD, false);
			break;
		}
		case PInstrCode::_putpixel:
		case PInstrCode::_line:
		case PInstrCode::_rectangle:
		case PInstrCode::_ellipse:
		case PInstrCode::_floodfill:
		case PInstrCode::_outtextxy: {
			DrawProc((Instr_graph*)instr);
			break;
		}
#endif
		case PInstrCode::_withgraphics: {
			WithGraphicsProc(((Instr_withshared*)instr)->WDoInstr);
			break;
		}
#ifndef FandRunV
		case PInstrCode::_memdiag: {
			MemDiagProc();
			break;
		}
#endif 
		case PInstrCode::_closefds: {
			// zavre soubor
			CFile = ((Instr_closefds*)instr)->clFD;
			if (CFile == nullptr) {
				ForAllFDs(ForAllFilesOperation::close_passive_fd);
			}
			else if (!CFile->FF->IsShared() || (CFile->FF->LMode == NullMode)) {
				CFile->CloseFile();
			}
			break;
		}
		case PInstrCode::_backup: {
			auto iPD = (Instr_backup*)instr;
			BackUp(iPD->IsBackup, !iPD->NoCompress, iPD->BrCatIRec, iPD->BrNoCancel);
			break;
		}
		case PInstrCode::_backupm: {
			BackupM((Instr_backup*)instr);
			break;
		}
		case PInstrCode::_resetcat: {
			ResetCatalog();
			break;
		}
		case PInstrCode::_setedittxt: {
			// TODO: this cannot work! doen't make sense
			std::unique_ptr<TextEditor> editor = std::make_unique<TextEditor>(EditorMode::Unknown, TextType::Unknown);
			editor->SetEditTxt((Instr_setedittxt*)instr);
			break;
		}
		case PInstrCode::_getindex: {
			int32_t err_no = GetIndex((Instr_getindex*)instr);
			if (err_no != 0) {
				RunError(err_no);
			}
			break;
		}
		case PInstrCode::_setmouse: {
			auto iPD = (Instr_setmouse*)instr;
			SetMouse(RunInt(CFile, iPD->MouseX, CRecPtr),
				RunInt(CFile, iPD->MouseY, CRecPtr),
				RunBool(CFile, iPD->Show, CRecPtr));
			break;
		}
		case PInstrCode::_checkfile: {
			auto iPD = (Instr_checkfile*)instr;
			SetTxtPathVol(iPD->cfPath, iPD->cfCatIRec);
			CheckFile(iPD->cfFD);
			break;
		}
#ifdef FandSQL
		case _sql: SQLProc(PD->Frml); break;
		case _login: /* !!! with PD^ do!!! */ StartLogIn(liName, liPassWord); break;
		case _sqlrdwrtxt: SQLRdWrTxt(PD); break;
#endif
		case PInstrCode::_asgnrand: {
			srand(RunInt(CFile, ((Instr_assign*)instr)->Frml, CRecPtr));
			break;
		}
		case PInstrCode::_randomize: {
			Random();
			break;
		}
		case PInstrCode::_asgnxnrecs: {
			((Instr_assign*)instr)->xnrIdx->Release(CFile);
			break;
		}
		case PInstrCode::_portout: {
			Instr_portout* iPD = (Instr_portout*)instr;
			PortOut(RunBool(CFile, iPD->IsWord, CRecPtr),
				(WORD)(RunInt(CFile, iPD->Port, CRecPtr)),
				(WORD)(RunInt(CFile, iPD->PortWhat, CRecPtr)));
			break;
		}
		default:
			break;
		}
	}
}

void RunProcedure(std::vector<Instr*>& PDRoot)
{
	bool ExP = ExitP;
	bool BrkP = BreakP;
	ExitP = false;
	BreakP = false;

	// ****** RUN INSTRUCTIONS ****** //
	RunInstr(PDRoot);
	// ****************************** //

	ExitP = ExP;
	BreakP = BrkP;
}

void CallProcedure(Instr_proc* PD)
{
	void* p1 = nullptr;
	void* p2 = nullptr;

	std::_Vector_iterator<std::_Vector_val<std::_Simple_types<LocVar*>>> it0;
	std::_Vector_iterator<std::_Vector_val<std::_Simple_types<LocVar*>>> it1;

	WORD i = 0, j = 0;
	int l = 0;
	KeyFldD* kf1 = nullptr;
	KeyFldD* kf2 = nullptr;

	if (PD == nullptr) return;
	MarkBoth(p1, p2);

	std::deque<LinkD*> ld = LinkDRoot;
	size_t lstFDindex = CRdb->v_files.size() - 1; // index of last item in FileDRoot;
	gc->SetInpTT(&PD->PPos, true);

#ifdef _DEBUG
	std::string srcCode = gc->input_string;
	if (srcCode.find("tuto sestavu vypnete") != std::string::npos) {
		printf("");
	}
#endif

	Compiler::ProcStack.push_front(LVBD);

	ReadProcHead(gc, "");
	PD->loc_var_block = LVBD;
	size_t params_count = PD->loc_var_block.NParam;

	if ((params_count != PD->N) && !((params_count == PD->N - 1) && PD->ExPar)) {
		gc->input_pos = 0;
		gc->Error(119);
	}

	it0 = PD->loc_var_block.variables.begin();

	// projdeme vstupni parametry funkce
	for (i = 0; i < params_count; i++) {
		if (PD->TArg[i].FTyp != (*it0)->f_typ) {
			gc->input_pos = 0;
			gc->Error(119);
		}
		switch (PD->TArg[i].FTyp) {
		case 'r':
		case 'i': {
			if ((*it0)->FD != PD->TArg[i].FD) {
				gc->input_pos = 0;
				gc->Error(119);
			}
			(*it0)->record = static_cast<uint8_t*>(PD->TArg[i].RecPtr);
			break;
		}
		case 'f': {
			if (PD->TArg[i].RecPtr != nullptr) {
				const auto state = gc->SaveCompState();
				std::string code = RunString(CFile, PD->TArg[i].TxtFrml, CRecPtr);
				gc->SetInpStdStr(code, true);
				CFile = RdFileD(PD->TArg[i].Name, DataFileType::FandFile, FandFileType::FAND16, "$");
				CRdb->v_files.push_back(CFile);
				gc->RestoreCompState(state);
			}
			else {
				CFile = PD->TArg[i].FD;
			}
			it1 = it0;
			while (it1 != PD->loc_var_block.variables.end()) {
				if (((*it1)->f_typ == 'i' || (*it1)->f_typ == 'r') && ((*it1)->FD == (*it0)->FD)) {
					(*it1)->FD = CFile;
				}
				++it1;
			}
			(*it0)->FD = CFile;
			FDLocVarAllowed = true;
			break;
		}
		default: {
			FrmlElem* z = PD->TArg[i].Frml;
			LocVar* lv = *it0;
			if (lv->is_return_param && (z->Op != _getlocvar)
				|| PD->TArg[i].FromProlog
				&& (PD->TArg[i].IsRetPar != lv->is_return_param)) {
				gc->input_pos = 0;
				gc->Error(119);
			}

			// input params has to be evaluated with previous LocVarBlkD
			LocVarBlock actual_lvbd = LVBD;
			LVBD = Compiler::ProcStack.front();
			// process input param
			LVAssignFrml(CFile, lv, false, PD->TArg[i].Frml, CRecPtr);
			// return LocVarBlkD back
			LVBD = actual_lvbd;
			break;
		}
		}
		++it0;
	}
	it1 = it0;
	while (it0 != PD->loc_var_block.variables.end()) {
		if ((*it0)->f_typ == 'r') {
			FileD* fd = (*it0)->FD;
			uint8_t* record = fd->GetRecSpace();
			fd->SetTWorkFlag(record);
			fd->ZeroAllFlds(record, false);
			fd->ClearDeletedFlag(record);
			(*it0)->record = record;
		}
		else if ((*it0)->f_typ == 'f') {
			// dynamic file definition
			//printf("");
		}
		++it0;
	}

	// ****** READ PROCEDURE BODY ****** //
	std::vector<Instr*> instructions = ReadProcBody(gc);
	// ********************************* //

	FDLocVarAllowed = false;
	it0 = it1;
	while (it0 != PD->loc_var_block.variables.end()) {
		if ((*it0)->f_typ == 'i') {
			XWKey* hX = (XWKey*)(*it0)->record;
			if (hX->KFlds.empty()) {
				hX->KFlds = (*it0)->FD->Keys[0]->KFlds;
			}
			XWKey* tmp = (XWKey*)(*it0)->record;
			tmp->Open(CFile, hX->KFlds, true, false);
		}
		++it0;
	}
	ReleaseStore(&p2);

	// ****** RUN PROCEDURE ****** //
	RunProcedure(instructions);
	// *************************** //

	// delete instructions
	for (Instr* instr : instructions) {
		delete instr;
	}
	instructions.clear();

	it0 = PD->loc_var_block.variables.begin();
	i = 0;
	while (it0 != PD->loc_var_block.variables.end()) {
		// projdeme navratove hodnoty (navratova hodnota funkce + VAR parametry)
		// a tyto navratove hodnoty ulozime zpet do patricneho FrmlElem
		if ((*it0)->is_return_param) {
			auto z18 = (FrmlElemLocVar*)PD->TArg[i].Frml;
			switch ((*it0)->f_typ) {
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
		if (i > params_count) {
			switch ((*it0)->f_typ) {
			case 'r': {
				LocVar* loc_var = *it0;
				loc_var->FD->ClearRecSpace(loc_var->record);
				delete[] loc_var->record;
				loc_var->record = nullptr;
				break;
			}
			case 'i': {
				CFile = (*it0)->FD;
				((XWKey*)(*it0)->record)->Close(CFile);
				break;
			}
			}
		}
		i++;
		++it0;
	}
	//PopProcStk();
	//ProcMyBP = (ProcStkD*)oldprocbp;

	LVBD = Compiler::ProcStack.front();
	Compiler::ProcStack.pop_front();

	LinkDRoot = ld;

	//CFile = lstFD->pChain;
	//while (CFile != nullptr) {
	//	CFile->CloseFile();
	//	CFile = CFile->pChain;
	//}
	//lstFD->pChain = nullptr;

	FileD::CloseAndRemoveAllAfter(lstFDindex + 1, CRdb->v_files);

	ReleaseStore(&p1);
	ReleaseStore(&p2);
}

void RunMainProc(RdbPos RP, bool NewWw)
{
	if (NewWw) {
		ProcAttr = screen.colors.uNorm;
		screen.Window(1, 2, TxtCols, TxtRows);
		TextAttr = ProcAttr;
		ClrScr(TextAttr);
		UserHeadLine("");
		MenuX = 1; MenuY = 2;
	}
	auto PD = new Instr_proc(0);
	PD->PPos = RP;
	CallProcedure(PD);
	delete PD; PD = nullptr;
	if (NewWw) screen.Window(1, 1, TxtCols, TxtRows);
}
