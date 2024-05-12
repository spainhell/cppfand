#include "runproj.h"

#include <memory>

#include "../Common/compare.h"
#include "../Common/textfunc.h"
#include "../TextEditor/EditorHelp.h"
#include "../TextEditor/TextEditor.h"
#include "../DataEditor/EditReader.h"
#include "..\DataEditor\DataEditor.h"
#include "../ExportImport/ExportImport.h"
#include "../fandio/FandTFile.h"
#include "../fandio/FandXFile.h"
#include "../fandio/files.h"
#include "../Logging/Logging.h"
#include "../MergeReport/ReportGenerator.h"
#include "../MergeReport/Merge.h"
#include "../MergeReport/Report.h"
#include "../Prolog/RunProlog.h"
#include "../Drivers/constants.h"
#include "access.h"
#include "Coding.h"
#include "compile.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "ChkD.h"
#include "KeyFldD.h"
#include "legacy.h"
#include "oaccess.h"
#include "obaseww.h"
#include "RunMessage.h"
#include "rdfildcl.h"
#include "rdproc.h"
#include "rdrun.h"
#include "runproc.h"
#include "wwmenu.h"

int UserW = 0;

struct RdbRecVars
{
	char Typ = 0;
	std::string Name;
	std::string Ext;
	int Txt = 0; int OldTxt = 0;
	FileType FTyp = FileType::UNKNOWN;
	int CatIRec = 0;
	bool isSQL = false;
};

int sz = 0; WORD nTb = 0; void* Tb = nullptr;

bool IsCurrChpt()
{
	return CRdb->rdb_file == CFile;
}

FileType ExtToTyp(const std::string& ext)
{
	if ((ext.empty()) || EquUpCase(ext, ".HLP")
#ifdef FandSQL
		|| SEquUpcase(Ext, ".SQL")
#endif	
		)
		return FileType::FAND16;
	else if (EquUpCase(ext, ".X")) return FileType::INDEX;
	else if (EquUpCase(ext, ".DTA")) return FileType::FAND8;
	else if (EquUpCase(ext, ".DBF")) return FileType::DBF;
	else if (EquUpCase(ext, ".RDB")) return FileType::RDB;
	else return FileType::UNKNOWN;
}

void ReleaseFilesAndLinksAfterChapter(EditD* edit)
{
	FileD* FD = nullptr;
	RdbD* R = nullptr;

	if (Chpt->pChain != nullptr) {
		CloseFilesAfter(Chpt->pChain);
	}
	Chpt->pChain = nullptr;
	LinkDRoot = CRdb->OldLDRoot;
	FuncDRoot = CRdb->OldFCRoot;
	CFile = Chpt;

	if (edit != nullptr) {
		CRecPtr = edit->NewRecPtr;
	}
	R = CRdb->ChainBack;
	if (R != nullptr) {
		CRdb->help_file = R->help_file;
	}
	else {
		CRdb->help_file = nullptr;
	}
	CompileFD = true;
}

bool NetFileTest(RdbRecVars* X)
{
	if ((X->Typ != 'F') || (X->CatIRec == 0) || X->isSQL) {
		return false;
	}
	CVol = CatFD->GetVolume(X->CatIRec);
	CPath = FExpand(CatFD->GetPathName(X->CatIRec));
	FSplit(CPath, CDir, CName, CExt);

	if (IsNetCVol()) return true;
	return false;
}

void GetSplitChapterName(FileD* file_d, void* record, std::string& name, std::string& ext)
{

	std::string chapter_name = file_d->loadS(ChptName, record);
	name = TrailChar(chapter_name, ' ');
	const size_t i = name.find('.');
	if (i == std::string::npos) {
		ext = "";
	}
	else {
		ext = name.substr(i, 255);
		name = name.substr(1, i - 1);
	}
}

void GetRdbRecVars(void* RecPtr, RdbRecVars* X)
{
	void* p = nullptr; void* p2 = nullptr; void* cr = nullptr;

	cr = CRecPtr;
	CRecPtr = RecPtr;
	std::string s1 = CFile->loadS(ChptTyp, CRecPtr);
	X->Typ = s1[0];
	GetSplitChapterName(CFile, CRecPtr, X->Name, X->Ext);
	X->Txt = CFile->loadT(ChptTxt, CRecPtr);
	X->OldTxt = CFile->loadT(ChptOldTxt, CRecPtr);
	if (X->Typ == 'F') {
		X->FTyp = ExtToTyp(X->Ext);
		X->CatIRec = CatFD->GetCatalogIRec(X->Name, false);
		X->isSQL = false;
		if (X->OldTxt != 0) {
			MarkBoth(p, p2);
			if (RdFDSegment(0, X->OldTxt)) {
				X->FTyp = CFile->FF->file_type;
				if (CFile->IsSQLFile) X->Ext = ".SQL";
				else {
					switch (X->FTyp) {
					case FileType::RDB: X->Ext = ".RDB"; break;
					case FileType::DBF: X->Ext = ".DBF"; break;
					case FileType::FAND8: X->Ext = ".DTA"; break;
					default: X->Ext = ".000"; break;
					}
				}
			}
			CFile = Chpt;
			ReleaseStore(&p);
			ReleaseStore(&p2);
		}
#ifdef FandSQL
		if (X->Ext == ".SQL") X->isSQL = true;
#endif
	}
	CRecPtr = cr;
}

bool ChptDelFor(EditD* edit, RdbRecVars* X)
{
	bool result = true;
	SetUpdHandle(ChptTF->Handle);
	ReleaseFilesAndLinksAfterChapter(edit);
	switch (X->Typ) {
	case ' ': {
		result = true;
		break;
	}
	case 'D':
	case 'P': {
		SetCompileAll();
		break;
	}
	case 'F': {
		if (X->OldTxt == 0) {
			result = true;
			break; /*don't delete if the record is new*/
		}
		SetCompileAll();
		if (X->isSQL) {
			result = true;
			break;
		}
		SetMsgPar(X->Name);
		if (!PromptYN(814) || NetFileTest(X) && !PromptYN(836)) {
			result = false;
			break;
		}
		if (X->CatIRec != 0) {
			CatFD->SetFileName(X->CatIRec, "");
			if (!PromptYN(815)) {
				result = true;
				break;
			}
			CVol = CatFD->GetVolume(X->CatIRec);
			CPath = FExpand(CatFD->GetPathName(X->CatIRec));
			FSplit(CPath, CDir, CName, CExt);
			TestMountVol(CPath[0]);
		}
		else {
			CDir = "";
			CName = X->Name;
			CExt = X->Ext;
		}
		MyDeleteFile(CDir + CName + CExt);
		CPath = CExtToT(CDir, CName, CExt);
		MyDeleteFile(CPath);
		if (X->FTyp == FileType::INDEX) {
			CPath = CExtToX(CDir, CName, CExt);
			MyDeleteFile(CPath);
		}
		break;
	}
	default: {
		ChptTF->CompileProc = true;
		break;
	}
	}
	return result;
}

bool ChptDel(EditD* edit)
{
	RdbRecVars New;
	if (!IsCurrChpt()) { return true; }
	GetRdbRecVars(edit->NewRecPtr, &New);
	return ChptDelFor(edit, &New);
}

bool IsDuplFileName(DataEditor* data_editor, std::string name)
{
	bool result;

	if (EquUpCase(name, Chpt->Name)) {
		result = true;
	}
	else {
		result = false;
		std::string n; std::string e;
		BYTE* record = CFile->GetRecSpace();
		for (int i = 1; i <= Chpt->FF->NRecs; i++) {
			if (i != data_editor->CRec()) {
				CFile->ReadRec(i, record);
				if (CFile->loadS(ChptTyp, record) == "F") {
					GetSplitChapterName(CFile, record, n, e);
					if (EquUpCase(name, n)) {
						result = true;
						break;
					}
				}
			}
		}
		delete[] record;
		record = nullptr;
	}

	return result;
}

