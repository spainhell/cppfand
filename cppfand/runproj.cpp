#include "access.h"
#include "ChkD.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "legacy.h"
#include "../Logging/Logging.h"
#include "oaccess.h"
#include "obaseww.h"
#include "rdfildcl.h"
#include "rdproc.h"
#include "rdrun.h"
#include "runproc.h"
#include "runproj.h"
#include "../Prolog/RunProlog.h"
#include "TFile.h"
#include "wwmenu.h"
#include "XFile.h"
#include <map>


#include "compile.h"
#include "../Editor/OldEditor.h"
#include "../Editor/EditorHelp.h"
#include "../Editor/rdedit.h"
#include "../Editor/runedi.h"
#include "../ExportImport/ExportImport.h"
#include "../MergeReport/genrprt.h"
#include "../MergeReport/rdmerg.h"
#include "../MergeReport/rdrprt.h"
#include "../MergeReport/runmerg.h"
#include "../MergeReport/runrprt.h"
#include "../textfunc/textfunc.h"


void* O(void* p) // ASM
{
	return p;
}

void* OCF(void* p) // ASM
{
	return p;
}

longint UserW = 0;

struct RdbRecVars
{
	char Typ = 0;
	std::string Name;
	std::string Ext;
	longint Txt = 0; longint OldTxt = 0;
	char FTyp = 0; WORD CatIRec = 0; bool isSQL = false;
};

FileD* CFileF = nullptr;
longint sz = 0; WORD nTb = 0; void* Tb = nullptr;

bool IsCurrChpt()
{
	return CRdb->FD == CFile;
}

char ExtToTyp(pstring Ext)
{
	if ((Ext == "") || SEquUpcase(Ext, ".HLP")
#ifdef FandSQL
		|| SEquUpcase(Ext, ".SQL")
#endif	
		)
		return '6';
	else if (SEquUpcase(Ext, ".X")) return 'X';
	else if (SEquUpcase(Ext, ".DTA")) return '8';
	else if (SEquUpcase(Ext, ".DBF")) return 'D';
	else if (SEquUpcase(Ext, ".RDB")) return '0';
	else return '?';
}

void ReleaseFDLDAfterChpt()
{
	FileD* FD = nullptr;
	RdbD* R = nullptr;

	if (Chpt->Chain != nullptr) CloseFAfter((FileD*)Chpt->Chain);
	Chpt->Chain = nullptr;
	LinkDRoot = CRdb->OldLDRoot;
	FuncDRoot = CRdb->OldFCRoot;
	CFile = Chpt;
	CRecPtr = E->NewRecPtr;
	R = CRdb->ChainBack;
	if (R != nullptr) CRdb->HelpFD = R->HelpFD;
	else CRdb->HelpFD = nullptr;
	CompileFD = true;
}

bool NetFileTest(RdbRecVars* X)
{
	if ((X->Typ != 'F') || (X->CatIRec == 0) || X->isSQL) return false;
	RdCatPathVol(X->CatIRec);
	if (IsNetCVol()) return true;
	return false;
}

void GetSplitChptName(std::string& Name, std::string& Ext)
{
	Ext = "";
	std::string chptName = _StdS(ChptName);
	Name = TrailChar(chptName, ' ');
	size_t i = Name.find('.');
	if (i == std::string::npos) return;
	Ext = Name.substr(i, 255);
	Name = Name.substr(1, i - 1);
}

void GetRdbRecVars(void* RecPtr, RdbRecVars* X)
{
	void* p = nullptr; void* p2 = nullptr; void* cr = nullptr;
	LinkDPtr ld = nullptr;

	cr = CRecPtr;
	CRecPtr = RecPtr;
	std::string s1 = _StdS(ChptTyp);
	X->Typ = s1[0];
	GetSplitChptName(X->Name, X->Ext);
	X->Txt = _T(ChptTxt);
	X->OldTxt = _T(ChptOldTxt);
	if (X->Typ == 'F') {
		X->FTyp = ExtToTyp(X->Ext);
		X->CatIRec = GetCatIRec(X->Name, false);
		X->isSQL = false;
		if (X->OldTxt != 0) {
			ld = LinkDRoot;
			MarkBoth(p, p2);
			if (RdFDSegment(0, X->OldTxt)) {
				X->FTyp = CFile->Typ;
				if (CFile->IsSQLFile) X->Ext = ".SQL";
				else {
					switch (X->FTyp) {
					case '0': X->Ext = ".RDB"; break;
					case 'D': X->Ext = ".DBF"; break;
					case '8': X->Ext = ".DTA"; break;
					default: X->Ext = ".000"; break;
					}
				}
			}
			LinkDRoot = ld;
			CFile = Chpt;
			ReleaseBoth(p, p2);
		}
#ifdef FandSQL
		if (X->Ext == ".SQL") X->isSQL = true;
#endif
	}
	CRecPtr = cr;
}

