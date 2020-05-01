#include "runedi.h"

#include "common.h"
#include "fileacc.h"
#include "index.h"
#include "kbdww.h"
#include "legacy.h"
#include "memory.h"
#include "oaccess.h"
#include "obaseww.h"
#include "projmgr1.h"
#include "recacc.h"

void PopEdit()
{
	E = E->PrevE;
}

bool TestIsNewRec()
{
	return IsNewRec;
}

void SetSelectFalse()
{
	Select = false;
}

WORD EditTxt(pstring* s, WORD pos, WORD maxlen, WORD maxcol, char typ, bool del, bool star, bool upd, bool ret,
	WORD Delta)
{
	BYTE* sLen = (BYTE*)s;
	WORD base, cx, cy, cx1, cy1;
	longint EndTime; bool InsMode;
	InsMode = true; base = 0; if (pos > maxlen + 1) pos = maxlen + 1;
	cx = WhereX(); cx1 = cx + WindMin.X - 1; cy = WhereY(); cy1 = cy + WindMin.Y - 1;
	CrsNorm();
	WriteStr(pos, base, maxlen, maxcol, sLen, s, star, cx, cy, cx1, cy1);
label1:
	switch (WaitEvent(Delta)) {
	case 1/*flags*/: goto label1; break;
	case 2/*timer*/: { KbdChar = _ESC_; goto label6; break; }
	}

	switch (Event.What) {
	case evMouseDown: {
		if (MouseInRect(cx1, cy1, maxcol, 1))
		{
			ClrEvent(); KbdChar = _M_; goto label6;
		}
		break;
	}
	case evKeyDown: {
		KbdChar = Event.KeyCode; ClrEvent();
		if (del) {
			if (KbdChar >= 0x20 && KbdChar <= 0xFE) { pos = 1; sLen = 0; WriteStr(pos, base, maxlen, maxcol, sLen, s, star, cx, cy, cx1, cy1); }
			del = false;
		}

		switch (KbdChar) {
		case _Ins_:
		case _V_: InsMode = !InsMode; break;
		case _U_: if (TxtEdCtrlUBrk) goto label6; break;
		case _CtrlF4_: if (TxtEdCtrlF4Brk) goto label6; break;
		case _ESC_:
		case _M_: {
		label6:
			DelBlk(sLen, s, pos);
			CrsHide(); TxtEdCtrlUBrk = false; TxtEdCtrlF4Brk = false;
			return 0;
		}
		case _left_:
		case _S_: if ((pos > 1)) pos--; break;
		case _right_:
		case _D_: {
			if (pos <= maxlen)
			{
				if ((pos > * sLen) && (*sLen < maxlen)) s = s + ' ';
				pos++;
			}
			break;
		}
		case _Q_: {
			if (ReadKbd() == _S_) goto label3;
			if (ReadKbd() == _D_) goto label4;
			break;
		}
		case _Home_:
		label3:
			pos = 1; break;
		case _End_:
		label4:
			pos = *sLen + 1; break;
		case _H_: if (upd && (pos > 1)) { pos--; goto label2; } break;
		case _Del_:
		case _G_: if (upd && (pos <= *sLen)) {
		label2:
			if (*sLen > pos) move(s[pos + 1], s[pos], sLen - pos);
			sLen--;
		} break;
		case _P_: if (upd) { ReadKbd(); if (KbdChar >= 0 && KbdChar <= 31) goto label5; }
		case _F4_: if (upd && (typ == 'A') && (pos <= *sLen)) {
			s[pos] = ToggleCS((*s)[pos]);
			break;
		}
		default:
			if (KbdChar >= 0x20 && KbdChar <= 0xFF) {
				if (upd) {
					switch (typ) {
					case 'N': { if (KbdChar < '0' || KbdChar > '9') goto label7; }
					case 'F': {
						if (!((KbdChar >= '0' && KbdChar <= '9') || KbdChar == '.' || KbdChar == ','
							|| KbdChar == '-')) goto label7;
					}
					case 'R': {
						if (!((KbdChar >= '0' && KbdChar <= '9') || KbdChar == '.' || KbdChar == ','
							|| KbdChar == '-' || KbdChar == '+' || KbdChar == 'e' || KbdChar == 'E'))
							goto label7;
					}
					}
				label5:
					if (pos > maxlen) { beep(); goto label7; }
					if (InsMode) {
						if (*sLen == maxlen)
							if ((*s)[*sLen] == ' ') (*sLen)--;
							else { beep(); goto label7; }
						move(s[pos], s[pos + 1], sLen - pos + 1); (*sLen)++;
					}
					else if (pos > * sLen) (*sLen)++;
					s[pos] = char(KbdChar); pos++;
				label7: {}
				}
			}
			else if (ret && ((KbdChar < 0x20) || (KbdChar >= 0x100))) {
				Event.What = evKeyDown;
				goto label8;
			}
			break;
		}
		WriteStr(pos, base, maxlen, maxcol, sLen, s, star, cx, cy, cx1, cy1);
	};
	}
	ClrEvent();
	if (!ret) goto label1;
label8:
	return pos;

}

void DelBlk(BYTE* sLen, pstring* s, WORD pos)
{
	while ((sLen > 0) && (s[*sLen] == ' ') && (pos <= *sLen)) sLen--;
}

void WriteStr(WORD& pos, WORD& base, WORD& maxLen, WORD& maxCol, BYTE* sLen, pstring* s, bool star,
	WORD cx, WORD cy, WORD cx1, WORD cy1)
{
	WORD BuffLine[MaxTxtCols];
	WORD i;
	struct { BYTE chr; BYTE attr; } x{ 0, 0 };
	WORD* item = (WORD*)&x;
	if (pos <= base) base = pos - 1; else if (pos > base + maxCol) {
		base = pos - maxCol; if (pos > maxLen) base--;
	}
	if ((pos == base + 1) && (base > 0)) base--;
	DelBlk(sLen, s, pos);
	for (i = 1; i < maxCol; i++) {
		x.attr = TextAttr;
		if (base + i <= *sLen) {
			if (star) x.chr = '*';
			else x.chr = (*s)[base + i];
			if (x.chr < ' ') {
				x.chr = x.chr + 64;
				x.attr = colors.tCtrl;
			}
		}
		else x.chr = ' ';
		BuffLine[i] = *item;
	}
	ScrWrBuf(cx1, cy1, BuffLine, maxCol);
	GotoXY(cx + pos - base - 1, cy);
}

void WrPromptTxt(pstring* S, FrmlPtr Impl, FieldDPtr F, pstring* Txt, double& R)
{
	WORD x, y, d, LWw; pstring SS, T; double RR; bool BB;
	WrStyleStr(*S, ProcAttr); T = ""; x = WhereX(); y = WhereY(); d = WindMax.X - WindMin.X + 1;
	if (x + F->L - 1 > d) LWw = d - x; else LWw = F->L;  TextAttr = colors.dHili;
	if (Impl != nullptr) {
		switch (F->FrmlTyp) {
		case 'R': RR = RunReal(Impl); break;
		case 'S': SS = RunShortStr(Impl); break;
		default: BB = RunBool(Impl); break;
		}
		DecodeFieldRSB(F, F->L, RR, SS, BB, T);
	}
	GotoXY(x, y); FieldEdit(F, nullptr, LWw, 1, &T, R, true, true, false, 0);
	TextAttr = ProcAttr;
	if (KbdChar == _ESC_) { EscPrompt = true; printf("\n"); }
	else {
		EscPrompt = false; Txt = &T;
		T[0] = char(LWw);
		GotoXY(x, y);
		printf("%s", T.c_str());
	}
}

bool PromptB(pstring* S, FrmlPtr Impl, FieldDPtr F)
{
	pstring Txt; double R;
	WrPromptTxt(S, Impl, F, &Txt, R);
	bool result = Txt[1] == AbbrYes;
	if (KbdChar == _ESC_) {
		if (Impl != nullptr) result = RunBool(Impl);
		else result = false;
	}
	return result;
}