void RenameWithOldExt(RdbRecVars New, RdbRecVars Old)
{
	CExt = Old.Ext;
	RenameFile56(Old.Name + CExt, New.Name + CExt, false);
	CPath = CExtToT(CDir, CName, CExt);
	RenameFile56(Old.Name + CExt, New.Name + CExt, false);
	CPath = CExtToX(CDir, CName, CExt);
	if (Old.FTyp == FileType::INDEX) RenameFile56(Old.Name + CExt, New.Name + CExt, false);
}

WORD ChptWriteCRec(DataEditor* data_editor, EditD* edit)
{
	RdbRecVars New, Old;
	short eq;
	WORD result = 0;
	if (!IsCurrChpt()) return result;
	if (!data_editor->TestIsNewRec()) {
		eq = CompArea(&((BYTE*)CRecPtr)[2], &((BYTE*)edit->OldRecPtr)[2], CFile->FF->RecLen - 2);
		if (eq == _equ) return result;
	}
	GetRdbRecVars(edit->NewRecPtr, &New);
	if (!data_editor->TestIsNewRec()) GetRdbRecVars(edit->OldRecPtr, &Old);
	result = 1;
#ifndef FandGraph
	if (New.Typ == 'L') { WrLLF10Msg(659); return result; }
#endif 
	if (New.Typ == 'D' || New.Typ == 'U') {
		if (!New.Name.empty()) {
			WrLLF10Msg(623);
			return result;
		}
	}
	else if (New.Typ != ' ')
		if (!g_compiler->IsIdentifStr(New.Name) || (New.Typ != 'F') && (New.Ext != "")) {
			WrLLF10Msg(138); return result;
		}
	if (New.Typ == 'F') {
		if (New.Name.length() > 8) { WrLLF10Msg(1002); return result; }
		if (New.FTyp == FileType::UNKNOWN) { WrLLF10Msg(1067); return result; }
		if (IsDuplFileName(data_editor, New.Name)) {
			WrLLF10Msg(1068);
			return result;
		}
		if ((New.FTyp == FileType::RDB) && (New.Txt != 0)) { WrLLF10Msg(1083); return result; }
		if (NetFileTest(&New) && !data_editor->TestIsNewRec() &&
			(Old.Typ == 'F') && (eq != _equ) && !PromptYN(824)) {
			result = 2; return result;
		}
	}
	if ((New.Typ == 'D' || New.Typ == 'I' || New.Typ == 'U')
		|| !data_editor->TestIsNewRec()
		&& (Old.Typ == 'D' || Old.Typ == 'I' || Old.Typ == 'U')) {
		ReleaseFilesAndLinksAfterChapter(edit);
		SetCompileAll();
	}
	if (data_editor->TestIsNewRec()) {
		ReleaseFilesAndLinksAfterChapter(edit);
		goto label2;
	}
	if (New.Typ != Old.Typ) {
	label1:
		if (!ChptDelFor(edit, &Old)) return result;
		CFile->saveT(ChptOldTxt, 0, CRecPtr);
		if (New.Typ == 'F') {
			ReleaseFilesAndLinksAfterChapter(edit);
		}
		goto label2;
	}
	if (New.Typ == ' ' || New.Typ == 'I') goto label2;
	if (New.Typ != 'F') {
		if (New.Name != Old.Name)
			if (New.Typ == 'E' || New.Typ == 'P') { ReleaseFilesAndLinksAfterChapter(edit); SetCompileAll(); }
			else ChptTF->CompileProc = true;
		if ((New.Typ == 'R') && (New.Txt == 0)) ReleaseFilesAndLinksAfterChapter(edit);
		goto label2;
	}
	ReleaseFilesAndLinksAfterChapter(edit);
	SetCompileAll();
	if ((New.OldTxt != 0) && (New.Name != Old.Name)) {
		if (Old.CatIRec != 0) {
			CatFD->SetFileName(Old.CatIRec, New.Name);
		}
		else {
			if (!Old.isSQL) {
				RenameWithOldExt(New, Old);
			}
		}
	}
label2:
	CFile->saveB(ChptVerif, true, CRecPtr);
	result = 0;
	SetUpdHandle(ChptTF->Handle);
	return result;
}

bool RdFDSegment(WORD FromI, int Pos)
{
	return false;
}

WORD FindHelpRecNr(FileD* FD, std::string& txt)
{
	FileD* cf = nullptr; void* cr = nullptr;
	LockMode md = LockMode::NullMode;
	FieldDescr* NmF = nullptr; FieldDescr* TxtF = nullptr;
	WORD i = 0;
	WORD result = 0;
	ConvToNoDiakr(&txt[0], txt.length(), fonts.VFont);
	cf = CFile; cr = CRecPtr;
	CFile = FD;
	CRecPtr = FD->GetRecSpace();
	md = CFile->NewLockMode(RdMode);
	if (CFile->FF->Handle == nullptr) goto label1;
	NmF = CFile->FldD.front();
	TxtF = NmF->pChain;
	for (i = 1; i <= CFile->FF->NRecs; i++) {
		CFile->ReadRec(i, CRecPtr);
		std::string NmFtext = CFile->loadS(NmF, CRecPtr);
		std::string nm = TrailChar(NmFtext, ' ');
		ConvToNoDiakr(&nm[0], nm.length(), fonts.VFont);
		if (EqualsMask(txt, nm)) {
			while ((i < CFile->FF->NRecs) && (CFile->loadT(TxtF, CRecPtr) == 0)) {
				i++;
				CFile->ReadRec(i, CRecPtr);
			}
			result = i;
			goto label2;
		}
	}
label1:
	result = 0;
label2:
	CFile->OldLockMode(md);
	ReleaseStore(&CRecPtr);
	CFile = cf;
	CRecPtr = cr;
	return result;
}

bool PromptHelpName(WORD& N)
{
	wwmix ww;
	std::string txt;
	auto result = false;
	ww.PromptLL(153, txt, 1, true, false, false);
	if ((txt.length() == 0) || (Event.Pressed.KeyCombination() == __ESC)) return result;
	N = FindHelpRecNr(CFile, txt);
	if (N != 0) result = true;
	return result;
}

void EditHelpOrCat(WORD cc, WORD kind, std::string txt)
{
	FileD* FD;
	WORD i, n;
	WORD nCat = 1;
	WORD iCat = 1;
	WORD nHelp = 1;
	WORD iHelp = 1;
	struct niFrml { char Op; double R; } nFrml{ 0, 0 }, iFrml{ 0,0 };
	std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>();
	EditD* EE = nullptr;

	if (cc == __ALT_F2) {
		FD = CRdb->help_file;
		if (kind == 1) {
			FD = CFile->ChptPos.rdb->help_file;
		}
		if (FD == nullptr) return;
		if (kind == 0) {
			i = iHelp;
			n = nHelp;
		}
		else {
			i = 3;
			n = FindHelpRecNr(FD, txt);
			if (n == 0) {
				keyboard.SetKeyBuf("\0\60" + txt); // TODO: tady ma byt KbdBuffer:=#0#60+txt
			}
		}
	}
	else {
		FD = CatFD->GetCatalogFile();
		i = iCat;
		n = nCat;
	}
	if (kind != 2) {
		EE = data_editor->WriteParamsToE();
	}
	EditOpt* EO = new EditOpt();
	EO->UserSelFlds = true; // GetEditOpt();
	EO->Flds = g_compiler->AllFldsList(FD, false);
	EO->WFlags = EO->WFlags | WPushPixel;
	if ((kind == 0) || (n != 0)) {
		iFrml.R = i;
		nFrml.R = n;
		EO->StartRecNoZ = (FrmlElem*)(&nFrml);
		EO->StartIRecZ = (FrmlElem*)(&iFrml);
	}
	data_editor->EditDataFile(FD, EO);
	delete EO; EO = nullptr;

	if (cc == __ALT_F2) {
		nHelp = EdRecNo;
		iHelp = EdIRec;
	}
	else {
		ResetCatalog();
		nCat = EdRecNo;
		iCat = EdIRec;
	}
	if (kind != 2) {
		data_editor->ReadParamsFromE(EE);
	}
}