bool ChptDelFor(RdbRecVars* X)
{
	bool result = true;
	SetUpdHandle(ChptTF->Handle);
	ReleaseFDLDAfterChpt();
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
			result = true; break; /*don't delete if the record is new*/
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
			WrCatField(X->CatIRec, CatFileName, "");
			if (!PromptYN(815)) {
				result = true;
				break;
			}
			RdCatPathVol(X->CatIRec);
			TestMountVol(CPath[1]);
		}
		else {
			CDir = "";
			CName = X->Name; CExt = X->Ext;
		}
		MyDeleteFile(CDir + CName + CExt);
		CExtToT();
		MyDeleteFile(CPath);
		if (X->FTyp == 'X') {
			CExtToX();
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

bool ChptDel()
{
	RdbRecVars New;
	if (!IsCurrChpt()) { return true; }
	GetRdbRecVars(E->NewRecPtr, &New);
	return ChptDelFor(&New);
}

bool IsDuplFileName(std::string name)
{
	std::string n; std::string e; void* cr;
	auto result = true;
	if (SEquUpcase(name, Chpt->Name)) return result;
	cr = CRecPtr;
	CRecPtr = GetRecSpace();
	for (WORD I = 1; I <= Chpt->NRecs; I++)
		if (I != CRec()) {
			ReadRec(CFile, I, CRecPtr);
			if (_ShortS(ChptTyp) == 'F') {
				GetSplitChptName(n, e);
				if (SEquUpcase(name, n)) goto label1;
			}
		}
	result = false;
label1:
	ReleaseStore(CRecPtr);
	CRecPtr = cr;
	return result;
}

void RenameWithOldExt(RdbRecVars New, RdbRecVars Old)
{
	CExt = Old.Ext;
	RenameFile56(Old.Name + CExt, New.Name + CExt, false);
	CExtToT();
	RenameFile56(Old.Name + CExt, New.Name + CExt, false);
	CExtToX();
	if (Old.FTyp == 'X') RenameFile56(Old.Name + CExt, New.Name + CExt, false);
}

WORD ChptWriteCRec()
{
	RdbRecVars New, Old;
	integer eq;
	WORD result = 0;
	if (!IsCurrChpt()) return result;
	if (!TestIsNewRec()) {
		eq = CompArea((char*)((uintptr_t)CRecPtr + 2), (char*)((uintptr_t)E->OldRecPtr + 2), CFile->RecLen - 2);
		if (eq == _equ) return result;
	}
	GetRdbRecVars(E->NewRecPtr, &New);
	if (!TestIsNewRec()) GetRdbRecVars(E->OldRecPtr, &Old);
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
		if (!IsIdentifStr(New.Name) || (New.Typ != 'F') && (New.Ext != "")) {
			WrLLF10Msg(138); return result;
		}
	if (New.Typ == 'F') {
		if (New.Name.length() > 8) { WrLLF10Msg(1002); return result;; }
		if (New.FTyp == '?') { WrLLF10Msg(1067); return result;; }
		if (IsDuplFileName(New.Name)) { WrLLF10Msg(1068); return result;; }
		if ((New.FTyp == '0') && (New.Txt != 0)) { WrLLF10Msg(1083); return result;; }
		if (NetFileTest(&New) && !TestIsNewRec() &&
			(Old.Typ == 'F') && (eq != _equ) && !PromptYN(824)) {
			result = 2; return result;
		}
	}
	if ((New.Typ == 'D' || New.Typ == 'I' || New.Typ == 'U')
		|| !TestIsNewRec()
		&& (Old.Typ == 'D' || Old.Typ == 'I' || Old.Typ == 'U')) {
		ReleaseFDLDAfterChpt(); SetCompileAll();
	}
	if (TestIsNewRec()) { ReleaseFDLDAfterChpt(); goto label2; }
	if (New.Typ != Old.Typ) {
	label1:
		if (!ChptDelFor(&Old)) return result; T_(ChptOldTxt, 0);
		if (New.Typ == 'F') ReleaseFDLDAfterChpt();
		goto label2;
	}
	if (New.Typ == ' ' || New.Typ == 'I') goto label2;
	if (New.Typ != 'F') {
		if (New.Name != Old.Name)
			if (New.Typ == 'E' || New.Typ == 'P') { ReleaseFDLDAfterChpt(); SetCompileAll(); }
			else ChptTF->CompileProc = true;
		if ((New.Typ == 'R') && (New.Txt == 0)) ReleaseFDLDAfterChpt();
		goto label2;
	}
	ReleaseFDLDAfterChpt(); SetCompileAll();
	if ((New.OldTxt != 0) && (New.Name != Old.Name)) {
		if (Old.CatIRec != 0) { WrCatField(Old.CatIRec, CatFileName, New.Name); }
		else { if (!Old.isSQL) RenameWithOldExt(New, Old); }
	}
label2:
	B_(ChptVerif, true); result = 0;
	SetUpdHandle(ChptTF->Handle);
	return result;
}

void OKF(KeyFldDPtr kf)
{
	while (kf->Chain != nullptr) {
		kf->Chain = (KeyFldD*)O(kf->Chain);
		kf = (KeyFldD*)kf->Chain;
		kf->FldD = (FieldDescr*)O(kf->FldD);
	}
}

void* OTb(pstring Nm)
{
	return nullptr;

	//	pstring* s = nullptr;
	//	WORD* sofs = (WORD*)s;
	//	WORD i;
	//	//s = (pstring*)Tb;
	//	for (i = 1; i <= nTb; i++) {
	//		if (SEquUpcase(*s, Nm)) goto label1;
	//		sofs += s->length() + 1;
	//	}
	//	nTb++; sz += Nm.length() + 1;
	//	if (sz > MaxLStrLen) RunError(664);
	//	s = StoreStr(Nm);
	//label1:
	//	return O(s);
}

void* OLinkD(LinkD* Ld)
{
	// TODO: spoèítat, co se má vrátit:
	// return ptr(WORD(OTb(Ld->FromFD->Name)), WORD(OTb(Ld->RoleName)));
	return nullptr;
}

void OFrml(FrmlPtr Z)
{
	//FileDPtr cf = nullptr; FileDPtr fd1 = nullptr;
	//FrmlList fl = nullptr; LinkDPtr ld = nullptr;
	//if (Z != nullptr) {
	//	Z = (FrmlElem*)O(Z);
	//	/* !!! with Z^ do!!! */
	//	switch (Z->Op) {
	//	case _field: { Z->Field = (FieldDescr*)OCF(Z->Field); break; }
	//	case _access: {
	//		Z->LD = (LinkD*)OCF(Z->LD);
	//		cf = CFileF;
	//		CFileF = Z->File2;
	//		Z->File2 = (FileD*)OTb(CFileF->Name);
	//		OFrml(Z->P1);
	//		CFileF = cf;
	//		break;
	//	}
	//	case _userfunc: {
	//		Z->FC = (FuncD*)OTb(Z->FC->Name);
	//		fl = FrmlList(&Z->FrmlL);
	//		while (fl->Chain != nullptr) {
	//			fl->Chain = (FrmlListEl*)O(fl->Chain);
	//			fl = (FrmlListEl*)fl->Chain;
	//			OFrml(fl->Frml);
	//		}
	//		break;
	//	}
	//	case _owned: {
	//		ld = Z->ownLD;
	//		fd1 = ld->FromFD;
	//		Z->ownLD = (LinkD*)OLinkD(ld);
	//		cf = CFileF;
	//		CFileF = fd1;
	//		OFrml(Z->ownBool);
	//		OFrml(Z->ownSum);
	//		CFileF = cf;
	//	}
	//	default: {
	//		if (Z->Op >= 0x60 && Z->Op <= 0xaf) { OFrml(Z->P1); break; }
	//		if (Z->Op >= 0xb0 && Z->Op <= 0xef) { OFrml(Z->P1); OFrml(Z->P2); break; }
	//		if (Z->Op >= 0xf0 && Z->Op <= 0xff) { OFrml(Z->P1); OFrml(Z->P2); OFrml(Z->P3); break; }
	//	}
	//	}
	//}
}

//FileD* GetFD(void* p, bool WithSelf, WORD Sg)
//{
//	if (p != nullptr) {
//		LexWord = *(pstring*)(p);
//		std::string lw = LexWord;
//		if (WithSelf && EquUpcase(CFile->Name, lw)) p = CFile;
//		else p = FindFileD();
//	}
//	return (FileD*)p;
//}

//FuncD* GetFC(void* p, WORD Sg)
//{
//	FuncD* fc;
//	LexWord = *(pstring*)(p);
//	fc = FuncDRoot; while (fc != nullptr) {
//		if (EquUpcase(fc->Name, LexWord)) goto label1;
//		fc = fc->Chain;
//	}
//label1:
//	return fc;
//}

//LinkD* GetLinkD(void* P, WORD Sg)
//{
//	LinkD* ld; FileD* fd;
//	LexWord = *(pstring*)(P);
//	fd = (FileD*)FindFileD;
//	ld = LinkDRoot;
//
//	while (ld != nullptr) {
//		if ((ld->FromFD == fd) && (ld->RoleName == *((pstring*)(WORD(P)))))
//		{
//			return ld;
//		} ld = ld->Chain;
//	}
//	return nullptr;
//}

void SgFrml(FrmlElem* Z, WORD Sg, WORD SgF)
{
	//FrmlList fl = nullptr;
	//WORD SgFold = 0;
	//if (Z != nullptr) {
	//	Z = (FrmlElem*)(Sg);
	//	/* !!! with Z^ do!!! */
	//	switch (Z->Op) {
	//	case _field: Z->Field = (FieldDescr*)SgF; break;
	//	case _access: {
	//		if (Z->LD != nullptr) Z->LD = (LinkD*)SgF;
	//		Z->File2 = GetFD(Z->File2, Z->LD != nullptr, Sg);
	//		SgFold = SgF;
	//		SgF = *(WORD*)Z->File2;
	//		SgFrml(Z->P1, Sg, SgF);
	//		SgF = SgFold;
	//		break; }
	//	case _userfunc: {
	//		Z->FC = GetFC(Z->FC, Sg);
	//		fl = (FrmlListEl*)Z->FrmlL;
	//		while (fl->Chain != nullptr) {
	//			fl->Chain = (FrmlListEl*)Sg;
	//			fl = (FrmlListEl*)fl->Chain;
	//			SgFrml(fl->Frml, Sg, SgF);
	//		}
	//		break;
	//	}
	//	case _owned: {
	//		Z->ownLD = GetLinkD(Z->ownLD, Sg);
	//		SgFold = SgF;
	//		//SgF = (Z->ownLD->FromFD);
	//		SgFrml(Z->ownBool, Sg, SgF);
	//		SgFrml(Z->ownSum, Sg, SgF);
	//		SgF = SgFold;
	//		break; }
	//	default: {
	//		if (Z->Op >= 0x60 && Z->Op <= 0xaf) { SgFrml(Z->P1, Sg, SgF); break; }
	//		if (Z->Op >= 0xb0 && Z->Op <= 0xef) { SgFrml(Z->P1, Sg, SgF); SgFrml(Z->P2, Sg, SgF); break; }
	//		if (Z->Op >= 0xf0 && Z->Op <= 0xff) { SgFrml(Z->P1, Sg, SgF); SgFrml(Z->P2, Sg, SgF); SgFrml(Z->P3, Sg, SgF); break; }
	//	}
	//	}
	//}
}

void WrFDSegment(longint RecNr)
{
	FileD* cf = nullptr; StringList s; FieldDescr* f = nullptr; XKey* k = nullptr;
	AddD* ad = nullptr; ChkD* c = nullptr; LinkD* ld = nullptr; WORD n = 0, oldsz = 0;
	void* fdsaved = nullptr; void* p2 = nullptr; LongStr* ss = nullptr;
	ImplD* id = nullptr; LiRoots* li = nullptr;

	//sz = AbsAdr(HeapPtr) - AbsAdr(CFile);
	if (sz > MaxLStrLen) RunError(664);
	oldsz = sz; nTb = 0; //Tb = O(HeapPtr);
	MarkStore2(p2);
	fdsaved = GetStore2(sz);
	Move(CFile, fdsaved, sz);

	FileD savedFD = *CFile; // toto snad nahradí "požadovanou" zálohu ...

	CFileF = CFile;
	/* !!! with CFile^ do!!! */
	CFile->TF = (TFile*)O(CFile->TF);
	CFile->XF = (XFile*)O(CFile->XF);
	if (CFile->OrigFD != nullptr) CFile->OrigFD = (FileD*)OTb(CFile->OrigFD->Name);
	s = CFile->ViewNames;
	if (s != nullptr) {
		while (s->Chain != nullptr) {
			s->Chain = (StringListEl*)O(s->Chain);
			s = (StringList)s->Chain;
		}
	}
	f = CFile->FldD.front();
	if (f != nullptr) {
		while (f->Chain != nullptr) {
			f->Chain = (FieldDescr*)O(f->Chain);
			f = (FieldDescr*)f->Chain;
			if ((f->Flg & f_Stored) == 0) OFrml(f->Frml);
		}
	}
	k = CFile->Keys;
	if (k != nullptr) {
		while (k->Chain != nullptr) {
			k->Chain = (XKey*)O(k->Chain);
			k = k->Chain;
			//k->Alias = (pstring*)O(k->Alias);
			OKF((KeyFldD*)(&k->KFlds));
		}
	}
	//ad = CFile->Add;

	//while (ad->Chain != nullptr) {
	//	ad->Chain = (AddD*)O(ad->Chain);
	//	ad = ad->Chain;
	//	ad->LD = (LinkD*)O(ad->LD);
	//	OFrml(ad->Frml);
	//	if (ad->Assign) OFrml(ad->Bool);
	//	else {
	//		c = ad->Chk;
	//		if (c != nullptr) {
	//			ad->Chk = (ChkD*)O(c);
	//			//c->HelpName = (pstring*)O(c->HelpName);
	//		}
	//	}
	//	cf = CFileF;
	//	CFileF = ad->File2;
	//	ad->File2 = (FileD*)OTb(CFileF->Name);
	//	ad->Field = (FieldDescr*)OCF(ad->Field);
	//	if (!ad->Assign && (c != nullptr)) { OFrml(c->Bool); OFrml(c->TxtZ); }
	//	CFileF = cf;
	//}

	ld = (LinkD*)O(LinkDRoot);
	n = CFile->nLDs;
	while (n > 0) {
		OKF(KeyFldDPtr(ld->Args));
		cf = CFileF;
		CFileF = ld->ToFD;
		ld->ToFD = (FileD*)OTb(CFileF->Name);
		ld->ToKey = (XKey*)OCF(ld->ToKey);
		CFileF = cf; n--;
		if (n > 0) { ld->Chain = (LinkD*)O(ld->Chain); ld = ld->Chain; }
	}
	CFile->Chain = (FileD*)O(LinkDRoot);
	CFile->IRec = FDVersion;
	CFile->Handle = (FILE*)Tb;

	/*if (CFile->LiOfs > 0) {
		li = (LiRoots*)Normalize(AbsAdr(CFile) + CFile->LiOfs);
		id = ImplDPtr(&li->Impls);
		if (id != nullptr) {
			while (id->Chain != nullptr) {
				id->Chain = (ImplD*)O(id->Chain);
				id = (ImplD*)id->Chain;
				id->FldD = (FieldDescr*)O(id->FldD);
				OFrml(id->Frml);
			}
		}
		c = ChkDPtr(&li->Chks);
		if (c != nullptr) {
			while (c->Chain != nullptr) {
				c->Chain = (ChkD*)O(c->Chain);
				c = (ChkD*)c->Chain;
				c->HelpName = (pstring*)O(c->HelpName);
				OFrml(c->TxtZ);
				OFrml(c->Bool);
			}
		}
	}*/
	ss = (LongStr*)&CFile->WasWrRec; // Ptr(PtrRec(CFile).Seg - 1, 14);
	ss->LL = sz;
	cf = CFile; CFile = Chpt;
	StoreChptTxt(ChptOldTxt, ss, false);
	WriteRec(CFile, RecNr, CRecPtr);
	CFile = cf;
	Move(fdsaved, CFile, oldsz);
	ReleaseStore2(p2);
}

void SgKF(KeyFldD* kf, WORD Sg)
{
	if (kf == nullptr) return;
	while (kf->Chain != nullptr) {
		// PtrRec(kf->Chain).Seg = Sg; 
		// kf = kf->Chain; 
		// PtrRec(kf->FldD).Seg = Sg;
	}
}

// preklopi data ze souboru (TTT, ...) do jednotlivych promennych objektu FileD
FileD* FileD_FromSegment(LongStr* ss) {
	char* A = ss->A;
	size_t index = 0;
	FileD* f = new FileD;
	f->Chain = (Chained*)(uintptr_t)(&A[index]); index += 4;
	f->RecLen = *(WORD*)&A[index]; index += 2;
	f->RecPtr = reinterpret_cast<FileD*>(*(unsigned int*)&A[index]); index += 4;
	f->NRecs = *(longint*)&A[index]; index += 4;
	f->WasWrRec = *(boolean*)&A[index]; index++;
	f->WasRdOnly = *(boolean*)&A[index]; index++;
	f->Eof = *(boolean*)&A[index]; index++;
	f->Typ = A[index]; index++;
	f->Handle = nullptr; index += 2; // V Pascalu byly jen 2B, tady je to ukazatel
	f->IRec = *(longint*)&A[index]; index += 4;
	f->FrstDispl = *(WORD*)&A[index]; index += 2;
	f->TF = reinterpret_cast<TFile*>(*(unsigned int*)&A[index]); index += 4;
	f->ChptPos.R = reinterpret_cast<RdbD*>(*(unsigned int*)&A[index]); index += 4;
	f->ChptPos.IRec = *(WORD*)&A[index]; index += 2;
	f->TxtPosUDLI = *(WORD*)&A[index]; index += 2;
	f->OrigFD = reinterpret_cast<FileD*>(*(unsigned int*)&A[index]); index += 4;
	f->Drive = *(BYTE*)&A[index]; index++;
	f->CatIRec = *(WORD*)&A[index]; index += 2;
	/*f->FldD = reinterpret_cast<FieldDescr*>(*(unsigned int*)&A[index]);*/ index += 4;
	f->IsParFile = *(boolean*)&A[index]; index++;
	f->IsJournal = *(boolean*)&A[index]; index++;
	f->IsHlpFile = *(boolean*)&A[index]; index++;
	f->typSQLFile = *(boolean*)&A[index]; index++;
	f->IsSQLFile = *(boolean*)&A[index]; index++;
	f->IsDynFile = *(boolean*)&A[index]; index++;
	BYTE mode = *(BYTE*)&A[index]; index++;
	f->UMode = (FileUseMode)mode;
	mode = *(BYTE*)&A[index]; index++;
	f->LMode = (LockMode)mode;
	mode = *(BYTE*)&A[index]; index++;
	f->ExLMode = (LockMode)mode;
	mode = *(BYTE*)&A[index]; index++;
	f->TaLMode = (LockMode)mode;
	f->ViewNames = reinterpret_cast<StringListEl*>(*(unsigned int*)&A[index]); index += 4;
	f->XF = reinterpret_cast<XFile*>(*(unsigned int*)&A[index]); index += 4;
	f->Keys = reinterpret_cast<XKey*>(*(unsigned int*)&A[index]); index += 4;
	//f->Add = reinterpret_cast<AddD*>(*(unsigned int*)&A[index]); index += 4;
	f->nLDs = *(WORD*)&A[index]; index += 2;
	f->LiOfs = *(WORD*)&A[index]; index += 2;
	f->Name[0] = *(BYTE*)&A[index]; index++;
	memcpy(&f->Name[1], &A[index], 8); index += 8;

	void* fff = &A[index];

	return f;
}

FrmlElem* createFrmlElemFromStr(BYTE* str, uintptr_t address, std::map<uintptr_t, FieldDescr*>* mFields)
{
	FrmlElem* frml = nullptr;
	WORD nextItemIndex = address & 0x0000FFFF;
	instr_type Op = (instr_type)str[nextItemIndex];
	//frml->Op = Op;
	switch (Op) {
	case _instr: {
		frml = new FrmlElemIn(_instr);
		break;
	}

	}


	if (Op == _field)
	{
		auto fDescAddress = *(uintptr_t*)&str[nextItemIndex + 1];
		auto it = mFields->find(fDescAddress);
		if (it == mFields->end()) throw std::exception("Field wasn't found.");
		frml = new FrmlElem7(Op, 0);
		((FrmlElem7*)frml)->Field = it->second;
	}
	else if (Op == _access)
	{
		//auto frml7 = new FrmlElem7(Op, 0);
		//if (frml7->LD != nullptr) frml7->LD = new LinkD();
		//frml7->File2 = GetFD(frml7->File2, frml7->File2 != nullptr, address); // tady bude adresa segmentu - tedy bez offsetu asi
		//return frml7;
		// nedopsáno
	}
	else if (Op == _userfunc)
	{
		//frml->FC = GetFC(frml->FC, address);
		//FrmlListEl* fl = frml->FrmlL;
		//while (fl->Chain != nullptr)
		//{
		//	fl->Chain = (FrmlListEl*)fl->Chain;
		//	fl->Frml = createFrmlElemFromStr(str, address, mFields); // taky nesmysl
		//}
	}
	else if (Op == _owned)
	{
		//frml->ownLD = GetLinkD(frml->ownLD, address);
		//// nedopsáno
	}
	else if (Op >= 0x60 && Op <= 0xAF)
	{
		//frml->P1 = createFrmlElemFromStr(str, *(uintptr_t*)&str[nextItemIndex + 1], mFields);
	}
	else if (Op >= 0xB0 && Op <= 0xEF)
	{
		/*frml->P1 = createFrmlElemFromStr(str, *(uintptr_t*)&str[nextItemIndex + 1], mFields);
		frml->P2 = createFrmlElemFromStr(str, *(uintptr_t*)&str[nextItemIndex + 5], mFields);*/
	}
	else if (Op >= 0xF0 && Op <= 0xFF)
	{
		/*frml->P1 = createFrmlElemFromStr(str, *(uintptr_t*)&str[nextItemIndex + 1], mFields);
		frml->P2 = createFrmlElemFromStr(str, *(uintptr_t*)&str[nextItemIndex + 5], mFields);
		frml->P3 = createFrmlElemFromStr(str, *(uintptr_t*)&str[nextItemIndex + 9], mFields);*/
	}
	else
	{
		//throw std::exception("createFrmlElemFromStr: OP unknown");
	}
	return frml;
}

void createFieldDescrFromStr(FileD* F, uintptr_t firstAddress, BYTE* str)
{
	// nacte vsechny zretezene FieldDescr z predaneho retezce, vytvori mapu, kde klicem je puvodni adresa prvku
	// nactene polozky vzajemne zretezi, posledni ma jako Chain NULL
	WORD nextItemIndex = (uintptr_t)CFile->FldD.front() & 0x0000FFFF;
	std::map<uintptr_t, FieldDescr*> mFieldD;
	FieldDescr* lastFieldD = new FieldDescr(&str[nextItemIndex]);
	nextItemIndex = (uintptr_t(lastFieldD->Chain) & 0x0000FFFF);
	mFieldD.insert(std::pair<uintptr_t, FieldDescr*>(firstAddress, lastFieldD));
	F->FldD.push_back(lastFieldD);

	while (nextItemIndex != 0) {
		FieldDescr* nFieldD = new FieldDescr(&str[nextItemIndex]);
		nextItemIndex = (uintptr_t(nFieldD->Chain) & 0x0000FFFF);
		mFieldD.insert(std::pair<uintptr_t, FieldDescr*>((uintptr_t)lastFieldD->Chain, nFieldD));
		lastFieldD->Chain = nFieldD;
		lastFieldD = nFieldD;
	}

	// jednotlive polozky v mape ted maji jako Frml uvedene stare adresy
	// je nutne i jednotlive Frml vytvorit a adresy nahradit
	// projdeme tedy mapu a ke vsem polozkam nacteme formulare
	for (auto&& d : mFieldD)
	{
		auto frml = d.second->Frml;
		if (frml == nullptr) continue;
		if ((d.second->Flg & f_Stored) != 0) continue; // opsana podminka z puvodni RdFDSegment();
		uintptr_t address = (uintptr_t)d.second->Frml;
		// TODO: musi to tady byt? d.second->Frml = new FrmlElem(0, 0);
		d.second->Frml = createFrmlElemFromStr(str, address, &mFieldD);
	}
}

KeyFldD* createKFldsFromStr(BYTE* str, uintptr_t address, std::map<uintptr_t, XKey*>* mKeyDs)
{
	WORD nextItemIndex = address & 0x0000FFFF;
	auto kfd = new KeyFldD(&str[nextItemIndex]);

	return kfd;
}

void createKeysFromStr(FileD* F, uintptr_t firstAddress, BYTE* str)
{
	// nacte vsechny zretezene FileDescr z predaneho retezce, vytvori mapu, kde klicem je puvodni adresa prvku
	// nactene polozky vzajemne zretezi, posledni ma jako Chain NULL
	WORD nextItemIndex = (uintptr_t)CFile->Keys & 0x0000FFFF;
	// pokud soubor klice nema, koncime
	if (nextItemIndex == 0) { CFile->Keys = nullptr; return; }
	std::map<uintptr_t, XKey*> mKeyD;
	XKey* lastKeyD = new XKey(&str[nextItemIndex]);
	nextItemIndex = (uintptr_t(lastKeyD->Chain) & 0x0000FFFF);
	mKeyD.insert(std::pair<uintptr_t, XKey*>(firstAddress, lastKeyD));
	F->Keys = lastKeyD;

	while (nextItemIndex != 0) {
		XKey* nKeyD = new XKey(&str[nextItemIndex]);
		nextItemIndex = (uintptr_t(nKeyD->Chain) & 0x0000FFFF);
		mKeyD.insert(std::pair<uintptr_t, XKey*>((uintptr_t)lastKeyD->Chain, nKeyD));
		lastKeyD->Chain = nKeyD;
		lastKeyD = nKeyD;
	}

	// jednotlive klice v mape ted maji jako KFlds uvedene stare adresy
	// je nutne i jednotlive KFlds vytvorit a adresy nahradit
	// projdeme tedy mapu a ke vsem polozkam nacteme KFlds
	for (auto&& k : mKeyD)
	{
		auto kflds = k.second->KFlds;
		if (kflds == nullptr) continue;
		uintptr_t address = (uintptr_t)k.second->KFlds;
		k.second->KFlds = createKFldsFromStr(str, address, &mKeyD);
	}
}

bool RdFDSegment(WORD FromI, longint Pos)
{
	return false;

	integer Sg = 0, SgF = 0;/*CFile-Seg*/
	ChkD* c = nullptr;
	LinkD* ld = nullptr; LinkD* ld1 = nullptr;
	integer n = 0; LongStr* ss = nullptr; pstring lw;
	void* p = nullptr;
	auto result = false;
	lw = LexWord;
	//AlignLongStr();
	ss = CFile->TF->Read(1, Pos);
	if ((ss->LL <= sizeof(FileD))) return result;
	if (CRdb->Encrypted) CodingLongStr(ss);
	//Sg = uintptr_t(ss + 1);
	//SgF = Sg;
	FILE* origHandle = CFile->Handle;
	CFile = FileD_FromSegment(ss);
	CFile->Handle = origHandle;

	if (CFile->TF != nullptr) CFile->TF = new TFile();

	//CFile = (FileD*)Sg;
	/* !!! with CFile^ do!!! */
	if (CFile->IRec != FDVersion) return result;
	result = true;
	Tb = &CFile->Handle;
	//CFile->Handle = nullptr;
	//if (CFile->TF != nullptr) Pr(CFile->TF).Seg = Sg;
	//if (CFile->TF != nullptr) throw std::exception("Not implemented.");
	//if (CFile->XF != nullptr) Pr(CFile->XF).Seg = Sg;
	//if (CFile->XF != nullptr) throw std::exception("Not implemented.");

	//WORD offset = uintptr_t(CFile->FldD) & 0x0000FFFF;
	//WORD ssDataLen = ss->LL - offset;
	createFieldDescrFromStr(CFile, uintptr_t(CFile->FldD.front()), (BYTE*)&ss->A[0]);
	createKeysFromStr(CFile, uintptr_t(CFile->Keys), (BYTE*)&ss->A[0]);

	//FieldDescr* f = CFile->FldD;
	//while (f->Chain != nullptr) {
	//	//Pr(f->Chain).Seg = Sg; 
	//	f = (FieldDescr*)f->Chain;
	//	if ((f->Flg & f_Stored) == 0) SgFrml(f->Frml, Sg, SgF);
	//}

	if (CFile->OrigFD != nullptr) {
		throw std::exception("Not implemented.");
	}
	//CFile->OrigFD = GetFD(CFile->OrigFD, false, Sg);
	//StringList s = CFile->ViewNames;
	//while (s->Chain != nullptr) {
	//	//Pr(s->Chain).Seg = Sg; 
	//	s = (StringList)s->Chain;
	//}

	if (CFile->Keys != nullptr) {
		throw std::exception("Not implemented.");
	}
	//XKey* k = CFile->Keys;
	//while (k->Chain != nullptr) {
	//	//(k->Chain).Seg = Sg;
	//	k = k->Chain;
	//	//Pr(k->Alias).Seg = Sg;
	//	SgKF(KeyFldDPtr(&k->KFlds), Sg);
	//}

	if (!CFile->Add.empty()) {
		throw std::exception("Not implemented.");
	}
	//AddD* ad = (AddD*)&Add;
	//while (ad->Chain != nullptr) {
	//	//Pr(ad->Chain).Seg = Sg; 
	//	ad = ad->Chain;
	//	//if (ad->LD != nullptr) Pr(ad->LD).Seg = Sg; 
	//	SgFrml(ad->Frml, Sg, SgF);
	//	if (ad->Assign) SgFrml(ad->Bool, Sg, SgF);
	//	else if (ad->Chk != nullptr) {
	//		//Pr(ad->Chk).Seg = Sg; 
	//		/* !!! with ad->Chk^ do!!! */
	//		//if (ad->Chk->HelpName != nullptr) Pr(ad->Chk->HelpName).Seg = Sg;
	//	}
	//	ad->File2 = GetFD(ad->File2, true, Sg);
	//	//SgF = Pr(ad->File2).Seg;
	//	//Pr(ad->Field).Seg = SgF;
	//	if (!ad->Assign) {
	//		c = ad->Chk;
	//		if (c != nullptr) { SgFrml(c->Bool, Sg, SgF); SgFrml(c->TxtZ, Sg, SgF); }
	//	}
	//	SgF = Sg;
	//}
	//
	ld1 = LinkDRoot;
	ld = (LinkD*)CFile->Chain;
	//Pr(ld).Seg = Sg;
	n = CFile->nLDs;
	if (n > 0) LinkDRoot = ld;
	//while (n > 0) {
	//	if (n == 1) ld->Chain = ld1;
	//	//else Pr(ld->Chain).Seg = Sg;
	//	SgKF((KeyFldD*)ld->Args, Sg);
	//	ld->FromFD = CFile;
	//	ld->ToFD = GetFD(ld->ToFD, true, Sg);
	//	//Pr(ld->ToKey).Seg = Pr(ld->ToFD).Seg;
	//	ld = ld->Chain; n--;
	//}
	CFile->CatIRec = GetCatIRec(CFile->Name, CFile->Typ == '0'/*multilevel*/);
	CFile->ChptPos.R = CRdb; CFile->ChptPos.IRec = FromI;
#ifdef FandSQL
	SetIsSQLFile;
#endif
	CompileRecLen();
	p = Tb;
	if (CFile->LiOfs > 0) {
		p = Normalize(AbsAdr(CFile) + CFile->LiOfs);
		CFile->LiOfs = 0;
	}
	ReleaseStore(p);
	LexWord = lw;
	return result;
}

WORD FindHelpRecNr(FileDPtr FD, pstring txt)
{
	FileD* cf = nullptr; void* cr = nullptr;
	LockMode md = LockMode::NullMode;
	FieldDescr* NmF = nullptr; FieldDescr* TxtF = nullptr;
	WORD i = 0;
	pstring nm;// (80);
	WORD result = 0;
	ConvToNoDiakr((WORD*)txt[1], txt.length(), fonts.VFont);
	cf = CFile; cr = CRecPtr;
	CFile = FD;
	CRecPtr = GetRecSpace();
	md = NewLMode(RdMode);
	if (CFile->Handle == nullptr) goto label1;
	NmF = CFile->FldD.front();
	TxtF = (FieldDescr*)NmF->Chain;
	for (i = 1; i < CFile->NRecs; i++) {
		ReadRec(CFile, i, CRecPtr); nm = OldTrailChar(' ', _ShortS(NmF));
		ConvToNoDiakr((WORD*)nm[1], nm.length(), fonts.VFont);
		if (EqualsMask(&txt[1], txt.length(), nm)) {
			while ((i < CFile->NRecs) && (_T(TxtF) == 0)) { i++; ReadRec(CFile, i, CRecPtr); }
			result = i; goto label2;
		}
	}
label1:
	result = 0;
label2:
	OldLMode(md); ReleaseStore(CRecPtr); CFile = cf; CRecPtr = cr;
	return result;
}

bool PromptHelpName(WORD& N)
{
	wwmix ww;
	std::string txt;
	auto result = false; txt = "";
	ww.PromptLL(153, txt, 1, true);
	if ((txt.length() == 0) || (Event.Pressed.KeyCombination() == __ESC)) return result;
	N = FindHelpRecNr(CFile, txt);
	if (N != 0) result = true;
	return result;
}

void EditHelpOrCat(WORD cc, WORD kind, pstring txt)
{
	FileD* FD;
	WORD i, n;
	WORD nCat = 1; WORD iCat = 1; WORD nHelp = 1; WORD iHelp = 1;
	struct niFrml { char Op; double R; } nFrml{ 0,0 }, iFrml{ 0,0 };
	if (cc == __ALT_F2) {
		FD = CRdb->HelpFD;
		if (kind == 1) FD = CFile->ChptPos.R->HelpFD;
		if (FD == nullptr) return;
		if (kind == 0) { i = iHelp; n = nHelp; }
		else {
			i = 3; n = FindHelpRecNr(FD, txt);
			if (n == 0) {
				keyboard.SetKeyBuf("\0\60" + std::string(txt)); // TODO: tady ma byt KbdBuffer:=#0#60+txt
			}
		}
	}
	else {
		FD = CatFD;
		i = iCat;
		n = nCat;
	}
	if (kind != 2) WrEStatus();
	EditOpt* EO = new EditOpt(); EO->UserSelFlds = true; // GetEditOpt();
	EO->Flds = AllFldsList(FD, false);
	EO->WFlags = EO->WFlags | WPushPixel;
	if ((kind == 0) || (n != 0)) {
		iFrml.R = i; nFrml.R = n;
		EO->StartRecNoZ = (FrmlElem*)(&nFrml);
		EO->StartIRecZ = (FrmlElem*)(&iFrml);
	}
	EditDataFile(FD, EO);
	ReleaseStore(EO);
	if (cc == __ALT_F2) {
		nHelp = EdRecNo;
		iHelp = EdIRec;
	}
	else {
		ResetCatalog();
		nCat = EdRecNo;
		iCat = EdIRec;
	}
	if (kind != 2) RdEStatus();
}

void StoreChptTxt(FieldDPtr F, LongStr* S, bool Del)
{
	LongStr* s2 = nullptr; void* p = nullptr;
	WORD LicNr; longint oldpos, pos;
	LicNr = ChptTF->LicenseNr;
	oldpos = _T(F);
	MarkStore(p);
	if (CRdb->Encrypted) {
		if (LicNr != 0) {
			s2 = (LongStr*)GetStore(0x8100); /*possibly longer*/
			XEncode(S, s2); S = s2;
		}
		else CodingLongStr(S);
	}
	if (Del) if (LicNr == 0) ChptTF->Delete(oldpos);
	else if (oldpos != 0) ChptTF->Delete(oldpos - LicNr);
	pos = ChptTF->Store(S->A, S->LL);
	if (LicNr == 0) T_(F, pos);
	else T_(F, pos + LicNr);
	ReleaseStore(p);
}

void SetChptFldDPtr()
{
	if (Chpt == nullptr) /*ChptTF = nullptr;*/ throw std::exception("SetChptFldDPtr: Chpt is NULL.");
	else {
		ChptTF = Chpt->TF;
		ChptTxtPos = Chpt->FldD.front();
		ChptVerif = (FieldDescr*)ChptTxtPos->Chain;
		ChptOldTxt = (FieldDescr*)ChptVerif->Chain;
		ChptTyp = (FieldDescr*)ChptOldTxt->Chain;
		ChptName = (FieldDescr*)ChptTyp->Chain;
		ChptTxt = (FieldDescr*)ChptName->Chain;
	}
}

void SetRdbDir(char Typ, std::string* Nm)
{
	RdbD* r = nullptr; RdbD* rb = nullptr;
	std::string d;
	r = CRdb;
	rb = r->ChainBack;
	if (rb == nullptr) TopRdb = *r; CVol = "";
	if (Typ == '\\') {
		rb = &TopRdb; CRdb = rb;
		CFile->CatIRec = GetCatIRec(*Nm, false);
		CRdb = r;
	}
	if (CFile->CatIRec != 0) {
		CPath = RdCatField(CFile->CatIRec, CatPathName);
		if (CPath[1] != ':') {
			d = rb->RdbDir;
			if (CPath[1] == '\\') CPath = copy(d, 1, 2) + CPath;
			else {
				AddBackSlash(d); CPath = d + CPath;
			}
		}
		FSplit(CPath, CDir, CName, CExt);
		DelBackSlash(CDir);
	}
	else if (rb == nullptr) CDir = TopRdbDir;
	else {
		CDir = rb->RdbDir;
		AddBackSlash(CDir);
		CDir = CDir + CFile->Name;
	}
	/* !!! with r^ do!!! */ {
		r->RdbDir = CDir;
		if (TopDataDir.empty()) r->DataDir = CDir;
		else if (rb == nullptr) r->DataDir = TopDataDir;
		else {
			d = rb->DataDir;
			AddBackSlash(d);
			r->DataDir = d + CFile->Name;
		}
	}
	CDir = CDir + '\\';
}

void ResetRdOnly()
{
	if (Chpt->UMode == RdOnly) {
		CloseFile();
		IsInstallRun = true;
		OpenF(Exclusive);
		IsInstallRun = false;
	}
}

void CreateOpenChpt(std::string Nm, bool create, wwmix* ww)
{
	std::string p; std::string s;
	integer i = 0, n = 0;
	std::string nr; std::string Nm1;
	FileUseMode um = Closed;

	bool top = (CRdb == nullptr);
	FileDRoot = nullptr;
	Chpt = nullptr;
	//R = (RdbD*)GetZStore(sizeof(*R));
	RdbD* R = new RdbD();
	TFile* oldChptTF = ChptTF;
	R->ChainBack = CRdb;
	R->OldLDRoot = LinkDRoot;
	R->OldFCRoot = FuncDRoot;
	//MarkStore2(R->Mark2);
	RdMsg(51);
	s = MsgLine;
	RdMsg(48);
	val(MsgLine, n, i);
	nr = std::to_string((TxtCols - n));
	s = s + nr;
	SetInpStr(s);
	if ((Nm[0] == '\\')) Nm1 = Nm.substr(2, 8);
	else Nm1 = Nm;
	RdFileD(Nm1, '0', ""); /*old CRdb for GetCatIRec*/
	R->FD = CFile;
	CRdb = R;
	CFile->RecPtr = GetRecSpace();
	SetRdbDir(Nm[0], &Nm1);
	p = CDir + Nm1 + ".RDB";
	CFile->Drive = TestMountVol(CPath[1]);
	SetChptFldDPtr();
	if (!spec.RDBcomment) ChptTxt->L = 1;
	SetMsgPar(p);
	if (top) {
		UserName = "";
		UserCode = 0;
		AccRight[0] = 0;
		goto label2;
	}
	if (CRdb->ChainBack != nullptr)	CRdb->HelpFD = CRdb->ChainBack->HelpFD;
label1:
	ChDir(R->RdbDir);
	if (IOResult() != 0)
		if (create && (IsTestRun || !top)) {
			MkDir(R->RdbDir);
			if (IOResult() != 0) RunError(620);
			goto label1;
		}
		else RunError(631);
label2:
	if (IsTestRun || !create) um = Exclusive;
	else um = RdOnly;
	if (OpenF(um)) {
		if (ChptTF->CompileAll) ResetRdOnly();
		else if (!top && oldChptTF != nullptr && (ChptTF->TimeStmp < oldChptTF->TimeStmp)) {
			// TODO: oldChptTF != nullptr je v podmince navic, protoze dalsi podminka vzdy vyhorela 
			ResetRdOnly();
			SetCompileAll();
		}
		goto label3;
	}
	if (!create || (top && !IsTestRun)) RunError(631);
	OpenCreateF(Exclusive);
	SetCompileAll();
label3:
	const bool hasPasswd = ww->HasPassWord(Chpt, 1, "");
	CRdb->Encrypted = hasPasswd ? false : true;
}

void CloseChpt()
{
	if (CRdb == nullptr) return;
	ClearHelpStkForCRdb();
	SaveFiles();
	bool del = Chpt->NRecs == 0;
	std::string d = CRdb->RdbDir;
	CloseFAfter(FileDRoot);
	LinkDRoot = CRdb->OldLDRoot;
	FuncDRoot = CRdb->OldFCRoot;
	void* p = CRdb;
	//void* p2 = CRdb->Mark2;
	CRdb = CRdb->ChainBack;
	//ReleaseBoth(p, p2);
	if (CRdb != nullptr) {
		FileDRoot = CRdb->FD;
		Chpt = FileDRoot;
		SetChptFldDPtr();
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
		for (WORD i = 1; i < FloppyDrives; i++) ReleaseDrive(i);
	}
}

void GoCompileErr(WORD IRec, WORD N)
{
	IsCompileErr = true;
	InpRdbPos.R = CRdb;
	InpRdbPos.IRec = IRec;
	CurrPos = 0;
	RdMsg(N);
	GoExit();
}

FileD* FindFD()
{
	FileD* FD = nullptr; std::string FName; std::string d;
	std::string name; std::string ext;
	FName = OldTrailChar(' ', _ShortS(ChptName));
	FSplit(FName, d, name, ext);
	FD = FileDRoot;
	while (FD != nullptr) {
		if (SEquUpcase(FD->Name, name)) break;
		FD = (FileD*)FD->Chain;
	}
	return FD;
}

void Diagnostics(void* MaxHp, longint Free, FileD* FD)
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

bool CompRunChptRec(WORD CC)
{
	pstring STyp(1); void* p = nullptr; void* p2 = nullptr; void* MaxHp = nullptr;
	ExitRecord er; EditD* OldE = nullptr;
	RdbPos RP; longint Free; bool uw = false, mv = false;
	FileD* FD = nullptr; LinkD* oldLd = nullptr; LinkD* ld = nullptr;
	EditOpt* EO = nullptr; WORD nStrm = 0;
	auto result = false;

	OldE = E; MarkBoth(p, p2); WrEStatus(); //NewExit(Ovr(), er);
	//goto label2;
	IsCompileErr = false; uw = false; mv = MausVisible;
	FileD* lstFD = (FileD*)LastInChain(FileDRoot);
	oldLd = LinkDRoot;
	bool WasError = true;
	bool WasGraph = IsGraphMode;
	FD = nullptr;
	STyp = _ShortS(ChptTyp);
	RP.R = CRdb;
	RP.IRec = CRec();
#ifdef FandSQL
	nStrm = nStreams;
#endif
	if (CC == __ALT_F9) {
		if (FindChpt('P', "MAIN", true, &RP)) goto label1;
		else WrLLF10Msg(58);
	}
	else
		switch (STyp[1]) {
		case 'F': {
			FD = FindFD();
			if ((FD != nullptr) && (CC == __CTRL_F9)) {
				EO = new EditOpt(); EO->UserSelFlds = true; // GetEditOpt();
				CFile = FD;
				EO->Flds = AllFldsList(CFile, false);
				if (SelFldsForEO(EO, nullptr)) EditDataFile(FD, EO);
			}
			break;
		}
		case 'E': {
			if (CC == __CTRL_F9) {
				EO = new EditOpt(); EO->UserSelFlds = true; // GetEditOpt();
				EO->FormPos = RP;
				EditDataFile(nullptr, EO);
			}
			else { PushEdit(); RdFormOrDesign(nullptr, nullptr, RP); }
			break;
		}
		case 'M': {
			SetInpTT(&RP, true);
			ReadMerge();
			if (CC == __CTRL_F9) RunMerge();
			break;
		}
		case 'R': {
			SetInpTT(&RP, true);
			ReadReport(nullptr);
			if (CC == __CTRL_F9) {
				RunReport(nullptr);
				SaveFiles();
				ViewPrinterTxt();
			}
			break;
		}
		case 'P': {
			if (CC == __CTRL_F9) {
			label1:
				if (UserW != 0) { PopW(UserW); uw = true; }
				RunMainProc(RP, CRdb->ChainBack = nullptr);
			}
			else {
				lstFD = (FileD*)LastInChain(FileDRoot); ld = LinkDRoot;
				SetInpTT(&RP, true);
				ReadProcHead("");
				ReadProcBody();
				lstFD->Chain = nullptr;
				LinkDRoot = ld;
			}
			break;
		}
#ifdef FandProlog
		case 'L': {
			if (CC == __CTRL_F9) {
				TextAttr = ProcAttr;
				ClrScr();
				RunProlog(&RP, nullptr);
			}
			break;
		}
#endif
		}
	WasError = false;
label2:
	MaxHp = nullptr;
	ReleaseStore2(p2); Free = StoreAvail(); RestoreExit(er);
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
		else ClrScr();
	}
	if (uw) { UserW = 0;/*mem overflow*/UserW = PushW(1, 1, TxtCols, TxtRows); }
	SaveFiles();
	if (mv) ShowMouse();
	if (WasError) ForAllFDs(ClearXFUpdLock);
	CFile = (FileD*)lstFD->Chain;
	while (CFile != nullptr) {
		CloseFile();
		CFile = (FileD*)CFile->Chain;
	}
	lstFD->Chain = nullptr;
	LinkDRoot = oldLd;
	ReleaseBoth(p, p2); E = OldE;
	RdEStatus();
	CRdb = RP.R;
	PrevCompInp = nullptr;
	ReadRec(CFile, CRec(), CRecPtr);
	if (IsCompileErr) result = false;
	else {
		result = true;
		if (WasError) return result;
		B_(ChptVerif, false);
		WriteRec(CFile, CRec(), CRecPtr);
		if (CC == __CTRL_F8) Diagnostics(MaxHp, Free, FD);
	}
	return result;
}

