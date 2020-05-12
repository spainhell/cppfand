#include "runproj.h"
#include "access.h"
#include "expimp.h"
#include "genrprt.h"
#include "legacy.h"
#include "oaccess.h"
#include "obaseww.h"
#include "rdedit.h"
#include "rdfildcl.h"
#include "rdmerg.h"
#include "rdproc.h"
#include "rdrprt.h"
#include "rdrun.h"
#include "runedi.h"
#include "runmerg.h"
#include "runproc.h"
#include "runrprt.h"
#include "wwmenu.h"
#include "wwmix.h"


void* O(void* p) // ASM
{
	return nullptr;
}

void* OCF(void* p) // ASM
{
	return nullptr;
}

//EditD* E = EditDRoot;
longint UserW = 0;

struct RdbRecVars
{
	char Typ = 0; pstring Name = pstring(12); pstring Ext;
	longint Txt = 0; longint OldTxt = 0;
	char FTyp = 0; WORD CatIRec = 0; bool isSQL = false;
};


FileD* CFileF;
longint sz; WORD nTb; void* Tb;

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

	if (Chpt->Chain != nullptr) CloseFAfter(Chpt->Chain);
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

void GetSplitChptName(pstring* Name, pstring* Ext)
{
	WORD i;
	*Ext = "";
	*Name = TrailChar(' ', _ShortS(ChptName));
	i = Name->first('.');
	if (i == 0) return;
	*Ext = Name->substr(i, 255);
	*Name = Name->substr(1, i - 1);
}

void GetRdbRecVars(void* RecPtr, RdbRecVars* X)
{
	pstring s1(1);
	void* p = nullptr; void* p2 = nullptr; void* cr = nullptr;
	LinkDPtr ld = nullptr;

	cr = CRecPtr;
	CRecPtr = RecPtr;
	s1 = _ShortS(ChptTyp);
	X->Typ = s1[1];
	GetSplitChptName(&X->Name, &X->Ext);
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
		CRecPtr = cr;
	}
}

bool ChptDelFor(RdbRecVars* X)
{
	SetUpdHandle(ChptTF->Handle); ReleaseFDLDAfterChpt();
	switch (X->Typ) {
	case ' ': return true; break;
	case 'D':
	case 'P': SetCompileAll(); break;
	case 'F': {
		if (X->OldTxt == 0) return true;  /*don't delete if the record is new*/
		SetCompileAll();
		if (X->isSQL) return true;
		SetMsgPar(X->Name);
		if (!PromptYN(814) || NetFileTest(X) && !PromptYN(836)) {
			return false;
		}
		if (X->CatIRec != 0) {
			WrCatField(X->CatIRec, CatFileName, "");
			if (!PromptYN(815)) return true;
			RdCatPathVol(X->CatIRec);
			TestMountVol(CPath[1]);
		}
		else { CDir = "";
			CName = X->Name; CExt = X->Ext; }
		MyDeleteFile(CDir + CName + CExt);
		CExtToT();
		MyDeleteFile(CPath);
		if (X->FTyp == 'X') { CExtToX(); MyDeleteFile(CPath); } }
	default: ChptTF->CompileProc = true; break;
	}
	return true;
}

bool ChptDel()
{
	RdbRecVars New;
	if (!IsCurrChpt()) { return true; }
	GetRdbRecVars(E->NewRecPtr, &New);
	return ChptDelFor(&New);
}