pstring PromptS(pstring* S, FrmlPtr Impl, FieldDPtr F)
{
	pstring Txt; double R;
	WrPromptTxt(S, Impl, F, &Txt, R);
	auto result = Txt;
	if (KbdChar == _ESC_) {
		if (Impl != nullptr) result = RunShortStr(Impl);
		else result = "";
	}
	return result;
}

double PromptR(pstring* S, FrmlPtr Impl, FieldDPtr F)
{
	pstring Txt; double R;
	WrPromptTxt(S, Impl, F, &Txt, R);
	auto result = R;
	if (KbdChar == _ESC_) {
		if (Impl != nullptr) result = RunReal(Impl);
		else result = 0;
	}
	return result;
}

longint CRec()
{
	return BaseRec + IRec - 1;
}

longint CNRecs()
{
	longint n;
	if (EdRecVar) { return 1; }
	if (Subset) n = WK->NRecs();
	else if (HasIndex) n = VK->NRecs();
	else n = CFile->NRecs;
	if (IsNewRec) n++;
	return n;
}

longint AbsRecNr(longint N)
{
	LockMode md;
	longint result = 0;
	if (EdRecVar
#ifdef FandSQL
		|| CFile->IsSQLFile
#endif
		) {
		if (IsNewRec) result = 0; else result = 1; return result;
	}
	if (IsNewRec) {
		if ((N == CRec()) && (N == CNRecs())) { result = 0; return result; }
		if (N > CRec()) N--;
	}
	if (Subset) N = WK->NrToRecNr(N);
	else if (HasIndex) {
		md = NewLMode(RdMode); TestXFExist();
		N = VK->NrToRecNr(N); OldLMode(md);
	}
	result = N;
	return result;
}

longint LogRecNo(longint N)
{
	LockMode md;
	longint result = 0; if ((N <= 0) || (N > CFile->NRecs)) return result;
	md = NewLMode(RdMode);
	ReadRec(N);
	if (!DeletedFlag()) {
		if (Subset) result = WK->RecNrToNr(N);
		else if (HasIndex) { TestXFExist(); result = VK->RecNrToNr(N); }
		else result = N;
	}
	OldLMode(md);
	return result;
}

bool IsSelectedRec(WORD I)
{
	XString x; void* cr; longint n;
	auto result = false;
	if ((E->SelKey == nullptr) || (I == IRec) && IsNewRec) return result;
	n = AbsRecNr(BaseRec + I - 1);
	cr = CRecPtr;
	if ((I == IRec) && WasUpdated) CRecPtr = E->OldRecPtr;
	result = E->SelKey->RecNrToPath(x, n);
	CRecPtr = cr;
	return result;
}

bool EquOldNewRec()
{
	return (CompArea(CRecPtr, E->OldRecPtr, CFile->RecLen) == _equ);
}

void RdRec(longint N)
{
	LockMode md; XString x;
	if (EdRecVar) return;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		if (IsNewRec && (N > CRec)) dec(N); x.S = WK->NrToStr(N);
		Strm1->KeyAcc(WK, @x);
	}
	else
#endif
	{
		md = NewLMode(RdMode); ReadRec(AbsRecNr(N)); OldLMode(md);
	}
}

bool ELockRec(EditD* E, longint N, bool IsNewRec, bool Subset)
{
	LockMode md;
	auto result = true; if (E->IsLocked) return; E->LockedRec = N;
	if (IsNewRec) return;
	if (!E->EdRecVar
#ifdef FandSQL
		&& !CFile->IsSQLFile
#endif
		) {
		if (CFile->NotCached()) {
			if (!TryLockN(N, 1/*withESC*/)) { result = false; return result; }
			md = NewLMode(RdMode); ReadRec(N); OldLMode(md);
			if (Subset && !
				((NoCondCheck || RunBool(E->Cond) && CheckKeyIn(E)) && CheckOwner(E))) {
				WrLLF10Msg(150); goto label1;
			}
		}
		else if (DeletedFlag()) {
			WrLLF10Msg(148);
		label1:
			UnLockN(N);
			result = false;
			return result;
		}
	}
	E->IsLocked = true;
	return result;
}

bool LockRec(bool Displ)
{
	bool b;
	auto result = false;
	if (E->IsLocked) { return true; }
	b = ELockRec(E, AbsRecNr(CRec()), IsNewRec, Subset);
	result = b;
	if (b && !IsNewRec && !EdRecVar && CFile->NotCached && Displ)
		DisplRec(IRec);
	return result;
}

void UnLockRec(EditD* E)
{
	if (E->FD->IsShared && E->IsLocked && !E->EdRecVar) UnLockN(E->LockedRec);
	E->IsLocked = false;
}

void NewRecExit()
{
	EdExitD* X = E->ExD;
	while (X != nullptr) {
		if (X->AtNewRec) {
			EdBreak = 18;
			LastTxtPos = -1;
			StartExit(X, false);
		}
		X = X->Chain;
	}
}

void SetWasUpdated()
{
	if (!WasUpdated) {
		if (EdRecVar) SetUpdFlag();
		Move(E->NewRecPtr, E->OldRecPtr, CFileRecSize());
		WasUpdated = true;
	}
}

void SetCPage()
{
	WORD i;
	CPage = CFld->Page;
	RT = (ERecTxtD*)E->RecTxt;
	for (i = 1; i < CPage; i++) RT = RT->Chain;
}

void AdjustCRec()
{
	if (CRec <= CNRecs) return;
	while (CRec > CNRecs) {
		if (IRec > 1) IRec--; else BaseRec--;
	}
	if (BaseRec == 0) {
		BaseRec = 1;
		if (!IsNewRec) {
			IsNewRec = true; Append = true; FirstEmptyFld = CFld; ZeroAllFlds();
			SetWasUpdated(); NewRecExit();
		}
		else SetWasUpdated();
		NewDisplLL = true;
	}
	UnLockRec(E); LockRec(false); DisplRecNr(CRec);
}

void WrEStatus()
{
	E->CFld = CFld;
	Move(FirstEmptyFld, &E->FirstEmptyFld, ofs(SelMode) - ofs(FirstEmptyFld) + 1);
}

void RdEStatus()
{
	LockMode md;
	Move(&E->FirstEmptyFld, FirstEmptyFld, ofs(SelMode) - ofs(FirstEmptyFld) + 1);
	if (VK == nullptr) OnlySearch = false;
	CFile = E->FD; CRecPtr = E->NewRecPtr; CFld = E->CFld;
	if (CFile->XF != nullptr) HasIndex = true; else HasIndex = false;
	if (CFile->TF != nullptr) HasTF = true; else HasTF = false;
	SetCPage();
}

void AssignFld(FieldDPtr F, FrmlPtr Z)
{
	SetWasUpdated(); AssgnFrml(F, Z, false, false);
}

void DuplFld(FileD* FD1, FileD* FD2, void* RP1, void* RP2, void* RPt, FieldDPtr F1, FieldDPtr F2)
{
	LongStr* ss; pstring s; double r; bool b; FileDPtr cf; void* cr;
	cf = CFile; cr = CRecPtr; CFile = FD1; CRecPtr = RP1;
	switch (F1->FrmlTyp) {
	case 'S': {
		if (F1->Typ == 'T') {
			ss = _LongS(F1);
			CFile = FD2; CRecPtr = RP2;
			if (RPt == nullptr) DelTFld(F2);
			else DelDifTFld(RP2, RPt, F2);
			LongS_(F2, ss); ReleaseStore(ss);
		}
		else { s = _ShortS(F1); CFile = FD2; CRecPtr = RP2; S_(F2, s); }
		break;
	}
	case 'R': { r = _R(F1); CFile = FD2; CRecPtr = RP2; R_(F2, r); break; }
	case 'B': { b = _B(F1); CFile = FD2; CRecPtr = RP2; B_(F2, b); break; }
	}
	CFile = cf; CRecPtr = cr;
}