void RdUserId(bool Chk)
{
	wwmix ww;
	FrmlPtr Z;
	pstring pw(20); pstring pw2(20); pstring name(20);
	WORD code; pstring acc;

	RdFldNameFrml = nullptr;
	RdLex();
	if (Lexem == 0x1A) return;
	if (Chk) pw = ww.PassWord(false);
label1:
	TestLex(_quotedstr); name = LexWord; RdLex(); Accept(',');
	code = RdInteger(); Accept(',');
	Z = RdStrFrml(); pw2 = RunShortStr(Z); ReleaseStore(Z);
	if (Lexem == ',') { RdLex(); RdByteList(&acc); }
	else { acc[0] = 1; acc[1] = char(code); }
	if (Chk) {
		if (SEquUpcase(pw, pw2)) {
			UserName = name; UserCode = code; UserPassWORD = pw2; AccRight = acc; return;
		}
	}
	else if (code == 0) {
		UserName = name; UserCode = code; UserPassWORD = pw2;
	}
	if (Lexem != 0x1A) { Accept(';'); if (Lexem != 0x1A) goto label1; }
	if (Chk) RunError(629);
}

WORD CompileMsgOn(CHAR_INFO* Buf, longint& w)
{
	pstring s;
	WORD result = 0;
	RdMsg(15);
	if (IsTestRun) {
		w = PushWFramed(0, 0, 30, 4, screen.colors.sNorm, MsgLine, "", WHasFrame + WDoubleFrame + WShadow);
		RdMsg(117);
		std::string ss = MsgLine;
		s = GetNthLine(ss, 1, 1, '/');
		screen.GotoXY(3, 2);
		printf("%s", s.c_str());
		result = s.length();
		screen.GotoXY(3, 3);
		printf("%s", GetNthLine(ss, 2, 1, '/').c_str());
	}
	else {
		screen.ScrRdBuf(0, TxtRows - 1, Buf, 40);
		w = 0;
		result = 0;
		screen.ScrClr(1, TxtRows, MsgLine.length() + 2, 1, ' ', screen.colors.zNorm);
		screen.ScrWrStr(2, TxtRows, MsgLine, screen.colors.zNorm);
	}
	return result;
}