void StoreChptTxt(FieldDescr* F, std::string text, bool Del)
{
	void* p = nullptr;
	WORD LicNr = ChptTF->LicenseNr;
	int oldpos = CFile->loadT(F, CRecPtr);
	MarkStore(p);

	if (CRdb->Encrypted) {
		if (LicNr != 0) {
			//Coding::XEncode(S, s2);
			text = Coding::XEncode(text);
		}
		else {
			text = Coding::CodingString(CFile, text);
		}
	}
	// this Del has been changed - is it right?
	if (Del) {
		if (LicNr == 0) {
			ChptTF->Delete(oldpos);
		}
		else if (oldpos != 0) {
			ChptTF->Delete(oldpos - LicNr);
		}
	}

	const int pos = ChptTF->Store((char*)text.c_str(), text.length());

	if (LicNr == 0) {
		CFile->saveT(F, pos, CRecPtr);
	}
	else {
		CFile->saveT(F, pos + LicNr, CRecPtr);
	}

	ReleaseStore(&p);
}

void SetChptFldD()
{
	if (Chpt == nullptr) /*ChptTF = nullptr;*/ {
		throw std::exception("SetChptFldDPtr: Chpt is NULL.");
	}
	else {
		ChptTF = Chpt->FF->TF;
		ChptTxtPos = Chpt->FldD.front();
		ChptVerif = ChptTxtPos->pChain;
		ChptOldTxt = ChptVerif->pChain;
		ChptTyp = ChptOldTxt->pChain;
		ChptName = ChptTyp->pChain;
		ChptTxt = ChptName->pChain;
	}
}

void SetRdbDir(FileD* file_d, char Typ, std::string* Nm)
{
	RdbD* r = nullptr; RdbD* rb = nullptr;
	std::string d;
	r = CRdb;
	rb = r->ChainBack;
	if (rb == nullptr) {
		TopRdb = r;
	}
	CVol = "";
	if (Typ == '\\') {
		rb = TopRdb;
		CRdb = rb;
		file_d->CatIRec = CatFD->GetCatalogIRec(*Nm, false);
		CRdb = r;
	}
	if (file_d->CatIRec != 0) {
		CPath = CatFD->GetPathName(file_d->CatIRec);
		if (CPath[1] != ':') {
			d = rb->RdbDir;
			if (CPath[1] == '\\') {
				CPath = copy(d, 1, 2) + CPath;
			}
			else {
				AddBackSlash(d);
				CPath = d + CPath;
			}
		}
		FSplit(CPath, CDir, CName, CExt);
		DelBackSlash(CDir);
	}
	else if (rb == nullptr) CDir = TopRdbDir;
	else {
		CDir = rb->RdbDir;
		AddBackSlash(CDir);
		CDir = CDir + file_d->Name;
	}
	/* !!! with r^ do!!! */ {
		r->RdbDir = CDir;
		if (TopDataDir.empty()) r->DataDir = CDir;
		else if (rb == nullptr) r->DataDir = TopDataDir;
		else {
			d = rb->DataDir;
			AddBackSlash(d);
			r->DataDir = d + file_d->Name;
		}
	}
	CDir = CDir + '\\';
}

void ResetRdOnly()
{
	if (Chpt->FF->UMode == RdOnly) {
		CFile->CloseFile();
		IsInstallRun = true;
		OpenF(CFile, CPath, Exclusive);
		IsInstallRun = false;
	}
}

RdbD* PrepareRdb(const std::string& name, std::string& name1)
{
	short i = 0, n = 0;
	RdbD* rdb_d = new RdbD();

	rdb_d->ChainBack = CRdb;
	rdb_d->OldLDRoot = LinkDRoot;
	rdb_d->OldFCRoot = FuncDRoot;
	//MarkStore2(rdb->Mark2);
	ReadMessage(51);
	std::string s = MsgLine;
	ReadMessage(48);
	val(MsgLine, n, i);
	std::string nr = std::to_string((TxtCols - n));
	s = s + nr;
	g_compiler->SetInpStr(s);
	g_compiler->SetInpStr(s);

	if ((name[0] == '\\')) name1 = name.substr(1, 8);
	else name1 = name;

	rdb_d->rdb_file = RdFileD(name1, FileType::RDB, ""); /*old CRdb for GetCatalogIRec*/

	return rdb_d;
}

void CreateOpenChpt(std::string Nm, bool create)
{
	std::string p; std::string s;
	short i = 0, n = 0;
	std::string nr; std::string Nm1;
	FileUseMode um = Closed;

	bool top = (CRdb == nullptr);
	FileDRoot = nullptr;
	Chpt = nullptr;
	FandTFile* oldChptTF = ChptTF;
	RdbD* R = PrepareRdb(Nm, Nm1);
	CRdb = R;
	Chpt->FF->RecPtr = Chpt->GetRecSpace();

	SetRdbDir(Chpt, Nm[0], &Nm1);
	p = CDir + Nm1 + ".RDB";
	Chpt->FF->Drive = TestMountVol(CPath[0]);
	SetChptFldD();
	if (!spec.RDBcomment) ChptTxt->L = 1;
	SetMsgPar(p);
	if (top) {
		UserName = "";
		UserCode = 0;
		AccRight[0] = 0;
	}
	else {
		if (CRdb->ChainBack != nullptr)	CRdb->help_file = CRdb->ChainBack->help_file;

		while (true) {
			ChDir(R->RdbDir);
			if (IOResult() != 0) {
				if (create && (IsTestRun || !top)) {
					MkDir(R->RdbDir);
					if (IOResult() != 0) RunError(620);
					continue;
				}
				else {
					RunError(631);
				}
			}
			break;
		}
	}

	if (IsTestRun || !create) um = Exclusive;
	else um = RdOnly;
	if (OpenF(Chpt, CPath, um)) {
		if (ChptTF->CompileAll) ResetRdOnly();
		else if (!top && oldChptTF != nullptr && (ChptTF->TimeStmp < oldChptTF->TimeStmp)) {
			// TODO: oldChptTF != nullptr je v podmince navic, protoze dalsi podminka vzdy vyhorela 
			ResetRdOnly();
			SetCompileAll();
		}
	}
	else {
		if (!create || (top && !IsTestRun)) {
			RunError(631);
		}
		OpenCreateF(Chpt, CPath, Exclusive);
		SetCompileAll();
	}

	const bool hasPasswd = Coding::HasPassword(Chpt, 1, "");
	CRdb->Encrypted = hasPasswd ? false : true;
}

void CloseChpt()
{
	if (CRdb == nullptr) return;
	ClearHelpStkForCRdb();
	SaveAndCloseAllFiles();
	bool del = Chpt->FF->NRecs == 0;
	std::string d = CRdb->RdbDir;
	CloseFilesAfter(FileDRoot);
	LinkDRoot = CRdb->OldLDRoot;
	FuncDRoot = CRdb->OldFCRoot;
	void* p = CRdb;
	//void* p2 = CRdb->Mark2;
	CRdb = CRdb->ChainBack;
	//ReleaseBoth(p, p2);
	if (CRdb != nullptr) {
		FileDRoot = CRdb->rdb_file;
		Chpt = FileDRoot;
		SetChptFldD();
		ChDir(CRdb->RdbDir);
		if (del) {
			RmDir(d);
			if (IOResult() != 0) {
				SetMsgPar(d);
				WrLLF10Msg(621);
			}
		}
	}
	else {
		ChDir(OldDir);
		for (WORD i = 1; i <= FloppyDrives; i++) {
			ReleaseDrive(i);
		}
	}
}

