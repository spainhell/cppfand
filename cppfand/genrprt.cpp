#include "genrprt.h"

#include "rdedit.h"
#include "rdrprt.h"
#include "runrprt.h"
#include "wwmenu.h"
#include "wwmix.h"

PFldD* PFldDs = nullptr;
bool KpLetter = false;
integer MaxCol = 0, MaxColOld = 0, MaxColUsed = 0, NLines = 0, NLevels = 0;
AutoRprtMode ARMode = _ALstg;
LongStrPtr Txt = nullptr;

void SubstChar(pstring S, char C1, char C2)
{
	/*asm  les di,S; xor ch,ch; mov cl,es:[di]; jcxz @3; inc di;
	 mov al,C1; mov ah,C2; cld;
@1:  cmp al,es:[di]; jne @2; mov es:[di],ah;
@2:  inc di; loop @1;
@3:  end;*/
}

void Design(RprtOpt* RO)
{
	integer L, L2, LTxt, LItem, Col;
	PFldD* D = nullptr; PFldD* D1 = nullptr;
	FieldDescr* F;
	bool WasTT, LastTT, First, frstOnLine;
	MaxCol = RO->Width; MaxColOld = MaxCol;
	First = true;
	switch (RO->Style) {
	case 'C': KpLetter = true; break;
	case '?': { KpLetter = true; MaxCol = trunc(MaxCol / 0.6); break; }
	}
label1:
	NLines = 1; Col = 1; WasTT = false; LastTT = false; D = PFldDs; frstOnLine = true;
	while (D != nullptr) {
		F = D->FldD; LTxt = F->Name.length(); LItem = F->L; L = MaxI(LTxt, LItem);
		if (D->IsSum) L2 = 2; else L2 = 0;
		D->NxtLine = false;
		if (LastTT || (F->Typ == 'T') || !frstOnLine && (Col + L2 + L > MaxCol + 1)) {
			D->NxtLine = true; NLines++; Col = 1; D1 = D;
		}
		frstOnLine = false;
		Col = Col + L2;
		D->ColItem = Col + (L - LItem + 1) / 2;
		if ((F->Typ == 'A' || F->Typ == 'N') && (F->M == LeftJust)) D->ColTxt = Col;
		else D->ColTxt = Col + L - LTxt;
		if (F->Typ == 'T') {
			D->ColItem = 1; D->ColTxt = 1; WasTT = true; LastTT = true;
		}
		else LastTT = false; Col += (L + 1); D = D->Chain;
	}
	if (NLines > 1) {
		if (First && (RO->Style == '?')) {
			KpLetter = false; MaxCol = RO->Width; First = false; goto label1;
		}
		MaxColUsed = MaxCol; L = MaxCol + 1 - Col; if (L > 0) D = D1;
		if (!WasTT) while (D != nullptr) {
			D->ColTxt = D->ColTxt + L; D->ColItem = D->ColItem + L; D = D->Chain;
		}
	}
	else {
		MaxColUsed = Col;
		if ((MaxColUsed <= RO->Width) && (RO->Style == '?')) {
			MaxCol = RO->Width; KpLetter = false;
		}
	}
}

void WrChar(char C)
{
	char* p = (char*)GetStore(1);
	*p = C;
	Txt->LL++;
}

void WrBlks(integer N)
{
	void* p;
	if (N <= 0) return;
	p = GetStore(N); FillChar(p, N, ' '); Txt->LL += N;
}

void WrStr(pstring S)
{
	void* p;
	p = GetStore(S.length()); Move(&S[1], p, S.length());
	Txt->LL += S.length();
}

