#include "wwmenu.h"


#include "common.h"
#include "kbdww.h"
#include "obaseww.h"

bool TRect::Contains(TPoint* P)
{
	return (P->X >= A.X) && (P->X < A.X + Size.X) && (P->Y >= A.Y) && (P->Y < A.Y + Size.Y);
}

TWindow::TWindow()
{
}

TWindow::TWindow(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, pstring top, pstring bottom, bool SaveLL)
{
	WORD i, n, m;
	pstring s(80);
	BYTE* l = (BYTE*)&s;
	Assign(C1, R1, C2, R2);
	if (GetState(sfShadow)) {
		Shadow.X = MinW(2, TxtCols - Col2());
		Shadow.Y = MinW(1, TxtRows - Row2());
	}
	WasCrsEnabled = Crs.Enabled;
	CrsHide();
	SavedW = PushW1(Orig.X + 1, Orig.Y + 1, Orig.X + Size.X + Shadow.X, Orig.Y + Size.Y + Shadow.Y, true, false);
	if (SaveLL) { SavedLLW = PushW1(1, TxtRows, TxtCols, TxtRows, true, false); }
	else { SavedLLW = 0; }
	if (Shadow.Y == 1) ScrColor(Orig.X + 2, Row2(), Size.X + Shadow.X - 2, colors.ShadowAttr);
	if (Shadow.X > 0)
		for (i = Row1(); i < Row2(); i++) ScrColor(Col2(), i, Shadow.X, colors.ShadowAttr);
	if (GetState(sfFramed)) {
		n = 0;
		if (GetState(sfFrDouble)) n = 9;
		ScrWrFrameLn(Orig.X, Orig.Y, n, Size.X, Attr);
		for (i = 1; i < Size.Y - 2; i++)
			ScrWrFrameLn(Orig.X, Orig.Y + i, n + 6, Size.X, Attr);
		ScrWrFrameLn(Orig.X, Orig.Y + Size.Y - 1, n + 3, Size.X, Attr);
		m = Size.X - 2;
		if (top.length() != 0) {
			s = " "; s += top + " ";
			*l = MinW(*l, m);
			ScrWrStr(Col1() + (m - *l) / 2, Orig.Y, s, Attr);
		}
		if (bottom.length() != 0) {
			s = " "; s += bottom + " "; *l = MinW(*l, m);
			ScrWrStr(Col1() + (m - *l) / 2, Row2() - 1, s, Attr);
		}
	}
	else ScrClr(Orig.X, Orig.Y, Size.X, Size.Y, ' ', Attr);
}

TWindow::~TWindow()
{
	if (SavedLLW != 0) PopW(SavedLLW);
	PopW(SavedW);
	if (WasCrsEnabled) CrsShow();
}

void TWindow::Assign(BYTE C1, BYTE R1, BYTE C2, BYTE R2)
{
	integer cols, rows, m;
	m = 0;
	if (GetState(sfFramed)) m = 2;
	cols = C2 + m; if (C1 != 0) cols = C2 - C1 + 1;
	cols = maxi(m + 1, mini(cols, TxtCols));
	if (C1 == 0) Orig.X = (TxtCols - cols) / 2;
	else Orig.X = mini(C1 - 1, TxtCols - cols);
	rows = R2 + m; if (R1 != 0) rows = R2 - R1 + 1;
	rows = maxi(m + 1, mini(rows, TxtRows));
	if (R1 == 0) Orig.Y = (TxtRows - rows) / 2;
	else Orig.Y = mini(R1 - 1, TxtRows - rows);
	Size = { cols, rows };
}

WORD TWindow::Col1()
{
	return Orig.X + 1;
}

WORD TWindow::Row1()
{
	return Orig.Y + 1;
}

WORD TWindow::Col2()
{
	return Orig.X + Size.X;
}

WORD TWindow::Row2()
{
	return Orig.Y + Size.Y;
}

bool TWindow::Contains(TPoint* T)
{
	TRect* R = (TRect*)&Orig;
	return R->Contains(T);
}

