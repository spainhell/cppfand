#include "runproc.h"

#include <fstream>

#include "../Common/Coding.h"
#include "../Common/random.h"
#include "Compiler.h"
#include "../Common/DateTime.h"
#include "OldDrivers.h"
#include "../fandio/FieldDescr.h"
#include "../Common/FileD.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "../Common/LinkD.h"
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
#include "../Common/Record.h"
#include "../Common/CommonVariables.h"


void RunProcedure::UserHeadLine(std::string UserHeader)
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

void RunProcedure::ReportProc(RprtOpt* RO, bool save)
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
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
			RO->Width = RunInt(nullptr, RO->WidthFrml, nullptr);
		}
		if (RO->Head != nullptr) {
			RO->HeadTxt = RunString(nullptr, RO->Head, nullptr);
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
	//_CrtSetDbgFlag(0);
}

void RunProcedure::PromptAutoRprt(RprtOpt* RO)
{
	wwmix ww;

	RprtOpt* RO2 = new RprtOpt();
	RO->CopyTo(RO2);

	for (FieldDescr* f : RO->Flds) {
		if (f->isStored()) {
			ww.PutSelect(f->Name);
		}
		else {
			ww.PutSelect((char)SelMark + f->Name);
		}
	}

	if (!ww.SelFieldList(RO->FDL[0]->FD, 36, true, RO2->Flds)) return;

	if ((RO->FDL[0]->Cond == nullptr) && !ww.PromptFilter("", &RO2->FDL[0]->Cond, &RO2->CondTxt)) return;

	const std::unique_ptr auto_report = std::make_unique<ReportGenerator>();
	if (auto_report->SelForAutoRprt(RO2)) {
		auto_report->RunAutoReport(RO2);
	}
}

void RunProcedure::AssignField(Instr_assign* PD)
{
	WORD msg = 0;
	LockMode md = PD->FD->NewLockMode(WrMode);
	FieldDescr* F = PD->FldD;
	int N = RunInt(PD->FD, PD->RecFrml, nullptr);
	if ((N <= 0) || (N > PD->FD->GetNRecs())) {
		msg = 640;
		SetMsgPar(PD->FD->Name, F->Name);
		PD->FD->RunErrorM(md);
		RunError(msg);
	}
	else {
		Record* rec = new Record(PD->FD);
		PD->FD->ReadRec(N, rec);
		if (PD->Indexarg && !rec->IsDeleted()) {
			msg = 627;
			SetMsgPar(PD->FD->Name, F->Name);
			PD->FD->RunErrorM(md);
			RunError(msg);
		}

		AssgnFrml(rec, F, PD->Frml, PD->Add);
		PD->FD->UpdateRec(N, rec);
		PD->FD->OldLockMode(md);

		delete rec; rec = nullptr;
	}
}

void RunProcedure::AssignRecVar(LocVar* LV1, LocVar* LV2, std::vector<AssignD*>& A)
{
	// FileD* FD1 = LV1->FD;  // destination record
	// FileD* FD2 = LV2->FD;  // source record
	Record* RP1 = LV1->record;
	Record* RP2 = LV2->record;


	for (AssignD* a : A) {
		switch (a->Kind) {
		case MInstrCode::_zero: {
			FieldDescr* F = a->outputField;
			switch (F->frml_type) {
			case 'S': { RP1->SaveS(F, ""); break; }
			case 'R': { RP1->SaveR(F, 0.0); break; }
			case 'B': { RP1->SaveB(F, false); break; }
			}
			break;
		}
		case MInstrCode::_output: {
			auto newFileFrml = (FrmlElemNewFile*)a->Frml;
			newFileFrml->NewRP = RP2;
			AssgnFrml(RP1, a->OFldD, a->Frml, /*false,*/ false);
			if (newFileFrml->NewFile->Name != newFileFrml->NewRP->GetFileD()->Name) {
				printf("AssignRecVar fail");
			}
			break;
		}
		}
	}

	RP1->SetUpdated(); //FD1->SetRecordUpdateFlag(RP1->GetRecord());
}

void RunProcedure::AssignRecFld(Instr_assign* PD)
{
	FieldDescr* field_d = PD->RecFldD;
	FileD* file_d = PD->AssLV->FD;
	Record* record = PD->AssLV->record;

	record->SetUpdated(); // file_d->SetRecordUpdateFlag(rec);
	AssgnFrml(record, field_d, PD->Frml, /*file_d->HasTWorkFlag(rec),*/ PD->Add);
}