void WrLevel(integer Level)
{
	bool first; PFldD* d; FieldDescr* f; pstring s(50); integer col, i, l, n, m;
	bool b; pstring x;
	b = (Level == 0) && (ARMode == _AErrRecs);
	if (b) WrStr("(warning) { noErrRecs+=1},");
	first = true; d = PFldDs; while (d != nullptr) {
		if ((Level == 0) || d->IsSum || d->IsCtrl && (d->Level >= Level)) {
			if (!first) WrChar(',');
			f = d->FldD; s = f->Name;
			if ((Level != 0) && d->IsSum) { s = "sum("; s += (s + ')'); }
			if (f->Typ == 'D') {
				WrStr("strdate("); WrStr(s); WrStr(",'");
				x = FieldDMask(f);
				SubstChar(x, '\'', '\"'); WrStr(x); WrStr("')");
			}
			else WrStr(s);
			first = false;
		}
		d = d->Chain;
	}
	if (b) {
		if (!first) WrChar(',');
		WrStr("errortext+cond(^error:' ??')");
	}
	WrStr(";\r\n"); col = 1; if (CFile->Typ == '0'/*RDB*/) WrChar(0x11);
	d = PFldDs; while (d != nullptr) {
		if ((CFile->Typ == '0') && (d->Chain == nullptr)) WrChar(0x11);
		if (d->NxtLine) { WrStr("\r\n"); col = 1; }
		f = d->FldD; l = f->L; n = d->ColItem - col; col = d->ColItem + l;
		if ((Level == 0) || d->IsSum || d->IsCtrl && (d->Level >= Level)) {
			if ((Level != 0) && d->IsSum) { n -= 2; l += 2; }
			WrBlks(n);
			if (f->Typ == 'F' || f->Typ == 'R') {
				m = f->M; if (m != 0) {
					for (i = 1; i < l - m - 1; i++) WrChar('_'); l = m;
					if (f->Flg && f_Comma != 0) WrChar(',');
					else WrChar('.');
				}
			}
			for (i = 1; i < l; i++) WrChar('_');
		}
		else WrBlks(n + l);
		d = d->Chain;
	}
	if (Level > 0) {
		WrBlks(MaxColUsed - col + 1);
		for (i = 1; i < Level; i++) WrChar('*');
	}
	if (b) {
		WrStr("\r\n\x17"); WrBlks(5); WrStr("_\x17");
	}
	if ((ARMode != _AErrRecs) && (NLines > 1)) WrStr("\r\n");
}

