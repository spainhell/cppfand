#include "runproj.h"

#include <memory>

#include "../Common/compare.h"
#include "../Common/textfunc.h"
#include "../TextEditor/EditorHelp.h"
#include "../TextEditor/TextEditor.h"
#include "../DataEditor/EditReader.h"
#include "../DataEditor/DataEditor.h"
#include "../ExportImport/ExportImport.h"
#include "../fandio/FandTFile.h"
#include "../fandio/FandXFile.h"
#include "../fandio/DbfFile.h"
#include "../Logging/Logging.h"
#include "../MergeReport/ReportGenerator.h"
#include "../MergeReport/Merge.h"
#include "../MergeReport/Report.h"
#include "../Prolog/RunProlog.h"
#include "../Drivers/constants.h"
#include "access.h"
#include "../Common/Coding.h"
#include "Compiler.h"
#include "../fandio/FieldDescr.h"
#include "../Common/FileD.h"
#include "GlobalVariables.h"
#include "../fandio/KeyFldD.h"
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
	FandFileType FTyp = FandFileType::UNKNOWN;
	int CatIRec = 0;
	bool isSQL = false;
};

int sz = 0; WORD nTb = 0; void* Tb = nullptr;

bool IsCurrChpt(FileD* file_d)
{
	return CRdb->v_files[0] == file_d;
}

FandFileType ExtToTyp(const std::string& ext)
{
	if ((ext.empty()) || EquUpCase(ext, ".HLP")
#ifdef FandSQL
		|| SEquUpcase(Ext, ".SQL")
#endif	
		)
		return FandFileType::FAND16;
	else if (EquUpCase(ext, ".X")) return FandFileType::INDEX;
	else if (EquUpCase(ext, ".DTA")) return FandFileType::FAND8;
	// else if (EquUpCase(ext, ".DBF")) return FandFileType::DBF;
	else if (EquUpCase(ext, ".RDB")) return FandFileType::RDB;
	else return FandFileType::UNKNOWN;
}

void ReleaseFilesAndLinksAfterChapter(EditD* edit)
{
	//if (Chpt->pChain != nullptr) {
	//	CloseFilesAfter(Chpt->pChain);
	//}
	//Chpt->pChain = nullptr;

	if (CRdb->v_files.size() > 1) {
		FileD::CloseAndRemoveAllAfter(1, CRdb->v_files);
	}

	LinkDRoot = CRdb->OldLDRoot;
	FuncDRoot = CRdb->OldFCRoot;
	CFile = Chpt;

	//if (edit != nullptr) {
	//	CRecPtr = edit->NewRec->GetRecord();
	//}
	RdbD* R = CRdb->ChainBack;
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
	CVol = catalog->GetVolume(X->CatIRec);
	CPath = FExpand(catalog->GetPathName(X->CatIRec));
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
		name = name.substr(0, i);
	}
}

void GetRdbRecVars(const EditD* edit, void* record, RdbRecVars* X)
{
	void* p = nullptr;
	void* p2 = nullptr;
	//void* cr = nullptr;

	//cr = CRecPtr;
	//CRecPtr = record;
	FileD* file_d = edit->FD;
	std::string s1 = file_d->loadS(ChptTyp, record);
	X->Typ = s1[0];
	GetSplitChapterName(file_d, record, X->Name, X->Ext);
	X->Txt = file_d->loadT(ChptTxt, record);
	X->OldTxt = file_d->loadT(ChptOldTxt, record);
	if (X->Typ == 'F') {
		X->FTyp = ExtToTyp(X->Ext);
		X->CatIRec = catalog->GetCatalogIRec(X->Name, false);
		X->isSQL = false;
		if (X->OldTxt != 0) {
			MarkBoth(p, p2);
			if (RdFDSegment(0, X->OldTxt)) {
				X->FTyp = file_d->FF->file_type;
				if (file_d->IsSQLFile) {
					X->Ext = ".SQL";
				}
				else if (file_d->FileType == DataFileType::DBF) {
					X->Ext = ".DBF";
				}
				else {
					switch (X->FTyp) {
					case FandFileType::RDB: X->Ext = ".RDB"; break;
					case FandFileType::FAND8: X->Ext = ".DTA"; break;
					default: X->Ext = ".000"; break;
					}
				}
			}
			//CFile = Chpt;
			ReleaseStore(&p);
			ReleaseStore(&p2);
		}
#ifdef FandSQL
		if (X->Ext == ".SQL") X->isSQL = true;
#endif
	}
	//CRecPtr = cr;
}

