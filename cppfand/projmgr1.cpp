#include "projmgr1.h"

#include "access.h"
#include "common.h"
#include "handle.h"
#include "kbdww.h"
#include "legacy.h"
#include "lexanal.h"
#include "memory.h"
#include "oaccess.h"
#include "rdmix.h"
#include "rdrun.h"
#include "recacc.h"
#include "wwmix.h"

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
	void* p; void* p2; void* cr;
	LinkDPtr ld;

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
		else { CDir = ""; CName = X->Name; CExt = X->Ext; }
		DeleteFile(CDir + CName + CExt);
		CExtToT();
		DeleteFile(CPath);
		if (X->FTyp == 'X') { CExtToX(); DeleteFile(CPath); } }
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

WORD ChptWriteCRec()
{
	RdbRecVars New, Old;
	FileDPtr FD1, FD2; void* p; void* p2;
	LongStr* s; longint pos; bool b;
	integer eq;
	WORD result = 0;
	if (!IsCurrChpt()) return result;
	if (!TestIsNewRec) {
		eq = CompArea(Pchar(CRecPtr) + 2, Pchar(E->OldRecPtr) + 2, CFile->RecLen - 2);
		if (eq == _equ) return result;
	}
	GetRdbRecVars(E->NewRecPtr, &New);
	if (!TestIsNewRec) GetRdbRecVars(E->OldRecPtr, &Old);
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
		if (IsDuplFileName(New.Name)) { WrLLf10Msg(1068); return result;; }
		if ((New.FTyp == '0') && (New.Txt != 0)) { WrLLF10Msg(1083); return result;; }
		if (NetFileTest(&New) && !TestIsNewRec &&
			(Old.Typ == 'F') && (eq != _equ) && !PromptYN(824)) {
			result = 2; return result;
		}
	}
	if ((New.Typ == 'D' || New.Typ == 'I' || New.Typ == 'U')
		|| !TestIsNewRec
		&& (Old.Typ == 'D' || Old.Typ == 'I' || Old.Typ == 'U')) {
		ReleaseFDLDAfterChpt(); SetCompileAll();
	}
	if (TestIsNewRec) { ReleaseFDLDAfterChpt(); goto label2; }
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
		else { if (!Old.isSQL) RenameWithOldExt(); }
	}
label2:
	B_(ChptVerif, true); result = 0;
	SetUpdHandle(ChptTF->Handle);
	return result;
}