LongStr* GenAutoRprt(RprtOpt* RO, bool WithNRecs)
{
	PFldD* d; FieldDescr* f; FieldListEl* fl; FieldListEl* fl1; KeyFldD* kf;
	integer i, l, col; char* p; bool first, point; pstring s;

	CFile = RO->FDL.FD; ARMode = RO->Mode;
	NLevels = ListLength(RO->Ctrl); PFldDs = nullptr;
	fl = RO->Flds;
	while (fl != nullptr) {
		d = (PFldD*)GetZStore(sizeof(PFldD));
		f = fl->FldD; d->FldD = f;
		d->IsSum = FieldInList(f, RO->Sum);
		fl1 = RO->Ctrl; i = NLevels;
		while (fl1 != nullptr) {
			if (fl1->FldD == f) { d->IsCtrl = true; d->Level = i; }
			i--; fl1 = (FieldListEl*)fl1->Chain;
		}
		if ((ARMode == _ATotal) && !d->IsSum && !d->IsCtrl) ReleaseStore(d);
		else ChainLast(PFldDs, d);
		fl = (FieldListEl*)fl->Chain;
	}
	Design(RO);

	Txt = (LongStr*)GetZStore(2);

	if ((ARMode = _AErrRecs)) WrStr("var noErrRecs:real;\r\n");
	WrStr("#I1_"); WrStr(CFile->Name);
	if (RO->SK != nullptr) WrChar('!');
	WrBlks(2);
	first = true; fl = RO->Ctrl; kf = RO->SK;
	while (fl != nullptr) {
		if (!first) WrChar(','); f = fl->FldD;
		if ((kf != nullptr) && (f == kf->FldD)) {
			if (kf->Descend) WrChar('>');
			if (kf->CompLex) WrChar('~');
			kf = (KeyFldD*)kf->Chain;
		}
		else if (f->Typ == 'A') WrChar('~');
		WrStr(f->Name);
		fl = (FieldListEl*)fl->Chain; first = false;
	}
	if (kf != nullptr) {
		if (!first) WrChar(';'); first = true;
		while (kf != nullptr) {
			if (!first) WrChar(',');
			if (kf->Descend) WrChar('>');
			if (kf->CompLex) WrChar('~');
			WrStr(kf->FldD->Name);
			kf = (KeyFldD*)kf->Chain; first = false;
		}
	}

	if ((ARMode == _ATotal) && (NLevels == 0)) WrStr("\r\n#RH");
	else WrStr("\r\n#PH ");

	if (RO->HeadTxt == nullptr) {
		WrStr("today,page;\r\n"); WrBlks(19);
		WrChar(0x11); s = CFile->Name; WrBlks(8 - s.length());
		SubstChar(s, '_', '-');
		WrStr(s); WrChar(0x11);
		WrBlks(14); WrStr("__.__.____");
		RdMsg(17);
		WrBlks(12 - MsgLine.length());
		WrStr(MsgLine); WrStr("___");
	}
	else {
		l = RO->HeadTxt->LL; p = (char*)(&RO->HeadTxt->A);
		i = 0; first = true;
		while (i < l) {
			if (p[i] == '_') {
				point = false;
				while ((i <= l) && (p[i] == '_' || p[i] == '.')) {
					if (p[i] == '.') point = true; i++;
				}
				if (!first) WrChar(','); first = false;
				if (point) WrStr("today");
				else WrStr("page");
			}
			i++;
		}
		WrStr(";\r\n");
		for (i = 0; i < l - 1; i++) WrChar(p[i]);
	}

	if (ARMode == _AErrRecs) {
		RdMsg(18); WrStr("\r\n\x17"); WrBlks((38 - MsgLine.length()) / 2);
		WrStr(MsgLine); WrChar(0x17);
	}
	if (RO->CondTxt != nullptr) {
		WrStr("\r\n\x17"); s = *RO->CondTxt;
		SubstChar(s, '{', '%'); SubstChar(s, '}', '%');
		SubstChar(s, '_', '-'); SubstChar(s, '@', '*');
		SubstChar(s, '#', '='); SubstChar(s, '\\', '|');
		if (s.length() > MaxCol) s[0] = char(MaxCol);
		WrBlks((MaxColOld - s.length()) / 2);
		WrStr(s); WrChar(0x17);
	}
	WrStr("\r\n");
	if (KpLetter) WrChar(0x05);
	d = PFldDs; col = 1; while (d != nullptr) {
		if (d->NxtLine) { WrStr("\r\n"); col = 1; }
		WrBlks(d->ColTxt - col); s = d->FldD->Name; SubstChar(s, '_', '-'); WrStr(s);
		col = d->ColTxt + d->FldD->Name.length(); d = d->Chain;
	}

	if (KpLetter) WrStr("\r\n#PF;\r\n\x05");

	WrStr("\r\n#DH .notsolo;\r\n");
	if (ARMode != _ATotal) { WrStr("\r\n#DE "); WrLevel(0); }
	for (i = 1; i < NLevels; i++) {
		WrStr("\r\n#CF_"); d = PFldDs;
		while (d != nullptr) {
			if (d->IsCtrl && (d->Level == i)) WrStr(d->FldD->Name);
			d = d->Chain;
		}
		WrChar(' '); WrLevel(i);
	}
	if ((RO->Ctrl != nullptr) || (RO->Sum != nullptr)) {
		WrStr("\r\n#RF (sum(1)>0) "); WrLevel(NLevels + 1);
	}
	if (WithNRecs) {
		WrStr("\r\n#RF "); if (ARMode == _AErrRecs) WrStr("noErrRecs,");
		WrStr("sum(1);\r\n\r\n");
		if (ARMode == _AErrRecs) {
			RdMsg(18); WrStr(MsgLine); WrStr(":_____\r\n");
		}
		RdMsg(20); WrStr(MsgLine); WrStr("_______");
	}
	return Txt;
	/* for i = 1 to Txt->LL do write(Txt->A[i]); writeln; wait; */
}

