#include "runedi.h"

#include "genrprt.h"
#include "legacy.h"
#include "oaccess.h"
#include "obaseww.h"
#include "rdedit.h"
#include "rdfildcl.h"
#include "runproc.h"
#include "runproj.h"
#include "wwmenu.h"
#include "wwmix.h"

int TimerRE = 0;
bool TxtEdCtrlUBrk, TxtEdCtrlF4Brk;
EFldD* CFld;

//EditD* E = EditDRoot;
EFldD* FirstEmptyFld;
KeyDPtr VK;
WKeyDPtr WK;
longint BaseRec;
BYTE IRec;
bool IsNewRec, Append, Select, WasUpdated, EdRecVar;
bool AddSwitch, ChkSwitch, WarnSwitch, Subset, NoDelTFlds, WasWK;
bool NoDelete, VerifyDelete, NoCreate, F1Mode, OnlyAppend, OnlySearch;
bool Only1Record, OnlyTabs, NoESCPrompt, MustESCPrompt, Prompt158;
bool NoSrchMsg, WithBoolDispl, Mode24, NoCondCheck, F3LeadIn;
bool LUpRDown, MouseEnter, TTExit;
bool MakeWorkX, NoShiftF7Msg, MustAdd, MustCheck, SelMode;
WORD UpdCount, CPage;
ERecTxtD* RT;
bool HasIndex, HasTF, NewDisplLL;

void PopEdit()
{
	E = (EditD*)E->Chain;
}

bool TestIsNewRec()
{
	return IsNewRec;
}

void SetSelectFalse()
{
	Select = false;
}

void DelBlk(BYTE* sLen, pstring* s, WORD pos)
{
	while ((*sLen > 0) && ((*s)[*sLen] == ' ') && (pos <= *sLen)) sLen--;
}

void WriteStr(WORD& pos, WORD& base, WORD& maxLen, WORD& maxCol, BYTE sLen, pstring* s, bool star,
	WORD cx, WORD cy, WORD cx1, WORD cy1)
{
	CHAR_INFO Buffer[MaxTxtCols];
	if (pos <= base) base = pos - 1;
	else if (pos > base + maxCol) { base = pos - maxCol; if (pos > maxLen) base--; }
	if ((pos == base + 1) && (base > 0)) base--;
	DelBlk(&sLen, s, pos);

	for (WORD i = 0; i < maxCol; i++) {
		Buffer[i].Attributes = TextAttr;
		if (base + i + 1 <= sLen) {
			if (star) Buffer[i].Char.AsciiChar = '*';
			else Buffer[i].Char.AsciiChar = (*s)[base + i + 1];
			if (Buffer[i].Char.AsciiChar >= '\0' && Buffer[i].Char.AsciiChar < ' ')
			{	// jedná se o netisknutelný znak ...
				Buffer[i].Char.AsciiChar = Buffer[i].Char.AsciiChar + 64;
				Buffer[i].Attributes = screen.colors.tCtrl;
			}
		}
		else Buffer[i].Char.AsciiChar = ' ';
		//BuffLine[i] = *item;
	}
	screen.ScrWrCharInfoBuf(cx1, cy1, Buffer, maxCol);
	screen.GotoXY(cx + pos - base - 1, cy);
}

WORD EditTxt(pstring* s, WORD pos, WORD maxlen, WORD maxcol, char typ, bool del, bool star, bool upd, bool ret,
	WORD Delta)
{
	auto sLen = &(*s)[0];
	WORD base = 0, cx = 0, cy = 0, cx1 = 0, cy1 = 0;
	longint EndTime = 0; bool InsMode = false;
	InsMode = true; base = 0;
	if (pos > maxlen + 1) pos = maxlen + 1;
	cx = screen.WhereX();
	cx1 = cx + WindMin.X - 1;
	cy = screen.WhereY();
	cy1 = cy + WindMin.Y - 1;
	screen.CrsNorm();
	WriteStr(pos, base, maxlen, maxcol, *sLen, s, star, cx, cy, cx1, cy1);
label1:
	switch (WaitEvent(Delta)) {
	case 1/*flags*/: goto label1; break;
	case 2/*timer*/: { KbdChar = VK_ESCAPE; goto label6; break; }
	}

	switch (Event.What) {
	case evMouseDown: {
		if (MouseInRect(cx1, cy1, maxcol, 1))
		{
			ClrEvent(); KbdChar = VK_RETURN; goto label6;
		}
		break;
	}
	case evKeyDown: {
		//KbdChar = Event.KeyCode;
		ClrEvent();
		if (del) {
			if (KbdChar >= 0x20 && KbdChar <= 0xFE)
			{
				pos = 1; *sLen = 0;
				WriteStr(pos, base, maxlen, maxcol, *sLen, s, star, cx, cy, cx1, cy1);
			}
			del = false;
		}
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
					Move(&(*s)[pos], &(*s)[pos + 1], *sLen - pos + 1); (*sLen)++;
				}
				else if (pos > * sLen) (*sLen)++;
				(*s)[pos] = char(KbdChar); pos++;
			label7: {}
			}
		}
		else if (ret && ((KbdChar < 0x20) || (KbdChar >= 0x100))) {
			Event.What = evKeyDown;
			goto label8;
		}

		if (KbdChar > 0x20) break; // jedná se o znak, ne o funkèní klávesu -> dál nezpracováváme

		//switch (KbdChar) {
		switch (Event.KeyCode) {
		case VK_INSERT:
		case _V_: InsMode = !InsMode; break;
		case _U_: if (TxtEdCtrlUBrk) goto label6; break;
		case _CtrlF4_: if (TxtEdCtrlF4Brk) goto label6; break;
		case VK_ESCAPE:
		case VK_RETURN: {
		label6:
			DelBlk(sLen, s, pos);
			screen.CrsHide(); TxtEdCtrlUBrk = false; TxtEdCtrlF4Brk = false;
			return 0;
		}
		case VK_LEFT:
		case _S_: if ((pos > 1)) pos--; break;
		case VK_RIGHT:
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
		case VK_HOME:
		label3:
			pos = 1; break;
		case VK_END:
		label4:
			pos = *sLen + 1; break;
		case VK_BACK: if (upd && (pos > 1)) { pos--; goto label2; } break;
		case VK_DELETE:
		case _G_: if (upd && (pos <= *sLen)) {
		label2:
			if (*sLen > pos) Move(&(*s)[pos + 1], &(*s)[pos], *sLen - pos);
			(*sLen)--;
		} break;
		case _P_: if (upd) { ReadKbd(); if (KbdChar >= 0 && KbdChar <= 31) goto label5; }
		case _F4_: if (upd && (typ == 'A') && (pos <= *sLen)) {
			s[pos] = ToggleCS((*s)[pos]);
			break;
		}
		default:
			break;
		}
	}
	}
	WriteStr(pos, base, maxlen, maxcol, *sLen, s, star, cx, cy, cx1, cy1);
	ClrEvent();
	if (!ret) goto label1;
label8:
	return pos;

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
		case '@': if (!IsLetter(c)) goto label3; break;
		case '?':
		case '$': { if (!IsLetter(c)) goto label3;
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

void SetWasUpdated()
{
	if (!WasUpdated) {
		if (EdRecVar) SetUpdFlag();
		Move(E->NewRecPtr, E->OldRecPtr, CFileRecSize());
		WasUpdated = true;
	}
}

void AssignFld(FieldDPtr F, FrmlPtr Z)
{
	SetWasUpdated(); AssgnFrml(F, Z, false, false);
}

WORD FieldEdit(FieldDPtr F, FrmlPtr Impl, WORD LWw, WORD iPos, pstring* Txt, double& RR, bool del, bool upd, bool ret,
	WORD Delta)
{
	WORD I = 0, N = 0, L = 0, M = 0;
	short Col = 0, Row = 0;
	char cc = '\0';
	pstring* Mask = nullptr;
	pstring* Msk = nullptr;
	pstring s;
	double r = 0;
	pstring T;
	bool b = false;
	WORD result = 0;
	pstring C999 = "999999999999999";
	Col = screen.WhereX();
	Row = screen.WhereY();
	if (F->Typ == 'B') {
		if (*Txt == "") screen.ScrFormatWrText(Col, Row, " ");
		else screen.ScrFormatWrText(Col, Row, "%s", Txt->c_str()); //printf("%s", Txt->c_str());
		screen.GotoXY(Col, Row); 
		screen.CrsNorm();
	label0:
		GetEvent();
		switch (Event.What) {
		case evKeyDown: {
			KbdChar = Event.KeyCode; 
			ClrEvent();
			if (KbdChar == _ESC_) { 
				screen.CrsHide(); 
				return result; 
			}
			if (KbdChar == _M_) {
			label11:
				if ((Txt->length() > 0) && ((*Txt)[1] == AbbrYes)) cc = AbbrYes; 
				else cc = AbbrNo;
				goto label1;
			}
			cc = toupper((char)KbdChar);
			if ((cc == AbbrYes) || (cc == AbbrNo)) goto label1;
			break;
		}
		case evMouseDown: {
			if (MouseInRect(WindMin.X + screen.WhereX() - 1, WindMin.Y + screen.WhereY() - 1, 1, 1)) {
				ClrEvent(); 
				KbdChar = _M_; 
				goto label11;
			}
		}
		}
		ClrEvent();
		goto label0;
	label1:
		//printf("%c", cc);
		screen.ScrFormatWrText(Col, Row, "%c", cc);
		*Txt = cc;
		screen.CrsHide();
		return 0;
	}
	L = F->L;
	M = F->M;
	Mask = new pstring(FieldDMask(F));
	if (((F->Flg & f_Mask) != 0) && (F->Typ == 'A')) Msk = Mask;
	else Msk = nullptr;      /*!!!!*/
label2:
	iPos = EditTxt(Txt, iPos, L, LWw, F->Typ, del, false, upd, (F->FrmlTyp == 'S')
		&& ret, Delta);
	result = iPos; if (iPos != 0) return result;
	if ((KbdChar == _ESC_) || !upd) return result;
	del = true; iPos = 1; r = 0;
	if ((Txt->length() == 0) && (Impl != nullptr)) {
		AssignFld(F, Impl);
		DecodeField(F, L, *Txt);
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
			str(r, L, M, *Txt);
			if ((F->Flg & f_Comma) != 0) {
				r = r * Power10[M];
				if (r >= 0) r = r + 0.5; else r = r - 0.5; r = (int)r;
			}
		}
		else /*'R'*/ str(r, L, 0, *Txt);
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
				screen.GotoXY(Col, Row); goto label2;
			}
		}
		*Txt = StrDate(r, *Mask); RR = r;
		break;
	}
	}
	return result;
}


void WrPromptTxt(pstring* S, FrmlElem* Impl, FieldDescr* F, pstring* Txt, double& R)
{
	WORD x = 0, y = 0, d = 0, LWw = 0;
	pstring SS, T; double RR = 0.0; bool BB = false;
	screen.WriteStyledStringToWindow(*S, ProcAttr);
	T = "";
	x = screen.WhereX();
	y = screen.WhereY();
	d = WindMax.X - WindMin.X + 1;
	if (x + F->L - 1 > d) LWw = d - x;
	else LWw = F->L;
	TextAttr = screen.colors.dHili;
	if (Impl != nullptr) {
		switch (F->FrmlTyp) {
		case 'R': RR = RunReal(Impl); break;
		case 'S': SS = RunShortStr(Impl); break;
		default: BB = RunBool(Impl); break;
		}
		DecodeFieldRSB(F, F->L, RR, SS, BB, T);
	}
	screen.GotoXY(x, y);
	FieldEdit(F, nullptr, LWw, 1, &T, R, true, true, false, 0);
	TextAttr = ProcAttr;
	if (KbdChar == _ESC_) {
		EscPrompt = true;
		printf("\n");
	}
	else {
		EscPrompt = false; Txt = &T;
		T[0] = char(LWw);
		screen.GotoXY(x, y);
		// printf("%s", T.c_str());
		screen.ScrFormatWrText(x, y, "%s", T.c_str());
	}
}

bool PromptB(pstring* S, FrmlElem* Impl, FieldDescr* F)
{
	pstring Txt; double R = 0.0;
	WrPromptTxt(S, Impl, F, &Txt, R);
	bool result = Txt[1] == AbbrYes;
	if (KbdChar == _ESC_) {
		if (Impl != nullptr) result = RunBool(Impl);
		else result = false;
	}
	return result;
}

pstring PromptS(pstring* S, FrmlElem* Impl, FieldDescr* F)
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
	//return 0;
}