void CompileMsgOff(CHAR_INFO* Buf, longint& w)
{
	if (w != 0) PopW(w);
	else screen.ScrWrCharInfoBuf(1, TxtRows, Buf, 40);
}

longint MakeDbfDcl(pstring Nm)
{
	DBaseHd Hd; DBaseFld Fd; WORD i, n;
	FILE* h; LongStr* t; char c;
	pstring s(80); pstring s1(10); void* p;

	CPath = FExpand(Nm + ".DBF"); CVol = "";
	i = GetCatIRec(Nm, true); if (i != 0) RdCatPathVol(i);
	h = OpenH(_isoldfile, RdOnly); TestCPathError();
	ReadH(h, 32, &Hd); n = (Hd.HdLen - 1) / 32 - 1; t = (LongStr*)GetStore(2); t->LL = 0;
	for (i = 1; i < n; i++) {
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
		p = GetStore(s.length());
		Move(&s[1], p, s.length());
		t->LL += s.length();
	}
	LongS_(ChptTxt, t);
	CloseH(&h);
	return 0;
}

void* RdF(std::string FileName)
{
	std::string d, name, ext;
	char FDTyp = '\0';
	std::string s;
	FieldDescr* IdF = nullptr; FieldDescr* TxtF = nullptr;
	integer i = 0, n = 0;
	std::string nr;
	FSplit(FileName, d, name, ext);

	FDTyp = ExtToTyp(ext);
	if (FDTyp == '0') {
		RdMsg(51);
		s = MsgLine;
		RdMsg(49);
		val(MsgLine, n, i);
		nr = std::to_string(TxtCols - n);
		s = s + nr;
		SetInpStr(s);
	}
	else {
		longint pos = _T(ChptTxt);
		SetInpTTPos(pos, CRdb->Encrypted);
	}
	return RdFileD(name, FDTyp, ext);
}