bool TestMask(pstring* S, pstring* Mask, bool TypeN)
{
	WORD i, ii, j, v, ls, lm; char c;
	auto result = true; if (Mask == nullptr) return result;
	v = 0; i = 0; ls = S->length(); j = 0; lm = Mask->length();
label1:
	if (j == lm) {
		while (i < ls) {
			i++; if (S[i] != ' ') goto label4;
		}
		return result;
	}
	j++;
	switch ((*Mask)[j]) {
	case ']':
	case ')': { v = 0; break; }
	case '[': { v = 1; ii = i; break; }
	case '(': { v = 2; ii = i; break; }
	case '|': { do { j++; } while ((*Mask)[j] != ')'); break; }
	default: {
		if (i == ls) goto label4; i++; c = (*S)[i];
		switch ((*Mask)[j]) {
		case '#':
		case '9': if (!isdigit(c)) goto label3; break;
		case '@': if (!isalpha(c)) goto label3; break;
		case '?':
		case '$': { if (!isalpha(c)) goto label3;
				else goto label2; break; }
		case '!':
		label2:
			S[i] = UpcCharTab[c]; break;
		default: { if (c != (*Mask)[j]) goto label3; break; }
		}
	}
	}
	goto label1;
label3:
	switch (v) {
	case 1: { do { j++; } while ((*Mask)[j] != ']'); v = 0; i = ii; goto label1; break; }
	case 2: { do { j++; } while (!((*Mask)[j] == '|' || (*Mask)[j] == ')')); i = ii;
		if ((*Mask)[j] == '|') goto label1; break; }
	}
label4:
	result = false; SetMsgPar(*Mask); WrLLF10Msg(653);
	return result;
}

WORD FieldEdit(FieldDPtr F, FrmlPtr Impl, WORD LWw, WORD iPos, pstring* Txt, double& RR, bool del, bool upd, bool ret,
	WORD Delta)
{
	WORD I, N, L, M, Col, Row; char cc;
	pstring* Mask; pstring* Msk;
	pstring s;
	double r;
	pstring T;
	bool b;
	WORD result = 0;
	pstring C999 = "999999999999999";
	Col = WhereX(); Row = WhereY();
	if (F->Typ == 'B') {
		if (*Txt == "") printf(" "); else printf("%s", Txt->c_str());
		GotoXY(Col, Row); CrsNorm();
	label0:
		GetEvent();
		switch (Event.What) {
		case evKeyDown: {
			KbdChar = Event.KeyCode; ClrEvent();
			if (KbdChar == _ESC_) { CrsHide(); return result; }
			if (KbdChar == _M_) {
			label11:
				if ((Txt->length() > 0) && ((*Txt)[1] == AbbrYes)) cc = AbbrYes; else cc = AbbrNo;
				goto label1;
			}
			cc = toupper((char)KbdChar);
			if ((cc == AbbrYes) || (cc == AbbrNo)) goto label1;
			break;
		}
		case evMouseDown: {
			if (MouseInRect(WindMin.X + WhereX() - 1, WindMin.Y + WhereY() - 1, 1, 1)) {
				ClrEvent(); KbdChar = _M_; goto label11;
			}
		}
		}
		ClrEvent(); goto label0;
	label1:
		printf("%c", cc); *Txt = cc; CrsHide(); return 0;
	}
	L = F->L; M = F->M; Mask = FieldDMask(F);
	if ((F->Flg && f_Mask != 0) && (F->Typ == 'A')) Msk = Mask; else Msk = nullptr;      /*!!!!*/
label2:
	iPos = EditTxt(Txt, iPos, L, LWw, F->Typ, del, false, upd, (F->FrmlTyp = 'S') && ret, Delta);
	result = iPos; if (iPos != 0) return result;
	if ((KbdChar == _ESC_) || !upd) return result;
	del = true; iPos = 1; r = 0;
	if ((Txt->length() == 0) && (Impl != nullptr)) {
		AssignFld(F, Impl); DecodeField(F, L, *Txt);
	}
	switch (F->Typ) {
	case 'F':
	case 'R': { T = LeadChar(' ', TrailChar(' ', *Txt));
		I = T.first(',');
		if (I > 0) { T = copy(T, 1, I - 1) + '.' + copy(T, I + 1, 255); }
		if (T.length() == 0) r = 0.0;
		else {
			val(T, r, I);
			if (F->Typ == 'F') {
				N = L - 2 - M; if (M == 0) N++;
				if ((I != 0) || (abs(r) >= Power10[N])) {
					s = copy(C999, 1, N) + '.' + copy(C999, 1, M);
					Set2MsgPar(s, s);
					WrLLF10Msg(617);
					goto label4;
				}
			}
			else /*'R'*/ if (I != 0) { WrLLF10Msg(639); goto label4; };
		}
		if (F->Typ == 'F') {
			str(L r : M, Txt);
			if (F->Flg && f_Comma != 0) {
				r = r * Power10[M];
				if (r >= 0) r = r + 0.5; else r = r - 0.5; r = (int)r;
			}
		}
		else /*'R'*/ str(L r, Txt);
		RR = r;
		break;
	}
	case 'A': { cc = ' '; goto label3; break; }
	case 'N': {
		cc = '0';
	label3:
		if (M == LeftJust) while (Txt->length() < L) Txt = Txt + cc;
		else while (Txt->length() < L) Txt = cc + Txt;
		if ((Msk != nullptr) && !TestMask(Txt, Msk, true)) goto label4;
		break;
	}
	case 'D': {
		T = LeadChar(' ', TrailChar(' ', *Txt));
		if (T == "") r = 0;
		else {
			r = ValDate(T, *Mask);
			if ((r == 0) && (T != LeadChar(' ', TrailChar(' ', StrDate(r, *Mask)))))
			{
				SetMsgPar(*Mask); WrLLF10Msg(618);
			label4:
				GotoXY(Col, Row); goto label2;
			}
		}
		Txt = StrDate(r, *Mask); RR = r;
		break;
	}
	}
	return result;
}

bool IsFirstEmptyFld()
{
	return IsNewRec && (CFld == FirstEmptyFld);
}

WORD FldRow(EFldD* D, WORD I)
{
	return E->FrstRow + E->NHdTxt + (I - 1) * RT->N + D->Ln - 1;
}

void SetFldAttr(EFldD* D, WORD I, WORD Attr)
{
	ScrColor(D->Col - 1, FldRow(D, I) - 1, D->L, Attr);
}

WORD RecAttr(WORD I)
{
	bool b;
	b = (I != IRec) || !IsNewRec;
	if (not IsNewRec && DeletedFlag()) return E->dDel;
	else if (b && Select && RunBool(E->Bool)) return E->dSubSet;
	else if (b && IsSelectedRec(I)) return E->dSelect;
	else return E->dNorm;
}

void IVoff()
{
	SetFldAttr(CFld, IRec, RecAttr(IRec));
}

void IVon()
{
	ScrColor(CFld->Col - 1, FldRow(CFld, IRec) - 1, CFld->L, E->dHiLi);
}

bool HasTTWw(FieldDPtr F)
{
	return (F->Typ == 'T') && (F->L > 1) && !E->IsUserForm;
}

void DisplFld(EFldD* D, WORD I)
{
	pstring Txt; WORD j, r; FieldDPtr F;
	r = FldRow(D, I); GotoXY(D->Col, r); F = D->FldD;
	DecodeField(F, D->L, Txt); for (j = 1; j < Txt.length(); j++)
		if (Txt[j] < ' ') Txt[j] = Txt[j] + 0x40;
	printf("%s", Txt.c_str());
	if (HasTTWw(F)) {
		GotoXY(D->Col + 2, r);
		Wr1Line(F);
	}
}