bool ChptDelFor(EditD* edit, RdbRecVars* X)
{
	bool result = true;
	ChptTF->SetUpdateFlag(); //SetUpdHandle(ChptTF->Handle);
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
		else {
			// this 'else' was added here, because there is no info in OldTxt about previous file params
			// (type, name, text) so an extension is incorrect -> causes error
			// TODO: after this info is added to OldTxt, remove this 'else'
			result = true;
			break;
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
			catalog->SetFileName(X->CatIRec, "");
			if (!PromptYN(815)) {
				result = true;
				break;
			}
			CVol = catalog->GetVolume(X->CatIRec);
			CPath = FExpand(catalog->GetPathName(X->CatIRec));
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
		if (X->FTyp == FandFileType::INDEX) {
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

bool ChptDel(FileD* file_d, DataEditor* data_editor)
{
	RdbRecVars New;
	if (!IsCurrChpt(file_d)) {
		return true;
	}
	GetRdbRecVars(data_editor->GetEditD(), data_editor->GetRecord(), &New);
	return ChptDelFor(data_editor->GetEditD(), &New);
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
		uint8_t* record = CFile->GetRecSpace();
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
	if (Old.FTyp == FandFileType::INDEX) RenameFile56(Old.Name + CExt, New.Name + CExt, false);
}

WORD ChptWriteCRec(DataEditor* data_editor, EditD* edit)
{
	RdbRecVars New, Old;
	short eq;
	WORD result = 0;
	if (!IsCurrChpt(data_editor->GetFileD())) {
		return result;
	}
	if (!data_editor->TestIsNewRec()) {
		eq = CompArea(&(data_editor->GetRecord())[2], &(data_editor->GetOriginalRecord())[2], edit->FD->FF->RecLen - 2);
		if (eq == _equ) {
			return result;
		}
	}
	GetRdbRecVars(edit, data_editor->GetRecord(), &New);
	if (!data_editor->TestIsNewRec()) {
		GetRdbRecVars(edit, data_editor->GetOriginalRecord(), &Old);
	}
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
		if (!gc->IsIdentifStr(New.Name) || (New.Typ != 'F') && (New.Ext != "")) {
			WrLLF10Msg(138);
			return result;
		}
	if (New.Typ == 'F') {
		if (New.Name.length() > 8) {
			WrLLF10Msg(1002);
			return result;
		}
		if (!EquUpCase(New.Ext, ".DBF") && New.FTyp == FandFileType::UNKNOWN) {
			WrLLF10Msg(1067);
			return result;
		}
		if (IsDuplFileName(data_editor, New.Name)) {
			WrLLF10Msg(1068);
			return result;
		}
		if ((New.FTyp == FandFileType::RDB) && (New.Txt != 0)) {
			WrLLF10Msg(1083);
			return result;
		}
		if (NetFileTest(&New) && !data_editor->TestIsNewRec() &&
			(Old.Typ == 'F') && (eq != _equ) && !PromptYN(824)) {
			result = 2;
			return result;
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
		edit->FD->saveT(ChptOldTxt, 0, data_editor->GetRecord());
		if (New.Typ == 'F') {
			ReleaseFilesAndLinksAfterChapter(edit);
		}
		goto label2;
	}
	if (New.Typ == ' ' || New.Typ == 'I') goto label2;
	if (New.Typ != 'F') {
		if (New.Name != Old.Name) {
			if (New.Typ == 'E' || New.Typ == 'P') {
				ReleaseFilesAndLinksAfterChapter(edit);
				SetCompileAll();
			}
			else {
				ChptTF->CompileProc = true;
			}
		}
		if ((New.Typ == 'R') && (New.Txt == 0)) {
			ReleaseFilesAndLinksAfterChapter(edit);
		}
		goto label2;
	}
	ReleaseFilesAndLinksAfterChapter(edit);
	SetCompileAll();
	if ((New.OldTxt != 0) && (New.Name != Old.Name)) {
		if (Old.CatIRec != 0) {
			catalog->SetFileName(Old.CatIRec, New.Name);
		}
		else {
			if (!Old.isSQL) {
				RenameWithOldExt(New, Old);
			}
		}
	}
label2:
	edit->FD->saveB(ChptVerif, true, data_editor->GetRecord());
	result = 0;
	ChptTF->SetUpdateFlag(); //SetUpdHandle(ChptTF->Handle);
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
	NmF = CFile->FldD[0];
	TxtF = CFile->FldD[1];
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
		FD = catalog->GetCatalogFile();
		i = iCat;
		n = nCat;
	}
	if (kind != 2) {
		//EE = data_editor->WriteParamsToE();
	}

	std::unique_ptr<DataEditor> data_editor2 = std::make_unique<DataEditor>();
	std::unique_ptr<EditOpt> edit_opt2 = std::make_unique<EditOpt>();
	edit_opt2->UserSelFlds = true; // GetEditOpt();
	edit_opt2->Flds = gc->AllFldsList(FD, false);
	edit_opt2->WFlags = edit_opt2->WFlags | WPushPixel;
	if ((kind == 0) || (n != 0)) {
		iFrml.R = i;
		nFrml.R = n;
		edit_opt2->StartRecNoZ = (FrmlElem*)(&nFrml);
		edit_opt2->StartIRecZ = (FrmlElem*)(&iFrml);
	}
	data_editor2->EditDataFile(FD, edit_opt2.get());

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
		//data_editor->ReadParamsFromE(EE);
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

	const int pos = ChptTF->Store(text);

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
		ChptTxtPos = Chpt->FldD[0];
		ChptVerif = Chpt->FldD[1];
		ChptOldTxt = Chpt->FldD[2];
		ChptTyp = Chpt->FldD[3];
		ChptName = Chpt->FldD[4];
		ChptTxt = Chpt->FldD[5];
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
		file_d->CatIRec = catalog->GetCatalogIRec(*Nm, false);
		CRdb = r;
	}
	if (file_d->CatIRec != 0) {
		CPath = catalog->GetPathName(file_d->CatIRec);
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

	r->RdbDir = CDir;
	if (TopDataDir.empty()) r->DataDir = CDir;
	else if (rb == nullptr) r->DataDir = TopDataDir;
	else {
		d = rb->DataDir;
		AddBackSlash(d);
		r->DataDir = d + file_d->Name;
	}

	CDir = CDir + '\\';
}

// Reopen RDB file in Exclusive mode
void ResetRdOnly()
{
	if (Chpt->FF->UMode == RdOnly) {
		Chpt->CloseFile();
		IsInstallRun = true;
		Chpt->OpenF(CPath, Exclusive);
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
	gc->SetInpStr(s);
	gc->SetInpStr(s);

	if ((name[0] == '\\')) name1 = name.substr(1, 8);
	else name1 = name;

	rdb_d->v_files.clear();
	FileD* rdb_file = RdFileD(name1, DataFileType::FandFile, FandFileType::RDB, ""); /*old CRdb for GetCatalogIRec*/
	rdb_d->v_files.push_back(rdb_file);

	rdb_d->help_file = HelpFD; // Default help file is UFANDHLP or FANDHLP. It can be changed during compilation.

	return rdb_d;
}

void CreateOpenChpt(std::string Nm, bool create)
{
	std::string p;
	std::string s;
	short i = 0, n = 0;
	std::string nr;
	std::string Nm1;
	FileUseMode um = Closed;

	bool top = (CRdb == nullptr);
	//CRdb = new RdbD();
	//CRdb->v_files.clear();
	Chpt = nullptr;
	FandTFile* oldChptTF = ChptTF;
	RdbD* R = PrepareRdb(Nm, Nm1);
	CRdb = R;
	Chpt = CRdb->v_files[0];
	Chpt->FF->RecPtr = Chpt->GetRecSpace();

	SetRdbDir(Chpt, Nm[0], &Nm1);
	p = CDir + Nm1 + ".RDB";
	Chpt->FF->Drive = TestMountVol(CPath[0]);
	SetChptFldD();
	if (!spec.RDBcomment) ChptTxt->L = 1;
	SetMsgPar(p);
	if (top) {
		//UserName = "";
		//UserCode = 0;
		//AccRight[0] = 0;
		user->clear();
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

	if (Chpt->OpenF(CPath, um)) {
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
		Chpt->OpenCreateF(CPath, Exclusive);
		SetCompileAll();
	}

	CRdb->Encrypted = !Coding::HasPassword(Chpt, 1, "");
}

void CloseChpt()
{
	if (CRdb == nullptr) return;
	ClearHelpStkForCRdb();
	SaveFiles();
	bool del = Chpt->FF->NRecs == 0;
	std::string d = CRdb->RdbDir;
	FileD::CloseAllAfter(CRdb->v_files[0], CRdb->v_files);
	LinkDRoot = CRdb->OldLDRoot;
	FuncDRoot = CRdb->OldFCRoot;
	void* p = CRdb;
	//void* p2 = CRdb->Mark2;
	CRdb = CRdb->ChainBack;
	//ReleaseBoth(p, p2);
	if (CRdb != nullptr) {
		Chpt = CRdb->v_files[0];
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

FileD* FindFD(uint8_t* record)
{
	FileD* result = nullptr;
	std::string d;
	std::string name;
	std::string ext;
	std::string FName = OldTrailChar(' ', Chpt->loadS(ChptName, record));
	FSplit(FName, d, name, ext);

	//FileD* FD = FileDRoot;
	//while (FD != nullptr) {
	//	if (EquUpCase(FD->Name, name)) break;
	//	FD = (FileD*)FD->pChain;
	//}

	for (FileD* file : CRdb->v_files) {
		if (EquUpCase(file->Name, name)) {
			result = file;
			break;
		}
	}

	return result;
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

bool CompRunChptRec(const std::unique_ptr<DataEditor>& rdb_editor, WORD CC)
{
	void* p = nullptr; void* p2 = nullptr; void* MaxHp = nullptr;
	//EditD* OldE = nullptr;
	RdbPos RP;
	bool uw = false, mv = false;
	bool result = false;

	//OldE = edit;
	MarkBoth(p, p2);
	//EditD* EE = data_editor->WriteParamsToE();

	bool WasError = true;
	bool WasGraph = IsGraphMode;
	//FileD* lstFD = (FileD*)LastInChain(FileDRoot);
	int lstFDindex = CRdb->v_files.size() - 1;
	std::deque<LinkD*> oldLd = LinkDRoot;

	FileD* FD = nullptr;

	try {
		IsCompileErr = false;
		uw = false;
		mv = MausVisible;
		uint8_t* rdb_record = rdb_editor->GetRecord();
		const char STyp = rdb_editor->GetFileD()->loadS(ChptTyp, rdb_record)[0];
		RP.rdb = CRdb;
		RP.i_rec = rdb_editor->CRec();
#ifdef FandSQL
		nStrm = nStreams;
#endif
		if (CC == __ALT_F9) {
			if (gc->FindChpt('P', "MAIN", true, &RP)) {
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
			switch (STyp) {
			case 'F': {
				FD = FindFD(rdb_record);
				if (FD != nullptr && CC == __CTRL_F9) {
					std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>(FD);
					std::unique_ptr<EditOpt> edit_opt = std::make_unique<EditOpt>();
					edit_opt->UserSelFlds = true;
					CFile = FD;
					edit_opt->Flds = gc->AllFldsList(FD, false);
					if (data_editor->SelFldsForEO(edit_opt.get(), nullptr)) {
						data_editor->EditDataFile(FD, edit_opt.get());
					}
				}
				break;
			}
			case 'E': {
				if (CC == __CTRL_F9) {
					std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>();
					std::unique_ptr<EditOpt> edit_opt = std::make_unique<EditOpt>();
					edit_opt->UserSelFlds = true;
					edit_opt->FormPos = RP;
					data_editor->EditDataFile(nullptr, edit_opt.get());
				}
				else {
					//PushEdit();
					std::vector<FieldDescr*> unusedFD;
					EditReader* reader = new EditReader();
					reader->RdFormOrDesign(unusedFD, RP);
					//edit = reader->GetEditD();
				}
				break;
			}
			case 'M': {
				const std::unique_ptr merge = std::make_unique<Merge>();
				merge->SetInput(&RP, true);
				merge->Read();
				if (CC == __CTRL_F9) {
					merge->Run();
				}
				break;
			}
			case 'R': {
				const std::unique_ptr report = std::make_unique<Report>();
				report->SetInput(&RP, true);
				report->Read(nullptr);
				if (CC == __CTRL_F9) {
					report->Run(nullptr);
					SaveFiles();
					std::unique_ptr<TextEditor> editor = std::make_unique<TextEditor>(EditorMode::Unknown, TextType::Unknown);
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
					lstFDindex = CRdb->v_files.size() - 1;
					std::deque<LinkD*> ld = LinkDRoot;
					gc->SetInpTT(&RP, true);
					ReadProcHead(gc, "");
					ReadProcBody(gc);
					FileD::CloseAndRemoveAllAfter(lstFDindex + 1, CRdb->v_files);
					//lstFD->pChain = nullptr;
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
	int Free = MemoryAvailable();
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
	SaveFiles();
	if (mv) {
		ShowMouse();
	}
	if (WasError) {
		ForAllFDs(ForAllFilesOperation::clear_xf_update_lock);
	}

	//CFile = lstFD->pChain;
	//while (CFile != nullptr) {
	//	CFile->CloseFile();
	//	CFile = CFile->pChain;
	//}
	//lstFD->pChain = nullptr;

	FileD::CloseAndRemoveAllAfter(lstFDindex + 1, CRdb->v_files);

	LinkDRoot = oldLd;
	ReleaseStore(&p);
	ReleaseStore(&p2);
	//edit = OldE;
	//EditDRoot = E;
	//data_editor->ReadParamsFromE(EE);
	CRdb = RP.rdb;
	PrevCompInp.clear();

	rdb_editor->GetFileD()->ReadRec(rdb_editor->CRec(), rdb_editor->GetRecord());
	if (IsCompileErr) {
		result = false;
	}
	else {
		result = true;
		if (WasError) {
			return result;
		}
		rdb_editor->GetFileD()->saveB(ChptVerif, false, rdb_editor->GetRecord());
		rdb_editor->GetFileD()->WriteRec(rdb_editor->CRec(), rdb_editor->GetRecord());
		if (CC == __CTRL_F8) {
			Diagnostics(MaxHp, Free, FD);
		}
	}
	return result;
}

void RdUserId(bool check)
{
	std::string pw;
	std::set<uint16_t> acc;

	//ptrRdFldNameFrml = nullptr;
	gc->rdFldNameType = FieldNameType::none;
	gc->RdLex();
	if (gc->Lexem == 0x1A) return;
	if (check) {
		wwmix ww;
		pw = ww.PassWord(false);
	}
	//label1:
	while (true) {
		gc->TestLex(_quotedstr);
		std::string name = gc->LexWord;
		gc->RdLex();
		gc->Accept(',');
		uint16_t code = gc->RdInteger();
		gc->Accept(',');

		FrmlElem* Z = gc->RdStrFrml(nullptr);
		std::string pw2 = RunString(gc->processing_F, Z, nullptr);
		delete Z; Z = nullptr;

		if (gc->Lexem == ',') {
			gc->RdLex();
			acc = RdAccRights();
		}
		else {
			//acc[0] = 1;
			//acc[1] = (char)code;
			acc.clear();
			acc.insert(code);
		}

		if (check) {
			if (EquUpCase(pw, pw2)) {
				//UserName = name;
				//UserCode = code;
				//UserPassWORD = pw2;
				//AccRight = acc;
				user->set(name, code, pw2, acc);
				return;
			}
		}
		else if (code == 0) {
			//UserName = name;
			//UserCode = code;
			//UserPassWORD = pw2;
			user->set_acc_0(name, code, pw2);
		}
		if (gc->Lexem != 0x1A) {
			gc->Accept(';');
			if (gc->Lexem != 0x1A) {
				continue;
				// goto label1;
			}
		}
		break;
	}
	if (check) {
		RunError(629);
	}
}

WORD CompileMsgOn(CHAR_INFO* Buf, int& w)
{
	pstring s;
	WORD result;
	ReadMessage(15);

	if (IsTestRun) {
		w = PushWFramed(0, 0, 30, 4, screen.colors.sNorm, MsgLine, "", WHasFrame + WDoubleFrame + WShadow);
		ReadMessage(117);
		std::string s1 = MsgLine;
		s = GetNthLine(s1, 1, 1, '/');
		result = s.length();
		screen.ScrFormatWrText(3, 2, "%s", s.c_str());
		screen.ScrFormatWrText(3, 3, "%s", GetNthLine(s1, 2, 1, '/').c_str());
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


FileD* RdF(FileD* file_d, std::string FileName)
{
	std::string d, name, ext;
	FSplit(FileName, d, name, ext);

	FandFileType FDTyp = ExtToTyp(ext);
	if (FDTyp == FandFileType::RDB) {
		ReadMessage(51);
		std::string s = MsgLine;
		ReadMessage(49);
		short i = 0, n = 0;
		val(MsgLine, n, i);
		std::string nr = std::to_string(TxtCols - n);
		s = s + nr;
		gc->SetInpStr(s);
	}
	else {
		int pos = file_d->loadT(ChptTxt, file_d->FF->RecPtr);
		gc->SetInpTTPos(file_d, pos, CRdb->Encrypted);
	}

	if (EquUpCase(ext, ".DBF")) {
		return RdFileD(name, DataFileType::DBF, FDTyp, ext);
	} else {
		return RdFileD(name, DataFileType::FandFile, FDTyp, ext);
	}
	
}

FileD* RdOldF(FileD* file_d, const std::string& file_name)
{
	std::string d, name, ext;
	FSplit(file_name, d, name, ext);
	FandFileType FDTyp = ExtToTyp(ext);

	int pos = file_d->loadT(ChptOldTxt, file_d->FF->RecPtr);
	gc->SetInpTTPos(file_d, pos, CRdb->Encrypted);

	if (EquUpCase(ext, ".DBF")) {
		return RdFileD(name, DataFileType::DBF, FDTyp, ext);
	}
	else {
		return RdFileD(name, DataFileType::FandFile, FDTyp, ext);
	}
}

bool EquStoredF(std::vector<FieldDescr*>& fields1, std::vector<FieldDescr*>& fields2)
{
	std::vector<FieldDescr*>::iterator it1 = fields1.begin();
	std::vector<FieldDescr*>::iterator it2 = fields2.begin();

	while (true) {
		while (it1 != fields1.end() && !(*it1)->isStored()) {
			++it1;
		}
		// in it1 is first stored field now
		while (it2 != fields2.end() && !(*it2)->isStored()) {
			++it2;
		}
		// in F2 is first stored field now

		if (it1 == fields1.end()) {
			if (it2 != fields2.end()) {
				return false;
			}
			else {
				// both are nullptr
				return true;
			}
		}
		if (it2 == fields2.end()
			|| !gc->FldTypIdentity(*it1, *it2)
			|| ((*it1)->Flg & ~f_Mask) != ((*it2)->Flg & ~f_Mask)) {
			return false;
		}
		++it1;
		++it2;
	}
}

bool MergeAndReplace(FileD* fd_old, FileD* fd_new)
{
	bool result;

	try {
		std::string s = "#I1_";
		s += fd_old->Name + " #O1_@";

		SpecFDNameAllowed = true;
		const std::unique_ptr merge = std::make_unique<Merge>();
		merge->SetInput(s);
		merge->Read();
		SpecFDNameAllowed = false;
		merge->Run();

		SaveFiles();
		//CFile = fd_old;
		fd_old->DeleteF();
		//CFile = fd_new;
		fd_new->CloseFile();
		fd_old->FF->file_type = fd_new->FF->file_type;
		fd_new->SetPathAndVolume();
		std::string p = CPath;
		//CFile = fd_old;
		fd_old->SetPathAndVolume();
		RenameFile56(p, CPath, false);
		//CFile = fd_new;

		/*TF->Format used*/
		CPath = fd_new->CExtToT(CDir, CName, CExt);
		p = CPath;
		fd_new->SetPathAndVolume();
		CPath = fd_new->CExtToT(CDir, CName, CExt);
		RenameFile56(CPath, p, false);
		result = true;
	}
	catch (std::exception& e) {
		// TODO: log error
		//CFile = fd_old;
		fd_old->CloseFile();
		fd_old = fd_new;
		fd_old->DeleteF();
		SpecFDNameAllowed = false;
		result = false;
	}

	return result;
}

bool EquKeys(std::vector<XKey*>& K1, std::vector<XKey*>& K2)
{
	//auto result = false;
	//while (K1 != nullptr) {
	//	if ((K2 == nullptr) || (K1->Duplic != K2->Duplic)) return result;
	//	std::vector<KeyFldD*>::iterator KF1 = K1->KFlds.begin();
	//	std::vector<KeyFldD*>::iterator KF2 = K2->KFlds.begin();
	//	while (KF1 != K1->KFlds.end()) {
	//		if ((KF2 == K2->KFlds.end())
	//			|| ((*KF1)->CompLex != (*KF2)->CompLex)
	//			|| ((*KF1)->Descend != (*KF2)->Descend)
	//			|| ((*KF1)->FldD->Name != (*KF2)->FldD->Name)) 
	//		{
	//			return result;
	//		}
	//		++KF1;
	//		++KF2;
	//	}
	//	if (KF2 != K2->KFlds.end()) return result;
	//	K1 = K1->Chain;
	//	K2 = K2->Chain;
	//}
	//if (K2 != nullptr) return result;
	//result = true;
	//return result;
	if (K1.size() != K2.size()) {
		return false;
	}
	for (size_t i = 0; i < K1.size(); i++) {
		if (K1[i]->Duplic != K2[i]->Duplic) {
			return false;
		}
		if (K1[i]->KFlds.size() != K2[i]->KFlds.size()) {
			return false;
		}
		for (size_t j = 0; j < K1[i]->KFlds.size(); j++) {
			if (K1[i]->KFlds[j]->CompLex != K2[i]->KFlds[j]->CompLex
				|| K1[i]->KFlds[j]->Descend != K2[i]->KFlds[j]->Descend
				|| K1[i]->KFlds[j]->FldD->Name != K2[i]->KFlds[j]->FldD->Name)
			{
				return false;
			}
		}
	}
	return true;
}

bool MergeOldNew(FileD* new_file, FileD* old_file)
{
	//std::deque<LinkD*> ld = LinkDRoot;
	bool result = false;
	FileD* FDOld = old_file;
	FileD* FDNew = new_file;
	new_file->SetPathAndVolume();

	std::string Name = FDNew->Name;
	FDNew->Name = "@";
	//if (!RdFDSegment(0, Pos)) goto label1;
	//ChainLast(FileDRoot, Chpt);
	//FDOld = Chpt;
	FDOld->Name = Name;
	if ((FDNew->FF->file_type != FDOld->FF->file_type) || !EquStoredF(FDNew->FldD, FDOld->FldD)
#ifdef FandSQL
		&& !FDNew->IsSQLFile && !FDOld->IsSQLFile
#endif
		) {
		MergeAndReplace(FDOld, FDNew);
		result = true;
	}
	else if ((FDOld->FF->file_type == FandFileType::INDEX) && !EquKeys(FDOld->Keys, FDNew->Keys)) {
		Chpt->SetPathAndVolume();
		CPath = CExtToX(CDir, CName, CExt);
		MyDeleteFile(CPath);
	}
label1:
	//FDNew->pChain = nullptr;
	//LinkDRoot = ld;
	FDNew->Name = Name;
	FDNew->FullPath = CPath;
	CRecPtr = Chpt->FF->RecPtr;
	return result;
}

bool CompileRdb(FileD* rdb_file, bool displ, bool run, bool from_CtrlF10)
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
	size_t lstFDindex = 0;
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
		bool rdb_top = (CRdb->ChainBack == nullptr);
		if (rdb_top) {
			//UserName[0] = 0;
			//UserCode = 0;
			//UserPassWORD[0] = 0;
			//AccRight[0] = 0;
			user->clear();

			if (ChptTF->CompileAll || CompileFD) {
				Switches[0] = 0;
			}
		}
		lmsg = CompileMsgOn(Buf, w);
		//CRecPtr = v_files->FF->RecPtr;
		Encryp = CRdb->Encrypted;
		for (I = 1; I <= rdb_file->FF->NRecs; I++) {
			rdb_file->ReadRec(I, rdb_file->FF->RecPtr);
			RP.i_rec = I;
			Verif = rdb_file->loadB(ChptVerif, rdb_file->FF->RecPtr);
			STyp = rdb_file->loadS(ChptTyp, rdb_file->FF->RecPtr);
			Typ = STyp[0];
			Name = OldTrailChar(' ', rdb_file->loadS(ChptName, rdb_file->FF->RecPtr));
			Txt = rdb_file->loadT(ChptTxt, rdb_file->FF->RecPtr);
			if (Verif && ((ChptTF->LicenseNr != 0) || Encryp || (rdb_file->FF->UMode == RdOnly))) {
				// verify mode and encrypted or read only RDB
				gc->GoCompileErr(I, 647);
				throw std::exception("Not all chapters all compiled (encrypted or RdOnly).");
			}
			if (Verif														// verify mode
				|| ChptTF->CompileAll											// compile all flag
				|| from_CtrlF10													// Ctrl F10 - 'finish rdb'
				|| (Typ == 'U')													// User rights chapter
				|| (Typ == 'F' || Typ == 'D') && CompileFD						// chapter F or D and compile FD flag
				|| (Typ == 'P') && ChptTF->CompileProc) {						// chapter P and compile proc flag	
				//OldTxt = v_files->loadT(ChptOldTxt, v_files->FF->RecPtr);
				InpRdbPos = RP;
				if (IsTestRun) {
					ClrScr(TextAttr);
					screen.ScrFormatWrText(3 + lmsg, 2, "%*i", 4, I);
					screen.ScrFormatWrText(3 + lmsg, 3, "%*s%*s", 4, STyp.c_str(), 14, rdb_file->loadS(ChptName, rdb_file->FF->RecPtr).c_str());

					if (!(Typ == ' ' || Typ == 'D' || Typ == 'U')) { /* dupclicate name checking */
						for (J = 1; J <= I - 1; J++) {
							rdb_file->ReadRec(J, rdb_file->FF->RecPtr);
							if ((STyp == rdb_file->loadS(ChptTyp, rdb_file->FF->RecPtr))
								&& EquUpCase(Name, OldTrailChar(' ', rdb_file->loadS(ChptName, rdb_file->FF->RecPtr)))) {
								gc->GoCompileErr(I, 649);
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

							std::unique_ptr<DbfFile> dbf_file = std::make_unique<DbfFile>(nullptr);
							dbf_file->MakeDbfDcl(nm);

							Txt = rdb_file->loadT(ChptTxt, rdb_file->FF->RecPtr);
							rdb_file->WriteRec(I, rdb_file->FF->RecPtr);
						}
					}
#ifndef FandSQL
					if (EquUpCase(ext, ".SQL")) {
						gc->GoCompileErr(I, 654);
					}
#endif
					// get position of old chapter code
					int old_txt_pos = rdb_file->loadT(ChptOldTxt, rdb_file->FF->RecPtr);

					p1 = RdF(rdb_file, Name);
					CRdb->v_files.push_back(p1);
					if (p1->IsHlpFile) {
						CRdb->help_file = p1;
					}

					if (Verif || ChptTF->CompileAll || old_txt_pos == 0) {
						if (!Encryp) {
							// get last successfully compiled code
							std::string old_chapter_code = rdb_file->loadS(ChptOldTxt, rdb_file->FF->RecPtr);
							// get current chapter code
							std::string chapter_code = rdb_file->loadS(ChptTxt, rdb_file->FF->RecPtr);

							// compare old and new chapter code
							if (old_chapter_code != chapter_code) {
								if (!ChptTF->CompileAll && old_txt_pos != 0) {
									FileD* previous_decl = RdOldF(rdb_file, Name);
									// TODO: should this file be added into CRdb->v_files?

									// transform the file
									std::deque<LinkD*> ld_old = LinkDRoot;
									const bool merged = MergeOldNew(p1, previous_decl);
									LinkDRoot = ld_old;

									if (merged) {
										// copy new chapter code (ChptTxt) to old chapter code (ChptOldTxt)
										rdb_file->saveS(ChptOldTxt, chapter_code, rdb_file->FF->RecPtr);
										rdb_file->WriteRec(I, rdb_file->FF->RecPtr);
									}
									else {
										throw std::exception("Merge of an old and a new file declaration unsuccessful.");
									}
								}
								else {
									// copy new chapter code (ChptTxt) to old chapter code (ChptOldTxt)
									rdb_file->saveS(ChptOldTxt, chapter_code, rdb_file->FF->RecPtr);
									rdb_file->WriteRec(I, rdb_file->FF->RecPtr);
								}
							}
						}
					}
					else {
						// do nothing more
					}

					//else if (!RdFDSegment(I, OldTxt)) {
					//	LinkDRoot = ld;
					//	//ReleaseStore(&p1);
					//	//CFile = Chpt;
					//	goto label2;
					//}

					//else {
					//	ChainLast(FileDRoot, v_files);
					//	MarkStore(p1);
					//	if (v_files->IsHlpFile) {
					//		CRdb->help_file = v_files;
					//	}
					//}
					break;
				}
				case 'M': {
					const std::unique_ptr merge = std::make_unique<Merge>();
					merge->SetInput(rdb_file, Txt, Encryp);
					merge->Read();
					break;
				}
				case 'R': {
					if (Txt == 0 && IsTestRun) {
						const std::unique_ptr auto_report = std::make_unique<ReportGenerator>();
						RprtTxt = auto_report->SelGenRprt(Name);
						//CFile = Chpt;
						if (RprtTxt.empty()) {
							gc->GoCompileErr(I, 1145);
						}
						rdb_file->saveS(ChptTxt, RprtTxt, rdb_file->FF->RecPtr);
						rdb_file->WriteRec(I, rdb_file->FF->RecPtr);
					}
					else {
						const std::unique_ptr report = std::make_unique<Report>();
						report->SetInput(rdb_file, Txt, Encryp);
						report->Read(nullptr);
					}
					break;
				}
				case 'P': {
					//if (FileDRoot->pChain == nullptr) {
					//	lstFD = FileDRoot;
					//}
					//else {
					//	lstFD = (FileD*)LastInChain(FileDRoot);
					//}
					if (CRdb->v_files.empty()) {
						throw std::exception("FileDRoot is empty");
					}
					else {
						lstFDindex = CRdb->v_files.size() - 1;
					}
					std::deque<LinkD*> ld = LinkDRoot;
					gc->SetInpTTPos(rdb_file, Txt, Encryp);
					ReadProcHead(gc, Name);
					ReadProcBody(gc);
					//lstFD->pChain = nullptr;
					FileD::CloseAndRemoveAllAfter(lstFDindex + 1, CRdb->v_files);
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
					if (!rdb_top || (I > 1)) {
						gc->GoCompileErr(I, 623);
					}
					if (Txt != 0) {
						gc->ResetCompilePars();
						gc->SetInpTTPos(rdb_file, Txt, Encryp);
						RdUserId(!IsTestRun || (ChptTF->LicenseNr != 0));
						MarkStore(p1);
					}
					break;
				}
				case 'D': {
					gc->ResetCompilePars();
					gc->SetInpTTPos(rdb_file, Txt, Encryp);
					ReadDeclChpt(gc);
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
			//CRecPtr = v_files->FF->RecPtr;
			if (Verif) {
				rdb_file->ReadRec(I, rdb_file->FF->RecPtr);
				rdb_file->saveB(ChptVerif, false, rdb_file->FF->RecPtr);
				rdb_file->WriteRec(I, rdb_file->FF->RecPtr);
			}
		}
		if (ChptTF->CompileAll || ChptTF->CompileProc) {
			ChptTF->CompileAll = false;
			ChptTF->CompileProc = false;
			ChptTF->SetUpdateFlag(); //SetUpdHandle(ChptTF->Handle);
		}
		CompileFD = false;
		result = true;
		//if (!run) {
		//	Chpt->ReadRec(CRec(), edit->NewRecPtr);
		//}
		CompileMsgOff(Buf, w);
#ifdef FandSQL
		if (top && (Strm1 != nullptr)) Strm1->Login(UserName, UserPassWORD);
#endif
		log->log(loglevel::DEBUG, "finish CompileRdb()");

		delete edit; edit = nullptr;
		delete reader; reader = nullptr;
	}
	catch (std::exception& e) {
		log->log(loglevel::EXCEPTION, "CompileRdb() exception: ", e.what());
		result = false;
		CompileMsgOff(Buf, w);
		ReleaseFilesAndLinksAfterChapter(edit);
		PrevCompInp.clear();
		//ReleaseBoth(p, p2);
		//E = OldE; EditDRoot = E;
		//CFile = Chpt;
		//if (!run) CRecPtr = edit->NewRecPtr;
		if (!IsCompileErr) {
			InpRdbPos.i_rec = I;
		}
		delete edit; edit = nullptr;
		delete reader; reader = nullptr;
	}

	return result;
}

void GotoErrPos(WORD& Brk, std::unique_ptr<DataEditor>& data_editor)
{
	IsCompileErr = false;
	std::string s = MsgLine;
	if (InpRdbPos.rdb != CRdb) {
		data_editor->DisplEditWw();
		SetMsgPar(s);
		WrLLF10Msg(110);
		if (InpRdbPos.i_rec == 0) {
			SetMsgPar("");
		}
		else {
			SetMsgPar(InpRdbPos.rdb->v_files[0]->Name);
		}
		WrLLF10Msg(622);
		Brk = 0;
		return;
	}
	if (gc->input_pos == 0) {
		data_editor->DisplEditWw();
		data_editor->GotoNextRecFld(InpRdbPos.i_rec, data_editor->GetEditD()->FirstFld.begin());
		SetMsgPar(s);
		WrLLF10Msg(110);
		Brk = 0;
		return;
	}
	data_editor->CFld = data_editor->GetEditD()->GetEFldIter(data_editor->GetEditD()->LastFld);
	data_editor->SetNewCRec(InpRdbPos.i_rec, true);
	data_editor->GetFileD()->saveR(ChptTxtPos, gc->input_pos, data_editor->GetRecord());
	data_editor->GetFileD()->WriteRec(data_editor->CRec(), data_editor->GetRecord());
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

void Finish_EditExecRdb(bool wasGraph, int w)
{
	if (!wasGraph && IsGraphMode) {
		// ScrTextMode(false, false);
		throw std::exception("CompRunChptRec() Graph <-> Text Mode switching not implemented.");
	}
	if (UserW != 0) {
		PopW(UserW);
	}
	UserW = w;
	RunMsgClear();
	CloseChpt();
#ifdef FandSQL
	if (top) SQLDisconnect;
#endif
}

bool EditExecRdb(const std::string& name, const std::string& proc_name, Instr_proc* proc_call, wwmix* ww)
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "starting EditExecRdb()");
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
		CreateOpenChpt(name, true);
		CompileFD = true;
#ifndef FandRunV
		if (!IsTestRun || (ChptTF->LicenseNr != 0) || !top && CRdb->Encrypted) {
#endif
			MarkStore(p);
			EditRdbMode = false;
			bool hasToCompileRdb = CompileRdb(Chpt, false, true, false);
			if (hasToCompileRdb) {
				bool procedureFound = gc->FindChpt('P', proc_name, true, &RP);
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
						Finish_EditExecRdb(wasGraph, w);
						return result;
					}
					catch (std::exception& e) {
						if (IsCompileErr) {
							WrErrMsg630(name);
						}
						Finish_EditExecRdb(wasGraph, w);
						return result;
					}
				}
				else {
					SetMsgPar(name, proc_name);
					WrLLF10Msg(632);
				}
			}
			else if (IsCompileErr) {
				WrErrMsg630(name);
			}
#ifndef FandRunV
			if ((ChptTF->LicenseNr != 0) || CRdb->Encrypted || (Chpt->FF->UMode == RdOnly)) {
				Finish_EditExecRdb(wasGraph, w);
				return result;
			}
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
		EO->UserSelFlds = true; //options = GetEditOpt();
		EO->Flds = gc->AllFldsList(Chpt, true);
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
				Finish_EditExecRdb(wasGraph, w);
				return result;
			}
		}
		std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>(edit);
		if (!data_editor->OpenEditWw()) {
			//data_editor->PopEdit();
			Finish_EditExecRdb(wasGraph, w);
			return result;
		}
		result = true;
		Chpt->FF->WasRdOnly = false;

		bool skip_editor = false; // skip editor and wait for next key event in the 'while' loop

		if (!top && (Chpt->FF->NRecs > 0))
			if (CompileRdb(Chpt, true, false, false)) {
				if (gc->FindChpt('P', proc_name, true, &RP)) {
					data_editor->GotoRecFld(RP.i_rec, data_editor->CFld);
				}
			}
			else {
				GotoErrPos(Brk, data_editor);
				if (Brk == 0) {
					data_editor->RunEdit(nullptr, Brk);
				}
				skip_editor = true;
			}
		else if (ChptTF->IRec <= Chpt->FF->NRecs) {
			data_editor->GotoRecFld(ChptTF->IRec, data_editor->CFld);
		}

		if (!skip_editor) {
			data_editor->RunEdit(nullptr, Brk);
		}

		while (true) {
			// TODO: je to potreba?
			cc = Event.Pressed.KeyCombination();
			SaveFiles();
			if ((cc == __CTRL_F10) || ChptTF->CompileAll || CompileFD) {
				ReleaseFilesAndLinksAfterChapter(edit);
				data_editor->SetSelectFalse();
				edit->Bool = nullptr;
				ReleaseStore(&edit->AfterE);
			}
			if (cc == __CTRL_F10) {
				ChptTF->SetUpdateFlag(); //SetUpdHandle(ChptTF->Handle);
				if (!CompileRdb(Chpt, true, false, true)) {
					if (IsCompileErr) {
						GotoErrPos(Brk, data_editor);
						if (Brk == 0) {
							data_editor->RunEdit(nullptr, Brk);
						}
						continue;
					}
					if (Brk == 1) {
						data_editor->DisplEditWw();
					}
					data_editor->GotoNextRecFld(InpRdbPos.i_rec, edit->FirstFld.begin());
					data_editor->RunEdit(nullptr, Brk);
					continue;
				}
				if (!PromptCodeRdb(edit)) {
					data_editor->DisplEditWw();
					data_editor->RunEdit(nullptr, Brk);
					continue;
				}
				Chpt->FF->WasRdOnly = true;
				//data_editor->PopEdit();
				Finish_EditExecRdb(wasGraph, w);
				return result;
			}
			if (Brk != 0) {
				if (!CompileRdb(Chpt, Brk == 2, false, false)) {
					if (IsCompileErr) {
						GotoErrPos(Brk, data_editor);
						if (Brk == 0) {
							data_editor->RunEdit(nullptr, Brk);
						}
						continue;
					}
					if (Brk == 1) data_editor->DisplEditWw();
					data_editor->GotoNextRecFld(InpRdbPos.i_rec, edit->FirstFld.begin());
					data_editor->RunEdit(nullptr, Brk);
					continue;
				}
				bool comp_run_chapter = true;
				if (cc == __ALT_F2) {
					EditHelpOrCat(cc, 0, "");
					comp_run_chapter = false;
					//goto label41;
				}
				if (comp_run_chapter) {
					if (!CompRunChptRec(data_editor, cc)) {
						GotoErrPos(Brk, data_editor);
						if (Brk == 0) {
							data_editor->RunEdit(nullptr, Brk);
						}
						continue;
					}
				}
				//label41:
				if (Brk == 1) {
					data_editor->EditFreeTxt(ChptTxt, "", true, Brk);
					if (Brk == 0) {
						data_editor->RunEdit(nullptr, Brk);
					}
					continue;
				}
				else {
					data_editor->DisplEditWw();
					data_editor->RunEdit(nullptr, Brk);
					continue;
				}

			}
			break;
		} // while

		ChptTF->IRec = data_editor->CRec();
		ChptTF->SetUpdateFlag(); //SetUpdHandle(ChptTF->Handle);

		//printf("");
		//data_editor->PopEdit();
#endif
	}
	catch (std::exception& e)
	{
		log->log(loglevel::EXCEPTION, "EditExecRdb() exception: ", e.what());
	}

	Finish_EditExecRdb(wasGraph, w);
	return result;
}

void UpdateCat()
{
	FileD* cat = catalog->GetCatalogFile();
	if (cat->FF->Handle == nullptr) {
		cat->OpenCreateF(CPath, Exclusive);
	}
	EditOpt* EO = new EditOpt();
	EO->UserSelFlds = true;
	EO->Flds = gc->AllFldsList(cat, true);

	std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>();
	data_editor->EditDataFile(cat, EO);

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
	size_t TxtPos = 1;
	TextAttr = screen.colors.tNorm;
	int OldPos = CFile->loadT(ChptTxt, CRecPtr);
	std::string s = CFile->loadS(ChptTxt, CRecPtr);

	if (CRdb->Encrypted) {
		s = Coding::CodingString(CFile, s);
	}

	gc->SetInpStdStr(s, false);
	MarkStore(p);
	RdUserId(false);
	ReleaseStore(&p);
	bool b = true;

	while (true) {
		try {
			std::unique_ptr<TextEditor> editor = std::make_unique<TextEditor>(EditorMode::Text, TextType::Unknown);
			editor->SimpleEditText(EditorMode::Text, "", "", s, 0x7FFF, TxtPos, Upd);
			gc->SetInpStdStr(s, false);
			MarkStore(p);
			RdUserId(false);
			ReleaseStore(&p);
			b = false;
			if (Upd) {
				StoreChptTxt(ChptTxt, s, true);
				CFile->WriteRec(1, CRecPtr);
			}
			break;
		}
		catch (std::exception& ex) {
			if (b) {
				WrLLF10MsgLine(MsgLine);
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
		ChptTF->SetUpdateFlag(); //SetUpdHandle(ChptTF->Handle);
	}
}