bool EquStoredF(FieldDPtr F1, FieldDPtr F2)
{
	auto result = false;
label1:
	while ((F1 != nullptr) && (F1->Flg && f_Stored == 0)) F1 = (FieldDescr*)F1->Chain;
	while ((F2 != nullptr) && (F2->Flg && f_Stored == 0)) F2 = (FieldDescr*)F2->Chain;
	if (F1 == nullptr)
	{
		if (F2 != nullptr) return result;
		result = true;
		return result;
	}
	if ((F2 == nullptr) || !FldTypIdentity(F1, F2) ||
		(F1->Flg && (!f_Mask) != F2->Flg && (!f_Mask))) return result;
	F1 = (FieldDescr*)F1->Chain; F2 = (FieldDescr*)F2->Chain;
	goto label1;
}

void DeleteF()
{
	CloseFile(); SetCPathVol(); MyDeleteFile(CPath);
	CExtToX(); if (CFile->XF != nullptr) MyDeleteFile(CPath);
	CExtToT(); if (CFile->TF != nullptr) MyDeleteFile(CPath);
}

bool MergAndReplace(FileD* FDOld, FileD* FDNew)
{
	std::string s; ExitRecord er; std::string p;
	auto result = false;
	//NewExit(Ovr(), er);
	//goto label1;
	s = "#I1_";
	s += FDOld->Name + " #O1_@";
	SetInpStr(s);
	SpecFDNameAllowed = true; ReadMerge(); SpecFDNameAllowed = false;
	RunMerge(); SaveFiles(); RestoreExit(er);
	CFile = FDOld; DeleteF(); CFile = FDNew; CloseFile(); FDOld->Typ = FDNew->Typ;
	SetCPathVol(); p = CPath; CFile = FDOld; SetCPathVol();
	RenameFile56(p, CPath, false);
	CFile = FDNew;
	/*TF->Format used*/
	CExtToT(); p = CPath;
	SetCPathVol();
	CExtToT();
	RenameFile56(CPath, p, false);
	result = true;
	return result;
label1:
	RestoreExit(er); CFile = FDOld; CloseFile(); CFile = FDNew; DeleteF();
	SpecFDNameAllowed = false; result = false;
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
			KF1 = (KeyFldD*)KF1->Chain;
			KF2 = (KeyFldD*)KF2->Chain;
		}
		if (KF2 != nullptr) return result;
		K1 = K1->Chain;
		K2 = K2->Chain;
	}
	if (K2 != nullptr) return result;
	result = true;
	return result;
}