longint CNRecs()
{
	longint n = 0;
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

bool CheckOwner(EditD* E)
{
	XString X, X1;
	auto result = true;
	if (E->DownSet && (E->OwnerTyp != 'i')) {
		X.PackKF(E->DownKey->KFlds);
		CFile = E->DownLD->ToFD;
		CRecPtr = E->DownRecPtr;
		X1.PackKF(E->DownLD->ToKey->KFlds);
		X.S[0] = char(MinW(X.S.length(), X1.S.length()));
		if (X.S != X1.S) result = false;
		CFile = E->FD;
		CRecPtr = E->NewRecPtr;
	}
	return result;
}

bool CheckKeyIn(EditD* E)
{
	KeyInD* k = E->KIRoot;
	XString X;
	pstring* p1;
	pstring* p2;
	auto result = true;
	if (k == nullptr) return result;
	X.PackKF(E->VK->KFlds);
	while (k != nullptr) {
		p1 = k->X1; p2 = k->X2;
		if (p2 == nullptr) p2 = p1;
		if ((p1->length() <= X.S.length()) && (X.S.length() <= p2->length() + 0xFF)) return result;
		k = (KeyInD*)k->Chain;
	}
	result = false;
	return result;
}

bool ELockRec(EditD* E, longint N, bool IsNewRec, bool Subset)
{
	LockMode md;
	auto result = true;
	if (E->IsLocked) return result;
	E->LockedRec = N;
	if (IsNewRec) return result;
	if (!E->EdRecVar
#ifdef FandSQL
		&& !CFile->IsSQLFile
#endif
		) {
		if (CFile->NotCached()) {
			if (!TryLockN(N, 1/*withESC*/)) {
				result = false;
				return result;
			}
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

WORD RecAttr(WORD I)
{
	bool b;
	b = (I != IRec) || !IsNewRec;
	if (!IsNewRec && DeletedFlag()) return E->dDel;
	else if (b && Select && RunBool(E->Bool)) return E->dSubSet;
	else if (b && IsSelectedRec(I)) return E->dSelect;
	else return E->dNorm;
}

WORD FldRow(EFldD* D, WORD I)
{
	return E->FrstRow + E->NHdTxt + (I - 1) * RT->N + D->Ln - 1;
}

bool HasTTWw(FieldDescr* F)
{
	return (F->Typ == 'T') && (F->L > 1) && !E->IsUserForm;
}

void DisplEmptyFld(EFldD* D, WORD I)
{
	WORD j; char c;
	screen.GotoXY(D->Col, FldRow(D, I));
	if (D->FldD->Flg && f_Stored != 0) c = '.';
	else c = ' ';
	for (j = 1; j < D->L; j++) printf("%c", c);
	if (HasTTWw(D->FldD)) printf("%*c", D->FldD->L - 1, ' ');
}

void Wr1Line(FieldDescr* F)
{
	pstring Txt;
	LongStr* s = CopyLine(_LongS(F), 1, 1);
	WORD max = F->L - 2;
	WORD l = s->LL;
	if (l > 255) l = 255;
	Move(s->A, &Txt[1], l);
	Txt[0] = char(l);
	l = LenStyleStr(Txt);
	if (l > max)
	{
		l = max;
		Txt[0] = char(LogToAbsLenStyleStr(Txt, l));
	}
	//WrStyleStr(Txt, E->dNorm);
	screen.WriteStyledStringToWindow(Txt, E->dNorm);
	ReleaseStore(s);
	TextAttr = E->dNorm;
	if (l < max) printf("%*c", max - l, ' ');
}

void DisplFld(EFldD* D, WORD I)
{
	pstring Txt;
	WORD r = FldRow(D, I);
	screen.GotoXY(D->Col - 1, r - 1);
	auto F = D->FldD;
	DecodeField(F, D->L, Txt);
	for (WORD j = 1; j <= Txt.length(); j++)
		if (Txt[j] < ' ') Txt[j] = Txt[j] + 0x40;
	printf("%s", Txt.c_str());
	if (HasTTWw(F)) {
		screen.GotoXY(D->Col + 2, r);
		Wr1Line(F);
	}
}

void DisplRec(WORD I)
{
	EFldD* D = nullptr;
	bool NewFlds = false;
	WORD a = E->dNorm;
	longint N = BaseRec + I - 1;
	bool IsCurrNewRec = IsNewRec && (I == IRec);
	void* p = GetRecSpace();
	if ((N > CNRecs()) && !IsCurrNewRec) {
		NewFlds = true;
		goto label1;
	}
	if (I == IRec) CRecPtr = E->NewRecPtr;
	else {
		CRecPtr = p;
		RdRec(N);
	}
	NewFlds = false;
	if (!IsNewRec) a = RecAttr(I);
label1:
	D = E->FirstFld;
	while (D != nullptr) {
		if (IsCurrNewRec && (D == FirstEmptyFld) && (D->Impl == nullptr)) NewFlds = true;
		TextAttr = a;
		if (D->Page == CPage) {
			if (NewFlds) DisplEmptyFld(D, I);
			else DisplFld(D, I);
		}
		if (IsCurrNewRec && (D == FirstEmptyFld)) NewFlds = true;
		D = (EFldD*)D->Chain;
	}
	ClearRecSpace(p);
	ReleaseStore(p);
	CRecPtr = E->NewRecPtr;
}

bool LockRec(bool Displ)
{

	auto result = false;
	if (E->IsLocked) { return true; }
	bool b = ELockRec(E, AbsRecNr(CRec()), IsNewRec, Subset);
	result = b;
	if (b && !IsNewRec && !EdRecVar && CFile->NotCached() && Displ)
		DisplRec(IRec);
	return result;
}

void UnLockRec(EditD* E)
{
	if (E->FD->IsShared() && E->IsLocked && !E->EdRecVar) UnLockN(E->LockedRec);
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
		X = (EdExitD*)X->Chain;
	}
}

void SetCPage()
{
	WORD i;
	CPage = CFld->Page;
	RT = (ERecTxtD*)E->RecTxt;
	for (i = 1; i < CPage; i++) RT = (ERecTxtD*)RT->Chain;
}


void DisplRecNr(longint N)
{
	if (E->RecNrLen > 0) {
		screen.GotoXY(E->RecNrPos, 1);
		TextAttr = screen.colors.fNorm;
		//printf("%*i", E->RecNrLen, N);
		screen.ScrFormatWrText(E->RecNrPos, 1, "%*i", E->RecNrLen, N);
	}
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
			IsNewRec = true;
			Append = true;
			FirstEmptyFld = CFld;
			ZeroAllFlds();
			SetWasUpdated();
			NewRecExit();
		}
		else SetWasUpdated();
		NewDisplLL = true;
	}
	UnLockRec(E); LockRec(false); DisplRecNr(CRec());
}

void WrEStatus()
{
	E->CFld = CFld;
	Move(FirstEmptyFld, &E->FirstEmptyFld, uintptr_t(SelMode) - uintptr_t(FirstEmptyFld) + 1);
}

void RdEStatus()
{
	LockMode md;
	//Move(&E->FirstEmptyFld, FirstEmptyFld, uintptr_t(SelMode) - uintptr_t(FirstEmptyFld) + 1);
	// predchozi move nahrazen jednotlivymi polozkami:
	FirstEmptyFld = E->FirstEmptyFld;
	VK = E->VK;
	WK = E->WK;
	BaseRec = E->BaseRec;
	IRec = E->IRec;
	IsNewRec = E->IsNewRec;	Append = E->Append; Select = E->Select;
	WasUpdated = E->WasUpdated; EdRecVar = E->EdRecVar;	AddSwitch = E->AddSwitch;
	ChkSwitch = E->ChkSwitch; WarnSwitch = E->WarnSwitch; Subset = E->SubSet;
	NoDelTFlds = E->NoDelTFlds; WasWK = E->WasWK;
	NoDelete = E->NoDelete; VerifyDelete = E->VerifyDelete; NoCreate = E->NoCreate;
	F1Mode = E->F1Mode;	OnlyAppend = E->OnlyAppend; OnlySearch = E->OnlySearch;
	Only1Record = E->Only1Record; OnlyTabs = E->OnlyTabs; NoESCPrompt = E->NoESCPrompt;
	MustESCPrompt = E->MustESCPrompt; Prompt158 = E->Prompt158; NoSrchMsg = E->NoSrchMsg;
	WithBoolDispl = E->WithBoolDispl; Mode24 = E->Mode24; NoCondCheck = E->NoCondCheck;
	F3LeadIn = E->F3LeadIn;	LUpRDown = E->LUpRDown; MouseEnter = E->MouseEnter;
	TTExit = E->TTExit;	MakeWorkX = E->MakeWorkX; NoShiftF7Msg = E->NoShiftF7Msg;
	MustAdd = E->MustAdd; MustCheck = E->MustCheck; SelMode = E->SelMode;

	if (VK == nullptr) OnlySearch = false;
	CFile = E->FD;
	CRecPtr = E->NewRecPtr;
	// TODO: CFld = E->CFld; // pokud je povoleno, spusteni spadne v SetCPage()
	if (CFile->XF != nullptr) HasIndex = true;
	else HasIndex = false;
	if (CFile->TF != nullptr) HasTF = true;
	else HasTF = false;
	SetCPage();
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


bool IsFirstEmptyFld()
{
	return IsNewRec && (CFld == FirstEmptyFld);
}



void SetFldAttr(EFldD* D, WORD I, WORD Attr)
{
	screen.ScrColor(D->Col - 1, FldRow(D, I) - 1, D->L, Attr);
}

void IVoff()
{
	SetFldAttr(CFld, IRec, RecAttr(IRec));
}

void IVon()
{
	screen.ScrColor(CFld->Col - 1, FldRow(CFld, IRec) - 1, CFld->L, E->dHiLi);
}

void SetRecAttr(WORD I)
{
	WORD TA; EFldD* D;
	TA = RecAttr(I); D = E->FirstFld;
	while (D != nullptr) {
		if (D->Page == CPage) SetFldAttr(D, I, TA); D = (EFldD*)D->Chain;
	}
}

void DisplTabDupl()
{
	EFldD* D = E->FirstFld;
	TextAttr = E->dTab;
	while (D != nullptr) {
		if (D->Page == CPage) {
			screen.GotoXY(D->Col + D->L, FldRow(D, 1));
			if (D->Tab) if (D->Dupl) printf("%c", 0x1F); else printf("%c", 0x11);
			else if (D->Dupl) printf("%c", 0x19); else printf(" ");
		}
		D = (EFldD*)D->Chain;
	}
}

void DisplSysLine()
{
	WORD i = 0, j = 0;
	pstring m, s, x, z;
	bool point = false;
	s = E->Head;
	if (s == "") return;
	screen.GotoXY(1, 1);
	TextAttr = screen.colors.fNorm;
	ClrEol();
	i = 1; x = "";
	while (i <= s.length())
		if (s[i] == '_') {
			m = "";
			point = false;
			while ((i <= s.length()) && (s[i] == '_' || s[i] == '.')) {
				if (s[i] == '.') point = true;
				m.Append(s[i]); i++;
			}
			if (point) {
				if (m == "__.__.__") x = x + StrDate(Today(), "DD.MM.YY");
				else if (m == "__.__.____") x = x + StrDate(Today(), "DD.MM.YYYY");
				else x = x + m;
			}
			else if (m.length() == 1) x = x + m;
			else {
				E->RecNrLen = m.length();
				E->RecNrPos = i - m.length();
				for (j = 1; j < m.length(); j++) x.Append(' ');
			}
		}
		else { x.Append(s[i]); i++; }
	if (x.length() > TxtCols) x[0] = char(TxtCols);
	//printf("%s", x.c_str());
	screen.ScrWrText(1, 1, x.c_str());
	DisplRecNr(CRec());
}

void DisplBool()
{
	pstring s;
	if (!WithBoolDispl) return; screen.GotoXY(1, 2); TextAttr = E->dSubSet; ClrEol();
	if (Select) {
		s = *E->BoolTxt;
		if (s.length() > TxtCols) s[0] = char(TxtCols);
		screen.GotoXY((TxtCols - s.length()) / 2 + 1, 2); printf("%s", s.c_str());
	}
}

// zobrazi zaznamy v editoru
void DisplAllWwRecs()
{
	LockMode md = NullMode;
	WORD n = E->NRecs; // pocet zaznamu k zobrazeni (na strance)
	if ((n > 1) && !EdRecVar) md = NewLMode(RdMode);
	AdjustCRec();
	if (!IsNewRec && !WasUpdated) RdRec(CRec());
	for (WORD i = 1; i <= n; i++) {
		DisplRec(i);
	}
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
			screen.ScrMove(D->Col - 1, r1, D->Col - 1, r2, D->L);
			if (HasTTWw(D->FldD))
				screen.ScrMove(D->Col + 1, r1, D->Col + 1, r2, D->FldD->L - 2);
			D = (EFldD*)D->Chain;
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
		Row = screen.WhereY();
		// WrStyleStr(SL->S, E->Attr);
		screen.WriteStyledStringToWindow(SL->S, E->dNorm);
		screen.GotoXY(E->FrstCol, Row + 1);
		SL = (StringList)SL->Chain;
	}
}

void DisplRecTxt()
{
	WORD i;
	screen.GotoXY(E->FrstCol, E->FrstRow + E->NHdTxt);
	for (i = 1; i < E->NRecs; i++) WriteSL(RT->SL);
}

void DisplEditWw()
{
	WORD i = 0, x = 0, y = 0;
	/* !!! with E->V do!!! */
	auto EV = E->V;
	if (E->ShdwY == 1) {
		screen.ScrColor(EV.C1 + 1, EV.R2, EV.C2 - EV.C1 + E->ShdwX - 1, screen.colors.ShadowAttr);
	}
	if (E->ShdwX > 0)
		for (i = EV.R1; i < EV.R2; i++) {
			screen.ScrColor(EV.C2, i, E->ShdwX, screen.colors.ShadowAttr);
		}
	screen.Window(EV.C1, EV.R1, EV.C2, EV.R2);
	TextAttr = E->Attr;
	ClrScr();

	WriteWFrame(E->WFlags, *E->Top, "");
	screen.Window(1, 1, TxtCols, TxtRows);
	DisplSysLine();
	DisplBool();
	screen.GotoXY(E->FrstCol - 1, E->FrstRow - 1);
	WriteSL(E->HdTxt);
	DisplRecTxt();
	DisplTabDupl();
	NewDisplLL = true;
	DisplAllWwRecs();
}

void DisplWwRecsOrPage()
{
	if (CPage != CFld->Page) {
		SetCPage(); TextAttr = E->Attr;
		Wind oldMin = WindMin;
		Wind oldMax = WindMax;
		screen.Window(E->FrstCol, E->FrstRow + E->NHdTxt, E->LastCol, E->FrstRow + E->Rows - 1);
		ClrScr();
		WindMin = oldMin;
		WindMax = oldMax;
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
		Arg = (KeyFldD*)Arg->Chain; KF = (KeyFldD*)KF->Chain;
	}
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
	XScan* Scan = nullptr; XScan* Scan2 = nullptr; void* p = nullptr;
	KeyDPtr K = nullptr; KeyFldDPtr KF = nullptr;
	XString xx; bool dupl, intvl, ok; WKeyDPtr wk2 = nullptr; KeyInD* ki = nullptr;
	FrmlPtr boolP = nullptr; WORD l = 0; FieldDPtr f = nullptr; ExitRecord er;

	K = nullptr; KF = nullptr; if (CFile->Keys != nullptr) KF = CFile->Keys->KFlds;
	dupl = true; intvl = false;
	if (HasIndex) {
		K = VK; KF = K->KFlds; dupl = K->Duplic; intvl = K->Intervaltest;
	}
	WK->Open(KF, dupl, intvl);
	if (OnlyAppend) return;
	boolP = E->Cond; ki = E->KIRoot; wk2 = nullptr;
	MarkStore(p); ok = false; f = nullptr;
	//NewExit(Ovr(), er);
	goto label1;
	if (E->DownSet) {
		//New(Scan, Init(CFile, E->DownKey, nullptr, false));
		Scan = new XScan(CFile, E->DownKey, nullptr, false);
		if (E->OwnerTyp == 'i') Scan->ResetOwnerIndex(E->DownLD, E->DownLV, boolP);
		else {
			CFile = E->DownLD->ToFD; CRecPtr = E->DownRecPtr;
			xx.PackKF(E->DownLD->ToKey->KFlds); CFile = E->FD; CRecPtr = E->NewRecPtr;
			Scan->ResetOwner(&xx, boolP);
		}
		if (ki != nullptr) {
			wk2 = (XWKey*)GetZStore(sizeof(*wk2)); wk2->Open(KF, true, false);
			CreateWIndex(Scan, wk2, 'W');
			//New(Scan2, Init(CFile, wk2, ki, false));
			Scan2 = new XScan(CFile, wk2, ki, false);
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
		//New(Scan, Init(CFile, K, ki, false));
		Scan = new XScan(CFile, K, ki, false);
		Scan->Reset(boolP, E->SQLFilter);
}
	CreateWIndex(Scan, WK, 'W');
	Scan->Close(); if (wk2 != nullptr) wk2->Close(); ok = true;
label1:
	if (f != nullptr) { CFile->FldD = f; WK->KFlds = KF; CFile->RecLen = l; }
	RestoreExit(er);
	if (!ok) GoExit();
	ReleaseStore(p);
}

void SetStartRec()
{
	longint n = 0;
	KeyFldD* kf = nullptr;;
	KeyD* k = VK;
	if (Subset) k = WK;
	if (k != nullptr) kf = k->KFlds;
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
		if (CNRecs() > 0) { RdRec(CRec()); n = AbsRecNr(CRec()); }
		else n = 0;
		if (Subset) WK->Close();
		Subset = true;
		if (n == 0) WK->Open(nullptr, true, false);
		else WK->OneRecIdx(kf, n);
		BaseRec = 1;
		IRec = 1;
	}
}

bool OpenEditWw()
{
	LockMode md, md1, md2;
	longint n = 0;
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
		if (HasIndex) TestXFExist();
		md = NoDelMode;
		if (OnlyAppend || (E->Cond != nullptr) || (E->KIRoot != nullptr) || E->DownSet ||
			MakeWorkX && HasIndex && CFile->NotCached() && !Only1Record)
		{
			Subset = true;
			if (HasIndex) md = NoExclMode;
			else md = NoCrMode;
		}
		else if ((VK != nullptr) && VK->InWork) md = NoExclMode;
	}
	if (Subset || Only1Record) WK = new XWKey(); // GetZStore(sizeof(*WK));
	if (!TryLMode(md, md1, 1)) { EdBreak = 15; goto label1; }
	md2 = NewLMode(RdMode);
	if (E->DownSet && (E->OwnerTyp == 'F')) {
		CFile = E->DownLD->ToFD; CRecPtr = E->DownRecPtr;
		md1 = NewLMode(RdMode);
		n = E->OwnerRecNo;
		if ((n == 0) || (n > CFile->NRecs)) RunErrorM(E->OldMd, 611);
		ReadRec(n);
		OldLMode(md1);
		CFile = E->FD;
		CRecPtr = E->NewRecPtr;
	}
	if (Subset) BuildWork();
	if (!Only1Record && HasIndex && VK->InWork) {
		if (!Subset) WK = (XWKey*)VK;
		VK = CFile->Keys;
		WasWK = true; Subset = true;
	}
#ifdef FandSQL
	if (CFile->IsSQLFile) Strm1->DefKeyAcc(WK);
#endif
	if (!OnlyAppend) SetStartRec();
	if (CNRecs() == 0)
		if (NoCreate) {
			if (Subset) CFileMsg(107, '0');
			else CFileMsg(115, '0');
			EdBreak = 13;
		label1:
			if (Subset && !WasWK) WK->Close();
			OldLMode(E->OldMd); result = false; return result;
		}
		else {
		label2:
			IsNewRec = true; Append = true;
			LockRec(false);
			ZeroAllFlds();
			DuplOwnerKey();
			SetWasUpdated();
		}
	else RdRec(CRec());
label3:
	MarkStore(E->AfterE);
	DisplEditWw();
	result = true;
	if (!EdRecVar) OldLMode(md2);
	if (IsNewRec) NewRecExit();
	return result;
	}