void Wr1Line(FieldDPtr F)
{
	pstring Txt; LongStr* s; WORD max, l;
	s = CopyLine(_LongS(F), 1, 1);
	max = F->L - 2; l = s->LL; if (l > 255) l = 255;
	Move(s->A, &Txt[1], l); Txt[0] = char(l); l = LenStyleStr(Txt);
	if (l > max)
	{
		l = max; Txt[0] = char(LogToAbsLenStyleStr(Txt, l));
	}
	WrStyleStr(Txt, E->dNorm);
	ReleaseStore(s); TextAttr = E->dNorm;
	if (l < max) printf(' ':max - l);
}

void DisplEmptyFld(EFldD* D, WORD I)
{
	WORD j; char c;
	GotoXY(D->Col, FldRow(D, I)); if (D->FldD->Flg && f_Stored != 0) c = '.'; else c = ' ';
	for (j = 1; j < D->L; j++) printf("%c", c);
	if (HasTTWw(D->FldD)) printf(' ':D->FldD->L - 1);
}

void SetRecAttr(WORD I)
{
	WORD TA; EFldD* D;
	TA = RecAttr(I); D = E->FirstFld;
	while (D != nullptr) {
		if (D->Page == CPage) SetFldAttr(D, I, TA); D = D->Chain;
	}
}

void DisplRec(WORD I)
{
	EFldD* D; bool NewFlds;
	WORD a; longint N; void* p;
	a = E->dNorm; N = BaseRec + I - 1;
	bool IsCurrNewRec = IsNewRec && (I == IRec);
	p = GetRecSpace();
	if ((N > CNRecs()) && !IsCurrNewRec) { NewFlds = true; goto label1; }
	if (I == IRec) CRecPtr = E->NewRecPtr; else { CRecPtr = p; RdRec(N); }
	NewFlds = false; if (!IsNewRec) a = RecAttr(I);
label1:
	D = E->FirstFld; while (D != nullptr) {
		if (IsCurrNewRec && (D == FirstEmptyFld) && (D->Impl == nullptr)) NewFlds = true;
		TextAttr = a;
		if (D->Page == CPage) if (NewFlds) DisplEmptyFld(D, I); else DisplFld(D, I);
		if (IsCurrNewRec && (D == FirstEmptyFld)) NewFlds = true;
		D = D->Chain;
	}
	ClearRecSpace(p); ReleaseStore(p); CRecPtr = E->NewRecPtr;
}

void DisplTabDupl()
{
	EFldD* D = E->FirstFld;
	TextAttr = E->dTab;
	while (D != nullptr) {
		if (D->Page == CPage) {
			GotoXY(D->Col + D->L, FldRow(D, 1));
			if (D->Tab) if (D->Dupl) printf("%c", 0x1F); else printf("%c", 0x11);
			else if (D->Dupl) printf("%c", 0x19); else printf(" ");
		}
		D = D->Chain;
	}
}

void DisplRecNr(longint N)
{
	if (E->RecNrLen > 0) {
		GotoXY(E->RecNrPos, 1); TextAttr = colors.fNorm; printf(E->RecNrLen:N);
	}
}

void DisplSysLine()
{
	WORD i, j; pstring m, s, x, z; bool point;
	s = E->Head;
	if (s == "") return; GotoXY(1, 1); TextAttr = colors.fNorm; ClrEol();
	i = 1; x = "";
	while (i <= s.length())
		if (s[i] == '_') {
			m = ""; point = false;
			while ((i <= s.length()) && (s[i] == '_' || s[i] == '.')) {
				if (s[i] == '.') point = true; m = m + s[i]; i++;
			}
			if (point) {
				if (m == "__.__.__") x = x + StrDate(Today(), "DD.MM.YY");
				else if (m == "__.__.____") x = x + StrDate(Today(), "DD.MM.YYYY");
				else x = x + m;
			}
			else if (m.length() == 1) x = x + m;
			else {
				E->RecNrLen = m.length(); E->RecNrPos = i - m.length();
				for (j = 1; j < m.length(); j++) x = x + ' ';
			}
		}
		else { x = x + s[i]; i++; }
	if (x.length() > TxtCols) x[0] = char(TxtCols); printf("%s", x.c_str());
	DisplRecNr(CRec());
}

void DisplBool()
{
	pstring s;
	if (!WithBoolDispl) return; GotoXY(1, 2); TextAttr = E->dSubSet; ClrEol();
	if (Select) {
		s = *E->BoolTxt;
		if (s.length() > TxtCols) s[0] = char(TxtCols);
		GotoXY((TxtCols - s.length()) / 2 + 1, 2); printf("%s", s.c_str());
	}
}

void DisplAllWwRecs()
{
	WORD i, n; LockMode md = NullMode;
	n = E->NRecs;
	if ((n > 1) && !EdRecVar) md = NewLMode(RdMode);
	AdjustCRec();
	if (!IsNewRec && !WasUpdated) RdRec(CRec());
	for (i = 1; i < n; i++) DisplRec(i);
	IVon();
	if ((n > 1) && !EdRecVar) OldLMode(md);
}

void SetNewWwRecAttr()
{
	WORD I;
	CRecPtr = GetRecSpace();
	for (I = 1; I < E->NRecs; I++) {
		if (BaseRec + I - 1 > CNRecs()) break;
		if (!IsNewRec || (I != IRec)) {
			RdRec(BaseRec + I - 1); SetRecAttr(I);
		}
	}
	IVon(); ClearRecSpace(CRecPtr); ReleaseStore(CRecPtr); CRecPtr = E->NewRecPtr;
}

void MoveDispl(WORD From, WORD Where, WORD Number)
{
	EFldD* D; WORD i, r1, r2;
	for (i = 1; i < Number; i++) {
		D = E->FirstFld; while (D != nullptr) {
			r1 = FldRow(D, From) - 1; r2 = FldRow(D, Where) - 1;
			ScrMove(D->Col - 1, r1, D->Col - 1, r2, D->L);
			if (HasTTWw(D->FldD))
				ScrMove(D->Col + 1, r1, D->Col + 1, r2, D->FldD->L - 2);
			D = D->Chain;
		}
		if (From < Where) { From--; Where--; }
		else { From++; Where++; };
	}
}

void SetNewCRec(longint N, bool withRead)
{
	longint Max, I;
	Max = E->NRecs; I = N - BaseRec + 1;
	if (I > Max) { BaseRec += I - Max; IRec = Max; }
	else if (I <= 0) { BaseRec -= abs(I) + 1; IRec = 1; }
	else IRec = I;
	if (withRead) RdRec(CRec());
}

void WriteSL(StringList SL)
{
	WORD Row;
	while (SL != nullptr) {
		Row = WhereY(); WrStyleStr(SL->S, E->Attr);
		GotoXY(E->FrstCol, Row + 1); SL = SL->Chain;
	}
}

void DisplRecTxt()
{
	WORD i;
	GotoXY(E->FrstCol, E->FrstRow + E->NHdTxt);
	for (i = 1; i < E->NRecs; i++) WriteSL(RT->SL);
}

void DisplEditWw()
{
	WORD i, x, y;
	/* !!! with E->V do!!! */
	auto EV = E->V;
	if (E->ShdwY == 1)
		ScrColor(EV.C1 + 1, EV.R2, EV.C2 - EV.C1 + E->ShdwX - 1, colors.ShadowAttr);
	if (E->ShdwX > 0)
		for (i = EV.R1; i < EV.R2; i++) ScrColor(EV.C2, i, E->ShdwX, colors.ShadowAttr);
	Window(EV.C1, EV.R1, EV.C2, EV.R2); TextAttr = E->Attr; ClrScr();

	WriteWFrame(E->WFlags, *E->Top, ""); Window(1, 1, TxtCols, TxtRows);
	DisplSysLine(); DisplBool();
	GotoXY(E->FrstCol, E->FrstRow); WriteSL(E->HdTxt);
	DisplRecTxt(); DisplTabDupl(); NewDisplLL = true;
	DisplAllWwRecs();
}