void GoCompileErr(WORD IRec, WORD N)
{
	IsCompileErr = true;
	InpRdbPos.rdb = CRdb;
	InpRdbPos.i_rec = IRec;
	CurrPos = 0;
	ReadMessage(N);
	GoExit();
}

FileD* FindFD()
{
	FileD* FD = nullptr;
	std::string d;
	std::string name;
	std::string ext;
	std::string FName = OldTrailChar(' ', CFile->loadS(ChptName, CRecPtr));
	FSplit(FName, d, name, ext);
	FD = FileDRoot;
	while (FD != nullptr) {
		if (EquUpCase(FD->Name, name)) break;
		FD = (FileD*)FD->pChain;
	}
	return FD;
}

void Diagnostics(void* MaxHp, int Free, FileD* FD)
{
	std::string s1 = "---";
	std::string s2 = "---";
	std::string s3 = "---";
	std::string s4 = std::to_string(getAvailPhysMemory() / 1024 / 1024) + " MB";
	RdbD* r = CRdb;

	while (r->ChainBack != nullptr) {
		r = r->ChainBack;
	}

	SetMsgPar(s1, s2, s3, s4);
	WrLLF10Msg(136);
}

bool CompRunChptRec(EditD* edit, WORD CC)
{
	pstring STyp(1);
	void* p = nullptr; void* p2 = nullptr; void* MaxHp = nullptr;
	EditD* OldE = nullptr;
	RdbPos RP; int Free; bool uw = false, mv = false;
	FileD* FD = nullptr;

	EditOpt* EO = nullptr;
	WORD nStrm = 0;
	auto result = false;
	std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>();

	OldE = edit;
	MarkBoth(p, p2);
	EditD* EE = data_editor->WriteParamsToE();

	bool WasError = true;
	bool WasGraph = IsGraphMode;
	FileD* lstFD = (FileD*)LastInChain(FileDRoot);
	std::deque<LinkD*> oldLd = LinkDRoot;

	try {
		IsCompileErr = false;
		uw = false;
		mv = MausVisible;

		FD = nullptr;
		STyp = CFile->loadS(ChptTyp, CRecPtr);
		RP.rdb = CRdb;
		RP.i_rec = data_editor->CRec();
#ifdef FandSQL
		nStrm = nStreams;
#endif
		if (CC == __ALT_F9) {
			if (g_compiler->FindChpt('P', "MAIN", true, &RP)) {
				if (UserW != 0) {
					PopW(UserW);
					uw = true;
				}
				RunMainProc(RP, CRdb->ChainBack = nullptr);
				WasError = false;
			}
			else {
				WrLLF10Msg(58);
			}
		}
		else {
			switch (STyp[1]) {
			case 'F': {
				FD = FindFD();
				if (FD != nullptr && CC == __CTRL_F9) {
					EO = new EditOpt();
					EO->UserSelFlds = true; // GetEditOpt();
					CFile = FD;
					EO->Flds = g_compiler->AllFldsList(FD, false);
					if (data_editor->SelFldsForEO(EO, nullptr)) {
						data_editor->EditDataFile(FD, EO);
					}
				}
				break;
			}
			case 'E': {
				if (CC == __CTRL_F9) {
					EO = new EditOpt();
					EO->UserSelFlds = true; // GetEditOpt();
					EO->FormPos = RP;
					data_editor->EditDataFile(nullptr, EO);
				}
				else {
					//PushEdit();
					std::vector<FieldDescr*> unusedFD;
					EditReader* reader = new EditReader();
					reader->RdFormOrDesign(unusedFD, RP);
					edit = reader->GetEditD();
				}
				break;
			}
			case 'M': {
				g_compiler->SetInpTT(&RP, true);
				const std::unique_ptr merge = std::make_unique<Merge>();
				merge->Read();
				if (CC == __CTRL_F9) {
					merge->Run();
				}
				break;
			}
			case 'R': {
				g_compiler->SetInpTT(&RP, true);
				const std::unique_ptr report = std::make_unique<Report>();
				report->Read(nullptr);
				if (CC == __CTRL_F9) {
					report->Run(nullptr);
					SaveAndCloseAllFiles();
					std::unique_ptr<TextEditor> editor = std::make_unique<TextEditor>();
					editor->ViewPrinterTxt();
				}
				break;
			}
			case 'P': {
				if (CC == __CTRL_F9) {
					if (UserW != 0) {
						PopW(UserW);
						uw = true;
					}
					RunMainProc(RP, CRdb->ChainBack == nullptr);
				}
				else {
					lstFD = (FileD*)LastInChain(FileDRoot);
					std::deque<LinkD*> ld = LinkDRoot;
					g_compiler->SetInpTT(&RP, true);
					ReadProcHead("");
					ReadProcBody();
					lstFD->pChain = nullptr;
					LinkDRoot = ld;
				}
				break;
			}
#ifdef FandProlog
			case 'L': {
				if (CC == __CTRL_F9) {
					TextAttr = ProcAttr;
					ClrScr();
					RunProlog(&RP, "");
				}
				break;
			}
#endif
			default:;
			}
			WasError = false;
		}
	}
	catch (std::exception& e) {
		// TODO: log error
	}

	MaxHp = nullptr;
	ReleaseStore(&p2);
	Free = StoreAvail();
	RunMsgClear();
	if (WasError) {
#ifdef FandSQL
		ShutDownStreams(nStrm);
#endif

		TextAttr = screen.colors.uNorm;
		if (IsGraphMode && !WasGraph) {
			// ScrTextMode(false, false);
			throw std::exception("CompRunChptRec() Graph <-> Text Mode not implemented.");
		}
		else {
			ClrScr(TextAttr);
		}
	}
	if (uw) {
		UserW = 0;/*mem overflow*/
		UserW = PushW(1, 1, TxtCols, TxtRows);
	}
	SaveAndCloseAllFiles();
	if (mv) {
		ShowMouse();
	}
	if (WasError) {
		ForAllFDs(ForAllFilesOperation::clear_xf_update_lock);
	}
	CFile = lstFD->pChain;
	while (CFile != nullptr) {
		CFile->CloseFile();
		CFile = CFile->pChain;
	}
	lstFD->pChain = nullptr;
	LinkDRoot = oldLd;
	ReleaseStore(&p);
	ReleaseStore(&p2);
	edit = OldE;
	//EditDRoot = E;
	data_editor->ReadParamsFromE(EE);
	CRdb = RP.rdb;
	PrevCompInp.clear();
	CFile->ReadRec(data_editor->CRec(), CRecPtr);
	if (IsCompileErr) {
		result = false;
	}
	else {
		result = true;
		if (WasError) {
			return result;
		}
		CFile->saveB(ChptVerif, false, CRecPtr);
		CFile->WriteRec(data_editor->CRec(), CRecPtr);
		if (CC == __CTRL_F8) {
			Diagnostics(MaxHp, Free, FD);
		}
	}
	return result;
}