bool MergeOldNew(bool Veriflongint, longint Pos)
{
	std::string Name;
	LinkD* ld = LinkDRoot;
	auto result = false;
	FileD* FDOld = nullptr;
	FileD* FDNew = CFile;
	SetCPathVol();
	Name = FDNew->Name;
	FDNew->Name = "@";
	CFile = Chpt;
	if (!RdFDSegment(0, Pos)) goto label1;
	ChainLast(FileDRoot, CFile);
	FDOld = CFile; FDOld->Name = Name;
	if ((FDNew->Typ != FDOld->Typ) || !EquStoredF(FDNew->FldD.front(), FDOld->FldD.front())
#ifdef FandSQL
		&& !FDNew->IsSQLFile && !FDOld->IsSQLFile
#endif
		) {
		MergAndReplace(FDOld, FDNew);
		result = true;
	}
	else if ((FDOld->Typ == 'X') && !EquKeys(FDOld->Keys, FDNew->Keys)) {
		SetCPathVol();
		CExtToX();
		MyDeleteFile(CPath);
	}
label1:
	FDNew->Chain = nullptr;
	LinkDRoot = ld;
	FDNew->Name = Name;
	FDNew->FullName = CPath;
	CFile = FDNew;
	CRecPtr = Chpt->RecPtr;
	return result;
}