void RunProcedure::SortProc(FileD* FD, std::vector<KeyFldD*>& SK)
{
	LockMode md = FD->NewLockMode(ExclMode);
	FD->SortByKey(SK);
	FD->OldLockMode(md);
	SaveFiles();
}

void RunProcedure::MergeProc(Instr_merge_display* PD)
{
	const std::unique_ptr merge = std::make_unique<Merge>();
	merge->SetInput(&PD->Pos, true);
	merge->Read();
	merge->Run();

	SaveFiles();
}

void RunProcedure::WritelnProc(Instr_writeln* PD)
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
				t += RunString(nullptr, W->Frml, nullptr);
			}
			else {
				printS += RunString(nullptr, W->Frml, nullptr);
			}
			//W = W->pChain;
			continue;
			break;
		}
		case 'B': {
			if (RunBool(nullptr, W->Frml, nullptr)) x = AbbrYes;
			else x = AbbrNo;
			break;
		}
		case 'F': {
			const double r = RunReal(nullptr, W->Frml, nullptr);
			if (W->M == 255) str(r, W->N, x);
			else str(r, W->N, W->M, x);
			break;
		}
		case 'D':
			x = StrDate(RunReal(nullptr, W->Frml, nullptr), W->Mask);
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
				Help(PD->mHlpRdb, RunString(nullptr, PD->mHlpFrml, nullptr), false);
				continue;
			}
			break;
		}
		case WriteType::message: {
			SetMsgPar(t);
			WrLLF10Msg(110);
			if (Event.Pressed.KeyCombination() == __F1) {
				Help(PD->mHlpRdb, RunString(nullptr, PD->mHlpFrml, nullptr), false);
				continue;
			}
			break;
		}
		}
		break;
	}
}

void RunProcedure::DisplayProc(Project* R, WORD IRec)
{
	std::string str;

	if (IRec == 0) {
		WORD i = 0;
		str = GetHlpText(CRdb, RunString(nullptr, (FrmlElem*)R, nullptr), true, i);
		if (str.empty()) return;
	}
	else {
		FileD* f = R->project_file;
		std::unique_ptr<Record> rec = std::make_unique<Record>(Chpt);
		f->ReadRec(IRec, rec.get());
		//int pos = f->loadT(ChptTxt, rec);
		//str = f->FF->TF->Read(pos);
		str = rec->LoadS(ChptTxt);
		if (R->Encrypted) {
			str = Coding::CodingString(Chpt->FF->TF->LicenseNr, str);
		}
	}
	screen.WriteStyledStringToWindow(str, ProcAttr);
}

void RunProcedure::ClrWwProc(Instr_clrww* PD)
{
	WRect v;
	RunWFrml(nullptr, PD->W2, 0, v, nullptr);
	WORD a = RunWordImpl(nullptr, PD->Attr2, screen.colors.uNorm, nullptr);
	char c = ' ';
	if (PD->FillC != nullptr) {
		std::string s = RunString(nullptr, PD->FillC, nullptr);
		if (s.length() > 0) {
			c = s[0];
		}
	}
	screen.ScrClr(v.C1, v.R1, v.C2 - v.C1 + 1, v.R2 - v.R1 + 1, c, a);
}