void RdUserId(bool Chk)
{
	wwmix ww;
	FrmlElem* Z;
	pstring pw(20); pstring pw2(20); pstring name(20);
	WORD code; pstring acc;

	//ptrRdFldNameFrml = nullptr;
	g_compiler->rdFldNameType = FieldNameType::none;
	g_compiler->RdLex();
	if (Lexem == 0x1A) return;
	if (Chk) pw = ww.PassWord(false);
label1:
	g_compiler->TestLex(_quotedstr); name = LexWord;
	g_compiler->RdLex(); g_compiler->Accept(',');
	code = g_compiler->RdInteger(); g_compiler->Accept(',');
	Z = g_compiler->RdStrFrml(nullptr);
	pw2 = RunShortStr(CFile, Z, CRecPtr);
	delete Z; Z = nullptr;
	if (Lexem == ',') { g_compiler->RdLex(); RdByteList(&acc); }
	else { acc[0] = 1; acc[1] = (char)code; }
	if (Chk) {
		if (EquUpCase(pw, pw2)) {
			UserName = name; UserCode = code; UserPassWORD = pw2; AccRight = acc; return;
		}
	}
	else if (code == 0) {
		UserName = name; UserCode = code; UserPassWORD = pw2;
	}
	if (Lexem != 0x1A) { g_compiler->Accept(';'); if (Lexem != 0x1A) goto label1; }
	if (Chk) RunError(629);
}

WORD CompileMsgOn(CHAR_INFO* Buf, int& w)
{
	pstring s;
	WORD result = 0;
	ReadMessage(15);
	if (IsTestRun) {
		w = PushWFramed(0, 0, 30, 4, screen.colors.sNorm, MsgLine, "", WHasFrame + WDoubleFrame + WShadow);
		ReadMessage(117);
		std::string s1 = MsgLine;
		s = GetNthLine(s1, 1, 1, '/');
		screen.GotoXY(3, 2);
		printf("%s", s.c_str());
		result = s.length();
		screen.GotoXY(3, 3);
		printf("%s", GetNthLine(s1, 2, 1, '/').c_str());
	}
	else {
		screen.ScrRdBuf(1, TxtRows, Buf, 40);
		w = 0;
		result = 0;
		screen.ScrClr(1, TxtRows, MsgLine.length() + 2, 1, ' ', screen.colors.zNorm);
		screen.ScrWrStr(2, TxtRows, MsgLine, screen.colors.zNorm);
	}
	return result;
}

void CompileMsgOff(CHAR_INFO* Buf, int& w)
{
	if (w != 0) {
		PopW(w);
	}
	else {
		screen.ScrWrCharInfoBuf(1, TxtRows, Buf, 40);
	}
}

int MakeDbfDcl(pstring Nm)
{
	DBaseHd Hd; DBaseFld Fd;
	LongStr* t; char c;
	pstring s(80); pstring s1(10); void* p;

	CPath = FExpand(Nm + ".DBF"); CVol = "";
	WORD i = CatFD->GetCatalogIRec(Nm, true);
	if (i != 0) {
		CVol = CatFD->GetVolume(i);
		CPath = FExpand(CatFD->GetPathName(i));
		FSplit(CPath, CDir, CName, CExt);
	}
	HANDLE h = OpenH(CPath, _isOldFile, RdOnly);
	TestCPathError();
	ReadH(h, 32, &Hd); WORD n = (Hd.HdLen - 1) / 32 - 1; t = new LongStr(2); t->LL = 0;
	for (i = 1; i <= n; i++) {
		ReadH(h, 32, &Fd);
		s = StrPas((char*)Fd.Name.c_str());
		switch (Fd.Typ)
		{
		case 'C': c = 'A'; break;
		case 'D': c = 'D'; break;
		case 'L': c = 'B'; break;
		case 'M': c = 'T'; break;
		case 'N':
		case 'F': c = 'F'; break;
		}
		s = s + ':' + c;
		switch (c) {
		case 'A': { str(Fd.Len, s1); s = s + ',' + s1; break; }
		case 'F': {
			Fd.Len -= Fd.Dec;
			if (Fd.Dec != 0) Fd.Len--;
			str(Fd.Len, s1); s = s + ',' + s1; str(Fd.Dec, s1); s = s + '.' + s1;
			break;
		}
		}
		s = s + ';' + 0x0D + 0x0A; // ^M + ^J
		p = new BYTE[s.length()];
		Move(&s[1], p, s.length());
		t->LL += s.length();
	}
	CFile->saveLongS(ChptTxt, t, CRecPtr);
	CloseH(&h);
	return 0;
}

FileD* RdF(FileD* file_d, std::string FileName)
{
	std::string d, name, ext;
	FileType FDTyp = FileType::UNKNOWN;
	std::string s;
	FieldDescr* IdF = nullptr; FieldDescr* TxtF = nullptr;
	short i = 0, n = 0;
	std::string nr;
	FSplit(FileName, d, name, ext);

	FDTyp = ExtToTyp(ext);
	if (FDTyp == FileType::RDB) {
		ReadMessage(51);
		s = MsgLine;
		ReadMessage(49);
		val(MsgLine, n, i);
		nr = std::to_string(TxtCols - n);
		s = s + nr;
		g_compiler->SetInpStr(s);
	}
	else {
		int pos = file_d->loadT(ChptTxt, file_d->FF->RecPtr);
		g_compiler->SetInpTTPos(file_d, pos, CRdb->Encrypted);
	}
	return RdFileD(name, FDTyp, ext);
}

bool EquStoredF(FieldDescr* F1, FieldDescr* F2)
{
	while (true) {
		while (F1 != nullptr && !F1->isStored()) {
			F1 = F1->pChain;
		}
		while (F2 != nullptr && !F2->isStored()) {
			F2 = F2->pChain;
		}
		if (F1 == nullptr) {
			if (F2 != nullptr) return false;
			else return true;
		}
		if (F2 == nullptr || !g_compiler->FldTypIdentity(F1, F2) || (F1->Flg & ~f_Mask) != (F2->Flg & ~f_Mask)) return false;
		F1 = F1->pChain;
		F2 = F2->pChain;
	}
}

void DeleteF()
{
	CFile->CloseFile();
	SetPathAndVolume(CFile);
	MyDeleteFile(CPath);
	CPath = CExtToX(CDir, CName, CExt);
	if (CFile->FF->XF != nullptr) {
		MyDeleteFile(CPath);
	}
	CPath = CExtToT(CDir, CName, CExt);
	if (CFile->FF->TF != nullptr) {
		MyDeleteFile(CPath);
	}
}

bool MergeAndReplace(FileD* fd_old, FileD* fd_new)
{
	bool result;

	try {
		std::string s = "#I1_";
		s += fd_old->Name + " #O1_@";
		g_compiler->SetInpStr(s);

		SpecFDNameAllowed = true;
		const std::unique_ptr merge = std::make_unique<Merge>();
		merge->Read();
		SpecFDNameAllowed = false;
		merge->Run();

		SaveAndCloseAllFiles();
		CFile = fd_old;
		DeleteF();
		CFile = fd_new;
		CFile->CloseFile();
		fd_old->FF->file_type = fd_new->FF->file_type;
		SetPathAndVolume(CFile);
		std::string p = CPath;
		CFile = fd_old;
		SetPathAndVolume(CFile);
		RenameFile56(p, CPath, false);
		CFile = fd_new;
		/*TF->Format used*/
		CPath = CExtToT(CDir, CName, CExt);
		p = CPath;
		SetPathAndVolume(CFile);
		CPath = CExtToT(CDir, CName, CExt);
		RenameFile56(CPath, p, false);
		result = true;
	}
	catch (std::exception& e) {
		// TODO: log error
		CFile = fd_old;
		CFile->CloseFile();
		CFile = fd_new;
		DeleteF();
		SpecFDNameAllowed = false;
		result = false;
	}

	return result;
}

bool EquKeys(XKey* K1, XKey* K2)
{
	auto result = false;
	while (K1 != nullptr) {
		if ((K2 == nullptr) || (K1->Duplic != K2->Duplic)) return result;
		KeyFldD* KF1 = K1->KFlds;
		KeyFldD* KF2 = K2->KFlds;
		while (KF1 != nullptr) {
			if ((KF2 == nullptr) || (KF1->CompLex != KF2->CompLex) || (KF1->Descend != KF2->Descend)
				|| (KF1->FldD->Name != KF2->FldD->Name)) return result;
			KF1 = KF1->pChain;
			KF2 = KF2->pChain;
		}
		if (KF2 != nullptr) return result;
		K1 = K1->Chain;
		K2 = K2->Chain;
	}
	if (K2 != nullptr) return result;
	result = true;
	return result;
}