void RefreshSubset()
{
	LockMode md = NewLMode(RdMode);
	if (Subset && !(OnlyAppend || Only1Record || WasWK)) {
		WK->Close();
		BuildWork();
	}
	DisplAllWwRecs();
	OldLMode(md);
}

void GotoRecFld(longint NewRec, EFldD* NewFld)
{
	longint NewIRec = 0, NewBase = 0, D = 0, Delta = 0;
	WORD i = 0, Max = 0; LockMode md;
	IVoff();
	CFld = NewFld;
	if (NewRec == CRec()) {
		if (CPage != CFld->Page) DisplWwRecsOrPage();
		else IVon();
		return;
	}
	if (!EdRecVar) md = NewLMode(RdMode);
	if (NewRec > CNRecs()) NewRec = CNRecs();
	if (NewRec <= 0) NewRec = 1;
	if (Select) SetRecAttr(IRec);
	CFld = NewFld; Max = E->NRecs;
	Delta = NewRec - CRec(); NewIRec = IRec + Delta;
	if ((NewIRec > 0) && (NewIRec <= Max)) {
		IRec = NewIRec; RdRec(CRec()); goto label1;
	}
	NewBase = BaseRec + Delta;
	if (NewBase + Max - 1 > CNRecs()) NewBase = CNRecs() - pred(Max);
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
	LinkDPtr LD = nullptr; XString x, xnew, xold; XScan* Scan = nullptr; FileDPtr cf = nullptr;
	void* cr = nullptr; void* p = nullptr; void* p2 = nullptr; bool sql;
	KeyDPtr k = nullptr; KeyFldDPtr kf = nullptr, kf1 = nullptr, kf2 = nullptr, Arg = nullptr;
	cf = CFile; cr = CRecPtr; LD = LinkDRoot; while (LD != nullptr) {
		if ((LD->MemberRef != 0) && (LD->ToFD == cf) &&
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
			// New(Scan, Init(CFile, k, nullptr, true));
			Scan = new XScan(CFile, k, nullptr, true);
			Scan->ResetOwner(&xold, nullptr);
#ifdef FandSQL
			if (!sql)
#endif
				ScanSubstWIndex(Scan, kf1, 'W');
		label1:
			CRecPtr = p; Scan->GetRec();
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
						Arg = (KeyFldD*)Arg->Chain; kf = (KeyFldD*)kf->Chain;
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
			Scan->Close(); ClearRecSpace(p); ReleaseStore(p);
	}
	label2:
		LD = LD->Chain;
}
	CFile = cf; CRecPtr = cr;
}