void RunProcedure::ExecPgm(Instr_exec* PD)
{
	const Wind wmin = WindMin;
	const Wind wmax = WindMax;
	const TCrs crs = screen.CrsGet();
	const int w = PushW(1, 1, TxtCols, 1);
	WindMin = wmin;
	WindMax = wmax;
	screen.CrsSet(crs);
	std::string s = RunString(nullptr, PD->Param, nullptr);
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

void RunProcedure::CallRdbProc(Instr_call* PD)
{
	bool b = false;
	uint8_t* p = nullptr;
	wwmix ww;
	MarkStore(p);
	// TODO: tady se ma ulozit stav (MyBP - ProcStkD)
	std::unique_ptr<ProjectRunner> runner = std::make_unique<ProjectRunner>();
	b = runner->EditExecRdb(PD->RdbNm, PD->ProcNm, PD->ProcCall, &ww);
	// TODO: tady se ma obnovit stav (MyBP - ProcStkD)
	ReleaseStore(&p);
	if (!b) {
		GoExit(MsgLine);
	}
}

void RunProcedure::MountProc(WORD CatIRec, bool NoCancel)
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

void RunProcedure::EditProc(Instr_edit* PD)
{
	EdUpdated = false;
	SaveFiles();

	//CFile = PD->EditFD; // TODO: to be certain

	// TODO: is needed to make copy of EditOptions before call edit?
	std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>(PD->EditFD);
	const bool selFlds = data_editor->SelFldsForEO(&PD->options, nullptr);
	if (!PD->options.UserSelFlds || selFlds) {
		data_editor->EditDataFile(PD->EditFD, &PD->options);
	}
	SaveFiles();

	// TODO: and here delete copy?
}

std::string RunProcedure::GetStr(FrmlElem* Z)
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

void RunProcedure::EditTxtProc(Instr_edittxt* PD)
{
	int i = 0;
	WRect v;
	WRect* pv = nullptr;
	uint8_t a = 0;
	std::string* lp = nullptr;
	MsgStr MsgS;
	uint8_t* p = nullptr;
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

void RunProcedure::PrintTxtProc(Instr_edittxt* PD)
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

void RunProcedure::DeleteRecProc(Instr_recs* PD)
{
	int n;
	XString x;
	FileD* f = PD->RecFD;
	Record* rec = new Record(PD->RecFD);

	if (PD->ByKey) {
		x.S = RunString(f, PD->RecNr, rec);
		// TODO: FandSQL condition removed
	}
	LockMode md = f->NewLockMode(DelMode);
	if (PD->ByKey) {
		if (!f->SearchXKey(PD->Key, x, n)) {
			f->OldLockMode(md);
			delete rec;
			return;
		}
	}
	else {
		n = RunInt(f, PD->RecNr, rec);
		if ((n <= 0) || (n > f->GetNRecs())) {
			f->OldLockMode(md);
			delete rec;
			return;
		}
	}
	f->ReadRec(n, rec);
	
	if (PD->AdUpd && !rec->IsDeleted()) {
		LastExitCode = (!RunAddUpdate(f, '-', nullptr, nullptr, rec));
	}
	
	f->DeleteRec(n, rec);

	f->OldLockMode(md);
	
	delete rec; rec = nullptr;
}

void RunProcedure::AppendRecProc(FileD* file_d)
{
	LockMode md = file_d->NewLockMode(CrMode);
	Record* record = new Record(file_d);
	record->Reset();
	record->SetDeleted();
	file_d->CreateRec(file_d->GetNRecs() + 1, record);
	delete record; record = nullptr;
	file_d->OldLockMode(md);
}

void RunProcedure::UpdateRecord(FileD* file_d, int rec_nr, bool ad_upd, Record* new_data)
{
	Record* old_data = new Record(file_d);
	file_d->ReadRec(rec_nr, old_data);

	const bool deleted = old_data->IsDeleted();

	if (ad_upd) {
		if (deleted) {
			LastExitCode = !RunAddUpdate(file_d, '+', nullptr, nullptr, new_data);
		}
		else {
			LastExitCode = !RunAddUpdate(file_d, 'd', old_data, nullptr, new_data);
		}
	}

	if (file_d->FileType == DataFileType::FandFile && file_d->IsIndexFile()) {
		file_d->UpdateRec(rec_nr, old_data, new_data);
	}
	else {
		file_d->UpdateRec(rec_nr, new_data);
	}

	delete old_data; old_data = nullptr;
}

void RunProcedure::ReadWriteRecProc(bool IsRead, Instr_recs* PD)
{
	// PD->LV is a local variable (record of)
	LocVar* lv = PD->LV;

	XString x;

	int N = 1;
	XKey* k = PD->Key;
	bool ad = PD->AdUpd;
	LockMode md = lv->FD->GetLockMode();
	bool app = false;
	Record* record1 = new Record(lv->FD);
	if (PD->ByKey) {
		x.S = RunString(lv->FD, PD->RecNr, lv->record);
		// TODO: FandSQL condition removed
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
		// TODO: isParFile condition removed
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
					//CFile->DelAllTFlds(CRecPtr);
					// TODO: how to delete T records?:
					lv->record->Reset(); //lv->FD->ZeroAllFlds(lv->record, true);
					delete record1; record1 = nullptr;
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
				//CFile->DelAllTFlds(CRecPtr);
				// TODO: how to delete T records?:
				lv->record->Reset(); // lv->FD->ZeroAllFlds(lv->record, true);
				lv->record->SetDeleted();
				delete record1; record1 = nullptr;
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
		record1->CopyTo(lv->record);
	}
	else {
		lv->record->CopyTo(record1);
		if (app) {
			if (lv->FD->FileType == DataFileType::FandFile && lv->FD->IsIndexFile()) {
				lv->FD->RecallRec(N, record1);
			}
			else {
				lv->FD->UpdateRec(N, record1);
			}

			if (ad) {
				LastExitCode = !RunAddUpdate(lv->FD, '+', nullptr, nullptr, record1);
			}
		}
		else {
			UpdateRecord(lv->FD, N, ad, record1);
		}
	}

	delete record1; record1 = nullptr;
	lv->FD->OldLockMode(md);
}

void RunProcedure::LinkRecProc(Instr_assign* assign_instr)
{
	int n = 0;
	LinkD* ld = assign_instr->LinkLD;
	Record* lr2 = assign_instr->RecLV2->record;

	Record* rec = LinkUpw(ld, n, true, assign_instr->RecLV1->record);

	if (rec == nullptr) {
		LastExitCode = 1;
	}
	else {
		LastExitCode = 0;
	}

	rec->CopyTo(lr2);

	delete rec; rec = nullptr;
}

void RunProcedure::ForAllProc(Instr_forall* PD)
{
	FileD* FD = nullptr; XKey* Key = nullptr; XKey* k = nullptr; FrmlElem* Bool = nullptr;
	LinkD* LD = nullptr;
	Record* cr = nullptr; uint8_t* p = nullptr;
	Record* lr = nullptr;
	XScan* xScan = nullptr; LockMode md, md1; XString xx;
	LocVar* LVi = nullptr; LocVar* LVr = nullptr;
	bool lk = false, b = false;

	MarkStore(p);
	FD = PD->CFD; Key = PD->CKey;
	LVi = PD->CVar; LVr = PD->CRecVar;
	LD = PD->CLD;
	//KI = PD->CKIRoot;
	Bool = RunEvalFrml(nullptr, PD->CBool, nullptr); //RunEvalFrml(CFile, PD->CBool, CRecPtr);
	lk = false;
#ifdef FandSQL
	if (PD->inSQL && !v_files->IsSQLFile) return;
#endif
	if (LD != nullptr) {
		//KF = Link->ToKey->KFlds;
		switch (PD->COwnerTyp) {
		case 'r': {
			xx.PackKF(LD->ToKey->KFlds, PD->CLV->record);
			break;
		}
		case 'F': {
			md = LD->ToFile->NewLockMode(RdMode);
			Record* rec = new Record(LD->ToFile); // ->GetRecSpace();
			LD->ToFile->ReadRec(RunInt(LD->ToFile, (FrmlElem*)PD->CLV, rec), rec);
			xx.PackKF(LD->ToKey->KFlds, rec);
			ReleaseStore(&p);
			LD->ToFile->OldLockMode(md);
			delete rec; rec = nullptr;
			break;
		}
		}
	}
	//CFile = FD;
	// TODO: FandSQL condition removed
	md = FD->NewLockMode(RdMode);
	cr = new Record(FD); // ->GetRecSpace();
	//CRecPtr = cr; lr = cr;
	lr = new Record(FD);
	xScan = new XScan(FD, Key, PD->CKIRoot, true);
	// TODO: FandSQL condition removed
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
		xScan->Reset(Bool, PD->CSQLFilter, cr);
	}
	// TODO: FandSQL condition removed
	if (Key != nullptr) {
		if (PD->CWIdx) {
			FD->FF->ScanSubstWIndex(xScan, Key->KFlds, OperationType::Work);
		}
		else {
			FD->FF->XF->UpdLockCnt++;
			lk = true;
		}
	}
	if (LVr != nullptr) {
		lr = LVr->record;
	}
	k = FD->Keys.empty() ? nullptr : FD->Keys[0];
	b = PD->CProcent;

	if (b) {
		RunMsgOn('F', xScan->NRecs);
	}

	while (true) {
		// TODO: FandSQL condition removed
		//CRecPtr = cr;
		xScan->GetRec(cr);

		if (b) {
			RunMsgN(xScan->IRec);
		}

		if (!xScan->eof) {
			// TODO: FandSQL condition removed
			if (LVr != nullptr) {
				//CRecPtr = lr;
				//FD->ClearRecordUpdateFlag(lr->GetRecord());
				lr->ClearUpdated();
				//CFile->DelAllTFlds(lr);
				//FD->CopyRec(cr->GetRecord(), lr->GetRecord(), true);
				cr->CopyTo(lr);

			}
			//if (LVi != nullptr) *(double*)(LocVarAd(LVi)) = Scan->RecNr; // metoda LocVarAd byla odstranena z access.cpp
			if (LVi != nullptr) {
				LVi->R = xScan->RecNr;
			}
			RunInstr(PD->CInstr);
			//CFile = FD;
			//CRecPtr = lr;
			// TODO: FandSQL condition removed

			FD->OpenCreateF(CPath, Shared, false);
			if ((LVr != nullptr) && (LVi == nullptr) && lr->IsUpdated()) {
				md1 = FD->NewLockMode(WrMode);
				//FD->CopyRec(lr->GetRecord(), cr->GetRecord(), false);
				lr->CopyTo(cr);
				UpdateRecord(FD, xScan->RecNr, true, cr);
				FD->OldLockMode(md1);
			}

			if (!(ExitP || BreakP)) {
				// TODO: FandSQL condition removed
				if ((Key == nullptr) && (xScan->NRecs > FD->GetNRecs())) {
					xScan->IRec--;
					xScan->NRecs--;
				}
				continue;
			}
		}
		break;
	} // while true

	if (lk) {
		FD->FF->XF->UpdLockCnt--;
	}

	xScan->Close();
	FD->OldLockMode(md);

	if (b) {
		RunMsgOff();
	}

	ReleaseStore(&p);
	BreakP = false;
}

void RunProcedure::HeadLineProc(FrmlElem* Z)
{
	UserHeadLine(RunString(nullptr, Z, nullptr));
}

void RunProcedure::SetKeyBufProc(FrmlElem* Z)
{
	//KbdBuffer = RunShortStr(Z);
	std::string keyBuf = RunString(nullptr, Z, nullptr);
	keyboard.SetKeyBuf(keyBuf);
}

void RunProcedure::SetWwViewPort()
{
	WORD x1, x2, y1, y2;
#ifdef FandGraph
	if (IsGraphMode) {
		RectToPixel(WindMin.X, WindMin.Y, WindMax.X, WindMax.Y, x1, y1, x2, y2);
		SetViewPort(x1, y1, x2, y2, true);
	}
#endif
}

void RunProcedure::WithWindowProc(Instr_window* PD)
{
	uint8_t PAttr = ProcAttr;
	int w1 = 0;
	WRect v;

	ProcAttr = RunWordImpl(nullptr, PD->Attr, screen.colors.uNorm, nullptr); // nacte barvy do ProcAttr
	RunWFrml(nullptr, PD->W, PD->WithWFlags, v, nullptr); // nacte rozmery okna
	std::string top = RunString(nullptr, PD->Top, nullptr); // nacte nadpis
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

void RunProcedure::WithLockedProc(Instr_withshared* PD)
{
	WORD msg;
	pstring ntxt(10);
	LockMode md;
	PInstrCode op = PD->Kind;
	if (op == PInstrCode::_withlocked) {
		for (LockD* ld : PD->WLD) {
			ld->N = RunInt(nullptr, ld->Frml, nullptr);
		}
	}
	int w = 0;

	std::vector<LockD*>::iterator it = PD->WLD.begin();
	while (it != PD->WLD.end()) {
		FileD* f = (*it)->FD;
		if (f->FF->Handle == nullptr) {
			if (f->OpenF1(CPath, Shared, false)) {
				if (f->TryLockMode(RdMode, md, 2)) {
					f->OpenF2(CPath, false);
					f->OldLockMode(NullMode);
				}
				else {
					f->FF->Close(); //CloseClearH(f->FF);
					goto label2;
				}
			}
			else {
				f->OpenCreateF(CPath, Shared, false);
			}
		}
		if (f->FF->IsShared()) {
			if (op == PInstrCode::_withlocked) {
				if (f->Lock((*it)->N, 2)) {
					++it;
					continue;
				}
			}
			else {
				if (f->TryLockMode((*it)->Md, (*it)->OldMd, 2)) {
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
			f = (*it)->FD;
			f->SetPathAndVolume();
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
				//TWork.Delete(w1);
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

void RunProcedure::HelpProc(Instr_help* PD)
{
	Help(PD->HelpRdb0, RunString(nullptr, PD->Frml0, nullptr), true);
}

HANDLE RunProcedure::OpenHForPutTxt(Instr_puttxt* PD)
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

void RunProcedure::PutTxt(Instr_puttxt* PD)
{
	HANDLE h = nullptr;
	std::string path;
	FrmlElem* z = PD->Txt;

	FileD* TFD02;
	FandTFile* TF02;
	int TF02Pos;

	FileD* file_d = nullptr;

	//if (file_d->FileType != DataFileType::FandFile) {
	//	throw std::exception("runproc.cpp PutTxt() not implemented for non-FandFile");
	//}

	//const bool canCopyT = CanCopyT(file_d, nullptr, z, &TF02, &TFD02, TF02Pos, nullptr);

	//if (canCopyT) {
	//	h = OpenHForPutTxt(PD);
	//	path = CPath;
	//	Fand0File::CopyTFStringToH(nullptr, h, TF02, TFD02, TF02Pos);
	//	CPath = path;
	//}
	//else {
	std::string s = RunString(nullptr, z, nullptr);
	h = OpenHForPutTxt(PD);
	WriteH(h, s.length(), s.c_str());
	//}

	CPath = path;
	TestCPathError();
	WriteH(h, 0, h); /*trunc*/
	CloseH(&h);
}

// ulozi do katalogu hodnotu promenne
void RunProcedure::AssgnCatFld(Instr_assign* PD, Record* record)
{
	if (PD->FD3 != nullptr) {
		PD->FD3->CloseFile();
	}
	std::string data = RunString(PD->FD3, PD->Frml3, record);
	catalog->SetField(PD->CatIRec, PD->CatFld, data);
}

void RunProcedure::AssgnAccRight(Instr_assign* PD)
{
	user->set_acc_rights(RunString(nullptr, PD->Frml, nullptr));
}

void RunProcedure::AssgnUserName(Instr_assign* PD)
{
	user->set_user_name(RunString(nullptr, PD->Frml, nullptr));
}

void RunProcedure::ReleaseDriveProc(FrmlElem* Z)
{
	SaveFiles();
	std::string s = RunString(nullptr, Z, nullptr);

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

void RunProcedure::WithGraphicsProc(std::vector<Instr*>& PD)
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
void RunProcedure::DrawProc(Instr_graph* PD)
{
}
#endif


void RunProcedure::ResetCatalog()
{
	Project* r = CRdb;
	while (CRdb != nullptr) {
		//CFile = CRdb->v_files->pChain;
		//while (CFile != nullptr) {
		for (size_t i = 0; i < CRdb->data_files.size(); i++) {
			FileD* f = CRdb->data_files[i];
			f->CloseFile();
			f->CatIRec = catalog->GetCatalogIRec(f->Name, f->FF->file_type == FandFileType::RDB);
#ifdef FandSQL
			SetIsSQLFile();
#endif
			//CFile = CFile->pChain;
		}
		CRdb = CRdb->ChainBack;
	}
	CRdb = r;
}

void RunProcedure::PortOut(bool IsWord, WORD Port, WORD What)
{
}

void RunProcedure::RecallRecProc(Instr_recs* PD)
{
	FileD* f = PD->RecFD;

	if (!f->IsIndexFile()) return;
	
	Record* rec = new Record(f);
	int N = RunInt(f, PD->RecNr, rec);
	LockMode md = f->NewLockMode(CrMode);
	
	if ((N > 0) && (N <= f->GetNRecs())) {
		f->ReadRec(N, rec);
		if (rec->IsDeleted()) {
			f->FF->RecallRec(N, rec);
			if (PD->AdUpd) {
				LastExitCode = !RunAddUpdate(f, '+', nullptr, nullptr, rec);
			}
		}
	}

	f->OldLockMode(md);
	delete rec; rec = nullptr;
}

void RunProcedure::UnLck(Instr_withshared* PD, LockD* Ld1, PInstrCode Op)
{
	//LockD* ld = &PD->WLD;
	//while (ld != Ld1) {
	for (LockD* ld : PD->WLD) {
		//CFile = ld->FD;
		if (ld->FD->FF->IsShared()) {
			if (Op == PInstrCode::_withlocked) {
				ld->FD->Unlock(ld->N);
			}
			ld->FD->OldLockMode(ld->OldMd);
		}
		//ld = ld->Chain;
	}
}

void RunProcedure::WaitProc() // r. 604
{
	WORD w;
	do {
		GetEvent();
		w = Event.What;
		ClrEvent();
	} while (w != evKeyDown && w != evMouseDown);
}

void RunProcedure::MemDiagProc()
{
}

void RunProcedure::RunInstr(const std::vector<Instr*>& instructions)
{
	for (size_t i = 0; i < instructions.size(); i++) {
		Instr* instr = instructions[i];

		if (ExitP || BreakP) break;

		switch (instr->Kind) {
		case PInstrCode::_ifthenelseP: {
			Instr_loops* iPD = (Instr_loops*)instr;
			if (RunBool(nullptr, iPD->Bool, nullptr)) {
				RunInstr(iPD->v_instr);
			}
			else {
				RunInstr(iPD->v_else_instr);
			}
			break;
		}
		case PInstrCode::_whiledo: {
			Instr_loops* iPD = (Instr_loops*)instr;
			while (!ExitP && !BreakP && RunBool(nullptr, iPD->Bool, nullptr)) {
				RunInstr(iPD->v_instr);
			}
			BreakP = false;
			break;
		}
		case PInstrCode::_repeatuntil: {
			Instr_loops* iPD = (Instr_loops*)instr;
			do {
				RunInstr(iPD->v_instr);
			} while (!(ExitP || BreakP || RunBool(nullptr, iPD->Bool, nullptr)));
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
			WORD x = static_cast<uint16_t>(RunInt(nullptr, iPD->GoX, nullptr));
			WORD y = static_cast<uint16_t>(RunInt(nullptr, iPD->GoY, nullptr));
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
			LVAssignFrml(nullptr, iPD->AssLV, iPD->Add, iPD->Frml, nullptr);
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
			//CFile = iPD->FD;
			iPD->FD->AssignNRecs(iPD->Add, RunInt(iPD->FD, iPD->Frml, nullptr));
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
			AssgnCatFld(static_cast<Instr_assign*>(instr), nullptr);
			break;
		}
		case PInstrCode::_asgnusercode: {
			//UserCode = RunInt(CFile, ((Instr_assign*)PD)->Frml, CRecPtr);
			//AccRight[0] = 0x01;
			//AccRight[1] = (char)UserCode;
			uint32_t userCode = RunInt(nullptr, ((Instr_assign*)instr)->Frml, nullptr);
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
			userToday = RunReal(nullptr, ((Instr_assign*)instr)->Frml, nullptr);
			break;
		}
		case PInstrCode::_asgnClipbd: {
			std::string s = RunString(nullptr, ((Instr_assign*)instr)->Frml, nullptr);
			//TWork.Delete(ClpBdPos);
			//ClpBdPos = TWork.Store(s);
			break;
		}
		case PInstrCode::_asgnEdOk: {
			EdOk = RunBool(nullptr, ((Instr_assign*)instr)->Frml, nullptr);
			break;
		}
		case PInstrCode::_turncat: {
			auto iPD = (Instr_turncat*)instr;
			catalog->TurnCat(iPD->NextGenFD, iPD->FrstCatIRec, iPD->NCatIRecs, RunInt(nullptr, iPD->TCFrml, nullptr));
			break;
		}
		case PInstrCode::_releasedrive: {
			ReleaseDriveProc(((Instr_releasedrive*)instr)->Drive);
			break;
		}
		case PInstrCode::_setprinter: {
			SetCurrPrinter(abs(RunInt(nullptr, ((Instr_assign*)instr)->Frml, nullptr)));
			break;
		}
		case PInstrCode::_indexfile: {
			Instr_indexfile* iPD = (Instr_indexfile*)instr;
			if (iPD->IndexFD->Name == "ROZPIS")
			{
				printf("");
			}
			iPD->IndexFD->IndexesMaintenance(iPD->Compress);
			SaveFiles();
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
			const int value = RunInt(nullptr, ((Instr_assign*)instr)->Frml, nullptr);
			Delay((value + 27) / 55);
			break;
		}
		case PInstrCode::_sound: {
			Sound(RunInt(nullptr, ((Instr_assign*)instr)->Frml, nullptr));
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
			FileD* f = ((Instr_closefds*)instr)->clFD;
			if (f == nullptr) {
				ForAllFDs(ForAllFilesOperation::close_passive_fd);
			}
			else if (!f->FF->IsShared() || (f->FF->LMode == NullMode)) {
				f->CloseFile();
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
			SetMouse(RunInt(nullptr, iPD->MouseX, nullptr),
				RunInt(nullptr, iPD->MouseY, nullptr),
				RunBool(nullptr, iPD->Show, nullptr));
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
			srand(RunInt(nullptr, ((Instr_assign*)instr)->Frml, nullptr));
			break;
		}
		case PInstrCode::_randomize: {
			Random();
			break;
		}
		case PInstrCode::_asgnxnrecs: {
			Instr_assign* ia = (Instr_assign*)instr;
			FileD* f = ia->FD;
			ia->xnrIdx->Release(f);
			break;
		}
		case PInstrCode::_portout: {
			Instr_portout* iPD = (Instr_portout*)instr;
			PortOut(RunBool(nullptr, iPD->IsWord, nullptr),
				(WORD)(RunInt(nullptr, iPD->Port, nullptr)),
				(WORD)(RunInt(nullptr, iPD->PortWhat, nullptr)));
			break;
		}
		default:
			break;
		}
	}
}

void RunProcedure::Run(std::vector<Instr*>& PDRoot)
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

void RunProcedure::CallProcedure(Instr_proc* PD)
{
	if (PD->ProcName == "Indexy05") {
		printf("");
	}

	uint8_t* p1 = nullptr;
	uint8_t* p2 = nullptr;

	std::_Vector_iterator<std::_Vector_val<std::_Simple_types<LocVar*>>> it0;
	std::_Vector_iterator<std::_Vector_val<std::_Simple_types<LocVar*>>> it1;

	WORD i = 0, j = 0;
	int l = 0;
	KeyFldD* kf1 = nullptr;
	KeyFldD* kf2 = nullptr;

	if (PD == nullptr) return;
	MarkBoth(p1, p2);

	std::deque<LinkD*> ld = LinkDRoot;
	size_t lstFDindex = CRdb->data_files.size() - 1; // index of last item in FileDRoot;
	gc->SetInpTT(&PD->PPos, true);

#ifdef _DEBUG
	std::string srcCode = gc->input_string;
	if (srcCode.find("var pgm:string; begin lproc(Indexy); pgm:=PARAM3.TTT;   proc([pgm]); end;") != std::string::npos) {
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
			(*it0)->record = PD->TArg[i].record;
			break;
		}
		case 'f': {
			FileD* proc_file = nullptr;
			if (PD->TArg[i].record != nullptr) {
				const auto state = gc->SaveCompState();
				std::string code = RunString(nullptr, PD->TArg[i].TxtFrml, nullptr);
				gc->SetInpStdStr(code, true);
				proc_file = RdFileD(PD->TArg[i].Name, DataFileType::FandFile, FandFileType::FAND16, "$");
				CRdb->data_files.push_back(proc_file);
				gc->RestoreCompState(state);
			}
			else {
				proc_file = PD->TArg[i].FD;
			}
			it1 = it0;
			while (it1 != PD->loc_var_block.variables.end()) {
				if (((*it1)->f_typ == 'i' || (*it1)->f_typ == 'r') && ((*it1)->FD == (*it0)->FD)) {
					(*it1)->FD = proc_file;
				}
				++it1;
			}
			(*it0)->FD = proc_file;
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
			LVAssignFrml(nullptr, lv, false, PD->TArg[i].Frml, nullptr);
			// return LocVarBlkD back
			LVBD = actual_lvbd;
			break;
		}
		}
		++it0;
	}
	it1 = it0;
	FileD* fd = nullptr;
	while (it0 != PD->loc_var_block.variables.end()) {
		if ((*it0)->f_typ == 'r') {
			fd = (*it0)->FD;
			Record* rec = new Record(fd); //->GetRecSpace();
			// TODO: !!! fd->SetTWorkFlag(rec->GetRecord());
			rec->Reset(); //fd->ZeroAllFlds(rec, false);
			rec->ClearDeleted(); //fd->ClearDeletedFlag(rec->GetRecord());
			(*it0)->record = rec;
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
			XWKey* hX = (*it0)->key;
			if (hX->KFlds.empty()) {
				hX->KFlds = (*it0)->FD->Keys[0]->KFlds;
			}
			XWKey* tmp = (*it0)->key;
			tmp->Open(fd, hX->KFlds, true, false);
		}
		++it0;
	}
	ReleaseStore(&p2);

	// ****** RUN PROCEDURE ****** //
	Run(instructions);
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
				// TODO: !!! loc_var->FD->ClearRecSpace(loc_var->record->GetRecord());
				delete loc_var->record;
				loc_var->record = nullptr;
				break;
			}
			case 'i': {
				FileD* f = (*it0)->FD;
				(*it0)->key->Close(f);
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

	FileD::CloseAndRemoveAllAfter(lstFDindex + 1, CRdb->data_files);

	ReleaseStore(&p1);
	ReleaseStore(&p2);
}

void RunProcedure::RunMainProc(RdbPos RP, bool NewWw)
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