bool MergeOldNew(FileD* file_d, bool Veriflongint, int Pos)
{
	std::string Name;
	std::deque<LinkD*> ld = LinkDRoot;
	auto result = false;
	FileD* FDOld = nullptr;
	FileD* FDNew = file_d;
	SetPathAndVolume(file_d);
	Name = FDNew->Name;
	FDNew->Name = "@";
	if (!RdFDSegment(0, Pos)) goto label1;
	ChainLast(FileDRoot, Chpt);
	FDOld = Chpt;
	FDOld->Name = Name;
	if ((FDNew->FF->file_type != FDOld->FF->file_type) || !EquStoredF(FDNew->FldD.front(), FDOld->FldD.front())
#ifdef FandSQL
		&& !FDNew->IsSQLFile && !FDOld->IsSQLFile
#endif
		) {
		MergeAndReplace(FDOld, FDNew);
		result = true;
	}
	else if ((FDOld->FF->file_type == FileType::INDEX) && !EquKeys(FDOld->Keys[0], FDNew->Keys[0])) {
		SetPathAndVolume(Chpt);
		CPath = CExtToX(CDir, CName, CExt);
		MyDeleteFile(CPath);
	}
label1:
	FDNew->pChain = nullptr;
	LinkDRoot = ld;
	FDNew->Name = Name;
	FDNew->FullPath = CPath;
	CRecPtr = Chpt->FF->RecPtr;
	return result;
}

bool CompileRdb(FileD* rdb_file, bool Displ, bool Run, bool FromCtrlF10)
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "starting CompileRdb()");
	CHAR_INFO Buf[40];
	int w = 0;
	int I = 0, J = 0, OldTxt = 0, Txt = 0;
	//int OldCRec = 0;
	std::string STyp;
	char Typ = '\0';
	std::string Name, dir, nm, ext;
	bool Verif = false, FDCompiled = false, Encryp = false;
	char Mode = '\0';
	RdbPos RP;
	void* p = nullptr;
	FileD* p1 = nullptr;
	void* p2 = nullptr;
	WORD lmsg = 0;
	std::string RprtTxt;
	bool top = false;
	FileD* lstFD = nullptr;
	auto result = false;

	//EditD* OldE = E;
	EditReader* reader = new EditReader();
	EditD* edit = nullptr;
	MarkBoth(p, p2);
	//p1 = p;

	try {
		IsCompileErr = false; FDCompiled = false;
		//OldCRec = data_editor->CRec();
		RP.rdb = CRdb;
		top = (CRdb->ChainBack == nullptr);
		if (top) {
			ChptTF->CompileAll = true;
			UserName[0] = 0; UserCode = 0; UserPassWORD[0] = 0; AccRight[0] = 0;
			if (ChptTF->CompileAll || CompileFD) Switches[0] = 0;
		}
		lmsg = CompileMsgOn(Buf, w);
		//CRecPtr = rdb_file->FF->RecPtr;
		Encryp = CRdb->Encrypted;
		for (I = 1; I <= rdb_file->FF->NRecs; I++) {
			rdb_file->ReadRec(I, rdb_file->FF->RecPtr);
			RP.i_rec = I;
			Verif = rdb_file->loadB(ChptVerif, rdb_file->FF->RecPtr);
			STyp = rdb_file->loadS(ChptTyp, rdb_file->FF->RecPtr);
			Typ = STyp[0];
			Name = OldTrailChar(' ', rdb_file->loadS(ChptName, rdb_file->FF->RecPtr));
			Txt = rdb_file->loadT(ChptTxt, rdb_file->FF->RecPtr);
			if (Verif && ((ChptTF->LicenseNr != 0) || Encryp || (rdb_file->FF->UMode == RdOnly))) GoCompileErr(I, 647);
			if (Verif || ChptTF->CompileAll || FromCtrlF10 || (Typ == 'U') ||
				(Typ == 'F' || Typ == 'D') && CompileFD ||
				(Typ == 'P') && ChptTF->CompileProc) {
				OldTxt = rdb_file->loadT(ChptOldTxt, rdb_file->FF->RecPtr);
				InpRdbPos = RP;
				if (IsTestRun) {
					ClrScr(TextAttr);
					screen.GotoXY(3 + lmsg, 2);
					printf("%*i", 4, I);
					screen.GotoXY(3 + lmsg, 3);
					printf("%*s%*s", 4, STyp.c_str(), 14, rdb_file->loadS(ChptName, rdb_file->FF->RecPtr).c_str());
					if (!(Typ == ' ' || Typ == 'D' || Typ == 'U')) { /* dupclicate name checking */
						for (J = 1; J <= I - 1; J++) {
							rdb_file->ReadRec(J, rdb_file->FF->RecPtr);
							if ((STyp == rdb_file->loadS(ChptTyp, rdb_file->FF->RecPtr))
								&& EquUpCase(Name, OldTrailChar(' ', rdb_file->loadS(ChptName, rdb_file->FF->RecPtr)))) {
								GoCompileErr(I, 649);
							}
						}
						rdb_file->ReadRec(I, rdb_file->FF->RecPtr);
					}
				}
				switch (Typ) {
				case 'F': {
					FDCompiled = true;
					std::deque<LinkD*> ld = LinkDRoot;
					MarkStore(p1);
					FSplit(Name, dir, nm, ext);
					if ((Txt == 0) && IsTestRun) {
						SetMsgPar(Name);
						if (EquUpCase(ext, ".DBF") && PromptYN(39)) {
							rdb_file->saveT(ChptOldTxt, 0, rdb_file->FF->RecPtr);
							OldTxt = 0;
							MakeDbfDcl(nm);
							Txt = rdb_file->loadT(ChptTxt, rdb_file->FF->RecPtr);
							rdb_file->WriteRec(I, rdb_file->FF->RecPtr);
						}
					}
#ifndef FandSQL
					if (EquUpCase(ext, ".SQL")) {
						GoCompileErr(I, 654);
					}
#endif
					if (Verif || ChptTF->CompileAll || OldTxt == 0) {
					label2:
						p1 = RdF(rdb_file, Name);
						if (p1->IsHlpFile) {
							CRdb->help_file = p1;
						}
						if (OldTxt > 0) {
							MergeOldNew(p1, Verif, OldTxt);
						}

					}
					else if (!RdFDSegment(I, OldTxt)) {
						LinkDRoot = ld;
						//ReleaseStore(&p1);
						//CFile = Chpt;
						goto label2;
					}
					else {
						ChainLast(FileDRoot, rdb_file);
						MarkStore(p1);
						if (rdb_file->IsHlpFile) {
							CRdb->help_file = rdb_file;
						}
					}
					break;
				}
				case 'M': {
					g_compiler->SetInpTTPos(rdb_file, Txt, Encryp);
					const std::unique_ptr merge = std::make_unique<Merge>();
					merge->Read();
					break;
				}
				case 'R': {
					if (Txt == 0 && IsTestRun) {
						const std::unique_ptr auto_report = std::make_unique<ReportGenerator>();
						RprtTxt = auto_report->SelGenRprt(Name);
						//CFile = Chpt;
						if (RprtTxt.empty()) GoCompileErr(I, 1145);
						rdb_file->saveS(ChptTxt, RprtTxt, rdb_file->FF->RecPtr);
						rdb_file->WriteRec(I, rdb_file->FF->RecPtr);
					}
					else {
						g_compiler->SetInpTTPos(rdb_file, Txt, Encryp);
						const std::unique_ptr report = std::make_unique<Report>();
						report->Read(nullptr);
					}
					break;
				}
				case 'P': {
					if (FileDRoot->pChain == nullptr) {
						lstFD = FileDRoot;
					}
					else {
						lstFD = (FileD*)LastInChain(FileDRoot);
					}
					std::deque<LinkD*> ld = LinkDRoot;
					g_compiler->SetInpTTPos(rdb_file, Txt, Encryp);
					ReadProcHead(Name);
					ReadProcBody();
					lstFD->pChain = nullptr;
					LinkDRoot = ld;
					break;
				}
				case 'E': {
					//PushEdit();
					std::vector<FieldDescr*> unusedFD;
					std::unique_ptr<EditReader> e_reader = std::make_unique<EditReader>();
					e_reader->RdFormOrDesign(unusedFD, RP);
					// replace last 'edit' (exception handling)
					delete edit;
					edit = e_reader->GetEditD();
					//E = OldE;
					//EditDRoot = E;
					break;
				}
				case 'U': {
					if (!top || (I > 1)) GoCompileErr(I, 623);
					if (Txt != 0) {
						ResetCompilePars();
						g_compiler->SetInpTTPos(rdb_file, Txt, Encryp);
						RdUserId(!IsTestRun || (ChptTF->LicenseNr != 0));
						MarkStore(p1);
					}
					break;
				}
				case 'D': {
					ResetCompilePars();
					g_compiler->SetInpTTPos(rdb_file, Txt, Encryp);
					ReadDeclChpt();
					MarkStore(p1);
					break;
				}
#ifdef FandProlog
						//case 'L': {
						//	SetInpTTPos(Txt, Encryp);
						//	TProgRoots* typeL = ReadProlog(I);
						//	delete typeL; typeL = nullptr;
						//	break;
						//}
#endif
				}
			}
			//ReleaseStore(&p1);
			ReleaseStore(&p2);
			//CFile = Chpt;
			//CRecPtr = rdb_file->FF->RecPtr;
			if (Verif) {
				rdb_file->ReadRec(I, rdb_file->FF->RecPtr);
				rdb_file->saveB(ChptVerif, false, rdb_file->FF->RecPtr);
				rdb_file->WriteRec(I, rdb_file->FF->RecPtr);
			}
		}
		if (ChptTF->CompileAll || ChptTF->CompileProc) {
			ChptTF->CompileAll = false;
			ChptTF->CompileProc = false;
			SetUpdHandle(ChptTF->Handle);
		}
		CompileFD = false;
		result = true;
		//if (!Run) {
		//	CRecPtr = edit->NewRecPtr;
		//	//CFile->ReadRec(data_editor->CRec(), CRecPtr);
		//}
		CompileMsgOff(Buf, w);
#ifdef FandSQL
		if (top && (Strm1 != nullptr)) Strm1->Login(UserName, UserPassWORD);
#endif
		log->log(loglevel::DEBUG, "finish CompileRdb()");
	}
	catch (std::exception& e) {
		log->log(loglevel::EXCEPTION, "CompileRdb() exception: ", e.what());
		result = false;
		ReleaseFilesAndLinksAfterChapter(edit);
		PrevCompInp.clear();
		//ReleaseBoth(p, p2);
		//E = OldE; EditDRoot = E;
		//CFile = Chpt;
		//if (!Run) CRecPtr = edit->NewRecPtr;
		if (!IsCompileErr) {
			InpRdbPos.i_rec = I;
		}
	}

	delete edit; edit = nullptr;
	delete reader; reader = nullptr;

	CompileMsgOff(Buf, w);
	return result;
}