void DisplWwRecsOrPage()
{
	WORD min, max;
	if (CPage != CFld->Page) {
		SetCPage(); TextAttr = E->Attr;
		min = (WORD)WindMin;
		max = (WORD)WindMax;
		Window(E->FrstCol, E->FrstRow + E->NHdTxt, E->LastCol, E->FrstRow + E->Rows - 1);
		ClrScr(); WORD(WindMin) = min; WORD(WindMax) = max;
		DisplRecTxt(); DisplTabDupl();
	}
	DisplAllWwRecs(); DisplRecNr(CRec());
}

void DuplOwnerKey()
{
	KeyFldDPtr KF, Arg;
	if (!E->DownSet || (E->OwnerTyp == 'i')) return;
	KF = E->DownLD->ToKey->KFlds; Arg = E->DownLD->Args;
	while (Arg != nullptr) {
		DuplFld(E->DownLD->ToFD, CFile, E->DownRecPtr, E->NewRecPtr, E->OldRecPtr,
			KF->FldD, Arg->FldD);
		Arg = Arg->Chain; KF = KF->Chain;
	}
}

bool CheckOwner(EditD* E)
{
	XString X, X1;
	auto result = true;
	if (E->DownSet && (E->OwnerTyp != 'i')) {
		X.PackKF(E->DownKey->KFlds);
		CFile = E->DownLD->ToFD; CRecPtr = E->DownRecPtr;
		X1.PackKF(E->DownLD->ToKey->KFlds);
		X.S[0] = char(MinW(X.S.length(), X1.S.length()));
		if (X.S != X1.S) result = false;
		CFile = E->FD; CRecPtr = E->NewRecPtr;
	}
	return result;
}

bool CheckKeyIn(EditD* E)
{
	KeyInD* k; XString X; pstring* p1; pstring* p2;
	auto result = true; k = E->KIRoot; if (k == nullptr) return result;
	X.PackKF(E->VK->KFlds); while (k != nullptr) {
		p1 = k->X1; p2 = k->X2; if (p2 == nullptr) p2 = p1;
		if ((*p1 <= X.S) && (X.S <= (*p2 + 0xFF))) return result;
		k = k->Chain;
	}
	result = false;

}

bool TestDuplKey(KeyDPtr K)
{
	XString x; longint N;
	x.PackKF(K->KFlds);
	return K->Search(x, false, N) && (IsNewRec || (E->LockedRec != N));
}

void DuplKeyMsg(KeyDPtr K)
{
	SetMsgPar(*K->Alias); WrLLF10Msg(820);
}

void BuildWork()
{
	XScan* Scan; XScan* Scan2; void* p; KeyDPtr K; KeyFldDPtr KF;
	XString xx; bool dupl, intvl, ok; WKeyDPtr wk2; KeyInD* ki;
	FrmlPtr boolP; WORD l; FieldDPtr f; ExitRecord er;

	K = nullptr; KF = nullptr; if (CFile->Keys != nullptr) KF = CFile->Keys->KFlds;
	dupl = true; intvl = false;
	if (HasIndex) {
		K = VK; KF = K->KFlds; dupl = K->Duplic; intvl = K->Intervaltest;
	}
	WK->Open(KF, dupl, intvl);
	if (OnlyAppend) return;
	boolP = E->Cond; ki = E->KIRoot; wk2 = nullptr;
	MarkStore(p); ok = false; f = nullptr; NewExit(Ovr(), er);
	goto label1;
	if (E->DownSet) {
		New(Scan, Init(CFile, E->DownKey, nullptr, false));
		if (E->OwnerTyp == 'i') Scan->ResetOwnerIndex(E->DownLD, E->DownLV, boolP);
		else {
			CFile = E->DownLD->ToFD; CRecPtr = E->DownRecPtr;
			xx.PackKF(E->DownLD->ToKey->KFlds); CFile = E->FD; CRecPtr = E->NewRecPtr;
			Scan->ResetOwner(&xx, boolP);
		}
		if (ki != nullptr) {
			wk2 = (XWKey*)GetZStore(sizeof(*wk2)); wk2->Open(KF, true, false);
			CreateWIndex(Scan, wk2, 'W'); New(Scan2, Init(CFile, wk2, ki, false));
			Scan2->Reset(nullptr, false); Scan = Scan2;
		}
	}
	else {
#ifdef FandSQL
		if (CFile->IsSQLFile && (bool = nullptr)) {
			l = CFile->RecLen; f = CFile->FldD; OnlyKeyArgFlds(WK);
		}
#endif
		if (
#ifdef FandSQL
			CFile->IsSQLFile ||
#endif
			(boolP != nullptr))
			if ((K != nullptr) && !K->InWork && (ki == nullptr)) K = nullptr;
		New(Scan, Init(CFile, K, ki, false)); Scan->Reset(boolP, E->SQLFilter);
	}
	CreateWIndex(Scan, WK, 'W');
	Scan->Close; if (wk2 != nullptr) wk2->Close; ok = true;
label1:
	if (f != nullptr) { CFile->FldD = f; WK->KFlds = KF; CFile->RecLen = l; }
	RestoreExit(er);
	if (!ok) GoExit();
	ReleaseStore(p);
}

void SetStartRec()
{
	longint n; KeyDPtr k; KeyFldDPtr kf;

	k = VK; if (Subset) k = WK; kf = nullptr; if (k != nullptr) kf = k->KFlds;
	if ((E->StartRecKey != nullptr) && (k != nullptr)) {
		if (k->FindNr(*(XString*)(E->StartRecKey), n)) goto label1;
	}
	else if (E->StartRecNo > 0) {
		n = LogRecNo(E->StartRecNo);
	label1:
		n = MaxL(1, MinL(n, CNRecs()));
		IRec = MaxW(1, MinW(E->StartIRec, E->NRecs)); BaseRec = n - IRec + 1;
		if (BaseRec <= 0) { IRec += BaseRec - 1; BaseRec = 1; };
	}
	if (Only1Record) {
		if (CNRecs > 0) { RdRec(CRec()); n = AbsRecNr(CRec()); }
		else n = 0;
		if (Subset) WK->Close; Subset = true;
		if (n == 0) WK->Open(nullptr, true, false);
		else WK->OneRecIdx(kf, n);
		BaseRec = 1; IRec = 1;
	}
}

bool OpenEditWw()
{
	LockMode md, md1, md2; longint n;
	auto result = false;
	CFile = E->Journal;
	if (CFile != nullptr) OpenCreateF(Shared);
	RdEStatus();
	if (EdRecVar) { if (OnlyAppend) goto label2; else goto label3; }
#ifdef FandSQL
	if (!CFile->IsSQLFile)
#endif
		OpenCreateF(Shared);
	E->OldMd = E->FD->LMode; UpdCount = 0;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		if ((VK = nullptr) || !VK->InWork) Subset = true
	}
	else
#endif
	{
		if (HasIndex) TestXFExist(); md = NoDelMode;
		if (OnlyAppend || (E->Cond != nullptr) || (E->KIRoot != nullptr) || E->DownSet or
			MakeWorkX && HasIndex && CFile->NotCached && !Only1Record)
		{
			Subset = true;
			if (HasIndex) md = NoExclMode;
			else md = NoCrMode;
		}
		else if ((VK != nullptr) && VK->InWork) md = NoExclMode;
	}
	if (Subset || Only1Record) WK = (WKeyDPtr)GetZStore(sizeof(*WK));
	if (!TryLMode(md, md1, 1)) { EdBreak = 15; goto label1; }
	md2 = NewLMode(RdMode);
	if (E->DownSet && (E->OwnerTyp == 'F')) {
		CFile = E->DownLD->ToFD; CRecPtr = E->DownRecPtr;
		md1 = NewLMode(RdMode); n = E->OwnerRecNo;
		if ((n == 0) || (n > CFile->NRecs)) RunErrorM(E->OldMd, 611); ReadRec(n);
		OldLMode(md1); CFile = E->FD; CRecPtr = E->NewRecPtr;
	}
	if (Subset) BuildWork();
	if (!Only1Record && HasIndex && VK->InWork) {
		if (!Subset) WK = WKeyDPtr(VK);
		VK = CFile->Keys; WasWK = true; Subset = true;
	}