bool TWindow::GetState(WORD Flag)
{
	return (State & Flag) == Flag;
}

void TWindow::SetState(WORD Flag, bool On)
{
	if (On) { State = State | Flag; }
	else { State = State & !Flag; }
}

TMenu::TMenu()
{
	mx = MenuX; my = MenuY;
}

void TMenu::ClearHlp()
{
	if (HlpRdb != nullptr) ClearLL(colors.uNorm);
}

bool TMenu::Enabled(WORD I)
{
	return true;
}

bool TMenu::ExecItem(WORD I)
{
	return false;
}

bool TMenu::FindChar()
{
	pstring s(80);
	WORD i, j, k; char c2;
	bool result = false; i = iTxt;
	for (j = 1; j <= nTxt; j++) if (Enabled(j)) {
		s = GetText(j); if (s.length() > 0) {
			k = s.first(0x17 /* CTRL+W */);
			if (k != 0) c2 = s[k + 1];
			else c2 = s[1];
			if (toupper(NoDiakr(c2)) == toupper(NoDiakr(char(KbdChar)))) {
				iTxt = j; WrText(i); return true;
			}
		}
	}
	return result;
}

void TMenu::HandleEvent()
{
	WORD i; pstring hlp(40); bool frst;
	WrText(iTxt); i = iTxt; frst = true;
	hlp = GetHlpName(); TestEvent();
label1:
	KbdChar = 0;
	/* !!! with Event do!!! */
	switch (Event.What) {
	case evMouseDown: {
		if (Contains(&Event.Where)) { KbdChar = _M_; LeadIn(&Event.Where); }
		else if (MouseInRect(0, TxtRows - 1, TxtCols, 1)) goto label2;
		else if (ParentsContain(&Event.Where)) { KbdChar = _ESC_; return; }}
	case evKeyDown: {
		switch (Event.KeyCode) {
		case _Home_:
		case _PgUp_: { iTxt = 0; Next(); WrText(i); break; }
		case _End_:
		case _PgDn_: { iTxt = nTxt + 1; Prev(); WrText(i); break; }
		case  _F1_: {
		label2:
			ClrEvent();
			if (HlpRdb != nullptr) Help(HlpRdb, hlp, false); KbdChar = 0;
			break;
		}
		case _AltF10_: { ClrEvent(); Help(nullptr, "", false); KbdChar = 0; break; }
		case _AltF2_: {
			if (IsTestRun && !IsBoxS) {
				ClrEvent();
				EditHelpOrCat(_AltF2_, 2, hlp);
				KbdChar = 0;
			}
			break;
		}
		default:
			KbdChar = Event.KeyCode; break;
		}
	}
	default: {
		if (frst) { DisplLLHelp(HlpRdb, hlp, false); frst = false; }
		ClrEvent(); WaitEvent(0); goto label1;
	}
	}
	ClrEvent();
}

bool TMenu::IsMenuBar()
{
	return Size.Y = 1;
}

void TMenu::LeadIn(TPoint* T)
{
	WORD i, j; TRect r = { {0,0},{0,0} };
	i = iTxt;
	for (j = 1; j <= nTxt; j++) {
		GetItemRect(j, r);
		if (r.Contains(T) && Enabled(j) && (GetText(j) != ""))
		{
			iTxt = j;
			WrText(i); WrText(j);
			return;
		}
	}
	KbdChar = 0;
}

void TMenu::Next()
{
	do {
		if (iTxt < nTxt) iTxt++;
		else iTxt = 1;
	} while (!(Enabled(iTxt) && (GetText(iTxt) != "")));
}

bool TMenu::ParentsContain(TPoint* T)
{
	TMenu* P = parent;
	bool result = true;
	while (P != nullptr) {
		if (P->Contains(T)) return result;
		P = P->parent;
	}
	return false;
}

void TMenu::Prev()
{
	do
	{
		if (iTxt > 1) iTxt--;
		else iTxt = nTxt;
	} while (!(Enabled(iTxt) && (GetText(iTxt) != "")));
}

bool TMenu::UnderMenuBar()
{
	return (parent != nullptr) && (parent->IsMenuBar);
}