bool CompileRdb(bool Displ, bool Run, bool FromCtrlF10)
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "starting CompileRdb()");
	CHAR_INFO Buf[40];
	longint w = 0;
	longint I = 0, J = 0, OldTxt = 0, Txt = 0, OldCRec = 0;
	pstring STyp(1);
	char Typ = '\0';
	std::string Name, dir, nm, ext;
	bool Verif = false, FDCompiled = false, Encryp = false;
	char Mode = '\0'; RdbPos RP;
	void* p = nullptr; void* p1 = nullptr; void* p2 = nullptr;
	ExitRecord er; WORD lmsg = 0;
	LinkD* ld = nullptr;
	std::string RprtTxt; 
	bool top = false;
	FileD* lstFD = nullptr;
	auto result = false;

	EditD* OldE = E;
	MarkBoth(p, p2);
	p1 = p;
	//NewExit(Ovr, er);
	//goto label1;
	IsCompileErr = false; FDCompiled = false;
	OldCRec = CRec(); RP.R = CRdb;
	top = (CRdb->ChainBack == nullptr);
	if (top) {
		UserName[0] = 0; UserCode = 0; UserPassWORD[0] = 0; AccRight[0] = 0;
		if (ChptTF->CompileAll || CompileFD) Switches[0] = 0;
	}
	lmsg = CompileMsgOn(Buf, w);
	CRecPtr = Chpt->RecPtr;
	Encryp = CRdb->Encrypted;
	for (I = 1; I <= Chpt->NRecs; I++) {
		//if (I >= 580) {
		//	printf("RunProj r1495, CompileRdb(), I = %i, strings: %i, total: %i\n", I, strcount, strbytes);
		//}
		ReadRec(CFile, I, CRecPtr);
		RP.IRec = I;
		Verif = _B(ChptVerif);
		STyp = _ShortS(ChptTyp);
		Typ = STyp[1];
		Name = OldTrailChar(' ', _ShortS(ChptName));
		Txt = _T(ChptTxt);
		if (Verif && ((ChptTF->LicenseNr != 0) || Encryp || (Chpt->UMode == RdOnly))) GoCompileErr(I, 647);
		if (Verif || ChptTF->CompileAll || FromCtrlF10 || (Typ == 'U') ||
			(Typ == 'F' || Typ == 'D') && CompileFD ||
			(Typ == 'P') && ChptTF->CompileProc) {
			OldTxt = _T(ChptOldTxt);
			InpRdbPos = RP;
			if (IsTestRun) {
				ClrScr();
				screen.GotoXY(3 + lmsg, 2);
				printf("%*i", 4, I);
				screen.GotoXY(3 + lmsg, 3);
				printf("%*s%*s", 4, STyp.c_str(), 14, _ShortS(ChptName).c_str());
				if (!(Typ == ' ' || Typ == 'D' || Typ == 'U')) { /* dupclicate name checking */
					for (J = 1; J < I - 1; J++) {
						ReadRec(CFile, J, CRecPtr);
						if ((STyp == _ShortS(ChptTyp)) && SEquUpcase(Name, OldTrailChar(' ', _ShortS(ChptName)))) GoCompileErr(I, 649);
					}
					ReadRec(CFile, I, CRecPtr);
				}
			}
			switch (Typ) {
			case 'F': {
				FDCompiled = true;
				ld = LinkDRoot;
				MarkStore(p1);
				FSplit(Name, dir, nm, ext);
				if ((Txt == 0) && IsTestRun) {
					SetMsgPar(Name);
					if (SEquUpcase(ext, ".DBF") && PromptYN(39)) {
						T_(ChptOldTxt, 0);
						OldTxt = 0;
						MakeDbfDcl(nm);
						Txt = _T(ChptTxt);
						WriteRec(CFile, I, CRecPtr);
					}
				}
#ifndef FandSQL
				if (SEquUpcase(ext, ".SQL")) GoCompileErr(I, 654);
#endif
				if (Verif || ChptTF->CompileAll || (OldTxt == 0)) {
				label2:
					p1 = RdF(Name);
					// TODO: toto se asi zase musi povolit !!! 
					//WrFDSegment(I);
					if (CFile->IsHlpFile) CRdb->HelpFD = CFile;
					if (OldTxt > 0) MergeOldNew(Verif, OldTxt);
					ReleaseStore(p1);
					CFile = Chpt;
					// Odmazani dat z TTT souboru nebudeme provadet!
					/*if (ChptTF->LicenseNr == 0) ChptTF->Delete(OldTxt);
					else if (OldTxt != 0) ChptTF->Delete(OldTxt - ChptTF->LicenseNr);*/
				}
				else if (!RdFDSegment(I, OldTxt)) {
					LinkDRoot = ld;
					ReleaseStore(p1);
					CFile = Chpt;
					goto label2;
				}
				else {
					ChainLast(FileDRoot, CFile); MarkStore(p1);
					if (CFile->IsHlpFile) CRdb->HelpFD = CFile;
				}
				break;
			}
			case 'M': {
				SetInpTTPos(Txt, Encryp);
				ReadMerge();
				break;
			}
			case 'R': {
				if ((Txt == 0) && IsTestRun) {
					RprtTxt = SelGenRprt(Name);
					CFile = Chpt;
					if (RprtTxt.empty()) GoCompileErr(I, 1145);
					S_(ChptTxt, RprtTxt);
					WriteRec(CFile, I, CRecPtr);
				}
				else {
					SetInpTTPos(Txt, Encryp);
					ReadReport(nullptr);
				}
				break;
			}
			case 'P': {
				if (FileDRoot->Chain == nullptr) lstFD = FileDRoot;
				else lstFD = (FileD*)LastInChain(FileDRoot);
				ld = LinkDRoot;
				SetInpTTPos(Txt, Encryp);
				ReadProcHead(Name);
				ReadProcBody();
				lstFD->Chain = nullptr;
				LinkDRoot = ld;
				break;
			}
			case 'E': {
				PushEdit();
				RdFormOrDesign(nullptr, nullptr, RP);
				E = OldE;
				break;
			}
			case 'U': {
				if (!top || (I > 1)) GoCompileErr(I, 623);
				if (Txt != 0) {
					ResetCompilePars();
					SetInpTTPos(Txt, Encryp);
					RdUserId(!IsTestRun || (ChptTF->LicenseNr != 0));
					MarkStore(p1);
				}
				break;
			}
			case 'D': {
				ResetCompilePars();
				SetInpTTPos(Txt, Encryp);
				ReadDeclChpt();
				MarkStore(p1);
				break;
			}
#ifdef FandProlog
			case 'L': {
				SetInpTTPos(Txt, Encryp);
				// ReadProlog(I); 
				break;
			}
#endif
			}
		}
		ReleaseBoth(p1, p2); CFile = Chpt; CRecPtr = Chpt->RecPtr;
		if (Verif) {
			ReadRec(CFile, I, CRecPtr);
			B_(ChptVerif, false);
			WriteRec(CFile, I, CRecPtr);
		}
	}
	/* !!! with ChptTF^ do!!! */
	if (ChptTF->CompileAll || ChptTF->CompileProc) {
		ChptTF->CompileAll = false;
		ChptTF->CompileProc = false;
		SetUpdHandle(ChptTF->Handle);
	}
	CompileFD = false;
	result = true;
	RestoreExit(er);
	if (!Run) { CRecPtr = E->NewRecPtr; ReadRec(CFile, CRec(), CRecPtr); }
	CompileMsgOff(Buf, w);
#ifdef FandSQL
	if (top && (Strm1 != nullptr)) Strm1->Login(UserName, UserPassWORD);
#endif
	log->log(loglevel::DEBUG, "finish CompileRdb()");
	return result;
label1:
	RestoreExit(er); result = false; CompileMsgOff(Buf, w);
	ReleaseFDLDAfterChpt(); PrevCompInp = nullptr;
	ReleaseBoth(p, p2); E = OldE; CFile = Chpt;
	if (!Run) CRecPtr = E->NewRecPtr;
	if (!IsCompileErr) { InpRdbPos.IRec = I; }
	return result;
}