void RunAutoReport(RprtOpt* RO)
{
	void* p = nullptr; void* p1 = nullptr;
	LongStr* txt;
	MarkStore(p);
	p1 = RO->FDL.FD->RecPtr;
	txt = GenAutoRprt(RO, true);
	SetInpLongStr(txt, false);
	ReadReport(RO);
	RunReport(RO);
	RO->FDL.FD->RecPtr = p1;
	ReleaseStore(p);
}

bool SelForAutoRprt(RprtOpt* RO)
{
	wwmix ww;

	FieldListEl* FL; WORD N;
	auto result = false;
	if ((RO->SK == nullptr) && !PromptSortKeys(RO->Flds, RO->SK)) return result;
	N = Menu(4, 1); if (N == 0) return result;
	RO->Mode = AutoRprtMode(N - 1); CFile = RO->FDL.FD;
	if (RO->Mode == _ARprt || RO->Mode == _ATotal) {
		FL = RO->Flds;
		while (FL != nullptr) {
			if (FL->FldD->Typ != 'T') ww.PutSelect(FL->FldD->Name);
			FL = (FieldListEl*)FL->Chain;
		}
		if (!ww.SelFieldList(37, false, RO->Ctrl)) return result;
		FL = RO->Flds;
		while (FL != nullptr) {
			if (FL->FldD->FrmlTyp == 'R') ww.PutSelect(FL->FldD->Name);
			FL = (FieldListEl*)FL->Chain;
		}
		if (!ww.SelFieldList(38, true, RO->Sum)) return result;
	}
	if (spec.AutoRprtPrint) { RO->Path = (pstring*)GetStore(5); *RO->Path = "LPT1"; }
	result = true;
	return result;
}

LongStr* SelGenRprt(pstring RprtName)
{
	wwmix ww;

	RdbD* r; FileD* fd; FieldDescr* f; RprtOpt* ro;
	pstring s; integer i;
	FieldListEl* fl;
	LongStr* result = nullptr;
	r = CRdb; 
	while (r != nullptr) {
		fd = (FileD*)r->FD->Chain; while (fd != nullptr) {
			s = fd->Name; 
			if (r != CRdb) s = r->FD->Name + '.' + s; 
			ww.PutSelect(s);
			fd = (FileD*)fd->Chain;
		}
		r = r->ChainBack;
	}
	ss.Abcd = true;
	pstring tmpP = "\"";
	ww.SelectStr(0, 0, 19, tmpP + RprtName + '\"');
	if (KbdChar == _ESC_) return result;
	s = ww.GetSelect();
	i = s.first('.'); r = CRdb;
	if (i != 0) {
		do { r = r->ChainBack; } while (r->FD->Name != s.substr(1, i - 1));
		s = s.substr(i + 1, 255);
	}
	fd = r->FD;
	do { fd = (FileD*)fd->Chain; } while (fd->Name != s);
	ro = GetRprtOpt(); ro->FDL.FD = fd;
	f = fd->FldD; while (f != nullptr) {
		s = f->Name;
		if ((f->Flg & f_Stored) == 0)
		{
			pstring oldS = s;
			s = SelMark; s += oldS;
		}
		ww.PutSelect(s);
		f = (FieldDescr*)f->Chain;
	}
	CFile = fd; ww.SelFieldList(36, true, ro->Flds);
	if (ro->Flds == nullptr) return result;
	ro->Mode = _ARprt;
	fl = ro->Flds;
	while (fl != nullptr) {
		ww.PutSelect(fl->FldD->Name); fl = (FieldListEl*)fl->Chain;
	}
	if (!ww.SelFieldList(37, false, ro->Ctrl)) return result;
	fl = ro->Flds;
	while (fl != nullptr) {
		if (fl->FldD->FrmlTyp == 'R') ww.PutSelect(fl->FldD->Name);
		fl = (FieldListEl*)fl->Chain;
	}
	if (!ww.SelFieldList(38, false, ro->Sum)) return result;
	result = GenAutoRprt(ro, false);
	return result;
}