void GotoErrPos(WORD& Brk)
{
	std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>();

	IsCompileErr = false;
	std::string s = MsgLine;
	if (InpRdbPos.rdb != CRdb) {
		data_editor->DisplEditWw();
		SetMsgPar(s);
		WrLLF10Msg(110);
		if (InpRdbPos.i_rec == 0) SetMsgPar("");
		else SetMsgPar(InpRdbPos.rdb->rdb_file->Name);
		WrLLF10Msg(622);
		Brk = 0;
		return;
	}
	if (CurrPos == 0) {
		data_editor->DisplEditWw();
		data_editor->GotoRecFld(InpRdbPos.i_rec, data_editor->GetEditD()->FirstFld->pChain);
		SetMsgPar(s);
		WrLLF10Msg(110);
		Brk = 0;
		return;
	}
	data_editor->CFld = data_editor->GetEditD()->LastFld;
	data_editor->SetNewCRec(InpRdbPos.i_rec, true);
	CFile->saveR(ChptTxtPos, short(CurrPos), CRecPtr);
	CFile->WriteRec(data_editor->CRec(), CRecPtr);
	data_editor->EditFreeTxt(ChptTxt, s, true, Brk);
}

void WrErrMsg630(std::string Nm)
{
	IsCompileErr = false;
	SetMsgPar(MsgLine);
	WrLLF10Msg(110);
	SetMsgPar(Nm);
	WrLLF10Msg(630);
}