#ifdef FandSQL
	if (CFile->IsSQLFile) Strm1->DefKeyAcc(WK);
#endif
	if (!OnlyAppend) SetStartRec();
	if (CNRecs() == 0)
		if (NoCreate) {
			if (Subset) CFileMsg(107, '0'); else CFileMsg(115, '0');
			EdBreak = 13;
		label1:
			if (Subset && !WasWK) WK->Close;
			OldLMode(E->OldMd); result = false; return result;
		}
		else {
		label2:
			IsNewRec = true; Append = true;
			LockRec(false); ZeroAllFlds();
			DuplOwnerKey(); SetWasUpdated();
		}
	else RdRec(CRec());
label3:
	MarkStore(E->AfterE); DisplEditWw(); result = true;
	if (!EdRecVar) OldLMode(md2);
	if (IsNewRec) NewRecExit();
	return result;
}

void RefreshSubset()
{
	LockMode md;
	md = NewLMode(RdMode);
	if (Subset && !(OnlyAppend || Only1Record || WasWK)) {
		WK->Close;
		BuildWork();
	}
	DisplAllWwRecs(); OldLMode(md);
}

void GotoRecFld(longint NewRec, EFldD* NewFld)
{
	longint NewIRec, NewBase, D, Delta; WORD i, Max; LockMode md;
	IVoff(); CFld = NewFld;
	if (NewRec == CRec()) {
		if (CPage != CFld->Page) DisplWwRecsOrPage();
		else IVon(); return;
	}
	if (not EdRecVar) md = NewLMode(RdMode);
	if (NewRec > CNRecs()) NewRec = CNRecs();
	if (NewRec <= 0) NewRec = 1;
	if (Select) SetRecAttr(IRec);
	CFld = NewFld; Max = E->NRecs;
	Delta = NewRec - CRec(); NewIRec = IRec + Delta;
	if ((NewIRec > 0) && (NewIRec <= Max)) {
		IRec = NewIRec; RdRec(CRec()); goto label1;
	}
	NewBase = BaseRec + Delta;
	if (NewBase + Max - 1 > CNRecs()) NewBase = CNRecs - pred(Max);
	if (NewBase <= 0) NewBase = 1;
	IRec = NewRec - NewBase + 1; D = NewBase - BaseRec; BaseRec = NewBase;
	RdRec(CRec());
	if (abs(D) >= Max) { DisplWwRecsOrPage(); goto label2; }
	if (D > 0) {
		MoveDispl(D + 1, 1, Max - D);
		for (i = Max - D + 1; i < Max; i++) DisplRec(i);
	}
	else {
		D = -D; MoveDispl(Max - D, Max, Max - D);
		for (i = 1; i < D; i++) DisplRec(i);
	}
label1:
	DisplRecNr(CRec());
	IVon();
label2:
	if (!EdRecVar) OldLMode(md);
}

void UpdMemberRef(void* POld, void* PNew)
{
	LinkDPtr LD; XString x, xnew, xold; XScan* Scan; FileDPtr cf;
	void* cr; void* p; void* p2; bool sql; KeyDPtr k; KeyFldDPtr kf, kf1, kf2, Arg;
	cf = CFile; cr = CRecPtr; LD = LinkDRoot; while (LD != nullptr) {
		if ((LD->MemberRef != 0) && (LD->ToFD = cf) &&
			((PNew != nullptr) || (LD->MemberRef != 2))) {
			CFile = cf; kf2 = LD->ToKey->KFlds; CRecPtr = POld; xold.PackKF(kf2);
			if (PNew != nullptr) {
				CRecPtr = PNew; xnew.PackKF(kf2); if (xnew.S == xold.S) goto label2;
			}
			CFile = LD->FromFD;
#ifdef FandSQL
			sql = CFile->IsSQLFile;
#endif
			k = GetFromKey(LD); kf1 = k->KFlds;
			p = GetRecSpace(); CRecPtr = p;
			if (PNew != nullptr) p2 = GetRecSpace();
			New(Scan, Init(CFile, k, nullptr, true)); Scan->ResetOwner(&xold, nullptr);
#ifdef FandSQL
			if (!sql)
#endif
				ScanSubstWIndex(Scan, kf1, 'W');
		label1:
			CRecPtr = p; Scan->GetRec;
			if (!Scan->eof) {
#ifdef FandSQL
				if (sql) x.PackKF(kf1);
#endif
				if (PNew == nullptr) {
					RunAddUpdte1('-', nullptr, false, nullptr, LD); UpdMemberRef(p, nullptr);
#ifdef FandSQL
					if (sql) Strm1->DeleteXRec(k, @x, false); else
#endif

						DeleteXRec(Scan->RecNr, true);
				}
				else {
					Move(CRecPtr, p2, CFile->RecLen);
					CRecPtr = p2; kf = kf2; Arg = LD->Args; while (kf != nullptr) {
						DuplFld(cf, CFile, PNew, p2, nullptr, kf->FldD, Arg->FldD);
						Arg = Arg->Chain; kf = kf->Chain;
					}
					RunAddUpdte1('d', p, false, nullptr, LD);
					UpdMemberRef(p, p2);
#ifdef FandSQL
					if (sql) Strm1->UpdateXRec(k, @x, false) else
#endif
						OverWrXRec(Scan->RecNr, p, p2);
				}
				goto label1;
			}
			Scan->Close; ClearRecSpace(p); ReleaseStore(p);
		}
	label2:
		LD = LD->Chain;
	}
	CFile = cf; CRecPtr = cr;
}

void WrJournal(char Upd, void* RP, double Time)
{
	WORD* RPOfs = (WORD*)RP; WORD l; FieldDPtr F; longint n; LockMode md;
	if (E->Journal == nullptr) goto label1;
	l = CFile->RecLen; n = AbsRecNr(CRec());
	if ((CFile->XF != nullptr)) { RPOfs++; l--; }
	CFile = E->Journal; CRecPtr = GetRecSpace(); F = CFile->FldD;
	S_(F, Upd); F = F->Chain; R_(F, int(n)); F = F->Chain;
	R_(F, int(UserCode)); F = F->Chain; R_(F, Time); F = F->Chain;
	Move(RP^, ptr(seg(CRecPtr^), ofs(CRecPtr^) + F->Displ)^, l);
	md = NewLMode(CrMode); IncNRecs(1); WriteRec(CFile->NRecs); OldLMode(md);
	ReleaseStore(CRecPtr); CFile = E->FD; CRecPtr = E->NewRecPtr;
label1:
	UpdCount++; if (UpdCount == E->SaveAfter) { SaveFiles; UpdCount = 0; }
}

bool LockForMemb(FileDPtr FD, WORD Kind, LockMode NewMd, LockMode& md)
{
	LinkDPtr ld; LockMode md1; /*0-ExLMode,1-lock,2-unlock*/
	auto result = false; ld = LinkDRoot;
	while (ld != nullptr) {
		if ((ld->ToFD == FD)
			&& ((NewMd != DelMode) && (ld->MemberRef != 0) || (ld->MemberRef == 1))
			&& (ld->FromFD != FD)) {
			CFile = ld->FromFD;
			switch (Kind) {
			case 0: CFile->TaLMode = CFile->LMode; break;
			case 1: { md = NewMd; if (!TryLMode(NewMd, md1, 2)) return result; break; }
			case 2: OldLMode(CFile->TaLMode); break;
			}
			if (!LockForAdd(CFile, Kind, true, md)) return result;
			if (!LockForMemb(ld->FromFD, Kind, NewMd, md)) return result;
		}
		ld = ld->Chain;
	}
	result = true;
	return result;
}