void WrJournal(char Upd, void* RP, double Time)
{
	WORD* RPOfs = (WORD*)RP; WORD l; FieldDPtr F; longint n; LockMode md;
	if (E->Journal != nullptr) {
		l = CFile->RecLen;
		n = AbsRecNr(CRec());
		if ((CFile->XF != nullptr)) { RPOfs++; l--; }
		CFile = E->Journal;
		CRecPtr = GetRecSpace();
		F = CFile->FldD;
		std::string UpdStr = std::string(Upd, 1);
		S_(F, UpdStr);
		F = (FieldDescr*)F->Chain;
		R_(F, int(n));
		F = (FieldDescr*)F->Chain;
		R_(F, int(UserCode));
		F = (FieldDescr*)F->Chain; R_(F, Time);
		F = (FieldDescr*)F->Chain;
		Move(RP, &CRecPtr + F->Displ, l);
		md = NewLMode(CrMode);
		IncNRecs(1);
		WriteRec(CFile->NRecs);
		OldLMode(md);
		ReleaseStore(CRecPtr);
		CFile = E->FD; CRecPtr = E->NewRecPtr;
	}
	UpdCount++;
	if (UpdCount == E->SaveAfter) { SaveFiles(); UpdCount = 0; }
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
	if (!LockForMemb(cf, 1, MembMd, md)) {
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

void UndoRecord()
{
	LockMode md; FieldDPtr f;
	if (!IsNewRec && WasUpdated) {
		if (HasTF) if (NoDelTFlds) {
			f = CFile->FldD;
			while (f != nullptr) {
				if (((f->Flg & f_Stored) != 0) && (f->Typ == 'T'))
					*(longint*)((char*)(E->OldRecPtr) + f->Displ) = *(longint*)(((char*)(CRecPtr)+f->Displ));
				f = (FieldDescr*)f->Chain;
			}
		}
		else DelAllDifTFlds(E->NewRecPtr, E->OldRecPtr);
		Move(E->OldRecPtr, E->NewRecPtr, CFile->RecLen);
		WasUpdated = false; NoDelTFlds = false; UnLockRec(E); DisplRec(IRec); IVon();
	}
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
		X = (EdExitD*)X->Chain;
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
	if (CleanUp()) {
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
	UndoRecord(); N = AbsRecNr(CRec()); RdRec(CRec());
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

ChkDPtr CompChk(EFldD* D, char Typ)
{
	ChkDPtr C; bool w, f;
	w = WarnSwitch && (Typ == 'W' || Typ == '?');
	f = (Typ == 'F' || Typ == '?');
	C = D->Chk;
	ChkD* result = nullptr;
	while (C != nullptr) {
		if ((w && C->Warning || f && !C->Warning) && !RunBool(C->Bool)) {
			result = C; return result;
		}
		C = (ChkD*)C->Chain;
	}
	return result;
}

void FindExistTest(FrmlPtr Z, LinkD** LD)
{
	*LD = nullptr;
	if (Z == nullptr) return;
	switch (Z->Op) {
	case _field: {
		auto iZ = (FrmlElem7*)Z;
		if ((iZ->Field->Flg & f_Stored) == 0) FindExistTest(iZ->Field->Frml, LD);
		break;
	}
	case _access: {
		auto iZ = (FrmlElem7*)Z;
		if (iZ->P011 == nullptr) *LD = iZ->LD; /*file.exist*/
		break;
	}
	default: {
		auto iZ = (FrmlElem0*)Z;
		if (Z->Op >= 0x60 && Z->Op <= 0xAF) /*1-ary*/ { FindExistTest(iZ->P1, LD); break; }
		if (Z->Op >= 0xB0 && Z->Op <= 0xEF) /*2-ary*/ {
			FindExistTest(iZ->P1, LD);
			if (*LD == nullptr) FindExistTest(iZ->P2, LD);
			break;
		}
		if (Z->Op >= 0xF0 && Z->Op <= 0xFF) /*3-ary*/ {
			FindExistTest(iZ->P1, LD);
			if (LD == nullptr) {
				FindExistTest(iZ->P2, LD);
				if (LD == nullptr) FindExistTest(iZ->P3, LD);
			}
		}
		break;
	}
	}
}

bool TestAccRight(StringList S)
{
	if (UserCode == 0) { return true; }
	return OverlapByteStr((void*)(uintptr_t(S) + 5 + S->S.length()), &AccRight);
}

bool ForNavigate(FileDPtr FD)
{
	StringList S;
	auto result = true;
	if (UserCode == 0) return result;
	S = FD->ViewNames;
	while (S != nullptr) {
		if (TestAccRight(S)) return result;
		S = (StringList)S->Chain;
	}
	result = false;
	return result;
}

pstring GetFileViewName(FileD* FD, StringList SL)
{
	if (SL == nullptr) { return FD->Name; }
	while (!TestAccRight(SL)) SL = (StringList)SL->Chain;
	pstring result = 0x01; // ^A
	result += SL->S;
	do { SL = (StringList)SL->Chain; } while (!(SL == nullptr) || TestAccRight(SL));
	return result;
}

void SetPointTo(LinkDPtr LD, pstring* s1, pstring* s2)
{
	KeyFldDPtr KF;
	KF = LD->Args;
	while (KF != nullptr) {
		if (KF->FldD == CFld->FldD) { s2 = s1; ss.Pointto = s2; }
		KF = (KeyFldD*)KF->Chain;
	}
}

void GetSel2S(pstring* s, pstring* s2, char C, WORD wh)
{
	wwmix ww;

	WORD i; pstring s1;
	*s = ww.GetSelect(); *s2 = ""; i = s->first(C);
	if (i > 0)
		if (wh == 1) {
			s1 = s->substr(i + 1, 255); *s2 = s->substr(1, i - 1); *s = s1;
		}
		else { *s2 = s->substr(i + 1, 255); *s = s->substr(1, i - 1); }

}

bool EquRoleName(pstring S, LinkD* LD)
{
	if (S == "") return LD->ToFD->Name == (std::string)LD->RoleName;
	else return S == LD->RoleName;
}

bool EquFileViewName(FileD* FD, pstring S, EditOpt* EO)
{
	StringList SL; FileDPtr cf;
	auto result = true; cf = CFile; CFile = FD;
	if (S[1] == 0x01) { // ^A
		S = copy(S, 2, 255); SL = CFile->ViewNames;
		while (SL != nullptr) {
			if (SL->S == S) { EO = GetEditOpt(); RdUserView(S, EO); goto label1; }
			SL = (StringList)SL->Chain;
		}
	}
	else if (S == CFile->Name) {
		EO = GetEditOpt(); EO->Flds = AllFldsList(CFile, false); return result;
	}
	result = false;
label1:
	CFile = cf;
	return result;
}

void UpwEdit(LinkDPtr LkD)
{
	wwmix ww;

	void* p = nullptr; pstring s, s1, s2; XString x; XString* px = nullptr;
	FieldDPtr F = nullptr; KeyFldDPtr KF = nullptr;
	KeyDPtr K = nullptr; EditOpt* EO = nullptr;
	WORD Brk; FileDPtr ToFD = nullptr; StringList SL, SL1; LinkDPtr LD = nullptr;
	longint w; bool b;
	MarkStore(p);
	w = PushW1(1, 1, TxtCols, TxtRows, true, true);
	CFile->IRec = AbsRecNr(CRec());
	WrEStatus();
	if (LkD == nullptr) {
		LD = LinkDRoot; while (LD != nullptr) {
			ToFD = LD->ToFD;
			if ((LD->FromFD == CFile) && ForNavigate(ToFD))
			{
				s = "";
				if (ToFD->Name != LD->RoleName) { s = '.'; s += LD->RoleName; }
				SL = ToFD->ViewNames;
				do {
					s1 = GetFileViewName(ToFD, SL) + s;
					ww.PutSelect(s1); SetPointTo(LD, &s1, &s2);
				} while (SL != nullptr);
			}
			LD = LD->Chain;
		}
		ss.Abcd = true;
		ww.SelectStr(0, 0, 35, "");
		if (KbdChar == _ESC_) goto label1;
		GetSel2S(&s1, &s2, '.', 2); LD = LinkDRoot;
		while (!((LD->FromFD == CFile) && EquRoleName(s2, LD)
			&& EquFileViewName(LD->ToFD, s1, EO))) LD = LD->Chain;
	}
	else {
		LD = LkD; EO = GetEditOpt(); EO->UserSelFlds = false; CFile = LD->ToFD;
		SL = CFile->ViewNames; SL1 = nullptr;  while (SL != nullptr) {
			if (TestAccRight(SL)) SL1 = SL; SL = (StringList)SL->Chain;
		}
		if (SL1 == nullptr) EO->Flds = AllFldsList(CFile, false);
		else RdUserView(SL1->S, EO);
		EO->SetOnlyView = true;
	}
	CFile = E->FD; x.PackKF(LD->Args); px = &x;
	K = LD->ToKey; CFile = LD->ToFD;
	if (EO->ViewKey == nullptr) EO->ViewKey = K;
	else if (&EO->ViewKey != &K) px = nullptr;
	if (SelFldsForEO(EO, nullptr)) {
		NewEditD(CFile, EO);
		E->ShiftF7LD = LkD;
		if (OpenEditWw()) RunEdit(px, Brk);
		SaveFiles();
		PopEdit();
	}
label1:
	PopW(w); ReleaseStore(p); RdEStatus(); DisplEditWw();
}

void DisplChkErr(ChkDPtr C)
{
	LinkD* LD = nullptr; FileDPtr cf = nullptr; void* cr = nullptr; bool b; longint n;

	FindExistTest(C->Bool, &LD);
	if (!C->Warning && (LD != nullptr) && ForNavigate(LD->ToFD)
		&& CFld->Ed(IsNewRec)) {
		cf = CFile; cr = CRecPtr; b = LinkUpw(LD, n, false); ReleaseStore(CRecPtr);
		CFile = cf; CRecPtr = cr;
		if (!b)
			if (NoShiftF7Msg) goto label1;
			else F10SpecKey = _ShiftF7_;
	}
	if (C->HelpName != nullptr)
		if (F10SpecKey == _ShiftF7_) F10SpecKey = 0xfffe; else F10SpecKey = _F1_;
	SetMsgPar(RunShortStr(C->TxtZ)); WrLLF10Msg(110);
	if (KbdChar == _F1_) Help(CFile->ChptPos.R, *C->HelpName, false);
	else if (KbdChar == _ShiftF7_)
		label1:
	UpwEdit(LD);
}

bool OldRecDiffers()
{
	XString x; FieldDPtr f;
	auto result = false;
	if (IsCurrChpt || (
#ifdef FandSQL
		!CFile->IsSQLFile &&
#endif 
		(!CFile->NotCached()))) return result;
	CRecPtr = GetRecSpace();
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

bool ExitCheck(bool MayDispl)
{
	EdExitD* X; bool ok;
	auto result = false; X = E->ExD; while (X != nullptr) {
		if (X->AtWrRec) {
			EdBreak = 16; ok = EdOk; EdOk = true; LastTxtPos = -1;
			if (StartExit(X, MayDispl) && EdOk) EdOk = ok;
			else { EdOk = ok; return result; }
		}
		X = (EdExitD*)X->Chain;
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
			KSel->DeleteOnPath(); CRecPtr = E->NewRecPtr; KSel->Insert(NNew, false);
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
			if (!IsNewRec) { CRecPtr = E->OldRecPtr; K->Delete(E->LockedRec); }
			CRecPtr = E->NewRecPtr; K->Insert(NNew, true);
		}
		K = K->Chain;
	}
	CRecPtr = E->NewRecPtr;
	return result;
}


bool WriteCRec(bool MayDispl, bool& Displ)
{
	longint N, CNew; ImplDPtr ID; double time; LongStr* s;
	EFldD* D; ChkDPtr C; LockMode OldMd; KeyDPtr K;
	Displ = false;
	auto result = false;
	if (!WasUpdated || !IsNewRec && EquOldNewRec()) {
		IsNewRec = false; WasUpdated = false; result = true; UnLockRec(E); return result;
	}
	result = false;
	if (IsNewRec) {
		ID = E->Impl; while (ID != nullptr) {
			AssgnFrml(ID->FldD, ID->Frml, true, false); ID = (ImplD*)ID->Chain;
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
			D = (EFldD*)D->Chain;
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
	if (!ExitCheck(MayDispl)) goto label1;
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
			if ((*(longint*)((char*)(E->OldRecPtr) + ChptTxt->Displ)
				== *(longint*)((char*)(CRecPtr)+ChptTxt->Displ))
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
	IsNewRec = false; WasUpdated = false; result = true; UnLockRec(E);
label1:
	UnLockWithDep(OldMd);
	return result;
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

void InsertRecProc(void* RP)
{
	GotoRecFld(CRec(), E->FirstFld); IsNewRec = true; LockRec(false);
	if (RP != nullptr) Move(RP, CRecPtr, CFile->RecLen);
	else ZeroAllFlds();
	DuplOwnerKey(); SetWasUpdated();
	IVoff(); MoveDispl(E->NRecs - 1, E->NRecs, E->NRecs - IRec);
	FirstEmptyFld = CFld; DisplRec(IRec); IVon(); NewDisplLL = true;
	NewRecExit();
}

void AppendRecord(void* RP)
{
	WORD Max;
	IVoff(); IsNewRec = true; Max = E->NRecs;
	CFld = E->FirstFld; FirstEmptyFld = CFld;
	if (IRec < Max)
	{
		IRec++; MoveDispl(Max - 1, Max, Max - IRec); DisplRec(IRec); IVon();
	}
	else if (Max == 1) { BaseRec++; DisplWwRecsOrPage(); }
	else { BaseRec += Max - 1; IRec = 2; DisplAllWwRecs(); }
	if (RP != nullptr) Move(RP, CRecPtr, CFile->RecLen); else ZeroAllFlds();
	DuplOwnerKey(); DisplRecNr(CRec()); SetWasUpdated(); LockRec(false);
	NewRecExit();
}

bool GotoXRec(XString* PX, longint& N)
{
	LockMode md; KeyDPtr k;
	auto result = false;
	md = NewLMode(RdMode); k = VK; if (Subset) k = WK;
	if (Subset || HasIndex) {
		result = k->SearchIntvl(*PX, false, N);
		N = k->PathToNr();
	}
	else result = SearchKey(*PX, k, N);
	RdRec(CRec()); GotoRecFld(N, CFld); OldLMode(md);
	return result;
}

EFldD* FindEFld(FieldDPtr F)
{

	EFldD* D = E->FirstFld;
	while (D != nullptr) {
		if (D->FldD == F) goto label1;
		D = (EFldD*)D->Chain;
	}
label1:
	return D;
}

void CreateOrErr(bool Create, void* RP, longint N)
{
	if (Create) if (N > CNRecs()) AppendRecord(RP); else InsertRecProc(RP);
	else if (!NoSrchMsg) WrLLF10Msg(118);
}

bool PromptSearch(bool Create)
{
	auto result = false;
	FieldDPtr F, F2; FileDPtr FD, FD2; void* RP; void* RP2; KeyFldDPtr KF, KF2;
	longint n; pstring s; double r; bool b, li, found; LockMode md;
	XString x, xOld; KeyDPtr K; longint w; WORD Col, LWw, pos; EFldD* D;
	FD = CFile; K = VK; if (Subset) K = WK; KF = K->KFlds;
	RP = GetRecSpace(); CRecPtr = RP; ZeroAllFlds(); x.Clear();
	li = F3LeadIn && !IsNewRec;
	w = PushW1(1, TxtRows, TxtCols, TxtRows, true, false);
	if (KF == nullptr) goto label1;
	if (HasIndex && E->DownSet && (VK == E->DownKey)) {
		FD2 = E->DownLD->ToFD; RP2 = E->DownRecPtr; KF2 = E->DownLD->ToKey->KFlds;
		CFile = FD2; CRecPtr = RP2;
		while (KF2 != nullptr) {
			CFile = FD2; CRecPtr = RP2; F = KF->FldD; F2 = KF2->FldD;
			switch (F->FrmlTyp) {
			case 'S': { s = _ShortS(F2); x.StoreStr(s, KF);
				CFile = FD; CRecPtr = RP; S_(F, s); break; }
			case 'R': { r = _R(F2); x.StoreReal(r, KF);
				CFile = FD; CRecPtr = RP; R_(F, r); break; }
			case 'B': { b = _B(F2); x.StoreBool(b, KF);
				CFile = FD; CRecPtr = RP; B_(F, b); break; }
			}
			KF2 = (KeyFldD*)KF2->Chain; KF = (KeyFldD*)KF->Chain;
		}
	}
	if (KF == nullptr) {
	label1:
		result = true; CRecPtr = E->NewRecPtr; goto label3;
	}
	while (KF != nullptr) {
		F = KF->FldD; if (li) {
			D = FindEFld(F); if (D != nullptr) GotoRecFld(CRec(), D);
		}
		screen.GotoXY(1, TxtRows);
		TextAttr = screen.colors.pTxt;
		ClrEol();
		printf("%s:", F->Name.c_str());
		s = ""; pos = 1;
		Col = screen.WhereX();
		if (Col + F->L > TxtCols) LWw = TxtCols - Col;
		else LWw = F->L;
	label2:
		TextAttr = screen.colors.pNorm;
		screen.GotoXY(Col, TxtRows);
		pos = FieldEdit(F, nullptr, LWw, pos, &s, r, false, true, li, E->WatchDelay);
		xOld = x;
		if ((KbdChar == _ESC_) || (Event.What == evKeyDown)) {
			CRecPtr = E->NewRecPtr; goto label3;
		}
		switch (F->FrmlTyp) {
		case 'S': { x.StoreStr(s, KF); S_(F, s); break; }
		case 'R': { x.StoreReal(r, KF); R_(F, r); break; }
		case 'B': { b = s[1] = AbbrYes; x.StoreBool(b, KF); B_(F, b); break; }
		}
		if (li) {
			CRecPtr = E->NewRecPtr; found = GotoXRec(&x, n);
			if ((pos == 0) && (F->FrmlTyp == 'S')) {
				x = xOld; x.StoreStr(_ShortS(F), KF);
			}
			CRecPtr = RP; if (pos != 0) { x = xOld; goto label2; };
		}
		KF = (KeyFldD*)KF->Chain;
	}
	CRecPtr = E->NewRecPtr;
	if (li) { if (!found) CreateOrErr(Create, RP, n); }
	else if (IsNewRec) Move(RP, CRecPtr, CFile->RecLen);
	else if (!GotoXRec(&x, n)) CreateOrErr(Create, RP, n);
	result = true;
label3:
	PopW(w); ReleaseStore(RP);
	return result;
}

bool PromptAndSearch(bool Create)
{
	auto result = false;
	if (VK == nullptr) { WrLLF10Msg(111); return result; }
	result = PromptSearch(Create);
	GotoRecFld(CRec(), E->FirstFld);
	return result;
}

void PromptGotoRecNr()
{
	wwmix ww;

	WORD I; pstring Txt; longint N; bool Del;
	I = 1; Txt = ""; Del = true;
	do {
		ww.PromptLL(122, &Txt, I, Del);
		if (KbdChar == _ESC_) return;
		val(Txt, N, I);
		Del = false;
	} while (I != 0);
	GotoRecFld(N, CFld);
}

void CheckFromHere()
{
	longint N; EFldD* D; ChkD* C; LockMode md;
	D = CFld; N = CRec(); md = NewLMode(RdMode);
label1:
	if (!DeletedFlag)
		while (D != nullptr) {
			C = CompChk(D, '?');
			if (C != nullptr) {
				if (BaseRec + E->NRecs - 1 < N) BaseRec = N;
				IRec = N - BaseRec + 1; CFld = D; DisplWwRecsOrPage(); OldLMode(md);
				DisplChkErr(C);
				return;
			}
			D = (EFldD*)D->Chain;
		}
	if (N < CNRecs()) {
		N++; DisplRecNr(N); RdRec(N); D = E->FirstFld; goto label1;
	}
	RdRec(CRec()); DisplRecNr(CRec()); OldLMode(md); WrLLF10Msg(120);
}

void Sorting()
{
	KeyFldD* SKRoot = nullptr; void* p = nullptr; ExitRecord er; LockMode md;
	SaveFiles; MarkStore(p);
	if (!PromptSortKeys(E->Flds, SKRoot) || (SKRoot == nullptr)) goto label2;
	if (!TryLMode(ExclMode, md, 1)) goto label2;
	//NewExit(Ovr(), er);
	goto label1;
	SortAndSubst(SKRoot);
	E->EdUpdated = true;
label1:
	RestoreExit(er); CFile = E->FD; OldLMode(md);
label2:
	ReleaseStore(p); CRecPtr = E->NewRecPtr; DisplAllWwRecs();
}

void AutoReport()
{
	void* p = nullptr; RprtOpt* RO = nullptr; FileUseMode UM = Closed;
	MarkStore(p); RO = GetRprtOpt(); RO->FDL.FD = CFile; RO->Flds = E->Flds;
	if (Select) { RO->FDL.Cond = E->Bool; RO->CondTxt = E->BoolTxt; }
	if (Subset) RO->FDL.ViewKey = WK; else if (HasIndex) RO->FDL.ViewKey = VK;
	PrintView = false;
	if (SelForAutoRprt(RO)) {
		SpecFDNameAllowed = IsCurrChpt();
		RunAutoReport(RO);
		SpecFDNameAllowed = false;
	}
	ReleaseStore(p); ViewPrinterTxt(); CRecPtr = E->NewRecPtr;
}

void AutoGraph()
{
	FrmlPtr Bool; KeyD* K;
#ifdef FandGraph
	Bool = nullptr; if (Select) Bool = E->Bool;
	K = nullptr; if (Subset) K = WK; else if (HasIndex) K = VK;
	RunAutoGraph(E->Flds, K, Bool);
#endif
	CFile = E->FD; CRecPtr = E->NewRecPtr;
}

bool IsDependItem()
{
	if (!IsNewRec && (E->NEdSet == 0)) return false;
	DepD* Dp = CFld->Dep;
	while (Dp != nullptr)
	{
		if (RunBool(Dp->Bool)) { return true; }
		Dp = (DepD*)Dp->Chain;
	}
	return false;
}

void SetDependItem()
{
	DepD* Dp = CFld->Dep;
	while (Dp != nullptr)
	{
		if (RunBool(Dp->Bool)) { AssignFld(CFld->FldD, Dp->Frml); return; }
		Dp = (DepD*)Dp->Chain;
	}
}

void SwitchToAppend()
{
	GotoRecFld(CNRecs(), CFld);
	Append = true;
	AppendRecord(nullptr);
	NewDisplLL = true;
}

bool CheckForExit(bool& Quit)
{
	auto result = false;
	EdExitD* X = E->ExD;
	while (X != nullptr) {
		bool b = FieldInList(CFld->FldD, X->Flds);
		if (X->NegFlds) b = !b;
		if (b) if (X->Typ == 'Q') Quit = true;
		else {
			EdBreak = 12; LastTxtPos = -1;
			if (!StartExit(X, true)) return result;
		}
		X = (EdExitD*)X->Chain;
	}
	result = true;
	return result;
}

bool FldInModeF3Key(FieldDPtr F)
{
	KeyFldD* KF;
	auto result = false;
	if ((F->Flg & f_Stored) == 0) return result;
	KF = VK->KFlds;
	while (KF != nullptr) {
		if (KF->FldD == F) { result = true; return result; }
		KF = (KeyFldD*)KF->Chain;
	}
	return result;
}

bool IsSkipFld(EFldD* D)
{
	/* !!! with D^ do!!! */
	return (!D->Tab && ((E->NTabsSet > 0) || ((D->FldD->Flg & f_Stored) == 0)
		|| OnlySearch && FldInModeF3Key(D->FldD)));
}

bool ExNotSkipFld()
{
	EFldD* D;
	auto result = false;
	if (E->NFlds == 1) return result;
	D = E->FirstFld;
	while (D != nullptr) {
		if ((D != CFld) && !IsSkipFld(D)) { result = true; return result; }
		D = (EFldD*)D->Chain;
	}
	return result;
}

bool CtrlMProc(WORD Mode)
{
	longint OldCRec, i; EFldD* OldCFld; bool b;
	ChkDPtr C; EdExitD* X = nullptr; WORD Brk, NR; KeyList KL;
	bool displ, skip, Quit, WasNewRec; LockMode md; char Typ;

	OldCRec = CRec(); OldCFld = CFld;
	auto result = true; NR = 0;
	if (Mode == 0 /*only bypass unrelevant fields*/) goto label2;
label1:
	if (IsFirstEmptyFld())FirstEmptyFld = (EFldD*)FirstEmptyFld->Chain;
	Quit = false; if (!CheckForExit(Quit)) return result;
	TextAttr = E->dHiLi; DisplFld(CFld, IRec);
	if (ChkSwitch) {
		if (Mode == 1 || Mode == 3) Typ = '?'; else Typ = 'F';
		C = CompChk(CFld, Typ);
		if (C != nullptr) {
			DisplChkErr(C);
			if (!C->Warning) return result;
		}
	}
	if (WasUpdated && !EdRecVar && HasIndex) {
		KL = CFld->KL;
		while (KL != nullptr) {
			md = NewLMode(RdMode); b = TestDuplKey(KL->Key); OldLMode(md);
			if (b) { DuplKeyMsg(KL->Key); return result; } KL = (KeyList)KL->Chain;
		}
	}
	if (Quit && !IsNewRec && (Mode == 1 || Mode == 3)) {
		EdBreak = 12; result = false; return result;
	}
	if (CFld->Chain != nullptr) {
		GotoRecFld(CRec(), (EFldD*)CFld->Chain);
		if (Mode == 1 || Mode == 3) Mode = 0;
	}
	else {
		WasNewRec = IsNewRec; Mode = 0; NR++;
		if (!WriteCRec(true, displ)) return result;
		if (displ) DisplAllWwRecs(); else SetRecAttr(IRec);
		if (Only1Record)
			if (NoESCPrompt) { EdBreak = 0; return false; }
			else { Append = false; goto label3; }
		if (OnlySearch) { Append = false; goto label3; }
		if (Append) AppendRecord(nullptr);
		else {
			if (WasNewRec) NewDisplLL = true;
			if (CRec < CNRecs)
				if (Select) {
					for (i = CRec() + 1; i < CNRecs(); i++) {
						if (KeyPressed && (ReadKey() != _M_) && PromptYN(23)) goto label4;
						RdRec(i); DisplRecNr(i); if (!DeletedFlag() && RunBool(E->Bool)) {
							RdRec(CRec()); GotoRecFld(i, E->FirstFld); goto label2;
						};
					}
				label4:
					RdRec(CRec()); DisplRecNr(CRec());
					GotoRecFld(OldCRec, OldCFld); beep; beep; return result;
				}
				else GotoRecFld(CRec() + 1, E->FirstFld);
			else {
			label3:
				GotoRecFld(CRec(), OldCFld); beep; beep; return result;
			}
		}
	}
label2:
	skip = false; displ = false;
	if (IsFirstEmptyFld()) {
		if ((CFld->Impl != nullptr) && LockRec(true)) {
			AssignFld(CFld->FldD, CFld->Impl); displ = true;
		}
		if (CFld->Dupl && (CRec() > 1) && LockRec(true)) {
			DuplFromPrevRec(); displ = true; skip = true;
		}
	}
	if (IsDependItem() && LockRec(true)) {
		SetDependItem(); displ = true; skip = true;
	}
	if (IsSkipFld(CFld)) skip = true; if (CFld->Tab) skip = false;
	if (displ) { TextAttr = E->dHiLi; DisplFld(CFld, IRec); }
	if (Mode == 2 /*bypass all remaining fields of the record */) goto label1;
	if (skip && ExNotSkipFld() && (NR <= 1)) goto label1;
	return result;
}

bool GoPrevNextRec(integer Delta, bool Displ)
{
	longint i, D, OldBaseRec; LockMode md; WORD w, Max;
	auto result = false; if (EdRecVar) return result;
	md = NewLMode(RdMode); i = CRec();
	if (Displ) IVoff();
label0:
	i += Delta;
	if ((i > 0) && (i <= CNRecs())) {
		RdRec(i);
		if (Displ) DisplRecNr(i);
		if (!Select || !DeletedFlag && RunBool(E->Bool)) goto label2;
		if (KeyPressed) {
			w = ReadKey();
			if (((Delta > 0) && (w != _down_) && (w != _CtrlEnd_) && (w != _PgDn_)
				|| (Delta < 0) && (w != _up_) && (w != _CtrlHome_) && (w != _PgUp_))
				&& PromptYN(23)) goto label1;
		}
		goto label0;
	}
	if (Select) WrLLF10Msg(16);
label1:
	RdRec(CRec());
	if (Displ) { DisplRecNr(CRec()); IVon(); } goto label4;
label2:
	result = true; OldBaseRec = BaseRec; SetNewCRec(i, false);
	if (Displ) {
		Max = E->NRecs; D = BaseRec - OldBaseRec;
		if (abs(D) >= Max) { DisplWwRecsOrPage(); goto label3; }
		if (D > 0) {
			MoveDispl(D + 1, 1, Max - D);
			for (i = Max - D + 1; i < Max; i++) DisplRec(i);
		}
		else if (D < 0) {
			D = -D; MoveDispl(Max - D, Max, Max - D);
			for (i = 1; i < D; i++) DisplRec(i);
		}
	}
label3:
	if (Displ)IVon();
label4:
	OldLMode(md);
	return result;
}

bool GetChpt(pstring Heslo, longint& NN)
{
	longint j; pstring s(12); integer i;
	auto result = true;
	for (j = 1; j < CFile->NRecs; j++) {
		ReadRec(j);
		if (IsCurrChpt()) {
			s = TrailChar(' ', _ShortS(ChptName)); i = s.first('.');
			if (i > 0) s.Delete(i, 255);
			if (SEquUpcase(Heslo, s)) goto label1;
		}
		else {
			s = TrailChar(' ', _ShortS(CFile->FldD));
			ConvToNoDiakr((WORD*)s[1], s.length(), fonts.VFont);
			if (EqualsMask(&Heslo[1], Heslo.length(), s))
				label1:
			{ NN = j; return result; }
		}
	}
	RdRec(CRec());
	result = false;
	return result;
}

void SetCRec(longint I)
{
	if (I > BaseRec + E->NRecs - 1) BaseRec = I - E->NRecs + 1;
	else if (I < BaseRec) BaseRec = I;
	IRec = I - BaseRec + 1; RdRec(CRec());
}

void UpdateEdTFld(LongStr* S)
{
	LockMode md;
	CRecPtr = E->NewRecPtr;
	if (!EdRecVar) md = NewLMode(WrMode);
	SetWasUpdated();
	DelDifTFld(E->NewRecPtr, E->OldRecPtr, CFld->FldD);
	LongS_(CFld->FldD, S);
	if (!EdRecVar) OldLMode(md);
}

void UpdateTxtPos(WORD TxtPos)
{
	LockMode md;
	if (IsCurrChpt) {
		md = NewLMode(WrMode); SetWasUpdated();
		R_(ChptTxtPos, integer(TxtPos)); OldLMode(md);
	}
}

bool EditFreeTxt(FieldDPtr F, pstring ErrMsg, bool Ed, WORD& Brk)
{
	const pstring BreakKeys =
#ifndef FandRunV
		_CtrlF1 +
#endif
		_F1 + _CtrlHome + _CtrlEnd + _F9 + _AltF10;
	//const BYTE BreakKeys1[8] = { _CtrlF1, _F1, _CtrlHome, _CtrlEnd, _F9, _AltF10, _ShiftF1, _F10 };
	pstring BreakKeys1 = _CtrlF1 + _F1 + _CtrlHome + _CtrlEnd + _F9 + _AltF10 + _ShiftF1 + _F10;
	pstring BreakKeys2 = _F1 + _CtrlHome + _CtrlEnd + _F9 + _F10 + _AltF10 +
		_CtrlF1 + _AltF1 + _ShiftF1 + _AltF2 + _AltF3 + _CtrlF8 + _CtrlF9 + _AltF9;
	const BYTE maxStk = 10;

	pstring Breaks(14);
	bool Srch, Upd, WasUpd, Displ, quit;
	pstring HdTxt(22);
	MsgStr TxtMsgS; MsgStr* PTxtMsgS;
	longint TxtXY;
	WORD R1, OldTxtPos, TxtPos, CtrlMsgNr, C, LastLen; LongStr* S;
	char Kind; LockMode md; void* p = nullptr; longint i, w;
	EdExitD* X;
	WORD iStk;
	struct { longint N = 0; longint I = 0; } Stk[maxStk];
	pstring heslo(80);

	MarkStore(p); Srch = false; Brk = 0; TxtPos = 1; iStk = 0; TxtXY = 0;
	auto result = true;
	w = 0; if (E->Head == "") w = PushW(1, 1, TxtCols, 1);
	if (E->TTExit)
		/* !!! with TxtMsgS do!!! */ {
		TxtMsgS.Head = nullptr; TxtMsgS.Last = E->Last; TxtMsgS.CtrlLast = E->CtrlLast;
		TxtMsgS.AltLast = E->AltLast; TxtMsgS.ShiftLast = E->ShiftLast;
		PTxtMsgS = &TxtMsgS;
	}
	else PTxtMsgS = nullptr;
label1:
	HdTxt = "    ";  WasUpd = false;
	if (CRec() > 1) HdTxt[3] = 0x18; // ^X
	if (CRec < CNRecs) HdTxt[4] = 0x19; // ^Y
	if (IsCurrChpt()) {
		HdTxt = _ShortS(ChptTyp) + ':' + _ShortS(ChptName) + HdTxt;
		TxtPos = trunc(_R(ChptTxtPos));
		Breaks = BreakKeys2;
		CtrlMsgNr = 131;
	}
	else {
		CtrlMsgNr = 151;
		if (CFile == CRdb->HelpFD) Breaks = BreakKeys1;
		else Breaks = BreakKeys;
	}
	R1 = E->FrstRow;
	if ((R1 = 3) && WithBoolDispl) R1 = 2;
	screen.Window(E->FrstCol, R1, E->LastCol, E->LastRow); TextAttr = screen.colors.tNorm;
	Kind = 'V'; OldTxtPos = TxtPos;
	if (Ed) LockRec(false);
	if ((F->Flg & f_Stored) != 0) {
		S = _LongS(F);
		if (Ed) Kind = 'T';
	}
	else S = RunLongStr(F->Frml);
label2:
	X = nullptr; if (TTExit) X = E->ExD; Upd = false;
	result =
		EditText(Kind, MemoT, HdTxt, ErrMsg, (char*)&S->A, MaxLStrLen, S->LL, TxtPos, TxtXY, Breaks, X,
			Srch, Upd, 141, CtrlMsgNr, PTxtMsgS);
	ErrMsg = ""; heslo = LexWord; LastLen = S->LL;
	if (EdBreak == 0xffff) C = KbdChar; else C = 0;
	if (C == _AltEqual_) C = _ESC_; else WasUpd = WasUpd || Upd;
	switch (C) {
	case _AltF3_: { EditHelpOrCat(C, 0, ""); goto label2; }
	case _U_: { ReleaseStore(S); TxtXY = 0; goto label1; }
	}
	screen.Window(1, 1, TxtCols, TxtRows);
	if (WasUpd) UpdateEdTFld(S);
	if ((OldTxtPos != TxtPos) && !Srch) UpdateTxtPos(TxtPos);
	ReleaseStore(S);
	if (Ed && !WasUpdated) UnLockRec(E);
	if (Srch) if (WriteCRec(false, Displ)) goto label31;
	switch (C) {
	case _F9_: {
		if (WriteCRec(false, Displ)) { SaveFiles; UpdCount = 0; }
		goto label4;
	}
	case _F1_: { RdMsg(6); heslo = MsgLine; goto label3; }
	case _CtrlF1_: goto label3;
	case _ShiftF1_:
		if (IsCurrChpt() || (CFile == CRdb->HelpFD)) {
			if ((iStk < maxStk) && WriteCRec(false, Displ) && GetChpt(heslo, i)) {
				Append = false; iStk++;
				/* !!! with Stk[iStk] do!!! */ { Stk[iStk].N = CRec(); Stk[iStk].I = TxtPos; }
				SetCRec(i);
			}
			TxtXY = 0; goto label4;
		}
	case _F10_: {
		if ((iStk > 0) && WriteCRec(false, Displ)) {
			Append = false;
			/* !!! with Stk[iStk] do!!! */
			{ SetCRec(Stk[iStk].N); TxtPos = Stk[iStk].I; }
			iStk--;
		}
		TxtXY = 0;
		goto label4;
	}
	case _AltF10_: { Help(nullptr, "", false); goto label4; }
	case _AltF1_: { heslo = _ShortS(ChptTyp);
	label3:
		Help((RdbDPtr)&HelpFD, heslo, false);
		goto label4;
	}
	}
	if ((C > 0xFF) && WriteCRec(false, Displ)) {
		Append = false;
		if (C == _CtrlHome_) { GoPrevNextRec(-1, false); TxtXY = 0; goto label4; }
		if (C == _CtrlEnd_) {
		label31:
			if (!GoPrevNextRec(+1, false) && Srch) {
				UpdateTxtPos(LastLen); Srch = false;
			}
			TxtXY = 0;
		label4:
			if (!Ed || LockRec(false)) goto label1; else goto label5;
		}
		WrEStatus; Brk = 1; KbdChar = C; goto label6;
	}
label5:
	ReleaseStore(p);
	DisplEditWw();
label6:
	if (w != 0) PopW(w);
	return result;
}

bool EditItemProc(bool del, bool ed, WORD& Brk)
{
	pstring Txt; double R = 0; bool b = false; ChkD* C = nullptr; WORD wd = 0;
	FieldDescr* F = CFld->FldD;
	auto result = true;
	if (F->Typ == 'T') {
		if (!EditFreeTxt(F, "", ed, Brk)) {
			return false;
		}
	}
	else {
		TextAttr = E->dHiLi;
		DecodeField(F, CFld->FldD->L, Txt);
		screen.GotoXY(CFld->Col, FldRow(CFld, IRec));
		wd = 0;
		if (CFile->NotCached()) wd = E->WatchDelay;
		FieldEdit(F, CFld->Impl, CFld->L, 1, &Txt, R, del, ed, false, wd);
		if ((KbdChar == _ESC_) || !ed) {
			DisplFld(CFld, IRec);
			if (ed && !WasUpdated) UnLockRec(E);
			return result;
		}
		SetWasUpdated();
		switch (F->FrmlTyp) {
		case 'B': B_(F, toupper(Txt[1]) == AbbrYes); break;
		case 'S': S_(F, Txt); break;
		case 'R': R_(F, R); break;
		}
	}
	if (Brk == 0) result = CtrlMProc(1);
	return result;
}

void SetSwitchProc()
{
	bool B; WORD N, iMsg;
	iMsg = 104; if (EdRecVar) goto label1; iMsg = 101;
	if (MustCheck) if (MustAdd) goto label1; else { iMsg = 102; goto label1; }
	iMsg = 103; if (MustAdd) goto label1; iMsg = 100;
label1:
	N = Menu(iMsg, 1);
	if (N == 0) return;
	switch (iMsg) {
	case 101: if (N == 4) N = 6; break;
	case 102: if (N == 5) N = 6; break;
	case 103: if (N >= 4) N++; break;
	case 104: N += 2; break;
	}
	switch (N) {
	case 1: { if (Select) Select = false; else if (E->Bool != nullptr) Select = true;
		DisplBool(); NewDisplLL = true; SetNewWwRecAttr(); break; }
	case 2: if (CFld->FldD->Flg && f_Stored != 0) {
		B = CFld->Dupl; CFld->Dupl = !B; DisplTabDupl();
		if (B) E->NDuplSet--; else E->NDuplSet++;
		break;
	}
	case 3: { B = CFld->Tab; CFld->Tab = !B; DisplTabDupl();
		if (B) E->NTabsSet--; else E->NTabsSet++; break; }
	case 4: { AddSwitch = !AddSwitch; NewDisplLL = true; break; }
	case 5: if (!MustCheck) {
		ChkSwitch = !ChkSwitch; NewDisplLL = true; break;
	}
	case 6: { WarnSwitch = !WarnSwitch; NewDisplLL = true;  break; }
	}
}

void PromptSelect()
{
	wwmix ww;

	pstring Txt;
	if (Select) Txt = *E->BoolTxt; else Txt = "";
	if (IsCurrChpt()) ReleaseFDLDAfterChpt();
	ReleaseStore(E->AfterE);
	ww.PromptFilter(Txt, E->Bool, E->BoolTxt);
	if (E->Bool == nullptr) Select = false; else Select = true;
	DisplBool(); SetNewWwRecAttr(); NewDisplLL = true;
}

void SwitchRecs(integer Delta)
{
	LockMode md; longint n1, n2; void* p1; void* p2; XString x1, x2; KeyDPtr k;
#ifdef FandSQL
	if (CFile->IsSQLFile) return;
#endif
	if (NoCreate && NoDelete || WasWK) return;
	if (!TryLMode(WrMode, md, 1)) return;
	p1 = GetRecSpace(); p2 = GetRecSpace();
	CRecPtr = p1; n1 = AbsRecNr(CRec()); ReadRec(n1);
	if (HasIndex) x1.PackKF(VK->KFlds);
	CRecPtr = p2; n2 = AbsRecNr(CRec() + Delta); ReadRec(n2);
	if (HasIndex) { x2.PackKF(VK->KFlds); if (x1.S != x2.S) goto label1; }
	WriteRec(n1); CRecPtr = p1; WriteRec(n2);
	if (HasIndex) {
		k = CFile->Keys; while (k != nullptr) {
			if (k != VK) {
				CRecPtr = p1; k->Delete(n1); CRecPtr = p2; k->Delete(n2);
				CRecPtr = p1; k->Insert(n2, true); CRecPtr = p2; k->Insert(n1, true);
			}
			k = k->Chain;
		}
	}
	SetNewCRec(CRec() + Delta, true); DisplAllWwRecs();
	DisplRecNr(CRec()); E->EdUpdated = true;
	if (IsCurrChpt()) SetCompileAll();
label1:
	OldLMode(md); ReleaseStore(p1); CRecPtr = E->NewRecPtr;
}

bool FinArgs(LinkD* LD, FieldDPtr F)
{
	KeyFldDPtr KF;
	auto result = true; KF = LD->Args; while (KF != nullptr) {
		if (KF->FldD == F) return result; KF = (KeyFldD*)KF->Chain;
	}
	result = false;
	return result;
}

bool SelFldsForEO(EditOpt* EO, LinkD* LD)
{
	wwmix ww;

	FieldDPtr F; FieldList FL, FL1; pstring s; void* p = nullptr;
	auto result = true; if (EO->Flds == nullptr) return result;
	FL = EO->Flds;
	if (!EO->UserSelFlds) {
		if (LD != nullptr) {
			FL1 = FieldList(EO->Flds);
			while (FL != nullptr) {
				if (FinArgs(LD, FL->FldD)) { FL1->Chain = FL; FL1 = FL; }
				FL = (FieldList)FL->Chain;
			}
			FL1->Chain = nullptr;
		}
		return result;
	}
	MarkStore(p);
	while (FL != nullptr) {
		F = FL->FldD;
		if ((LD == nullptr) || !FinArgs(LD, F)) {
			s = F->Name;
			if ((F->Flg & f_Stored) == 0) { pstring olds = s; s = SelMark; s += olds; }
			ww.PutSelect(s);
		}
		FL = (FieldList)FL->Chain;
	}
	if (EO->Flds == nullptr) WrLLF10Msg(156); else ww.SelFieldList(36, true, EO->Flds);
	if (EO->Flds == nullptr) { ReleaseStore(p); result = false; }
	return result;
}

void ImbeddEdit()
{
	wwmix ww;

	void* p = nullptr; pstring s, s1, s2; WORD Brk; StringList SL;
	EditOpt* EO = nullptr; FileDPtr FD = nullptr; RdbD* R = nullptr; longint w;

	MarkStore(p); w = PushW1(1, 1, TxtCols, TxtRows, true, true);
	CFile->IRec = AbsRecNr(CRec());
	WrEStatus(); R = CRdb;
	while (R != nullptr) {
		FD = (FileD*)R->FD->Chain;
		while (FD != nullptr) {
			if (ForNavigate(FD)) {
				SL = FD->ViewNames;
				do {
					s = GetFileViewName(FD, SL);
					if (R != CRdb) s = R->FD->Name + '.' + s;
					ww.PutSelect(s);
				} while (SL != nullptr);
			}
			FD = (FileD*)FD->Chain;
		}
		R = R->ChainBack;
	}
	ss.Abcd = true; ww.SelectStr(0, 0, 35, "");
	if (KbdChar == _ESC_) goto label1;
	GetSel2S(&s1, &s2, '.', 1);
	R = CRdb; if (s2 != "") do {
		R = R->ChainBack;
	} while (R->FD->Name != s2);
	CFile = R->FD;
	while (!EquFileViewName(CFile, s1, EO)) CFile = (FileD*)CFile->Chain;
	if (SelFldsForEO(EO, nullptr)) {
		NewEditD(CFile, EO); if (OpenEditWw()) RunEdit(nullptr, Brk);
		SaveFiles; PopEdit();
	}
label1:
	PopW(w); ReleaseStore(p); RdEStatus(); DisplEditWw();
}

void DownEdit()
{
	wwmix ww;

	LinkDPtr LD = nullptr; FileDPtr FD = nullptr; StringList SL = nullptr; KeyDPtr K = nullptr;
	EditOpt* EO = nullptr; WORD Brk, i; void* p = nullptr; pstring s, s1, s2; longint w;
	MarkStore(p); w = PushW1(1, 1, TxtCols, TxtRows, true, true);
	CFile->IRec = AbsRecNr(CRec());
	WrEStatus();
	LD = LinkDRoot;
	while (LD != nullptr) {
		FD = LD->FromFD;
		if ((LD->ToFD == CFile) && ForNavigate(FD) && (LD->IndexRoot != 0))
			/*own key with equal beginning*/
		{
			SL = FD->ViewNames; K = GetFromKey(LD);
			do {
				s = GetFileViewName(FD, SL);
				if (*K->Alias != "") s = s + '/' + *K->Alias; ww.PutSelect(s);
			} while (SL != nullptr);
		}
		LD = LD->Chain;
	}
	ss.Abcd = true; ww.SelectStr(0, 0, 35, ""); if (KbdChar == _ESC_) goto label1;
	GetSel2S(&s1, &s2, '/', 2);
	LD = LinkDRoot;
	while ((LD->ToFD != E->FD) || (LD->IndexRoot == 0) || (s2 != *GetFromKey(LD)->Alias)
		|| !EquFileViewName(LD->FromFD, s1, EO)) LD = LD->Chain;
	CFile = LD->FromFD;
	if (SelFldsForEO(EO, LD)) {
		EO->DownLD = LD; EO->DownRecPtr = CRecPtr;
		NewEditD(CFile, EO);
		if (OpenEditWw()) RunEdit(nullptr, Brk); SaveFiles; PopEdit();
	}
label1:
	PopW(w); ReleaseStore(p); RdEStatus(); DisplEditWw();
}

void ShiftF7Proc()
{
	FieldDPtr F; KeyFldDPtr KF; LinkDPtr LD, LD1;
	/* find last (first decl.) foreign key link with CFld as an argument */
	F = CFld->FldD; LD = LinkDRoot; LD1 = nullptr;
	while (LD != nullptr) {
		KF = LD->Args;
		while (KF != nullptr) {
			if ((KF->FldD == F) && ForNavigate(LD->ToFD)) LD1 = LD;
			KF = (KeyFldD*)KF->Chain;
		}
		LD = LD->Chain;
	}
	if (LD1 != nullptr) UpwEdit(LD1);
}

bool ShiftF7Duplicate()
{
	EditD* ee; KeyFldD* kf; KeyFldD* kf2;
	auto result = false;
	ee = (EditD*)E->Chain;
	{ /* !!! with ee^ do!!! */
		CFile = ee->FD; CRecPtr = ee->NewRecPtr;
		if (!ELockRec(ee, CFile->IRec, ee->IsNewRec, ee->SubSet)) return result;
		if (!WasUpdated) {
			Move(CRecPtr, ee->OldRecPtr, CFile->RecLen);
			WasUpdated = true;
		}
		kf = E->ShiftF7LD->Args; kf2 = E->ShiftF7LD->ToKey->KFlds;
		while (kf != nullptr) {
			DuplFld(E->FD, CFile, E->NewRecPtr, CRecPtr, ee->OldRecPtr, kf2->FldD, kf->FldD);
			kf = (KeyFldD*)kf->Chain; kf2 = (KeyFldD*)kf2->Chain;
		}
		SetUpdFlag();
	}

	CFile = E->FD; CRecPtr = E->NewRecPtr;
	result = true;
	pstring oldKbdBuffer = KbdBuffer;
	KbdBuffer = 0x0D; // ^M
	KbdBuffer += oldKbdBuffer;
	return result;
}

bool DuplToPrevEdit()
{
	LockMode md;
	auto result = false; EditD* ee = (EditD*)E->Chain; if (ee == nullptr) return result;
	FieldDPtr f1 = CFld->FldD;
	/* !!! with ee^ do!!! */
	{
		FieldDPtr f2 = CFld->FldD;
		if ((f2->Flg && f_Stored == 0) || (f1->Typ != f2->Typ) || (f1->L != f2->L)
			|| (f1->M != f2->M) || !CFld->Ed(IsNewRec)) {
			WrLLF10Msg(140); return result;
		}
		CFile = ee->FD; CRecPtr = ee->NewRecPtr;
		if (!ELockRec(ee, CFile->IRec, ee->IsNewRec, ee->SubSet)) return result;
		if (!WasUpdated) { {}
		Move(CRecPtr, ee->OldRecPtr, CFile->RecLen); WasUpdated = true;
		}
		DuplFld(E->FD, CFile, E->NewRecPtr, CRecPtr, ee->OldRecPtr, f1, f2); SetUpdFlag(); }
	CFile = E->FD; CRecPtr = E->NewRecPtr; result = true;
	pstring oldKbdBuffer = KbdBuffer;
	KbdBuffer = 0x0D; // ^M
	KbdBuffer += oldKbdBuffer;
	return result;
}

void Calculate2()
{
	wwmix ww;

	FrmlPtr Z; pstring Txt; ExitRecord er; WORD I; pstring Msg;
	void* p = nullptr; char FTyp; double R; FieldDPtr F; bool Del;
	MarkStore(p);
	//NewExit(Ovr(), er);
	goto label2; ResetCompilePars();
label0:
	Txt = CalcTxt;
label4:
	I = 1;
	Del = true;
label1:
	TxtEdCtrlUBrk = true;
	TxtEdCtrlF4Brk = true;
	ww.PromptLL(114, &Txt, I, Del);
	if (KbdChar == _U_) goto label0;
	if ((KbdChar == _ESC_) || (Txt.length() == 0)) goto label3;
	CalcTxt = Txt;
	SetInpStr(Txt); RdLex();
	Z = RdFrml(FTyp);
	if (Lexem != 0x1A) Error(21);
	if (KbdChar == _CtrlF4_) {
		F = CFld->FldD;
		if (CFld->Ed(IsNewRec) && (F->FrmlTyp == FTyp)) {
			if (LockRec(true)) {
				if ((F->Typ == 'F') && ((F->Flg & f_Comma) != 0)) {
					auto iZ0 = (FrmlElem0*)Z;
					auto iZ02 = (FrmlElem2*)iZ0->P1;
					if ((Z->Op = _const)) R = ((FrmlElem2*)Z)->R;
					else if ((Z->Op == _unminus) && (iZ02->Op == _const)) R = -iZ02->R;
					else goto label5;
					SetWasUpdated(); R_(F, R * Power10[F->M]);
				}
				else
					label5:
				AssignFld(F, Z);
				DisplFld(CFld, IRec); IVon(); goto label3;
			}
		}
		else WrLLF10Msg(140);
	}
	switch (FTyp) {
	case 'R': {
		R = RunReal(Z); str(R, 30, 10, Txt);
		Txt = LeadChar(' ', TrailChar('0', Txt));
		if (Txt[Txt.length()] == '.') Txt[0]--;
		break; }
	case 'S': Txt = RunShortStr(Z); break;  /* wie RdMode fuer T ??*/
	case 'B': if (RunBool(Z)) Txt = AbbrYes; else Txt = AbbrNo; break;
	}
	goto label4;
label2:
	Msg = MsgLine; I = CurrPos; SetMsgPar(Msg); WrLLF10Msg(110);
	IsCompileErr = false; Del = false; CFile = E->FD; ReleaseStore(p); goto label1;
label3:
	ReleaseStore(p); RestoreExit(er);
}

void DelNewRec()
{
	LockMode md;
	DelAllDifTFlds(CRecPtr, nullptr); if (CNRecs() == 1) return;
	IsNewRec = false; Append = false; WasUpdated = false; CFld = E->FirstFld;
	if (CRec > CNRecs) if (IRec > 1) IRec--; else BaseRec--;
	RdRec(CRec()); NewDisplLL = true; DisplWwRecsOrPage();
}

EFldD* FrstFldOnPage(WORD Page)
{
	EFldD* D = E->FirstFld;
	while (D->Page < Page) D = (EFldD*)D->Chain;
	return D;
}

void F6Proc()
{
	WORD iMsg;
	iMsg = 105;
	if (Subset || HasIndex || NoCreate || NoDelete
#ifdef FandSQL
		|| CFile->IsSQLFile
#endif
		) iMsg = 106;
	switch (Menu(iMsg, 1)) {
	case 1: AutoReport(); break;
	case 2: CheckFromHere(); break;
	case 3: PromptSelect(); break;
	case 4: AutoGraph(); break;
	case 5: Sorting(); break;
	}
}

longint GetEdRecNo()
{
	if (IsNewRec) return 0;
	else if (E->IsLocked) return E->LockedRec;
	else return AbsRecNr(CRec());
}

void SetEdRecNoEtc(longint RNr)
{
	XString* x = (XString*)&EdRecKey; void* cr; KeyD* k;
	EdField = CFld->FldD->Name; EdIRec = IRec; EdRecKey = ""; EdKey = "";
	EdRecNo = RNr; if (RNr == 0) EdRecNo = GetEdRecNo(); if (VK == nullptr) return;
	if (!WasWK && (VK->Alias != nullptr)) {
		EdKey = *VK->Alias; if (EdKey == "") EdKey = '@';
	}
	if (!IsNewRec) {
		cr = CRecPtr; if (WasUpdated) CRecPtr = E->OldRecPtr;
		k = VK; if (Subset) k = WK;
		x->PackKF(k->KFlds); CRecPtr = cr;
	}
}

bool StartProc(Instr_proc* ExitProc, bool Displ)
{
	bool upd; bool b, b2, lkd; char* p = nullptr; FieldDPtr f = nullptr;
	WORD d; LockMode md; /*float t;*/

	auto result = false;
	CFile->WasWrRec = false;
	if (HasTF) {
		p = (char*)GetRecSpace();
		Move(CRecPtr, p, CFile->RecLen);
	}
	SetEdRecNoEtc(0);
	lkd = E->IsLocked;
	if (!lkd && !LockRec(false)) return result;
	b = WasUpdated;
	EdUpdated = b;
	b2 = HasUpdFlag();
	SetWasUpdated();
	ClearUpdFlag();
	/* !!! with ExitProc->TArg[ExitProc->N] do!!! */
	{
		auto tempX = ExitProc->TArg[ExitProc->N];
		tempX.FD = CFile;
		tempX.RecPtr = CRecPtr;
	}
	md = CFile->LMode;
	WrEStatus();                            /*t = currtime;*/
	CallProcedure(ExitProc);
	RdEStatus();
	NewLMode(md);
	upd = CFile->WasWrRec;      /*writeln(strdate(currtime-t,"ss mm.ttt"));wait;*/
	if (HasUpdFlag()) { b = true; upd = true; }
	WasUpdated = b;
	if (b2) SetUpdFlag();
	if (!WasUpdated && !lkd) UnLockRec(E);
	if (Displ && upd) DisplAllWwRecs();
	if (Displ) NewDisplLL = true;
	result = true;
	if (HasTF) {
		f = CFile->FldD;
		while (f != nullptr) {
			if ((f->Typ == 'T') && ((f->Flg & f_Stored) != 0) &&
				(*(longint*)(p + f->Displ) == *(longint*)(E->OldRecPtr) + f->Displ))
				NoDelTFlds = true;
			f = (FieldDescr*)f->Chain;
		}
		ReleaseStore(p);
	}
	return result;
}

void StartRprt(RprtOpt* RO)
{
	bool displ; WKeyDPtr k; KeyFldD* kf;
	if (IsNewRec || EdRecVar || (EdBreak == 16) || !WriteCRec(true, displ)) return;
	if (displ) DisplAllWwRecs(); kf = nullptr; if (VK != nullptr) kf = VK->KFlds;
	k = (WKeyDPtr)GetZStore(sizeof(*k));
	k->OneRecIdx(kf, AbsRecNr(CRec()));
	RO->FDL.FD = CFile;
	RO->FDL.ViewKey = k;
	ReportProc(RO, false);
	CFile = E->FD;
	CRecPtr = E->NewRecPtr;
}


bool StartExit(EdExitD* X, bool Displ)
{
	auto result = true;
	switch (X->Typ) {
	case 'P': result = StartProc(X->Proc, Displ); break;
	case 'R': StartRprt((RprtOpt*)X->RO); break;
	}
	return result;
}

WORD ExitKeyProc()
{
	WORD w = 0;
	WORD c = KbdChar;
	EdExitD* X = E->ExD;
	while (X != nullptr) {
		if (TestExitKey(c, X)) {
			LastTxtPos = -1;
			if (X->Typ == 'Q') w = 1;
			else {
				bool ok = EdOk; EdOk = false;
				StartExit(X, true);
				if (EdOk) w = 3;
				else w = 2;
				EdOk = ok;
			}
		}
		X = (EdExitD*)X->Chain;
	}
	if (((w == 0) || (w == 3)) && (c == _ShiftF7_) && CFld->Ed(IsNewRec)) {
		ShiftF7Proc(); w = 2;
	}
	KbdChar = c;
	return w;
}

void FieldHelp()
{
	Help(CFile->ChptPos.R, CFile->Name + '.' + CFld->FldD->Name, false);
}

void DisplLASwitches()
{
	if (!ChkSwitch) screen.ScrWrStr(0, TxtRows - 1, "L", screen.colors.lSwitch);
	if (!WarnSwitch) screen.ScrWrStr(2, TxtRows - 1, "?", screen.colors.lSwitch);
	if (!EdRecVar && !AddSwitch) screen.ScrWrStr(3, TxtRows - 1, "A", screen.colors.lSwitch);
	if (!WithBoolDispl && Select) screen.ScrWrStr(5, TxtRows - 1, "\x12", screen.colors.lSwitch);
}

void DisplLL()
{
	WORD n;
	//if (E->Last != nullptr) {
	if (!E->Last.empty()) {
		MsgLine = E->Last;
		if (MsgLine.length() > 0) {
			WrLLMsgTxt();
			DisplLASwitches();
		}
		return;
	}
	if (E->ShiftF7LD != nullptr) n = 144;
	else if (NoCreate || Only1Record)
		if (IsNewRec) n = 129;
		else if (EdRecVar) n = 130;
		else n = 128;
	else if (IsNewRec) n = 123;
	else n = 124;
	if (!F1Mode || Mode24) { WrLLMsg(n); DisplLASwitches(); }
}

void DisplCtrlAltLL(WORD Flags)
{
	if ((Flags & 0x04) != 0) {        /* Ctrl */
		if (!E->CtrlLast.empty()) {
			MsgLine = E->CtrlLast;
			WrLLMsgTxt();
		}
		else if (IsCurrChpt) WrLLMsg(125);
		else if (EdRecVar) WrLLMsg(154);
		else WrLLMsg(127);
	}
	else if ((Flags & 0x03) != 0)         /* Shift */
		if (!E->ShiftLast.empty()) {
			MsgLine = E->ShiftLast;
			WrLLMsgTxt();
		}
		else DisplLL();
	else if ((Flags & 0x08) != 0)          /* Alt */
		if (!E->AltLast.empty()) {
			MsgLine = E->AltLast;
			WrLLMsgTxt();
		}
		else DisplLL();
}

void DisplLLHlp()
{
	if (CRdb->HelpFD != nullptr) {
		DisplLLHelp(CFile->ChptPos.R, CFile->Name + '.' + CFld->FldD->Name, Mode24);
	}
}

// po nacteni editoru se smycka drzi tady a ceka na stisknuti klavesy
void CtrlReadKbd()
{
	BYTE flgs = 0;
	longint TimeBeg = TimerRE;
	WORD D = 0;
	TestEvent();
	if (Event.What == evKeyDown || Event.What == evMouseDown) goto label2;
	ClrEvent();
	if (NewDisplLL) {
		DisplLL();
		NewDisplLL = false;
	}
	if (CFile->NotCached()) {
		if (!E->EdRecVar && ((spec.ScreenDelay == 0) || (E->RefreshDelay < spec.ScreenDelay)))
			D = E->RefreshDelay;
		if (E->WatchDelay != 0) {
			if (D == 0) D = E->WatchDelay;
			else D = MinW(D, E->WatchDelay);
		}
	}
	if (F1Mode && Mode24) DisplLLHlp();
label1:
	if (LLKeyFlags != 0) { flgs = LLKeyFlags; goto label11; }
	else if ((KbdFlgs & 0x0f) != 0) {
		flgs = KbdFlgs;
	label11:
		DisplCtrlAltLL(flgs);
	}
	else {
		DisplLL();
		flgs = 0;
		if (F1Mode && !Mode24) DisplLLHlp();
	}
	if (D > 0) {
		if (TimerRE >= TimeBeg + D) goto label2;
		else WaitEvent(TimeBeg + D - TimerRE);
	}
	else WaitEvent(0);
	if (!(Event.What == evKeyDown || Event.What == evMouseDown)) {
		ClrEvent();
		goto label1;
	}
label2:
	if (flgs != 0) {
		LLKeyFlags = 0;
		DisplLL();
		AddCtrlAltShift(flgs);
	}
}

void MouseProc()
{
	WORD i; longint n; EFldD* D; bool Displ;
	for (i = 1; i < E->NRecs; i++) {
		n = BaseRec + i - 1; if (n > CNRecs()) goto label1;
		D = E->FirstFld; while (D != nullptr) {
			if (IsNewRec && (i == IRec) && (D == FirstEmptyFld)) goto label1;
			if ((D->Page == CPage) && MouseInRect(D->Col - 1, FldRow(D, i) - 1, D->L, 1)) {
				if ((i != IRec) && (IsNewRec || !WriteCRec(true, Displ))) goto label1;
				GotoRecFld(n, D); if ((Event.Buttons && mbDoubleClick) != 0) {
					if (MouseEnter) Event.KeyCode = _M_; else Event.KeyCode = _Ins_;
					Event.What = evKeyDown; return;
				}
				else ClrEvent();
				return;
			}
			D = (EFldD*)D->Chain;
		};
	}
label1:
	ClrEvent();
}

void ToggleSelectRec()
{
	XString x; WKeyDPtr k; longint n; LockMode md;
	k = E->SelKey; n = AbsRecNr(CRec());
	if (k->RecNrToPath(x, n)) { k->NR--; k->DeleteOnPath(); }
	else { k->NR++; k->Insert(n, false); }
	SetRecAttr(IRec); IVon();
}

void ToggleSelectAll()
{
	WKeyDPtr k;
	k = E->SelKey;
	if (k == nullptr) return;
	if (k->NR > 0) k->Release();
	else if (Subset) CopyIndex(k, WK); else CopyIndex(k, VK);
	DisplAllWwRecs();
}

void GoStartFld(EFldD* SFld)
{
	while ((CFld != SFld) && (CFld->Chain != nullptr)) {
		if (IsFirstEmptyFld()) {
			if ((CFld->Impl != nullptr) && LockRec(true)) AssignFld(CFld->FldD, CFld->Impl);
			FirstEmptyFld = (EFldD*)FirstEmptyFld->Chain; DisplFld(CFld, IRec);
		}
		GotoRecFld(CRec(), (EFldD*)CFld->Chain);
	}
}

void RunEdit(XString* PX, WORD& Brk)
{
	WORD i = 0, LongBeep = 0, w = 0; bool Displ = false, b = false;
	EdExitD* X = nullptr;
	longint OldTimeW = 0, OldTimeR = 0, n = 0;
	BYTE EdBr = 0;
	longint Timer = 0; // pùvodnì: Timer:longint absolute 0:$46C;

	Brk = 0;
	DisplLL();
	if (OnlySearch) goto label2;
	if (!IsNewRec && (PX != nullptr)) GotoXRec(PX, n);
	if (Select && !RunBool(E->Bool)) GoPrevNextRec(+1, true);
	//if (/*E->StartFld != nullptr*/ true) { GoStartFld(&E->StartFld); goto label1; }
	if (E->StartFld != nullptr) { GoStartFld(E->StartFld); goto label1; }
label0:
	if (!CtrlMProc(0)) goto label7;
label1:
	LongBeep = 0;
label8:
	OldTimeW = Timer;
label81:
	OldTimeR = Timer;
	CtrlReadKbd();
	if (CFile->NotCached()) {
		if (!EdRecVar && (E->RefreshDelay > 0) && (OldTimeR + E->RefreshDelay < Timer))
			DisplAllWwRecs();
		if (Event.What == 0)
			if ((E->WatchDelay > 0) && (OldTimeW + E->WatchDelay < Timer))
				if (LongBeep < 3) {
					for (i = 1; i < 4; i++) beep();
					LongBeep++;
					goto label8;
				}
				else { UndoRecord(); EdBreak = 11; goto label7; }
			else goto label81;
	}
	switch (Event.What) {
	case evMouseDown: {
		if (F1Mode && (CRdb->HelpFD != nullptr) && (Mode24 && (Event.Where.Y == TxtRows - 2) ||
			!Mode24 && (Event.Where.Y == TxtRows - 1))) {
			ClrEvent();
			FieldHelp();
		}
		else MouseProc();
		break;
	}
	case evKeyDown: {
		KbdChar = Event.KeyCode;
		ClrEvent();
		switch (ExitKeyProc())
		{
		case 1:/*quit*/ goto label7; break;
		case 2:/*exit*/ goto label1; break;
		}
		switch (KbdChar) {
		case _F1_: { RdMsg(7); Help((RdbD*)&HelpFD, MsgLine, false); break; }
		case _CtrlF1_: FieldHelp(); break;
		case _AltF10_: Help(nullptr, "", false); break;
		case _ESC_: {
			if (OnlySearch) {
				if (IsNewRec) { if (CNRecs() > 1) DelNewRec(); else goto label9; }
				else if (!WriteCRec(true, Displ)) goto label1;
			label2:
				if (PromptAndSearch(!NoCreate)) goto label0;
			}
		label9:
			EdBreak = 0;
		label7:
			if (IsNewRec && !EquOldNewRec())
				if (!Prompt158 || PromptYN(158)) goto fin;
				else goto label1;
			EdBr = EdBreak; n = GetEdRecNo();
			if ((IsNewRec || WriteCRec(true, Displ)) and
				((EdBreak == 11) || NoESCPrompt or
					!spec.ESCverify && !MustESCPrompt
					|| PromptYN(137))) {
				EdBreak = EdBr; SetEdRecNoEtc(n); goto label71;
			fin:
				SetEdRecNoEtc(0);
			label71:
				if (IsNewRec && !EdRecVar) DelNewRec();
				IVoff(); EdUpdated = E->EdUpdated;
				if (!EdRecVar) ClearRecSpace(E->NewRecPtr);
				if (Subset && !WasWK) WK->Close();
				if (!EdRecVar) {
#ifdef FandSQL
					if (CFile->IsSQLFile) Strm1->EndKeyAcc(WK);
#endif
					OldLMode(E->OldMd);
			}
				return;
		}
			break;
		}
		case _AltEqual_: { UndoRecord(); EdBreak = 0; goto fin; }
		case _U_: if (PromptYN(108)) UndoRecord(); break;
		case 0x1C /*^\*/: if (!CtrlMProc(2)) goto label7;
		case _F2_: {
			if (!EdRecVar)
				if (IsNewRec) {
					if ((CNRecs() > 1) && (!Prompt158 || EquOldNewRec() || PromptYN(158))) DelNewRec();
				}
				else if (!NoCreate && !Only1Record && WriteCRec(true, Displ))
				{
					if (Displ) DisplAllWwRecs(); SwitchToAppend();
				} goto label0;
			break;
		}
		case _up_: if (LUpRDown) goto label11; else goto label13;
		case _down_: if (LUpRDown) goto label12; else goto label13;
		case _left_:
		case _S_:
		label11:
			if (CFld->ChainBack != nullptr) GotoRecFld(CRec(), CFld->ChainBack);
			break;
		case _right_:
		case _D_:
		label12:
			if ((CFld->Chain != nullptr) && !IsFirstEmptyFld())
				GotoRecFld(CRec(), (EFldD*)CFld->Chain);
			break;
		case _Home_:
		label3:
			GotoRecFld(CRec(), E->FirstFld); break;
		case _End_:
		label4:
			if (IsNewRec && (FirstEmptyFld != nullptr))
				GotoRecFld(CRec(), FirstEmptyFld);
			else GotoRecFld(CRec(), E->LastFld);
			break;
		case _M_:
			if (SelMode && (E->SelKey != nullptr) && !IsNewRec) {
				if (WriteCRec(true, Displ)) {
					if ((E->SelKey != nullptr) && (E->SelKey->NRecs() == 0)) ToggleSelectRec();
					EdBreak = 12; goto fin;
				}
			}
			else
				if ((E->ShiftF7LD != nullptr) && !IsNewRec) {
					if (ShiftF7Duplicate()) goto label9;
				}
				else
					if (!CtrlMProc(3)) goto label7;
			break;
		case _Ins_: { b = false; if (CFld->Ed(IsNewRec) && LockRec(true)) b = true;
			if (!EditItemProc(false, b, Brk)) goto label7; if (Brk != 0) goto fin; break; }
		case _F4_:
			if ((CRec() > 1) && (IsFirstEmptyFld() || PromptYN(121)) && LockRec(true))
			{
				DuplFromPrevRec(); if (!CtrlMProc(1)) goto label7;
			}
		case _F5_: SetSwitchProc(); break;
		case _F7_: UpwEdit(nullptr); break;
		case _CtrlF5_: Calculate2(); break;
		default: {
			if (KbdChar >= 0x20 && KbdChar <= 0xFE)
			{
				if (CFld->Ed(IsNewRec) && ((CFld->FldD->Typ != 'T') || (_T(CFld->FldD) == 0))
					&& LockRec(true)) {
					pstring oldKbdBuffer = KbdBuffer;
					KbdBuffer = char(KbdChar);
					KbdBuffer += oldKbdBuffer;
					if (!EditItemProc(true, true, Brk)) goto label7;
					if (Brk != 0) goto fin;
				}
			label13:
				if (!IsNewRec) {
					w = KbdChar;
					if (w == _Y_) {
						if (!NoDelete) if (DeleteRecProc()) {
							ClearKeyBuf(); b = true;
						label14:
							if (((CNRecs() == 0) || (CNRecs() == 1) && IsNewRec) && NoCreate) {
								WrLLF10Msg(112); EdBreak = 13; goto fin;
							}
							if (b && !CtrlMProc(0)) goto label7;
						}
					}
					else if (WriteCRec(true, Displ)) {
						if (Displ) DisplAllWwRecs(); KbdChar = w;       /*only in edit mode*/
						switch (w) {
						case _F9_: { SaveFiles; UpdCount = 0; break; }
						case _N_:
							if (!NoCreate && !Only1Record)
							{
								InsertRecProc(nullptr); goto label0;
							}
							break;
						case _up_:
						case _E_:
							if (E->NRecs > 1) GoPrevNextRec(-1, true); break;
						case _CtrlHome_:
							GoPrevNextRec(-1, true); break;
						case _down_:
						case _X_:
							if (E->NRecs > 1) GoPrevNextRec(+1, true); break;
						case _CtrlEnd_:
							GoPrevNextRec(+1, true); break;
						case _PgUp_:
						case _R_:
							if (E->NPages == 1)
								if (E->NRecs == 1) GoPrevNextRec(-1, true);
								else GotoRecFld(CRec() - E->NRecs, CFld);
							else if (CPage > 1) GotoRecFld(CRec(), FrstFldOnPage(CPage - 1));
							break;
						case _PgDn_:
						case _C_:
							if (E->NPages == 1)
								if (E->NRecs == 1) GoPrevNextRec(+1, true);
								else GotoRecFld(CRec() + E->NRecs, CFld);
							else if (CPage < E->NPages) GotoRecFld(CRec(), FrstFldOnPage(CPage + 1));
							break;
						case _Q_:
							switch (ReadKbd())
							{
							case _S_: goto label3;
							case _D_: goto label4;
							case _R_: goto label5;
							case _C_: goto label6;
							}
							break;
						case _CtrlPgUp_:
						label5:
							GotoRecFld(1, E->FirstFld); break;
						case _CtrlPgDn_:
						label6:
							GotoRecFld(CNRecs(), E->LastFld); break;
						case _CtrlLeft_:
							if (CRec() > 1) SwitchRecs(-1); break;
						case _CtrlRight_:
							if (CRec < CNRecs) SwitchRecs(+1); break;
						case _F3_: {
							if (!EdRecVar)
								if (CFile == CRdb->HelpFD) {
									if (PromptHelpName(i)) { GotoRecFld(i, CFld); goto label1; }
								}
								else { PromptAndSearch(false); goto label0; }
							break;
						}
						case _CtrlF2_:
						{ if (!EdRecVar) RefreshSubset(); b = false; goto label14; break; }
						case _AltF2_:
						case _AltF3_:
							if (IsCurrChpt())
								if (w == _AltF3_) {
									ForAllFDs(ClosePassiveFD); EditHelpOrCat(w, 0, "");
								}
								else { Brk = 2; goto fin; }
							else if (IsTestRun && (CFile != CatFD) && (w == _AltF2_))
								EditHelpOrCat(w, 1, CFile->Name + '.' + CFld->FldD->Name);
							break;
						case _CtrlF3_:
							if (!EdRecVar) PromptGotoRecNr(); break;
						case _F6_: if (!EdRecVar) F6Proc(); break;
						case _CtrlF4_: if (DuplToPrevEdit()) { EdBreak = 14; goto fin; } break;
						case _CtrlF7_: DownEdit(); break;
						case _F8_: {
							if (E->SelKey != nullptr) {
								ToggleSelectRec(); GoPrevNextRec(+1, true);
							}
							break;
						}
						case _ShiftF8_: ToggleSelectAll(); break;
						case _CtrlF8_:
						case _CtrlF9_:
						case _CtrlF10_:
						case _AltF9_:
							if (IsCurrChpt()) { Brk = 2; goto fin; }
							break;
						case _AltF7_:
							ImbeddEdit();
							break;
						}
					}
				}
			}
		}
	}
	}
	default: ClrEvent(); break;
}
	goto label1;
}

void EditDataFile(FileDPtr FD, EditOpt* EO)
{
	void* p = nullptr;
	longint w1 = 0, w2 = 0, w3 = 0;
	WORD Brk = 0, r1 = 0, r2 = 0;
	bool pix = false; ExitRecord er;
	MarkStore(p);
	if (EO->SyntxChk) {
		IsCompileErr = false;
		//NewExit(Ovr(), er);
		//goto label1; 
		NewEditD(FD, EO);
	label1:
		RestoreExit(er);
		if (IsCompileErr) {
			EdRecKey = MsgLine;
			LastExitCode = CurrPos + 1;
			IsCompileErr = false;
		}
		else LastExitCode = 0;
		goto label2;
	}
	NewEditD(FD, EO);
	w2 = 0; w3 = 0;
	pix = (E->WFlags & WPushPixel) != 0;
	if (E->WwPart) /* !!! with E^ do!!! */
	{
		if (E->WithBoolDispl) r2 = 2;
		else r2 = 1;
		r1 = TxtRows; if (E->Mode24) r1--;
		w1 = PushW1(1, 1, TxtCols, r2, pix, true);
		w2 = PushW1(1, r1, TxtCols, TxtRows, pix, true);
		if ((E->WFlags & WNoPop) == 0)
			w3 = PushW1(E->V.C1, E->V.R1, E->V.C2 + E->ShdwX, E->V.R2 + E->ShdwY, pix, true);
	}
	else w1 = PushW1(1, 1, TxtCols, TxtRows, pix, true);
	if (OpenEditWw()) {
		if (OnlyAppend && !Append) SwitchToAppend();
	}
	RunEdit(nullptr, Brk);
	if (w3 != 0) PopW(w3);
	if (w2 != 0) PopW(w2);
	PopW(w1);
label2:
	PopEdit();
	ReleaseStore(p);
}