void GotoErrPos(WORD& Brk)
{
	pstring s;
	IsCompileErr = false; s = MsgLine; if (InpRdbPos.R != CRdb) {
		DisplEditWw(); SetMsgPar(s); WrLLF10Msg(110);
		if (InpRdbPos.IRec == 0) SetMsgPar("");
		else SetMsgPar(InpRdbPos.R->FD->Name);
		WrLLF10Msg(622); Brk = 0; return;
	}
	if (CurrPos == 0) {
		DisplEditWw();
		GotoRecFld(InpRdbPos.IRec, (EFldD*)E->FirstFld->Chain);
		SetMsgPar(s); WrLLF10Msg(110); Brk = 0; return;
	}
	CFld = E->LastFld;
	SetNewCRec(InpRdbPos.IRec, true);
	R_(ChptTxtPos, integer(CurrPos));
	WriteRec(CFile, CRec(), CRecPtr);
	EditFreeTxt(ChptTxt, s, true, Brk);
}

void WrErrMsg630(std::string Nm)
{
	IsCompileErr = false;
	SetMsgPar(MsgLine); WrLLF10Msg(110);
	SetMsgPar(Nm); WrLLF10Msg(630);
}

bool EditExecRdb(std::string Nm, std::string ProcNm, Instr_proc* ProcCall, wwmix* ww)
{
	WORD Brk = 0, cc = 0;
	void* p = nullptr;
	pstring passw(20);
	bool b = false;
	ExitRecord er, er2;
	RdbPos RP;
	EditOpt* EO = nullptr;

	auto result = false;
	bool top = CRdb == nullptr;
	bool EscCode = false;
	longint w = UserW; UserW = 0;
	bool wasGraph = IsGraphMode;
#ifdef FandSQL
	if (top) SQLConnect();
#endif
	//NewExit(Ovr(), er);
	//goto label9;
	CreateOpenChpt(Nm, true, ww);
	CompileFD = true;
#ifndef FandRunV
	if (!IsTestRun || (ChptTF->LicenseNr != 0) ||
		!top && CRdb->Encrypted) {
#endif
		MarkStore(p);
		EditRdbMode = false;
		bool hasToCompileRdb = CompileRdb(false, true, false);
		if (hasToCompileRdb) {
			bool procedureFound = FindChpt('P', ProcNm, true, &RP);
			if (procedureFound)
			{
				//NewExit(Ovr(), er2);
				//goto label0;
				IsCompileErr = false;
				if (ProcCall != nullptr) {
					ProcCall->PPos = RP;
					CallProcedure(ProcCall);
				}
				else RunMainProc(RP, top);
				result = true; goto label9;
			label0:
				if (IsCompileErr) WrErrMsg630(Nm);
				goto label9;
			}
			else {
				SetMsgPar(Nm, ProcNm);
				WrLLF10Msg(632);
			}
		}
		else if (IsCompileErr) WrErrMsg630(Nm);
#ifndef FandRunV
		if ((ChptTF->LicenseNr != 0) || CRdb->Encrypted
			|| (Chpt->UMode == RdOnly)) goto label9;
		ReleaseFDLDAfterChpt();
		ReleaseStore(p);
	}
	else if (!top) UserW = PushW(1, 1, TxtCols, TxtRows);
	EditRdbMode = true;
	if (CRdb->Encrypted) {
		// ask for the project password
		passw = ww->PassWord(false);
	}
	IsTestRun = true;
	EO = new EditOpt(); EO->UserSelFlds = true; //EO = GetEditOpt();
	EO->Flds = AllFldsList(Chpt, true);
	EO->Flds = (FieldList)EO->Flds->Chain->Chain->Chain;
	NewEditD(Chpt, EO);
	E->MustCheck = true; /*ChptTyp*/
	if (CRdb->Encrypted) {
		if (ww->HasPassWord(Chpt, 1, passw)) {
			CRdb->Encrypted = false;
			ww->SetPassWord(Chpt, 1, "");
			CodingCRdb(false);
		}
		else {
			WrLLF10Msg(629);
			goto label9;
		}
	}
	if (!OpenEditWw()) goto label8;
	result = true;
	Chpt->WasRdOnly = false;
	if (!top && (Chpt->NRecs > 0))
		if (CompileRdb(true, false, false)) {
			if (FindChpt('P', ProcNm, true, &RP)) GotoRecFld(RP.IRec, CFld);
		}
		else goto label4;
	else if (ChptTF->IRec <= Chpt->NRecs) GotoRecFld(ChptTF->IRec, CFld);
label1:
	RunEdit(nullptr, Brk);
label2:
	// TODO: je to potreba?
	cc = Event.Pressed.KeyCombination();
	SaveFiles();
	if ((cc == __CTRL_F10) || ChptTF->CompileAll || CompileFD) {
		ReleaseFDLDAfterChpt();
		SetSelectFalse();
		E->Bool = nullptr;
		ReleaseStore(E->AfterE);
	}
	if (cc == __CTRL_F10) {
		SetUpdHandle(ChptTF->Handle);
		if (!CompileRdb(true, false, true)) goto label3;
		if (!PromptCodeRdb()) goto label6;
		Chpt->WasRdOnly = true;
		goto label8;
	}
	if (Brk != 0) {
		if (!CompileRdb(Brk == 2, false, false)) {
		label3:
			if (IsCompileErr) goto label4;
			if (Brk == 1) DisplEditWw();
			GotoRecFld(InpRdbPos.IRec, (EFldD*)E->FirstFld->Chain);
			goto label1;
		}
		if (cc == __ALT_F2) {
			EditHelpOrCat(cc, 0, "");
			goto label41;
		}
		if (!CompRunChptRec(cc)) {
		label4:
			GotoErrPos(Brk);
			goto label5;
		}
	label41:
		if (Brk == 1) {
			EditFreeTxt(ChptTxt, "", true, Brk);
		label5:
			if (Brk != 0) goto label2;
			else goto label1;
		}
		else {
		label6:
			DisplEditWw();
			goto label1;
		}
	}
	ChptTF->IRec = CRec();
	SetUpdHandle(ChptTF->Handle);
label8:
	PopEdit();
#endif

label9:
	RestoreExit(er);
	if (!wasGraph && IsGraphMode) {
		// ScrTextMode(false, false);
		throw std::exception("CompRunChptRec() Graph <-> Text Mode not implemented.");
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
	CFile = CatFD;
	if (CatFD->Handle == nullptr) OpenCreateF(Exclusive);
	EditOpt* EO = new EditOpt(); EO->UserSelFlds = true; // GetEditOpt();
	EO->Flds = AllFldsList(CatFD, true);
	EditDataFile(CatFD, EO);
	ChDir(OldDir);
	ReleaseStore(EO);
}

void UpdateUTxt()
{
	longint w; WORD TxtPos, LicNr; LongStr* S = nullptr; LongStr* s2 = nullptr;
	bool Srch, Upd, b;
	longint OldPos, Pos; ExitRecord er; void* p = nullptr; void* p1 = nullptr;
	size_t LL;
	CFile = Chpt;
	CRecPtr = Chpt->RecPtr;
	LicNr = ChptTF->LicenseNr;
	MarkStore(p1);
	if (CFile->NRecs == 0) goto label1;
	ReadRec(CFile, 1, CRecPtr);
	if (_ShortS(ChptTyp) != 'U') {
	label1:
		WrLLF10Msg(9); /*exit*/;
	}
	w = PushW(1, 1, TxtCols, TxtRows - 1);
	TxtPos = 1;
	TextAttr = screen.colors.tNorm;
	OldPos = _T(ChptTxt);
	S = _LongS(ChptTxt); b = false;
	if (CRdb->Encrypted) CodingLongStr(S); // NewExit(Ovr, er);
	goto label4;
	SetInpLongStr(S, false);
	MarkStore(p);
	RdUserId(false);
	ReleaseStore(p);
	b = true;
label2:
	SimpleEditText('T', "", "", S, 0x7FFF, TxtPos, Upd);
	SetInpLongStr(S, false);
	MarkStore(p);
	RdUserId(false);
	ReleaseStore(p);
	b = false;
	if (Upd) {
		StoreChptTxt(ChptTxt, S, true);
		WriteRec(CFile, 1, CRecPtr);
	}
label3:
	PopW(w);
	ReleaseStore(p1);
	return;
label4:
	if (b) {
		WrLLF10MsgLine();
		ReleaseStore(p);
		if (PromptYN(59)) goto label2;
		goto label3;
	}
	WrLLF10Msg(9);
	goto label3;
}

void InstallRdb(std::string n)
{
	wwmix ww;

	ExitRecord er;
	pstring passw(20);
	TMenuBoxS* w = nullptr;
	WORD i = 0;

	CreateOpenChpt(n, false, &ww);
	if (!ww.HasPassWord(Chpt, 1, "") && !ww.HasPassWord(Chpt, 2, "")) {
		passw = ww.PassWord(false);
		if (!ww.HasPassWord(Chpt, 2, passw)) {
			WrLLF10Msg(629);
			goto label1;
		}
	}
	if (Chpt->UMode == RdOnly) {
		UpdateCat();
		goto label1;
	}
	RdMsg(8);

	i = 1;
	w = new TMenuBoxS(43, 6, MsgLine);
label0:
	i = w->Exec(i);
	switch (i) {
	case 0: { delete w; w = nullptr; goto label1; }
	case 1: { UpdateCat(); goto label0; }
	case 2: { UpdateUTxt(); break; }
	case 3: { ww.SetPassWord(Chpt, 2, ww.PassWord(true)); break; }
	}
	SetUpdHandle(ChptTF->Handle);
	goto label0;
label1:
	RestoreExit(er);
	CloseChpt();
}