bool LockWithDep(LockMode CfMd, LockMode MembMd, LockMode& OldMd)
{
	FileDPtr cf, cf2; bool b; longint w, w1; LockMode md;
	auto result = true; if (EdRecVar) return result;
	cf = CFile; w = 0;
	LockForAdd(cf, 0, true, md); LockForMemb(cf, 0, MembMd, md);
label1:
	CFile = cf; if (!TryLMode(CfMd, OldMd, 1)) {
		md = CfMd; goto label3;
	}
	if (!LockForAdd(cf, 1, true, md)) {
		cf2 = CFile; goto label2;
	}
	if (MembMd == NullMode) goto label4;
	if (not LockForMemb(cf, 1, MembMd, md)) {
		cf2 = CFile; LockForMemb(cf, 2, MembMd, md);
	label2:
		LockForAdd(cf, 2, true, md);
		CFile = cf; OldLMode(OldMd); CFile = cf2;
	label3:
		SetCPathVol(); Set2MsgPar(CPath, LockModeTxt[md]);
		w1 = PushWrLLMsg(825, true);
		if (w == 0) w = w1;
		else TWork.Delete(w1);
		LockBeep();
		if (KbdTimer(spec.NetDelay, 1)) goto label1;
		result = false;
	}
label4:
	CFile = cf; if (w != 0) PopW(w);
	return result;
}

void UnLockWithDep(LockMode OldMd)
{
	FileDPtr cf; LockMode md;
	if (EdRecVar) return;
	cf = CFile; OldLMode(OldMd); LockForAdd(cf, 2, true, md);
	LockForMemb(cf, 2, md, md); CFile = cf;
}

bool DeleteRecProc()
{
	longint I, J, N, oBaseRec; WORD oIRec; bool Group, fail; LockMode OldMd;
	bool b;
	auto result = false; Group = false;
	if (Select) {
		F10SpecKey = _ESC_; Group = PromptYN(116);
		if (KbdChar == _ESC_) return result;
	}
	if (!Group) if (VerifyDelete && !PromptYN(109)) return result;
	if (!LockWithDep(DelMode, DelMode, OldMd)) return result;
	UndoRecord; N = AbsRecNr(CRec()); RdRec(CRec());
	oIRec = IRec; oBaseRec = BaseRec;    /* exit proc uses CRec for locking etc.*/
	if (HasIndex
#ifdef FandSQL
		|| CFile->IsSQLFile
#endif
		) {
		TestXFExist(); if (Group) {
			IRec = 1; BaseRec = 1;
			while (BaseRec <= CNRecs()) {
				N = AbsRecNr(BaseRec); ClearDeletedFlag();/*prevent err msg 148*/
				if (!ELockRec(E, N, false, Subset)) goto label1;
				RdRec(BaseRec);
				if (RunBool(E->Bool)) b = DelIndRec(BaseRec, N);
				else { b = true; BaseRec++; }
				UnLockRec(E); if (!b) goto label1;
			}
		label1:
			{}
		}
		else {
			if (!ELockRec(E, N, false, Subset)) goto label1;
			DelIndRec(CRec(), N); UnLockRec(E);
		}
	}
	else if (Group) {
		J = 0; fail = false; BaseRec = 1; IRec = 1; E->EdUpdated = true;
		for (I = 1; I < CFile->NRecs; I++) {
			ReadRec(I); if (fail) goto label2;
			if (Subset) /* !!! with WK^ do!!! */ {
				if ((BaseRec > WK->NRecs()) || (WK->NrToRecNr(BaseRec) != J + 1)) goto label2;
			}
			else BaseRec = I;
			if (RunBool(E->Bool)) {
				if (!CleanUp()) { fail = true; goto label2; }
				if (Subset) /* !!! with WK^ do!!! */ {
					WK->DeleteAtNr(BaseRec); WK->AddToRecNr(J + 1, -1);
				}
				DelAllDifTFlds(CRecPtr, nullptr);
			}
			else {
				if (Subset) BaseRec++;
			label2:
				J++; WriteRec(J);
			}
		}
		DecNRecs(CFile->NRecs - J);
	}
	else if (CleanUp()) {
		E->EdUpdated = true;
		if (Subset) /* !!! with WK^ do!!! */ { WK->DeleteAtNr(CRec()); WK->AddToRecNr(N, -1); }
		DeleteRec(N);
	}
	CFld = E->FirstFld; IRec = oIRec; BaseRec = oBaseRec;
	ClearDeletedFlag(); AdjustCRec();
	if (IsNewRec) DuplOwnerKey();
	else RdRec(CRec());
	DisplWwRecsOrPage();
	UnLockWithDep(OldMd); result = true;
	return result;
}

bool CleanUp()
{
	EdExitD* X; bool ok, b; LinkDPtr ld;
	auto result = false;
	if (HasIndex && DeletedFlag()) return result;
	X = E->ExD; while (X != nullptr) {
		if (X->AtWrRec) {
			EdBreak = 17; ok = EdOk; EdOk = true; LastTxtPos = -1;
			if (!StartExit(X, false) || !EdOk) { EdOk = ok; return result; }
			EdOk = ok; WasUpdated = false;
		}
		X = X->Chain;
	}
	if (AddSwitch) {
		ld = LinkDRoot;
		while (ld != nullptr) {
			if ((ld->MemberRef == 2) && (ld->ToFD == CFile) &&
				(Owned(nullptr, nullptr, ld) > 0)) {
				WrLLF10Msg(662); return result;
			}
			ld = ld->Chain;
		}
		if (!RunAddUpdte1('-', nullptr, false, nullptr, nullptr)) return result;
		UpdMemberRef(CRecPtr, nullptr);
	}
	if (!ChptDel) return result;
	WrJournal('-', CRecPtr, Today() + CurrTime());
	result = true;
	return result;
}

bool DelIndRec(longint I, longint N)
{
	XString x;
	auto result = false;
	if (CleanUp) {
#ifdef FandSQL
		if (CFile->IsSQLFile) {
			x.PackKF(VK->KFlds); Strm1->DeleteXRec(VK, @x, false);
		}
		else
#endif
			DeleteXRec(N, true);
		if ((E->SelKey != nullptr) && E->SelKey->Delete(N)) E->SelKey->NR--;
		if (Subset) WK->DeleteAtNr(I); result = true; E->EdUpdated = true;
	}
	return result;
}

bool WriteCRec(bool MayDispl, bool& Displ)
{
	longint N, CNew; ImplDPtr ID; double time; LongStr* s;
	EFldD* D; ChkDPtr C; LockMode OldMd; KeyDPtr K;
	Displ = false;
	auto result = false;
	if (not WasUpdated || !IsNewRec && EquOldNewRec()) {
		IsNewRec = false; WasUpdated = false; result = true; UnLockRec(E); return result;
	}
	result = false;
	if (IsNewRec) {
		ID = E->Impl; while (ID != nullptr) {
			AssgnFrml(ID->FldD, ID->Frml, true, false); ID = ID->Chain;
		}
	}
	if (MustCheck) {   /* repeat field checking */
		D = E->FirstFld;
		while (D != nullptr) {
			C = CompChk(D, 'F');
			if (C != nullptr) {
				if (MayDispl) GotoRecFld(CRec(), D);
				else CFld = D;
				DisplChkErr(C); return result;
			}
			D = D->Chain;
		}
	}
	if (IsNewRec) {
		if (!LockWithDep(CrMode, NullMode, OldMd)) return result;
	}
	else if (!EdRecVar) {
		if (!LockWithDep(WrMode, WrMode, OldMd)) return result;
		if (OldRecDiffers()) {
			UnLockRec(E); UnLockWithDep(OldMd); WrLLF10Msg(149);
			DisplRec(CRec()); IVon(); return result;
		}
	}
	if (Subset && !(NoCondCheck || RunBool(E->Cond) && CheckKeyIn(E))) {
		UnLockWithDep(OldMd); WrLLF10Msg(823); return result;
	}
	if (E->DownSet) { DuplOwnerKey(); Displ = true; }
	if (!ExitCheck()) goto label1;
	if (EdRecVar) goto label2;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		if (UpdSQLFile) goto label2; else goto label1;
	}