bool IsDuplFileName(pstring name)
{
	WORD I; pstring n; pstring e; void* cr;
	auto result = true;
	if (SEquUpcase(name, Chpt->Name)) return result;
	cr = CRecPtr;
	CRecPtr = GetRecSpace();
	for (I = 1; I < Chpt->NRecs; I++)
		if (I != CRec) {
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

void WrFDSegment(longint RecNr)
{
	FileDPtr cf; StringList s; FieldDPtr f; KeyDPtr k; AddDPtr ad;
	ChkDPtr c; LinkDPtr ld; WORD n, oldsz; void* fdsaved; void* p2; LongStr* ss;
	ImplDPtr id; LiRootsPtr li;

	sz = AbsAdr(HeapPtr) - AbsAdr(CFile);
	if (sz > MaxLStrLen) RunError(664);
	oldsz = sz; nTb = 0; Tb = O(HeapPtr);
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
		OKF(KeyFldDPtr(@k->KFlds));
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
		li = Normalize(AbsAdr(CFile) + CFile->LiOfs);
		id = ImplDPtr(@li->Impls); while (id->Chain != nullptr) {
			id->Chain = (ImplD*)O(id->Chain); id = id->Chain;
			id->FldD = (FieldDescr*)O(id->FldD);
			OFrml(id->Frml);
		}
		c = ChkDPtr(@li->Chks); while (c->Chain != nullptr) {
			c->Chain = (ChkD*)O(c->Chain); c = c->Chain;
			c->HelpName = (pstring*)O(c->HelpName);
			OFrml(c->TxtZ);
			OFrml(c->Bool);
		}
	}
	ss = Ptr(PtrRec(CFile).Seg - 1, 14);
	ss->LL = sz; cf = CFile; CFile = Chpt;
	StoreChptTxt(ChptOldTxt, ss, false); WriteRec(RecNr);
	CFile = cf; Move(fdsaved, CFile, oldsz);
	ReleaseStore2(p2);
}

void* OTb(pstring Nm)
{
	pstring* s;
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
	// TODO: spoèítat, co se má vrátit:
	// return ptr(WORD(OTb(Ld->FromFD->Name)), WORD(OTb(Ld->RoleName)));
	return nullptr;
}

void OFrml(FrmlPtr Z)
{
	FileDPtr cf, fd1; FrmlList fl; LinkDPtr ld;
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
			fl = FrmlList(&FrmlL);
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

void OKF(KeyFldDPtr kf)
{
	while (kf->Chain != nullptr) {
		kf->Chain = (KeyFldD*)O(kf->Chain);
		kf = kf->Chain;
		kf->FldD = (FieldDescr*)O(kf->FldD);
	}
}

bool RdFDSegment(WORD FromI, longint Pos)
{
	WORD Sg, SgF;/*CFile-Seg*/
	StringList s; FieldDPtr f; KeyDPtr k; AddDPtr ad; ChkDPtr c;
	LinkDPtr ld, ld1; integer n; LongStr* ss; pstring lw; void* p;
	auto result = false; lw = LexWord;
	AlignLongStr; ss = CFile->TF->Read(1, Pos);
	if ((ss->LL <= sizeof(FileD))) return result;
	if (CRdb->Encrypted) CodingLongStr(ss);
	Sg = Pr(ss).Seg + 1; SgF = Sg; CFile = Ptr(Sg, 0);
	/* !!! with CFile^ do!!! */
	if (CFile->IRec != FDVersion) return result;  result = true;
	Tb = Ptr(Sg, CFile->Handle); CFile->Handle = nullptr;
	if (CFile->TF != nullptr) Pr(CFile->TF).Seg = Sg;
	if (CFile->XF != nullptr) Pr(CFile->XF).Seg = Sg;
	f = FieldDPtr(CFile->FldD);
	while (f->Chain != nullptr) {
		Pr(f->Chain).Seg = Sg; f = f->Chain;
		if (f->Flg && f_Stored == 0) SgFrml(f->Frml);
	}
	CFile->OrigFD = GetFD(CFile->OrigFD, false);
	s = StringList(CFile->ViewNames); while (s->Chain != nullptr) {
		Pr(s->Chain).Seg = Sg; s = s->Chain;
	}
	k = KeyDPtr(CFile->Keys); while (k->Chain != nullptr) {
		Pr(k->Chain).Seg = Sg; k = k->Chain; Pr(k->Alias).Seg = Sg;
		SgKF(KeyFldDPtr(@k->KFlds));
	}
	ad = AddDPtr(@Add); while (ad->Chain != nullptr) {
		Pr(ad->Chain).Seg = Sg; ad = ad->Chain;
		if (ad->LD != nullptr) Pr(ad->LD).Seg = Sg; SgFrml(ad->Frml);
		if (ad->Assign) SgFrml(ad->Bool); else if (ad->Chk != nullptr) {
			Pr(ad->Chk).Seg = Sg; /* !!! with ad->Chk^ do!!! */
			if (HelpName != nullptr) Pr(HelpName).Seg = Sg;
		}
		ad->File2 = GetFD(ad->File2, true);
		SgF = Pr(ad->File2).Seg;
		Pr(ad->Field).Seg = SgF;
		if (!ad->Assign) {
			c = ad->Chk;
			if (c != nullptr) { SgFrml(c->Bool); SgFrml(c->TxtZ); }
		}
		SgF = Sg;
	}
	ld1 = LinkDRoot; ld = LinkDPtr(CFile->Chain); Pr(ld).Seg = Sg;
	n = CFile->nLDs;
	if (n > 0) LinkDRoot = ld;
	while (n > 0) {
		if (n == 1) ld->Chain = ld1;
		else Pr(ld->Chain).Seg = Sg;
		SgKF(KeyFldDPtr(@ld->Args));
		ld->FromFD = CFile;
		ld->ToFD = GetFD(ld->ToFD, true);
		Pr(ld->ToKey).Seg = Pr(ld->ToFD).Seg;
		ld = ld->Chain; n--;
	}
	CFile->CatIRec = GetCatIRec(CFile->Name, CFile->Typ = '0'/*multilevel*/);
	CFile->ChptPos.R = CRdb; CFile->ChptPos.IRec = FromI;
#ifdef FandSQL
	SetIsSQLFile;
#endif
	CompileRecLen();
	p = Tb; if (CFile->LiOfs > 0) {
		p = Normalize(AbsAdr(CFile) + CFile->LiOfs); CFile->LiOfs = 0;
	}
	ReleaseStore(p);
	LexWord = lw;
	return result;
}

WORD FindHelpRecNr(FileDPtr FD, pstring txt)
{
	FileDPtr cf; void* cr; LockMode md; FieldDPtr NmF, TxtF; WORD i;
	pstring nm(80);
	WORD result = 0;
	ConvToNoDiakr(&txt[1], txt.length(), Fonts.VFont);
	cf = CFile; cr = CRecPtr;
	CFile = FD;
	CRecPtr = GetRecSpace();
	md = NewLMode(RdMode); if (CFile->Handle == nullptr) goto label1;
	NmF = CFile->FldD; TxtF = NmF->Chain;
	for (i = 1; i < CFile->NRecs; i++) {
		ReadRec(i); nm = TrailChar(' ', _ShortS(NmF));
		ConvToNoDiakr(&nm[1], nm.length(), Fonts.VFont);
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
	pstring txt;
	auto result = false; txt = "";
	PromptLL(153, &txt, 1, true);
	if ((txt.length() == 0) || (KbdChar == _ESC_)) return result;
	N = FindHelpRecNr(CFile, txt);
	if (N != 0) result = true;
	return result;
}

void EditHelpOrCat(WORD cc, WORD kind, pstring txt)
{
	FileDPtr FD; EditOptPtr EO; WORD i, n;
	WORD nCat = 1; WORD iCat = 1; WORD nHelp = 1; WORD iHelp = 1;
	struct niFrml { char Op; double R; } nFrml{ 0,0 }, iFrml{ 0,0 };
	if (cc == _AltF2_) {
		FD = CRdb->HelpFD; if (kind == 1) FD = CFile->ChptPos.R->HelpFD;
		if (FD == nullptr) return;
		if (kind == 0) { i = iHelp; n = nHelp; }
		else {
			i = 3; n = FindHelpRecNr(FD, txt);
			if (n == 0) KbdBuffer = txt; // TODO: tady má být KbdBuffer:=#0#60+txt
		}
	}
	else { FD = CatFD; i = iCat; n = nCat; }
	if (kind != 2) WrEStatus;
	EO = GetEditOpt; EO->Flds = AllFldsList(FD, false);
	EO->WFlags = EO->WFlags || WPushPixel;
	if ((kind == 0) || (n != 0)) {
		iFrml.R = i; nFrml.R = n;
		EO->StartRecNoZ = (FrmlPtr)(&nFrml); EO->StartIRecZ = (FrmlPtr)(&iFrml);
	}
	EditDataFile(FD, EO); ReleaseStore(EO);
	if (cc == _AltF2_) { nHelp = EdRecNo; iHelp = EdIRec; }
	else { ResetCatalog; nCat = EdRecNo; iCat = EdIRec; }
	if (kind != 2) RdEStatus;
}

void StoreChptTxt(FieldDPtr F, LongStr* S, bool Del)
{
	LongStr* s2; void* p; WORD LicNr; longint oldpos, pos;
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
		PtrRec(kf->Chain).Seg = Sg; kf = kf->Chain; PtrRec(kf->FldD).Seg = Sg;
	}
}

FileD* GetFD(void* p, bool WithSelf, WORD Sg)
{
	if (p != nullptr) {
		LexWord = *(pstring*)(Ptr(Sg, PtrRec(p).Ofs));
		if (WithSelf && EquUpcase(CFile->Name)) p = CFile;
		else p = FindFileD();
	}
	return (FileD*)p;
}

FuncD* GetFC(void* p, WORD Sg)
{
	FuncD* fc;
	LexWord = *(pstring*)(Ptr(Sg, PtrRec(p).Ofs));
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
	LexWord = *(pstring*)(Ptr(Sg, PtrRec(P).Seg));
	fd = (FileD*)FindFileD;
	ld = LinkDRoot;
	while (ld != nullptr) {
		if ((ld->FromFD == fd) && (ld->RoleName == *((pstring*)(Ptr(Sg, WORD(P))))))
		{
			return ld;
		} ld = ld->Chain;
	}
	return nullptr;
}

void SgFrml(FrmlPtr Z, WORD Sg, WORD SgF)
{
	FrmlList fl; WORD SgFold;
	if (Z != nullptr) {
		PtrRec(Z).Seg = Sg;
		/* !!! with Z^ do!!! */
		switch (Z->Op) {
		case _field: PtrRec(Z->Field).Seg = SgF; break;
		case _access: {
			if (Z->LD != nullptr) PtrRec(Z->LD).Seg = SgF;
			Z->File2 = GetFD(Z->File2, Z->LD != nullptr);
			SgFold = SgF;
			SgF = PtrRec(Z->File2).Seg;
			SgFrml(Z->P1);
			SgF = SgFold;
			break; }
		case _userfunc: {
			Z->FC = GetFC(Z->FC);
			fl = FrmlList(FrmlL);
			while (fl->Chain != nullptr) {
				PtrRec(fl->Chain).Seg = Sg;
				fl = fl->Chain;
				SgFrml(fl->Frml);
			}
			break;
		}
		case _owned: {
			Z->ownLD = GetLinkD(Z->ownLD);
			SgFold = SgF;
			SgF = PtrRec(Z->ownLD->FromFD).Seg;
			SgFrml(Z->ownBool);
			SgFrml(Z->ownSum);
			SgF = SgFold;
			break; }
		default: {
			if (Z->Op >= 0x60 && Z->Op <= 0xaf) { SgFrml(Z->P1); break; }
			if (Z->Op >= 0xb0 && Z->Op <= 0xef) { SgFrml(Z->P1); SgFrml(Z->P2); break; }
			if (Z->Op >= 0xf0 && Z->Op <= 0xff) { SgFrml(Z->P1); SgFrml(Z->P2); SgFrml(Z->P3); break; }
		}
		}
	}
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