void TMenu::WrText(WORD I)
{
	pstring s(80);
	WORD j, posw, x, x2, y;
	BYTE attr;
	TRect r = { {0,0}, {0,0} };
	bool red, ena;

	s = GetText(I);
	if (s.length() == 0) {  /* menubox only */
		ScrWrFrameLn(Orig.X, Orig.Y + I, 18, Size.X, Palette[0]);
		return;
	}
	GetItemRect(I, r); x = r.A.X; y = r.A.Y; x2 = x + r.Size.X;
	ena = Enabled(I);

	if (I == iTxt) attr = Palette[1];
	else if (ena) attr = Palette[0];
	else attr = Palette[3];

	posw = s.first(0x17); ScrWrChar(x, y, ' ', attr); x++;
	red = false;
	for (j = 1; j <= s.length(); j++)
	{
		if ((s[j] == 0x17) || (posw == 0) && ((j == 1) || (j == 2)))
			if (ena && (I != iTxt)) {
				if (red) { attr = Palette[0]; red = false; }
				else { attr = Palette[2]; red = true; }
			}
		if (s[j] != 0x17) { ScrWrChar(x, y, s[j], attr); x++; };
	} while (x < x2) { ScrWrChar(x, y, ' ', attr); x++; }
}

void TMenu::SetPalette(Instr* aPD)
{
	WORD i;
	/* !!! with aPD^ do!!! */
	Palette[0] = RunWordImpl(aPD->mAttr[0], colors.mNorm);
	Palette[1] = RunWordImpl(aPD->mAttr[1], colors.mHili);
	Palette[2] = RunWordImpl(aPD->mAttr[2], colors.mFirst);
	Palette[3] = RunWordImpl(aPD->mAttr[3], colors.mDisabled);
}

TMenuBox::TMenuBox(WORD C1, WORD R1)
{
	WORD cols, c2, r2, i, l; pstring hd(80);
	hd = GetText(0); cols = hd.length();
	for (i = 1; i <= nTxt; i++) {
		l = LenStyleStr(GetText(i));
		if (l > cols) cols = l;
	}
	cols += 4;
	if ((cols + 2 > TxtCols) || (nTxt + 2 > TxtRows)) RunError(636);
	c2 = cols; r2 = nTxt;
	if (C1 != 0) c2 += C1 + 1;
	if (R1 != 0) r2 += R1 + 1;
	SetState(sfFramed, true);
	TWindow(C1, R1, c2, r2, Palette[0], hd, "", HlpRdb != nullptr);
	for (i = 1; i <= nTxt; i++) { WrText(i); }
}

WORD TMenuBox::Exec(WORD IStart)
{
	WORD i, j; PMenuBox w;
	if (nTxt == 0) { return 0; }
	j = 0; iTxt = IStart; if (iTxt == 0) iTxt = 1;
	Prev(); Next();  /*get valid iTxt*/
label1:
	HandleEvent();
	i = iTxt;
	switch (KbdChar) {
	case _M_: goto label2; break;
	case _ESC_: { i = 0; goto label3; break; }
	case _up_: { Prev(); WrText(i); break; }
	case _down_: { Next(); WrText(i); break; }
	case _left_: if (UnderMenuBar()) { j = 1; goto label4; } break;
	case _right_: if (UnderMenuBar()) { j = 2; goto label4; } break;
	default: {
		if (not FindChar) goto label1; WrText(iTxt);
	label2:
		i = iTxt; MenuX = Orig.X + 4; MenuY = Orig.Y + i + 2;
	label3:
		ClearHlp(); if (not ExecItem(i)) {
		label4:
			return (j << 8) + i;
		}
		break;
	}
	}
	goto label1;
}

void TMenuBox::GetItemRect(WORD I, TRect R)
{
	R.A.X = Orig.X + 2; R.A.Y = Orig.Y + I; R.Size.X = Size.X - 4; R.Size.Y = 1;
}

TMenu::~TMenu()
{
	MenuX = mx; MenuY = my;
}