#endif
	if (HasIndex) {   /* test duplicate keys */
		K = CFile->Keys; while (K != nullptr) {
			if (!K->Duplic && TestDuplKey(K)) {
				UnLockWithDep(OldMd); DuplKeyMsg(K);
				return result;
			}
			K = K->Chain;
		}
	}
	ClearDeletedFlag();
	if (HasIndex) {
		TestXFExist();
		if (IsNewRec) {
			if (AddSwitch && !RunAddUpdte1('+', nullptr, false, nullptr, nullptr)) goto label1;
			CNew = UpdateIndexes(); CreateRec(CFile->NRecs + 1);
		}
		else {
			if (AddSwitch) {
				if (!RunAddUpdte1('d', E->OldRecPtr, false, nullptr, nullptr)) goto label1;
				UpdMemberRef(E->OldRecPtr, CRecPtr);
			}
			CNew = UpdateIndexes(); WriteRec(E->LockedRec);
		}
		if (CNew != CRec()) {
			SetNewCRec(CNew, true); if (E->NRecs > 1) Displ = true;
		}
	}
	else if (IsNewRec) {
		N = E->LockedRec; if (N == 0) {
			N = CRec(); if (N == CNRecs()) N = CFile->NRecs + 1;
			else if (Subset) N = WK->NrToRecNr(N);
		}
		if (AddSwitch && !RunAddUpdte1('+', nullptr, false, nullptr, nullptr)) goto label1;
		if (ChptWriteCRec() != 0) goto label1;
		CreateRec(N);
		if (Subset) /* !!! with WK^ do!!! */ {
			WK->AddToRecNr(N, 1); WK->InsertAtNr(CRec(), N);
		}
	}
	else {
		if (AddSwitch) {
			if (!RunAddUpdte1('d', E->OldRecPtr, false, nullptr, nullptr)) goto label1;
			UpdMemberRef(E->OldRecPtr, CRecPtr);
		}
		switch (ChptWriteCRec()) {
		case 1: goto label1; break;
		case 2: {
			if ((*(longint*)(Pchar(E->OldRecPtr) + ChptTxt->Displ)
				== *(longint*)(Pchar(CRecPtr) + ChptTxt->Displ))
				&& PromptYN(157)) {
				s = _LongS(ChptTxt); TWork.Delete(ClpBdPos);
				ClpBdPos = TWork.Store(s); ReleaseStore(s);
			}
			UndoRecord(); goto label1; }
		}
		WriteRec(E->LockedRec);
	}
	time = Today() + CurrTime();
	if (IsNewRec) WrJournal('+', CRecPtr, time);
	else { WrJournal('O', E->OldRecPtr, time); WrJournal('N', CRecPtr, time); }
label2:
	if (!IsNewRec && !NoDelTFlds) DelAllDifTFlds(E->OldRecPtr, E->NewRecPtr);
	E->EdUpdated = true; NoDelTFlds = false;
	IsNewRec = false; WasUpdated = false; result = true; UnlockRec(E);
label1:
	UnLockWithDep(OldMd);
	return result;
}

bool ExitCheck()
{
	EdExitD* X; bool ok;
	auto result = false; X = E->ExD; while (X != nullptr) {
		if (X->AtWrRec) {
			EdBreak = 16; ok = EdOk; EdOk = true; LastTxtPos = -1;
			if (StartExit(X, MayDispl) && EdOk) EdOk = ok; else {
				EdOk = ok; return result;
			};
		}
		X = X->Chain;
	}
	result = true;
	return result;
}

longint UpdateIndexes()
{
	KeyDPtr K; WKeyDPtr KSel; longint N, NNew; XString x;
	NNew = E->LockedRec; KSel = E->SelKey;
	if (IsNewRec) { NNew = CFile->NRecs + 1; CFile->XF->NRecs++; }
	else if (KSel != nullptr) {
		CRecPtr = E->OldRecPtr;
		if (KSel->RecNrToPath(x, NNew)) {
			KSel->DeleteOnPath; CRecPtr = E->NewRecPtr; KSel->Insert(NNew, false);
		}
		CRecPtr = E->NewRecPtr;
	}
	if (VK->RecNrToPath(x, E->LockedRec) && !WasWK) {
		if (IsNewRec) {
			VK->InsertOnPath(x, NNew); if (Subset) WK->InsertAtNr(CRec(), NNew);
		}
		N = CRec();
	}
	else {
		if (!IsNewRec) {
			CRecPtr = E->OldRecPtr; VK->Delete(E->LockedRec);
			if (Subset) WK->DeleteAtNr(CRec());
			CRecPtr = E->NewRecPtr; x.PackKF(VK->KFlds);
			VK->Search(x, true, N);
		}
		N = VK->PathToNr(); VK->InsertOnPath(x, NNew);
		if (VK->InWork) WKeyDPtr(VK)->NR++;
		if (Subset) N = WK->InsertGetNr(NNew);
	}
	WORD result = N;
	K = CFile->Keys;
	while (K != nullptr) {
		if (K != VK) {
			if (not IsNewRec) { CRecPtr = E->OldRecPtr; K->Delete(E->LockedRec); }
			CRecPtr = E->NewRecPtr; K->Insert(NNew, true);
		}
		K = K->Chain;
	}
	CRecPtr = E->NewRecPtr;
	return result;
}

bool OldRecDiffers()
{
	XString x; FieldDPtr f;
	auto result = false;
	if (IsCurrChpt || (
#ifdef FandSQL
		!CFile->IsSQLFile &&
#endif 
		(!CFile->NotCached))) return result;
	CRecPtr = GetRecSpace;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		x.S = WK->NrToStr(CRec); Strm1->KeyAcc(WK, @x); f = CFile->FldD;
		while (f != nullptr) {
			/* !!! with f^ do!!! */ if (Flg && f_Stored != 0) && (Typ != 'T') and
				(CompArea(Pchar(CRecPtr) + Displ, Pchar(E->OldRecPtr) + Displ, NBytes) != ord(_equ)) then
				goto label1;
			f = f->Chain;
}
		goto label2;
	}
	else
#endif
		ReadRec(E->LockedRec);
	if (CompArea(CRecPtr, E->OldRecPtr, CFile->RecLen) != _equ) {
	label1:
		DelAllDifTFlds(E->NewRecPtr, E->OldRecPtr);
		Move(CRecPtr, E->NewRecPtr, CFile->RecLen); WasUpdated = false;
		result = true;
	}
label2:
	ClearRecSpace(CRecPtr); ReleaseStore(CRecPtr); CRecPtr = E->NewRecPtr;
	return result;
}

void UndoRecord()
{
	LockMode md; FieldDPtr f;
	if (!IsNewRec && WasUpdated) {
		if (HasTF) if (NoDelTFlds) {
			f = CFile->FldD;
			while (f != nullptr) {
				if ((f->Flg && f_Stored != 0) && (f->Typ == 'T'))
					*(longint*)(Pchar(E->OldRecPtr) + f->Displ)
					== *(longint*)(Pchar(CRecPtr) + f->Displ);
				f = f->Chain;
			}
		}
		else DelAllDifTFlds(E->NewRecPtr, E->OldRecPtr);
		Move(E->OldRecPtr, E->NewRecPtr, CFile->RecLen);
		WasUpdated = false; NoDelTFlds = false; UnlockRec(E); DisplRec(IRec); IVon();
	}
}

void DuplFromPrevRec()
{
	FieldDPtr F; LockMode md; void* cr;
	if (CFld->Ed(IsNewRec)) {
		F = CFld->FldD; md = RdMode;
		if (F->Typ == 'T') md = WrMode;
		md = NewLMode(md); SetWasUpdated();
		cr = CRecPtr; CRecPtr = GetRecSpace(); RdRec(CRec() - 1);
		DuplFld(CFile, CFile, CRecPtr, E->NewRecPtr, E->OldRecPtr, F, F);
		ClearRecSpace(CRecPtr); ReleaseStore(CRecPtr); CRecPtr = cr; OldLMode(md);
	}
}