bool EditExecRdb(std::string Nm, std::string proc_name, Instr_proc* proc_call, wwmix* ww)
{
	WORD Brk = 0, cc = 0;
	void* p = nullptr;
	pstring passw(20);
	bool b = false;
	RdbPos RP;
	EditOpt* EO = nullptr;

	auto result = false;
	bool top = CRdb == nullptr;
	bool EscCode = false;
	int w = UserW; UserW = 0;
	bool wasGraph = IsGraphMode;

#ifdef FandSQL
	if (top) SQLConnect();
#endif
	try {
		CreateOpenChpt(Nm, true);
		CompileFD = true;
#ifndef FandRunV

		if (!IsTestRun || (ChptTF->LicenseNr != 0) ||
			!top && CRdb->Encrypted) {
#endif
			MarkStore(p);
			EditRdbMode = false;
			bool hasToCompileRdb = CompileRdb(Chpt, false, true, false);
			if (hasToCompileRdb) {
				bool procedureFound = g_compiler->FindChpt('P', proc_name, true, &RP);
				if (procedureFound) {
					try {
						IsCompileErr = false;
						if (proc_call != nullptr) {
							proc_call->PPos = RP;
							CallProcedure(proc_call);
						}
						else {
							RunMainProc(RP, top);
						}
						result = true;
						goto label9;
					}
					catch (std::exception& e) {
						if (IsCompileErr) {
							WrErrMsg630(Nm);
						}
						goto label9;
					}
				}
				else {
					SetMsgPar(Nm, proc_name);
					WrLLF10Msg(632);
				}
			}
			else if (IsCompileErr) WrErrMsg630(Nm);
#ifndef FandRunV
			if ((ChptTF->LicenseNr != 0) || CRdb->Encrypted
				|| (Chpt->FF->UMode == RdOnly)) goto label9;
			ReleaseFilesAndLinksAfterChapter(nullptr);
			ReleaseStore(&p);
		}
		else if (!top) {
			UserW = PushW(1, 1, TxtCols, TxtRows);
		}
		EditRdbMode = true;
		if (CRdb->Encrypted) {
			// ask for the project password
			passw = ww->PassWord(false);
		}
		IsTestRun = true;
		EO = new EditOpt();
		EO->UserSelFlds = true; //EO = GetEditOpt();
		EO->Flds = g_compiler->AllFldsList(Chpt, true);
		EO->Flds.erase(EO->Flds.begin(), EO->Flds.begin() + 3);

		EditReader* reader = new EditReader();
		reader->NewEditD(Chpt, EO, static_cast<uint8_t*>(CRecPtr));
		EditD* edit = reader->GetEditD();
		edit->params_->MustCheck = true; /*ChptTyp*/
		if (CRdb->Encrypted) {
			if (Coding::HasPassword(Chpt, 1, passw)) {
				CRdb->Encrypted = false;
				Coding::SetPassword(Chpt, 1, "");
				CodingCRdb(edit, false);
			}
			else {
				WrLLF10Msg(629);
				goto label9;
			}
		}
		std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>(edit);
		if (!data_editor->OpenEditWw()) {
			goto label8;
		}
		result = true;
		Chpt->FF->WasRdOnly = false;
		if (!top && (Chpt->FF->NRecs > 0))
			if (CompileRdb(Chpt, true, false, false)) {
				if (g_compiler->FindChpt('P', proc_name, true, &RP)) {
					data_editor->GotoRecFld(RP.i_rec, data_editor->CFld);
				}
			}
			else {
				GotoErrPos(Brk);
				goto label5;
			}
		else if (ChptTF->IRec <= Chpt->FF->NRecs) {
			data_editor->GotoRecFld(ChptTF->IRec, data_editor->CFld);
		}

		data_editor->RunEdit(nullptr, Brk);
	label2:
		// TODO: je to potreba?
		cc = Event.Pressed.KeyCombination();
		SaveAndCloseAllFiles();
		if ((cc == __CTRL_F10) || ChptTF->CompileAll || CompileFD) {
			ReleaseFilesAndLinksAfterChapter(edit);
			data_editor->SetSelectFalse();
			edit->Bool = nullptr;
			ReleaseStore(&edit->AfterE);
		}
		if (cc == __CTRL_F10) {
			SetUpdHandle(ChptTF->Handle);
			if (!CompileRdb(Chpt, true, false, true)) {
				if (IsCompileErr) {
					GotoErrPos(Brk);
					goto label5;
				}
				if (Brk == 1) {
					data_editor->DisplEditWw();
				}
				data_editor->GotoRecFld(InpRdbPos.i_rec, edit->FirstFld->pChain);
				data_editor->RunEdit(nullptr, Brk);
				goto label2;
			}
			if (!PromptCodeRdb(edit)) {
				goto label6;
			}
			Chpt->FF->WasRdOnly = true;
			goto label8;
		}
		if (Brk != 0) {
			if (!CompileRdb(Chpt, Brk == 2, false, false)) {
				if (IsCompileErr) {
					GotoErrPos(Brk);
					goto label5;
				}
				if (Brk == 1) data_editor->DisplEditWw();
				data_editor->GotoRecFld(InpRdbPos.i_rec, edit->FirstFld->pChain);
				data_editor->RunEdit(nullptr, Brk);
				goto label2;
			}
			if (cc == __ALT_F2) {
				EditHelpOrCat(cc, 0, "");
				goto label41;
			}
			if (!CompRunChptRec(edit, cc)) {
				GotoErrPos(Brk);
				goto label5;
			}
		label41:
			if (Brk == 1) {
				data_editor->EditFreeTxt(ChptTxt, "", true, Brk);
			label5:
				if (Brk != 0) {
					goto label2;
				}
				else {
					data_editor->RunEdit(nullptr, Brk);
					goto label2;
				}
			}
			else {
			label6:
				data_editor->DisplEditWw();
				data_editor->RunEdit(nullptr, Brk);
				goto label2;
			}
		}
		ChptTF->IRec = data_editor->CRec();
		SetUpdHandle(ChptTF->Handle);
	label8:
		printf("");
		//data_editor->PopEdit();
#endif
	}
	catch (std::exception& e)
	{

	}

label9:
	//RestoreExit(er);
	if (!wasGraph && IsGraphMode) {
		// ScrTextMode(false, false);
		throw std::exception("CompRunChptRec() Graph <-> Text Mode switching not implemented.");
	}
	if (UserW != 0) PopW(UserW);
	UserW = w;
	RunMsgClear();
	CloseChpt();
#ifdef FandSQL
	if (top) SQLDisconnect;
#endif
	return result;
}

void UpdateCat()
{
	CFile = CatFD->GetCatalogFile();
	if (CatFD->GetCatalogFile()->FF->Handle == nullptr) {
		OpenCreateF(CFile, CPath, Exclusive);
	}
	EditOpt* EO = new EditOpt();
	EO->UserSelFlds = true;
	EO->Flds = g_compiler->AllFldsList(CatFD->GetCatalogFile(), true);

	std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>();
	data_editor->EditDataFile(CatFD->GetCatalogFile(), EO);

	ChDir(OldDir);
	delete EO; EO = nullptr;
}

void UpdateUTxt()
{
	bool Upd;
	int Pos = 0;
	void* p = nullptr;
	void* p1 = nullptr;
	size_t LL = 0;
	CFile = Chpt;
	CRecPtr = Chpt->FF->RecPtr;
	WORD LicNr = (WORD)ChptTF->LicenseNr;
	MarkStore(p1);
	if (CFile->FF->NRecs == 0) {
		WrLLF10Msg(9);
		return;
	}
	CFile->ReadRec(1, CRecPtr);
	if (CFile->loadS(ChptTyp, CRecPtr) != "U") {
		WrLLF10Msg(9);
		return;
	}
	int w = PushW(1, 1, TxtCols, TxtRows - 1);
	WORD TxtPos = 1;
	TextAttr = screen.colors.tNorm;
	int OldPos = CFile->loadT(ChptTxt, CRecPtr);
	LongStr* S = CFile->loadLongS(ChptTxt, CRecPtr);

	if (CRdb->Encrypted) {
		Coding::CodingLongStr(CFile, S);
	}

	g_compiler->SetInpLongStr(S, false);
	MarkStore(p);
	RdUserId(false);
	ReleaseStore(&p);
	bool b = true;

	while (true) {
		try {
			std::unique_ptr<TextEditor> editor = std::make_unique<TextEditor>();
			editor->SimpleEditText('T', "", "", S, 0x7FFF, TxtPos, Upd);
			g_compiler->SetInpLongStr(S, false);
			MarkStore(p);
			RdUserId(false);
			ReleaseStore(&p);
			b = false;
			if (Upd) {
				std::string store = std::string(S->A, S->LL);
				StoreChptTxt(ChptTxt, store, true);
				CFile->WriteRec(1, CRecPtr);
			}
			delete S; S = nullptr;
			break;
		}
		catch (std::exception& ex) {
			if (b) {
				WrLLF10MsgLine();
				ReleaseStore(&p);
				if (PromptYN(59)) {
					continue;
				}
			}
			else {
				WrLLF10Msg(9);
			}
			break;
		}
	}

	PopW(w);
	ReleaseStore(&p1);
}

void InstallRdb(std::string n)
{
	wwmix ww;

	std::string passw;
	TMenuBoxS* w = nullptr;
	WORD i = 0;

	CreateOpenChpt(n, false);
	if (!Coding::HasPassword(Chpt, 1, "") && !Coding::HasPassword(Chpt, 2, "")) {
		passw = ww.PassWord(false);
		if (!Coding::HasPassword(Chpt, 2, passw)) {
			WrLLF10Msg(629);
			CloseChpt();
			return;
		}
	}
	if (Chpt->FF->UMode == RdOnly) {
		UpdateCat();
		CloseChpt();
		return;
	}
	ReadMessage(8);

	i = 1;
	w = new TMenuBoxS(43, 6, MsgLine);

	while (true) {
		i = w->Exec(i);
		switch (i) {
		case 0: {
			delete w;
			w = nullptr;
			CloseChpt();
			return;
		}
		case 1: {
			UpdateCat();
			continue;
		}
		case 2: {
			UpdateUTxt();
			break;
		}
		case 3: {
			Coding::SetPassword(Chpt, 2, ww.PassWord(true));
			break;
		}
		default:;
		}
		SetUpdHandle(ChptTF->Handle);
	}
}