bool IsDuplFileName(pstring name)
{
	WORD I; pstring n; pstring e; void* cr;
	auto result = true;
	if (SEquUpcase(name, Chpt->Name)) return result;
	cr = CRecPtr;
	CRecPtr = GetRecSpace();
	for (I = 1; I < Chpt->NRecs; I++)
		if (I != CRec()) {
			ReadRec(I);
			if (_ShortS(ChptTyp) == 'F') {
				GetSplitChptName(&n, &e);
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
	FileDPtr FD1, FD2; void* p; void* p2;
	LongStr* s; longint pos; bool b;
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
		if (New.Name != "") { WrLLF10Msg(623); return result; }
	}
	else if (New.Typ != ' ')
		if (not IsIdentifStr(New.Name) || (New.Typ != 'F') && (New.Ext != "")) {
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
		kf = kf->Chain;
		kf->FldD = (FieldDescr*)O(kf->FldD);
	}
}

void* OTb(pstring Nm)
{
	pstring* s = nullptr;
	WORD* sofs = (WORD*)s;
	WORD i;
	s = (pstring*)Tb;
	for (i = 1; i < nTb; i++) {
		if (SEquUpcase(*s, Nm)) goto label1;
		sofs += s->length() + 1;
	}
	nTb++; sz += Nm.length() + 1;
	if (sz > MaxLStrLen) RunError(664);
	s = StoreStr(Nm);
label1:
	return O(s);
}

void* OLinkD(LinkD* Ld)
{
	// TODO: spo��tat, co se m� vr�tit:
	// return ptr(WORD(OTb(Ld->FromFD->Name)), WORD(OTb(Ld->RoleName)));
	return nullptr;
}

void OFrml(FrmlPtr Z)
{
	FileDPtr cf = nullptr; FileDPtr fd1 = nullptr;
	FrmlList fl = nullptr; LinkDPtr ld = nullptr;
	if (Z != nullptr) {
		Z = (FrmlElem*)O(Z);
		/* !!! with Z^ do!!! */
		switch (Z->Op) {
		case _field: { Z->Field = (FieldDescr*)OCF(Z->Field); break; }
		case _access: {
			Z->LD = (LinkD*)OCF(Z->LD);
			cf = CFileF;
			CFileF = Z->File2;
			Z->File2 = (FileD*)OTb(CFileF->Name);
			OFrml(Z->P1);
			CFileF = cf;
			break;
		}
		case _userfunc: {
			Z->FC = (FuncD*)OTb(Z->FC->Name);
			//fl = FrmlList(&FrmlL);
			while (fl->Chain != nullptr) {
				fl->Chain = (FrmlListEl*)O(fl->Chain);
				fl = fl->Chain;
				OFrml(fl->Frml);
			}
			break;
		}
		case _owned: {
			ld = Z->ownLD;
			fd1 = ld->FromFD;
			Z->ownLD = (LinkD*)OLinkD(ld);
			cf = CFileF;
			CFileF = fd1;
			OFrml(Z->ownBool);
			OFrml(Z->ownSum);
			CFileF = cf;
		}
		default: {
			if (Z->Op >= 0x60 && Z->Op <= 0xaf) { OFrml(Z->P1); break; }
			if (Z->Op >= 0xb0 && Z->Op <= 0xef) { OFrml(Z->P1); OFrml(Z->P2); break; }
			if (Z->Op >= 0xf0 && Z->Op <= 0xff) { OFrml(Z->P1); OFrml(Z->P2); OFrml(Z->P3); break; }
		}
		}
	}
}

void WrFDSegment(longint RecNr)
{
	FileDPtr cf = nullptr; StringList s; FieldDPtr f = nullptr; KeyDPtr k = nullptr;
	AddDPtr ad = nullptr; ChkDPtr c = nullptr; LinkDPtr ld = nullptr; WORD n = 0, oldsz = 0;
	void* fdsaved = nullptr; void* p2 = nullptr; LongStr* ss = nullptr;
	ImplDPtr id = nullptr; LiRootsPtr li = nullptr;

	//sz = AbsAdr(HeapPtr) - AbsAdr(CFile);
	if (sz > MaxLStrLen) RunError(664);
	oldsz = sz; nTb = 0; //Tb = O(HeapPtr);
	MarkStore2(p2); fdsaved = GetStore2(sz); Move(CFile, fdsaved, sz);
	CFileF = CFile;
	/* !!! with CFile^ do!!! */
	CFile->TF = (TFile*)O(CFile->TF); CFile->XF = (XFile*)O(CFile->XF);
	if (CFile->OrigFD != nullptr) CFile->OrigFD = (FileD*)OTb(CFile->OrigFD->Name);
	s = CFile->ViewNames;
	while (s->Chain != nullptr) {
		s->Chain = (StringListEl*)O(s->Chain);
		s = s->Chain;
	}
	f = CFile->FldD;
	while (f->Chain != nullptr) {
		f->Chain = (FieldDescr*)O(f->Chain);
		f = f->Chain;
		if (f->Flg & f_Stored == 0) OFrml(f->Frml);
	}
	k = CFile->Keys; while (k->Chain != nullptr) {
		k->Chain = (XKey*)O(k->Chain); k = k->Chain;
		k->Alias = (pstring*)O(k->Alias);
		OKF((KeyFldD*)(&k->KFlds));
	}
	ad = CFile->Add; while (ad->Chain != nullptr) {
		ad->Chain = (AddD*)O(ad->Chain); ad = ad->Chain;
		ad->LD = (LinkD*)O(ad->LD); OFrml(ad->Frml);
		if (ad->Assign) OFrml(ad->Bool);
		else {
			c = ad->Chk;
			if (c != nullptr) {
				ad->Chk = (ChkD*)O(c);
				c->HelpName = (pstring*)O(c->HelpName);
			}
		}
		cf = CFileF; CFileF = ad->File2;
		ad->File2 = (FileD*)OTb(CFileF->Name);
		ad->Field = (FieldDescr*)OCF(ad->Field);
		if (!ad->Assign && (c != nullptr)) { OFrml(c->Bool); OFrml(c->TxtZ); }
		CFileF = cf;
	}
	ld = (LinkD*)O(LinkDRoot); n = CFile->nLDs;
	while (n > 0) {
		OKF(KeyFldDPtr(ld->Args)); cf = CFileF; CFileF = ld->ToFD;
		ld->ToFD = (FileD*)OTb(CFileF->Name); ld->ToKey = (XKey*)OCF(ld->ToKey);
		CFileF = cf; n--;
		if (n > 0) { ld->Chain = (LinkD*)O(ld->Chain); ld = ld->Chain; }
	}
	CFile->Chain = (FileD*)O(LinkDRoot);
	CFile->IRec = FDVersion;
	CFile->Handle = (FILE*)Tb;

	if (CFile->LiOfs > 0) {
		li = (LiRoots*)Normalize(AbsAdr(CFile) + CFile->LiOfs);
		id = ImplDPtr(&li->Impls); while (id->Chain != nullptr) {
			id->Chain = (ImplD*)O(id->Chain); id = id->Chain;
			id->FldD = (FieldDescr*)O(id->FldD);
			OFrml(id->Frml);
		}
		c = ChkDPtr(&li->Chks); while (c->Chain != nullptr) {
			c->Chain = (ChkD*)O(c->Chain); c = c->Chain;
			c->HelpName = (pstring*)O(c->HelpName);
			OFrml(c->TxtZ);
			OFrml(c->Bool);
		}
	}
	ss = nullptr; // Ptr(PtrRec(CFile).Seg - 1, 14);
	ss->LL = sz; cf = CFile; CFile = Chpt;
	StoreChptTxt(ChptOldTxt, ss, false); WriteRec(RecNr);
	CFile = cf; Move(fdsaved, CFile, oldsz);
	ReleaseStore2(p2);
}

bool RdFDSegment(WORD FromI, longint Pos)
{
	integer Sg, SgF;/*CFile-Seg*/
	StringList s; FieldDPtr f; KeyDPtr k; AddDPtr ad; ChkDPtr c;
	LinkDPtr ld, ld1; integer n; LongStr* ss; pstring lw; void* p;
	auto result = false; lw = LexWord;
	//	AlignLongStr(); ss = CFile->TF->Read(1, Pos);
	//	if ((ss->LL <= sizeof(FileD))) return result;
	//	if (CRdb->Encrypted) CodingLongStr(ss);
	//	Sg = uintptr_t(ss + 1); SgF = Sg; CFile = (FileD*)Sg;
	//	/* !!! with CFile^ do!!! */
	//	if (CFile->IRec != FDVersion) return result;  result = true;
	//	Tb = &CFile->Handle; CFile->Handle = nullptr;
	//	if (CFile->TF != nullptr) Pr(CFile->TF).Seg = Sg;
	//	if (CFile->XF != nullptr) Pr(CFile->XF).Seg = Sg;
	//	f = FieldDPtr(CFile->FldD);
	//	while (f->Chain != nullptr) {
	//		Pr(f->Chain).Seg = Sg; f = f->Chain;
	//		if (f->Flg && f_Stored == 0) SgFrml(f->Frml);
	//	}
	//	CFile->OrigFD = GetFD(CFile->OrigFD, false);
	//	s = StringList(CFile->ViewNames); while (s->Chain != nullptr) {
	//		Pr(s->Chain).Seg = Sg; s = s->Chain;
	//	}
	//	k = KeyDPtr(CFile->Keys); while (k->Chain != nullptr) {
	//		Pr(k->Chain).Seg = Sg; k = k->Chain; Pr(k->Alias).Seg = Sg;
	//		SgKF(KeyFldDPtr(&k->KFlds));
	//	}
	//	ad = AddDPtr(&Add); while (ad->Chain != nullptr) {
	//		Pr(ad->Chain).Seg = Sg; ad = ad->Chain;
	//		if (ad->LD != nullptr) Pr(ad->LD).Seg = Sg; SgFrml(ad->Frml);
	//		if (ad->Assign) SgFrml(ad->Bool); else if (ad->Chk != nullptr) {
	//			Pr(ad->Chk).Seg = Sg; /* !!! with ad->Chk^ do!!! */
	//			if (HelpName != nullptr) Pr(HelpName).Seg = Sg;
	//		}
	//		ad->File2 = GetFD(ad->File2, true);
	//		SgF = Pr(ad->File2).Seg;
	//		Pr(ad->Field).Seg = SgF;
	//		if (!ad->Assign) {
	//			c = ad->Chk;
	//			if (c != nullptr) { SgFrml(c->Bool); SgFrml(c->TxtZ); }
	//		}
	//		SgF = Sg;
	//	}
	//	ld1 = LinkDRoot; ld = LinkDPtr(CFile->Chain); Pr(ld).Seg = Sg;
	//	n = CFile->nLDs;
	//	if (n > 0) LinkDRoot = ld;
	//	while (n > 0) {
	//		if (n == 1) ld->Chain = ld1;
	//		else Pr(ld->Chain).Seg = Sg;
	//		SgKF(KeyFldDPtr(@ld->Args));
	//		ld->FromFD = CFile;
	//		ld->ToFD = GetFD(ld->ToFD, true);
	//		Pr(ld->ToKey).Seg = Pr(ld->ToFD).Seg;
	//		ld = ld->Chain; n--;
	//	}
	//	CFile->CatIRec = GetCatIRec(CFile->Name, CFile->Typ = '0'/*multilevel*/);
	//	CFile->ChptPos.R = CRdb; CFile->ChptPos.IRec = FromI;
	//#ifdef FandSQL
	//	SetIsSQLFile;
	//#endif
	//	CompileRecLen();
	//	p = Tb; if (CFile->LiOfs > 0) {
	//		p = Normalize(AbsAdr(CFile) + CFile->LiOfs); CFile->LiOfs = 0;
	//	}
	//	ReleaseStore(p);
	//	LexWord = lw;
	return result;
}

WORD FindHelpRecNr(FileDPtr FD, pstring txt)
{
	FileDPtr cf; void* cr; LockMode md; FieldDPtr NmF, TxtF; WORD i;
	pstring nm(80);
	WORD result = 0;
	ConvToNoDiakr((WORD*)txt[1], txt.length(), fonts.VFont);
	cf = CFile; cr = CRecPtr;
	CFile = FD;
	CRecPtr = GetRecSpace();
	md = NewLMode(RdMode); if (CFile->Handle == nullptr) goto label1;
	NmF = CFile->FldD; TxtF = NmF->Chain;
	for (i = 1; i < CFile->NRecs; i++) {
		ReadRec(i); nm = TrailChar(' ', _ShortS(NmF));
		ConvToNoDiakr((WORD*)nm[1], nm.length(), fonts.VFont);
		if (EqualsMask(&txt[1], txt.length(), nm)) {
			while ((i < CFile->NRecs) && (_T(TxtF) == 0)) { i++; ReadRec(i); }
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
	pstring txt;
	auto result = false; txt = "";
	ww.PromptLL(153, &txt, 1, true);
	if ((txt.length() == 0) || (KbdChar == _ESC_)) return result;
	N = FindHelpRecNr(CFile, txt);
	if (N != 0) result = true;
	return result;
}

void EditHelpOrCat(WORD cc, WORD kind, pstring txt)
{
	FileDPtr FD; EditOpt* EO; WORD i, n;
	WORD nCat = 1; WORD iCat = 1; WORD nHelp = 1; WORD iHelp = 1;
	struct niFrml { char Op; double R; } nFrml{ 0,0 }, iFrml{ 0,0 };
	if (cc == _AltF2_) {
		FD = CRdb->HelpFD; if (kind == 1) FD = CFile->ChptPos.R->HelpFD;
		if (FD == nullptr) return;
		if (kind == 0) { i = iHelp; n = nHelp; }
		else {
			i = 3; n = FindHelpRecNr(FD, txt);
			if (n == 0) KbdBuffer = txt; // TODO: tady m� b�t KbdBuffer:=#0#60+txt
		}
	}
	else { FD = CatFD; i = iCat; n = nCat; }
	if (kind != 2) WrEStatus();
	EO = GetEditOpt(); EO->Flds = AllFldsList(FD, false);
	EO->WFlags = EO->WFlags | WPushPixel;
	if ((kind == 0) || (n != 0)) {
		iFrml.R = i; nFrml.R = n;
		EO->StartRecNoZ = (FrmlPtr)(&nFrml); EO->StartIRecZ = (FrmlPtr)(&iFrml);
	}
	EditDataFile(FD, EO); ReleaseStore(EO);
	if (cc == _AltF2_) { nHelp = EdRecNo; iHelp = EdIRec; }
	else { ResetCatalog(); nCat = EdRecNo; iCat = EdIRec; }
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
	pos = ChptTF->Store(S);
	if (LicNr == 0) T_(F, pos); else T_(F, pos + LicNr);
	ReleaseStore(p);
}

void SgKF(KeyFldDPtr kf, WORD Sg)
{
	while (kf->Chain != nullptr) {
		// PtrRec(kf->Chain).Seg = Sg; kf = kf->Chain; PtrRec(kf->FldD).Seg = Sg;
	}
}

FileD* GetFD(void* p, bool WithSelf, WORD Sg)
{
	if (p != nullptr) {
		LexWord = *(pstring*)(p);
		if (WithSelf && EquUpcase(CFile->Name)) p = CFile;
		else p = FindFileD();
	}
	return (FileD*)p;
}

FuncD* GetFC(void* p, WORD Sg)
{
	FuncD* fc;
	LexWord = *(pstring*)(p);
	fc = FuncDRoot; while (fc != nullptr) {
		if (EquUpcase(fc->Name)) goto label1;
		fc = fc->Chain;
	}
label1:
	return fc;
}

LinkD* GetLinkD(void* P, WORD Sg)
{
	LinkD* ld; FileD* fd;
	LexWord = *(pstring*)(P);
	fd = (FileD*)FindFileD;
	ld = LinkDRoot;
	while (ld != nullptr) {
		if ((ld->FromFD == fd) && (ld->RoleName == *((pstring*)(WORD(P)))))
		{
			return ld;
		} ld = ld->Chain;
	}
	return nullptr;
}

void SgFrml(FrmlPtr Z, WORD Sg, WORD SgF)
{
	FrmlList fl = nullptr; WORD SgFold;
	if (Z != nullptr) {
		Z = (FrmlElem*)(Sg);
		/* !!! with Z^ do!!! */
		switch (Z->Op) {
		case _field: Z->Field = (FieldDescr*)SgF; break;
		case _access: {
			if (Z->LD != nullptr) Z->LD = (LinkD*)SgF;
			Z->File2 = GetFD(Z->File2, Z->LD != nullptr, Sg);
			SgFold = SgF;
			SgF = *(WORD*)Z->File2;
			SgFrml(Z->P1, Sg, SgF);
			SgF = SgFold;
			break; }
		case _userfunc: {
			Z->FC = GetFC(Z->FC, Sg);
			//fl = FrmlList(FrmlL);
			while (fl->Chain != nullptr) {
				fl->Chain = (FrmlListEl*)Sg;
				fl = fl->Chain;
				SgFrml(fl->Frml, Sg, SgF);
			}
			break;
		}
		case _owned: {
			Z->ownLD = GetLinkD(Z->ownLD, Sg);
			SgFold = SgF;
			//SgF = (Z->ownLD->FromFD);
			SgFrml(Z->ownBool, Sg, SgF);
			SgFrml(Z->ownSum, Sg, SgF);
			SgF = SgFold;
			break; }
		default: {
			if (Z->Op >= 0x60 && Z->Op <= 0xaf) { SgFrml(Z->P1, Sg, SgF); break; }
			if (Z->Op >= 0xb0 && Z->Op <= 0xef) { SgFrml(Z->P1, Sg, SgF); SgFrml(Z->P2, Sg, SgF); break; }
			if (Z->Op >= 0xf0 && Z->Op <= 0xff) { SgFrml(Z->P1, Sg, SgF); SgFrml(Z->P2, Sg, SgF); SgFrml(Z->P3, Sg, SgF); break; }
		}
		}
	}
}

void SetChptFldDPtr()
{
	ChptTF = Chpt->TF;
	ChptTxtPos = Chpt->FldD; ChptVerif = ChptTxtPos->Chain;
	ChptOldTxt = ChptVerif->Chain; ChptTyp = ChptOldTxt->Chain;
	ChptName = ChptTyp->Chain; ChptTxt = ChptName->Chain;
}

void SetRdbDir(char Typ, pstring* Nm)
{
	RdbD* r; RdbD* rb; pstring d;

	r = CRdb; rb = r->ChainBack;
	if (rb == nullptr) TopRdb = *r; CVol = "";
	if (Typ == '\\') {
		rb = &TopRdb; CRdb = rb; CFile->CatIRec = GetCatIRec(*Nm, false);
		CRdb = r;
	}
	if (CFile->CatIRec != 0) {
		CPath = RdCatField(CFile->CatIRec, CatPathName);
		if (CPath[2] != ':') {
			d = rb->RdbDir; if (CPath[1] == '\\') CPath = copy(d, 1, 2) + CPath;
			else {
				AddBackSlash(d); CPath = d + CPath;
			}
		}
		FSplit(CPath, CDir, CName, CExt); DelBackSlash(CDir);
	}
	else if (rb == nullptr) CDir = TopRdbDir; else {
		CDir = rb->RdbDir; AddBackSlash(CDir); CDir = CDir + CFile->Name;
	}
	/* !!! with r^ do!!! */ {
		r->RdbDir = CDir; if (TopDataDir == "") r->DataDir = CDir;
		else if (rb == nullptr) r->DataDir = TopDataDir;
		else {
			d = rb->DataDir; AddBackSlash(d); r->DataDir = d + CFile->Name;
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

void CreateOpenChpt(pstring* Nm, bool create)
{
	wwmix ww;

	RdbDPtr R; bool top; pstring p; pstring s; TFilePtr oldChptTF;
	integer i, n; pstring nr(10); pstring Nm1(8); FileUseMode um;

	top = CRdb = nullptr; FileDRoot = nullptr;
	R = (RdbD*)GetZStore(sizeof(*R));
	oldChptTF = ChptTF;
	R->ChainBack = CRdb; R->OldLDRoot = LinkDRoot; R->OldFCRoot = FuncDRoot;
	MarkStore2(R->Mark2);
	RdMsg(51); s = MsgLine; RdMsg(48); val(MsgLine, n, i);
	str(TxtCols - n, nr); s = s + nr; SetInpStr(s);
	if ((Nm[1] == '\\')) Nm1 = Nm->substr(2, 8);
	else Nm1 = *Nm;
	RdFileD(Nm1, '0', ""); /*old CRdb for GetCatIRec*/
	R->FD = CFile; CRdb = R; CFile->RecPtr = GetRecSpace();
	SetRdbDir((*Nm)[1], &Nm1);
	p = CDir + Nm1 + ".RDB";
	CFile->Drive = TestMountVol(CPath[1]);
	SetChptFldDPtr(); if (!spec.RDBcomment) ChptTxt->L = 1;
	SetMsgPar(p);
	if (top) { UserName = ""; UserCode = 0; AccRight = 0; goto label2; }
	CRdb->HelpFD = CRdb->ChainBack->HelpFD;
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
	if (IsTestRun || !create) um = Exclusive; else um = RdOnly;
	if (OpenF(um)) {
		if (ChptTF->CompileAll) ResetRdOnly();
		else if (!top && (ChptTF->TimeStmp < oldChptTF->TimeStmp)) {
			ResetRdOnly(); SetCompileAll();
		}
		goto label3;
	}
	if (!create || (top && !IsTestRun)) RunError(631);
	OpenCreateF(Exclusive); SetCompileAll();
label3:
	if (ww.HasPassWord(Chpt, 1, "")) CRdb->Encrypted = false;
	else CRdb->Encrypted = true;
}

void CloseChpt()
{
	void* p; void* p2; bool del; pstring d; WORD i;
	if (CRdb == nullptr) return; ClearHelpStkForCRdb();
	SaveFiles(); del = Chpt->NRecs = 0;
	d = CRdb->RdbDir; CloseFAfter(FileDRoot);
	LinkDRoot = CRdb->OldLDRoot; FuncDRoot = CRdb->OldFCRoot;
	p = CRdb; p2 = CRdb->Mark2; CRdb = CRdb->ChainBack;
	ReleaseBoth(p, p2);
	if (CRdb != nullptr) {
		FileDRoot = CRdb->FD; SetChptFldDPtr(); ChDir(CRdb->RdbDir);
		if (del) {
			RmDir(d);
			if (IOResult() != 0) {
				SetMsgPar(d); WrLLF10Msg(621);
			}
		}
	}
	else {
		ChDir(OldDir); for (i = 1; i < FloppyDrives; i++) ReleaseDrive(i);
	}
}

void GoCompileErr(WORD IRec, WORD N)
{
	IsCompileErr = true; InpRdbPos.R = CRdb; InpRdbPos.IRec = IRec;
	CurrPos = 0; RdMsg(N); GoExit();
}

void ClearXFUpdLock()
{
	if (CFile->XF != nullptr) CFile->XF->UpdLockCnt = 0;
}

FileD* FindFD()
{
	FileD* FD = nullptr; pstring FName(12); pstring d; pstring name; pstring ext;
	FName = TrailChar(' ', _ShortS(ChptName));
	FSplit(FName, d, name, ext);
	FD = FileDRoot;
	while (FD != nullptr) {
		if (SEquUpcase(FD->Name, name)) break;
		FD = FD->Chain;
	}
	return FD;
}

void Diagnostics(void* MaxHp, longint Free, FileD* FD)
{
	void* p; pstring s1(8); pstring s2(8);
	pstring s3(8); pstring s4(8); RdbD* r;
	r = CRdb;
	while (r->ChainBack != nullptr) r = r->ChainBack;
	str(AbsAdr(CRdb) - AbsAdr(r), s1);  /* BYTEs on top of this RDB */
	str(AbsAdr(0/*HeapPtr*/) - AbsAdr(E->AfterE), s2); /* BYTEs for FileD's */
	if (FD != nullptr) {
		p = FD->Chain;
		if (p == nullptr) p = 0 /*HeapPtr*/; str(AbsAdr(p) - AbsAdr(FD), s3);
	}
	else str(AbsAdr(MaxHp) - AbsAdr(0 /*HeapPtr*/), s3);  /* BYTEs of this chapter */
	str(Free, s4); Set4MsgPar(s1, s2, s3, s4); WrLLF10Msg(136);
}

bool CompRunChptRec(WORD CC)
{
	pstring STyp(1); void* p = nullptr; void* p2 = nullptr; void* MaxHp = nullptr;
	ExitRecord er; EditD* OldE = nullptr;
	RdbPos RP; longint Free; bool WasError = false, WasGraph = false, uw = false, mv = false;
	FileD* FD = nullptr; FileD* lstFD = nullptr; LinkD* oldLd = nullptr; LinkD* ld = nullptr;
	EditOpt* EO = nullptr; WORD nStrm = 0;
	auto result = false;

	OldE = E; MarkBoth(p, p2); WrEStatus(); //NewExit(Ovr(), er);
	goto label2;
	IsCompileErr = false; uw = false; mv = MausVisible;
	lstFD = (FileD*)LastInChain(FileDRoot);
	oldLd = LinkDRoot;
	WasError = true; WasGraph = IsGraphMode;
	FD = nullptr; STyp = _ShortS(ChptTyp); RP.R = CRdb; RP.IRec = CRec();
#ifdef FandSQL
	nStrm = nStreams;
#endif
	if (CC == _AltF9_) {
		if (FindChpt('P', "MAIN", true, &RP)) goto label1;
		else WrLLF10Msg(58);
	}
	else
		switch (STyp[1]) {
		case 'F': {
			FD = FindFD();
			if ((FD != nullptr) && (CC == _CtrlF9_)) {
				EO = GetEditOpt(); CFile = FD; EO->Flds = AllFldsList(CFile, false);
				if (SelFldsForEO(EO, nullptr)) EditDataFile(FD, EO);
			}
			break;
		}
		case 'E': {
			if (CC == _CtrlF9_) {
				EO = GetEditOpt(); EO->FormPos = RP; EditDataFile(nullptr, EO);
			}
			else { PushEdit(); RdFormOrDesign(nullptr, nullptr, RP); }
			break;
		}
		case 'M': {
			SetInpTT(RP, true);
			ReadMerge();
			if (CC == _CtrlF9_) RunMerge();
			break;
		}
		case 'R': {
			SetInpTT(RP, true); ReadReport(nullptr);
			if (CC == _CtrlF9_) { RunReport(nullptr); SaveFiles(); ViewPrinterTxt(); }
			break;
		}
		case 'P': {
			if (CC == _CtrlF9_) {
			label1:
				if (UserW != 0) { PopW(UserW); uw = true; }
				RunMainProc(RP, CRdb->ChainBack = nullptr);
			}
			else {
				lstFD = (FileD*)LastInChain(FileDRoot); ld = LinkDRoot;
				SetInpTT(RP, true); ReadProcHead(); ReadProcBody();
				lstFD->Chain = nullptr; LinkDRoot = ld;
			}
			break;
		}
#ifdef FandProlog
		case 'L': if (CC = _CtrlF9_) { TextAttr = ProcAttr; ClrScr; RunProlog(RP, nullptr); break; }
#endif
		}
	WasError = false;
label2:
	MaxHp = 0;
	ReleaseStore2(p2); Free = StoreAvail(); RestoreExit(er);
	RunMsgClear();
	if (WasError) {
#ifdef FandSQL
		ShutDownStreams(nStrm);
#endif

		TextAttr = colors.uNorm;
		if (IsGraphMode && !WasGraph) ScrTextMode(false, false);
		else ClrScr();
	}
	if (uw) { UserW = 0;/*mem overflow*/UserW = PushW(1, 1, TxtCols, TxtRows); }
	SaveFiles(); if (mv) ShowMouse();
	if (WasError) ForAllFDs(ClearXFUpdLock);
	CFile = lstFD->Chain;
	while (CFile != nullptr) {
		CloseFile(); CFile = CFile->Chain;
	}
	lstFD->Chain = nullptr;
	LinkDRoot = oldLd;
	ReleaseBoth(p, p2); E = OldE; RdEStatus(); CRdb = RP.R; PrevCompInp = nullptr;
	ReadRec(CRec());
	if (IsCompileErr) result = false;
	else {
		result = true;
		if (WasError) return result;
		B_(ChptVerif, false);
		WriteRec(CRec());
		if (CC == _CtrlF8_) Diagnostics(MaxHp, Free, FD);
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

WORD CompileMsgOn(WORD* Buf, longint& w)
{
	pstring s(12);
	WORD result = 0;
	RdMsg(15);
	if (IsTestRun) {
		w = PushWFramed(0, 0, 30, 4, colors.sNorm, MsgLine, "", WHasFrame + WDoubleFrame + WShadow);
		RdMsg(117);
		s = GetDLine(&MsgLine[1], MsgLine.length(), '/', 1);
		GotoXY(3, 2);
		printf("%s", s.c_str()); result = s.length();
		GotoXY(3, 3);
		printf("%s", GetDLine(&MsgLine[1], MsgLine.length(), '/', 2).c_str());
	}
	else {
		ScrRdBuf(0, TxtRows - 1, Buf, 40); w = 0;
		result = 0;
		ScrClr(0, TxtRows - 1, MsgLine.length() + 2, 1, ' ', colors.zNorm);
		ScrWrStr(1, TxtRows - 1, MsgLine, colors.zNorm);
	}
	return result;
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
	CloseH(h);
	return 0;
}

void* RdF(pstring* FileName)
{
	pstring d; pstring name; pstring ext; char FDTyp; pstring s;
	FieldDPtr IdF, TxtF;  integer i, n; pstring nr(10);
	FSplit(*FileName, d, name, ext); FDTyp = ExtToTyp(ext);
	if (FDTyp == '0') {
		RdMsg(51); s = MsgLine; RdMsg(49); val(MsgLine, n, i);
		str(TxtCols - n, nr); s = s + nr; SetInpStr(s);
	}
	else SetInpTTPos(_T(ChptTxt), CRdb->Encrypted);
	return RdFileD(name, FDTyp, ext);
}

bool EquStoredF(FieldDPtr F1, FieldDPtr F2)
{
	auto result = false;
label1:
	while ((F1 != nullptr) && (F1->Flg && f_Stored == 0)) F1 = F1->Chain;
	while ((F2 != nullptr) && (F2->Flg && f_Stored == 0)) F2 = F2->Chain;
	if (F1 == nullptr)
	{
		if (F2 != nullptr) return result;
		result = true;
		return result;
	}
	if ((F2 == nullptr) || !FldTypIdentity(F1, F2) ||
		(F1->Flg && !f_Mask != F2->Flg && !f_Mask)) return result;
	F1 = F1->Chain; F2 = F2->Chain;
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
	pstring s; ExitRecord er; pstring p;
	auto result = false;
	//NewExit(Ovr(), er);
	goto label1;
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
	SetCPathVol(); CExtToT(); RenameFile56(CPath, p, false);
	result = true;
	return result;
label1:
	RestoreExit(er); CFile = FDOld; CloseFile(); CFile = FDNew; DeleteF();
	SpecFDNameAllowed = false; result = false;
	return result;
}

bool EquKeys(KeyD* K1, KeyD* K2)
{
	auto result = false; while (K1 != nullptr) {
		if ((K2 == nullptr) || (K1->Duplic != K2->Duplic)) return result;
		KeyFldD* KF1 = K1->KFlds;
		KeyFldD* KF2 = K2->KFlds;
		while (KF1 != nullptr) {
			if ((KF2 == nullptr) || (KF1->CompLex != KF2->CompLex) || (KF1->Descend != KF2->Descend)
				|| (KF1->FldD->Name != KF2->FldD->Name)) return result;
			KF1 = KF1->Chain; KF2 = KF2->Chain;
		}
		if (KF2 != nullptr) return result;
		K1 = K1->Chain; K2 = K2->Chain;
	}
	if (K2 != nullptr) return result;
	result = true;
	return result;
}

bool MergeOldNew(bool Veriflongint, bool Pos)
{
	pstring Name(20);
	LinkD* ld = LinkDRoot;
	auto result = false;
	FileD* FDOld;
	FileD* FDNew = CFile; SetCPathVol(); Name = FDNew->Name; FDNew->Name = '@';
	CFile = Chpt; if (!RdFDSegment(0, Pos)) goto label1;
	ChainLast(FileDRoot, CFile);
	FDOld = CFile; FDOld->Name = Name;
	if ((FDNew->Typ != FDOld->Typ) || !EquStoredF(FDNew->FldD, FDOld->FldD)
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
	Move(&Name, &FDNew->Name, Name.length() + 1);
	CFile = FDNew;
	CRecPtr = Chpt->RecPtr;
	return result;
}

void CompileMsgOff(WORD* Buf, longint& w)
{
	if (w != 0) PopW(w); else ScrWrBuf(0, TxtRows - 1, Buf, 40);
}

bool CompileRdb(bool Displ, bool Run, bool FromCtrlF10)
{
	WORD Buf[40];
	longint w;
	longint I = 0, J, OldTxt, Txt, OldCRec; pstring STyp(1); char Typ;
	pstring Name(12); pstring dir; pstring nm; pstring ext;
	bool Verif, FDCompiled, Encryp; char Mode;  RdbPos RP;
	void* p = nullptr; void* p1 = nullptr; void* p2 = nullptr;
	ExitRecord er; EditD* OldE = nullptr; WORD lmsg;
	LinkD* ld = nullptr;
	LongStr* RprtTxt = nullptr; bool top;
	FileD* lstFD = nullptr;
	auto result = false;

	OldE = E; MarkBoth(p, p2); p1 = p; //NewExit(Ovr, er);
	goto label1;
	IsCompileErr = false; FDCompiled = false; OldCRec = CRec(); RP.R = CRdb;
	top = CRdb->ChainBack = nullptr;
	if (top) {
		UserName[0] = 0; UserCode = 0; UserPassWORD[0] = 0; AccRight[0] = 0;
		if (ChptTF->CompileAll || CompileFD) Switches[0] = 0;
	}
	lmsg = CompileMsgOn(Buf, w);
	CRecPtr = Chpt->RecPtr; Encryp = CRdb->Encrypted;
	for (I = 1; I < Chpt->NRecs; I++) {
		ReadRec(I); RP.IRec = I;
		Verif = _B(ChptVerif); STyp = _ShortS(ChptTyp); Typ = STyp[1];
		Name = TrailChar(' ', _ShortS(ChptName)); Txt = _T(ChptTxt);
		if (Verif && ((ChptTF->LicenseNr != 0) || Encryp || (Chpt->UMode == RdOnly))) GoCompileErr(I, 647);
		if (Verif || ChptTF->CompileAll || FromCtrlF10 || (Typ == 'U') ||
			(Typ == 'F' || Typ == 'D') && CompileFD ||
			(Typ == 'P') && ChptTF->CompileProc) {
			OldTxt = _T(ChptOldTxt); InpRdbPos = RP;
			if (IsTestRun) {
				ClrScr(); GotoXY(3 + lmsg, 2); printf("%*i", 4, I);
				GotoXY(3 + lmsg, 3);
				printf("%*s%*s", 4, STyp.c_str(), 14, _ShortS(ChptName).c_str());
				if (!(Typ == ' ' || Typ == 'D' || Typ == 'U')) { /* dupclicate name checking */
					for (J = 1; J < I - 1; J++) {
						ReadRec(J);
						if ((STyp == _ShortS(ChptTyp)) && SEquUpcase(Name, TrailChar(' ', _ShortS(ChptName)))) GoCompileErr(I, 649);
					}
					ReadRec(I);
				}
			}
			switch (Typ) {
			case 'F': {
				FDCompiled = true; ld = LinkDRoot; MarkStore(p1);
				FSplit(Name, dir, nm, ext);
				if ((Txt == 0) && IsTestRun) {
					SetMsgPar(Name);
					if (SEquUpcase(ext, ".DBF") && PromptYN(39)) {
						T_(ChptOldTxt, 0); OldTxt = 0; MakeDbfDcl(nm);
						Txt = _T(ChptTxt); WriteRec(I);
					}
				}
#ifndef FandSQL
				if (SEquUpcase(ext, ".SQL")) GoCompileErr(I, 654);
#endif
				if (Verif || ChptTF->CompileAll || (OldTxt == 0)) {
				label2:
					p1 = RdF(&Name); WrFDSegment(I);
					if (CFile->IsHlpFile) CRdb->HelpFD = CFile;
					if (OldTxt > 0)
						MergeOldNew(Verif, OldTxt); ReleaseStore(p1);
					CFile = Chpt;
					if (ChptTF->LicenseNr == 0) ChptTF->Delete(OldTxt);
					else if (OldTxt != 0) ChptTF->Delete(OldTxt - ChptTF->LicenseNr);
				}
				else if
					(!RdFDSegment(I, OldTxt)) {
					LinkDRoot = ld; ReleaseStore(p1); CFile = Chpt; goto label2;
				}
				else
				{
					ChainLast(FileDRoot, CFile); MarkStore(p1);
					if (CFile->IsHlpFile) CRdb->HelpFD = CFile;
				}
				break;
			}
			case 'M': { SetInpTTPos(Txt, Encryp); ReadMerge(); break; }
			case 'R': {
				if ((Txt == 0) && IsTestRun) {
					RprtTxt = SelGenRprt(Name); CFile = Chpt; if (RprtTxt = nullptr) GoCompileErr(I, 1145);
					LongS_(ChptTxt, RprtTxt); WriteRec(I);
				}
				else { SetInpTTPos(Txt, Encryp); ReadReport(nullptr); }
				break;
			}
			case 'P': {
				lstFD = (FileD*)LastInChain(FileDRoot); ld = LinkDRoot;
				SetInpTTPos(Txt, Encryp); ReadProcHead(); ReadProcBody();
				lstFD->Chain = nullptr; LinkDRoot = ld;
				break;
			}
			case 'E': { PushEdit(); RdFormOrDesign(nullptr, nullptr, RP); E = OldE; break; }
			case 'U': {
				if (!top || (I > 1)) GoCompileErr(I, 623);
				if (Txt != 0) {
					ResetCompilePars(); SetInpTTPos(Txt, Encryp);
					RdUserId(not IsTestRun || (ChptTF->LicenseNr != 0)); MarkStore(p1);
				}
				break;
			}
			case 'D': { ResetCompilePars(); SetInpTTPos(Txt, Encryp); ReadDeclChpt();
				MarkStore(p1);
				break;
			}
#ifdef FandProlog
			case 'L': { SetInpTTPos(Txt, Encryp); ReadProlog(I); break; }
#endif
			}
		}
	}
	ReleaseBoth(p1, p2); CFile = Chpt; CRecPtr = Chpt->RecPtr;
	if (Verif) { ReadRec(I); B_(ChptVerif, false); WriteRec(I); }
	/* !!! with ChptTF^ do!!! */
	if (ChptTF->CompileAll || ChptTF->CompileProc) {
		ChptTF->CompileAll = false; ChptTF->CompileProc = false; SetUpdHandle(ChptTF->Handle);
	}
	CompileFD = false; result = true; RestoreExit(er);
	if (!Run) { CRecPtr = E->NewRecPtr; ReadRec(CRec()); }
	CompileMsgOff(Buf, w);
#ifdef FandSQL
	if (top && (Strm1 != nullptr)) Strm1->Login(UserName, UserPassWORD);
#endif
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
		if (InpRdbPos.IRec == 0) SetMsgPar(""); else SetMsgPar(InpRdbPos.R->FD->Name);
		WrLLF10Msg(622); Brk = 0; return;
	}
	if (CurrPos == 0) {
		DisplEditWw();
		GotoRecFld(InpRdbPos.IRec, E->FirstFld->Chain);
		SetMsgPar(s); WrLLF10Msg(110); Brk = 0; return;
	}
	CFld = &E->LastFld; SetNewCRec(InpRdbPos.IRec, true);
	R_(ChptTxtPos, integer(CurrPos)); WriteRec(CRec());
	EditFreeTxt(ChptTxt, s, true, Brk);
}

void WrErrMsg630(pstring* Nm)
{
	IsCompileErr = false; SetMsgPar(MsgLine); WrLLF10Msg(110);
	SetMsgPar(*Nm); WrLLF10Msg(630);
}

bool EditExecRdb(pstring* Nm, pstring* ProcNm, Instr* ProcCall)
{
	WORD Brk, cc; void* p; pstring passw(20); bool b;
	ExitRecord er, er2; RdbPos RP; EditOpt* EO;

	auto result = false;
	bool top = CRdb == nullptr;
	bool EscCode = false;
	longint w = UserW; UserW = 0;
	bool wasGraph = IsGraphMode;
#ifdef FandSQL
	if (top) SQLConnect();
#endif
	//NewExit(Ovr(), er);
	goto label9;
	CreateOpenChpt(Nm, true); CompileFD = true;
#ifndef FandRunV
	if (!IsTestRun || (ChptTF->LicenseNr != 0) ||
		!top && CRdb->Encrypted) {
#endif
		MarkStore(p); EditRdbMode = false;
		if (CompileRdb(false, true, false))
			if (FindChpt('P', *ProcNm, true, &RP))
			{
				//NewExit(Ovr(), er2);
				goto label0;
				IsCompileErr = false;
				if (ProcCall != nullptr) { ProcCall->Pos = RP; CallProcedure(ProcCall); }
				else RunMainProc(RP, top);
				result = true; goto label9;
			label0:
				if (IsCompileErr) WrErrMsg630(Nm);
				goto label9;
			}
			else { Set2MsgPar(*Nm, *ProcNm); WrLLF10Msg(632); }
		else if (IsCompileErr) WrErrMsg630(Nm);
#ifndef FandRunV
		if ((ChptTF->LicenseNr != 0) || CRdb->Encrypted
			|| (Chpt->UMode == RdOnly)) goto label9;
		ReleaseFDLDAfterChpt(); ReleaseStore(p);
	}
	else if (!top) UserW = PushW(1, 1, TxtCols, TxtRows);
	EditRdbMode = true; if (CRdb->Encrypted) passw = PassWord(false);
	IsTestRun = true; EO = GetEditOpt();
	EO->Flds = AllFldsList(Chpt, true);
	EO->Flds = EO->Flds->Chain->Chain->Chain;
	NewEditD(Chpt, EO);
	E->MustCheck = true; /*ChptTyp*/
	if (CRdb->Encrypted)
		if (HasPassWord(Chpt, 1, passw)) {
			CRdb->Encrypted = false; SetPassWord(Chpt, 1, ""); CodingCRdb(false);
		}
		else { WrLLF10Msg(629); goto label9; }
	if (!OpenEditWw()) goto label8; result = true; Chpt->WasRdOnly = false;
	if (!top && (Chpt->NRecs > 0))
		if (CompileRdb(true, false, false)) {
			if (FindChpt('P', *ProcNm, true, &RP)) GotoRecFld(RP.IRec, CFld);
		}
		else goto label4;
	else if (ChptTF->IRec <= Chpt->NRecs) GotoRecFld(ChptTF->IRec, CFld);
label1:
	RunEdit(nullptr, Brk);
label2:
	cc = KbdChar; SaveFiles();
	if ((cc == _CtrlF10_) || ChptTF->CompileAll || CompileFD) {
		ReleaseFDLDAfterChpt(); SetSelectFalse(); E->Bool = nullptr;
		ReleaseStore(E->AfterE);
	}
	if (cc == _CtrlF10_) {
		SetUpdHandle(ChptTF->Handle);
		if (!CompileRdb(true, false, true)) goto label3;
		if (!PromptCodeRdb) goto label6; Chpt->WasRdOnly = true; goto label8;
	}
	if (Brk != 0) {
		if (!CompileRdb(Brk = 2, false, false)) {
		label3:
			if (IsCompileErr) goto label4; if (Brk == 1) DisplEditWw();
			GotoRecFld(InpRdbPos.IRec, E->FirstFld->Chain); goto label1;
		}
		if (cc == _AltF2_) {
			EditHelpOrCat(cc, 0, ""); goto label41;
		}
		if (!CompRunChptRec(cc)) {
		label4:
			GotoErrPos(Brk); goto label5;
		}
	label41:
		if (Brk == 1) {
			EditFreeTxt(ChptTxt, "", true, Brk);
		label5:
			if (Brk != 0) goto label2; else goto label1;
		}
		else {
		label6:
			DisplEditWw(); goto label1;
		}
	}
	ChptTF->IRec = CRec(); SetUpdHandle(ChptTF->Handle);
label8:
	PopEdit();
#endif

label9:
	RestoreExit(er);
	if (!wasGraph && IsGraphMode) ScrTextMode(false, false);
	if (UserW != 0) PopW(UserW); UserW = w; RunMsgClear();
	CloseChpt();
#ifdef FandSQL
	if (top) SQLDisconnect;
#endif
	return result;
}

void UpdateCat()
{
	EditOpt* EO = nullptr;
	CFile = CatFD; if (CatFD->Handle == nullptr) OpenCreateF(Exclusive);
	EO = GetEditOpt(); EO->Flds = AllFldsList(CatFD, true);
	EditDataFile(CatFD, EO); ChDir(OldDir); ReleaseStore(EO);
}

void UpdateUTxt()
{
	longint w; WORD TxtPos, LicNr; LongStr* S = nullptr; LongStr* s2 = nullptr;
	bool Srch, Upd, b;
	longint OldPos, Pos; ExitRecord er; void* p = nullptr; void* p1 = nullptr;
	CFile = Chpt; CRecPtr = Chpt->RecPtr; LicNr = ChptTF->LicenseNr; MarkStore(p1);
	if (CFile->NRecs = 0) goto label1; ReadRec(1);
	if (_ShortS(ChptTyp) != 'U') {
	label1:
		WrLLF10Msg(9); /*exit*/;
	}
	w = PushW(1, 1, TxtCols, TxtRows - 1); TxtPos = 1; TextAttr = colors.tNorm;
	OldPos = _T(ChptTxt); S = _LongS(ChptTxt); b = false;
	if (CRdb->Encrypted) CodingLongStr(S); // NewExit(Ovr, er);
	goto label4;
	SetInpLongStr(S, false); MarkStore(p); RdUserId(false); ReleaseStore(p); b = true;
label2:
	SimpleEditText('T', "", "", &S->A, 0x7FFF, S->LL, TxtPos, Upd);
	SetInpLongStr(S, false); MarkStore(p); RdUserId(false); ReleaseStore(p); b = false;
	if (Upd) { StoreChptTxt(ChptTxt, S, true); WriteRec(1); }
label3:
	PopW(w); ReleaseStore(p1); return;
label4:
	if (b) {
		WrLLF10MsgLine(); ReleaseStore(p); if (PromptYN(59)) goto label2; goto label3;
	}
	WrLLF10Msg(9); goto label3;
}

void InstallRdb(pstring n)
{
	wwmix ww;

	ExitRecord er;
	pstring passw(20);
	TMenuBoxS* w = nullptr;
	WORD i;

	//NewExit(Ovr(), er);
	goto label1;
	CreateOpenChpt(&n, false);
	if (!ww.HasPassWord(Chpt, 1, "") && !ww.HasPassWord(Chpt, 2, "")) {
		passw = ww.PassWord(false);
		if (!ww.HasPassWord(Chpt, 2, passw)) {
			WrLLF10Msg(629); goto label1;
		}
	}
	if (Chpt->UMode == RdOnly) { UpdateCat(); goto label1; }
	RdMsg(8);
	//New(w, Init(43, 6, StringPtr(@MsgLine)));
	i = 1;
	w = new TMenuBoxS(43, 6, &MsgLine);
label0:
	i = w->Exec(i);
	switch (i) {
	case 0: { delete w; ReleaseStore(w); goto label1; }
	case 1: { UpdateCat(); goto label0; }
	case 2: UpdateUTxt(); break;
	case 3: ww.SetPassWord(Chpt, 2, ww.PassWord(true)); break;
	}
	SetUpdHandle(ChptTF->Handle); goto label0;
label1:
	RestoreExit(er); CloseChpt();
}