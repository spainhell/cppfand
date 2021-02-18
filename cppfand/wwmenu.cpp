#include "wwmenu.h"


#include "GlobalVariables.h"
#include "legacy.h"
#include "obaseww.h"
#include "runfrml.h"
#include "runproc.h"
#include "runproj.h"
#include "../textfunc/textfunc.h"


WORD CountNTxt(ChoiceD* C, bool IsMenuBar)
{
	WORD n = 0, nValid = 0;
	//pstring s; 
	std::string s;
	bool b = false;
	while (C != nullptr) {
		b = RunBool(C->Bool);
		C->Displ = false;
		if (b || C->DisplEver) {
			C->Displ = true;
			n++;
			s = RunShortStr(C->TxtFrml);
			if (s.length() != 0) {
				short maxLen = min(s.length(), TxtCols - 6);
				if (s.length() > maxLen) s = s.substr(0, maxLen);
				char ctrlW = '\x17';
				if (s.find(ctrlW) == std::string::npos) {
					char arr[MaxTxtCols]{ 0 };
					sprintf_s(arr, "\x17%c\x17%s", s[0], &(s.c_str()[1]));
					s = arr;
				}
			}
			else if (IsMenuBar) s = " ";
			C->Txt = s;
			if (C->Txt == "") b = false;
			if (b) nValid++;
		}
		C->Enabled = b;
		C = (ChoiceD*)C->Chain;
	}
	if (nValid == 0) n = 0;
	return n;
}

ChoiceD* CI(ChoiceD* C, WORD I)
{
label1:
	if (C->Displ) I--;
	if (I == 0) { return C; }
	C = (ChoiceD*)C->Chain;
	goto label1;
}

bool TRect::Contains(TPoint* P)
{
	return (P->X >= A.X) && (P->X < A.X + Size.X) && (P->Y >= A.Y) && (P->Y < A.Y + Size.Y);
}

TWindow::TWindow()
{
	Orig.X = 0;
	Orig.Y = 0;
	Size.X = 0;
	Size.Y = 0;
	Shadow.X = 0;
	Shadow.Y = 0;
}

TWindow::TWindow(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, pstring top, pstring bottom, bool SaveLL)
{
	//InitTWindow(C1, R1, C2, R2, Attr, top, bottom, SaveLL);
}

void TWindow::InitTWindow(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, std::string top, std::string bottom, bool SaveLL)
{
	WORD i = 0, n = 0, m = 0;
	//pstring s;
	//BYTE* l = (BYTE*)&s;
	Assign(C1, R1, C2, R2);
	if (GetState(sfShadow)) {
		Shadow.X = MinW(2, TxtCols - Col2());
		Shadow.Y = MinW(1, TxtRows - Row2());
	}
	WasCrsEnabled = Crs.Enabled;
	screen.CrsHide();
	SavedW = PushW1(Orig.X + 1, Orig.Y + 1, Orig.X + Size.X + Shadow.X, Orig.Y + Size.Y + Shadow.Y, true, false);
	if (SaveLL) {
		SavedLLW = PushW1(1, TxtRows, TxtCols, TxtRows, true, false);
	}
	else {
		SavedLLW = 0;
	}
	if (Shadow.Y == 1) screen.ScrColor(Orig.X + 2, Row2(), Size.X + Shadow.X - 2, screen.colors.ShadowAttr);
	if (Shadow.X > 0)
		for (i = Row1(); i <= Row2(); i++)
			screen.ScrColor(Col2(), i, Shadow.X, screen.colors.ShadowAttr);
	if (GetState(sfFramed)) {
		n = 0;
		if (GetState(sfFrDouble)) n = 9;
		screen.ScrWrFrameLn(Orig.X + 1, Orig.Y + 1, n, Size.X, Attr);
		for (i = 1; i <= Size.Y - 2; i++)
			screen.ScrWrFrameLn(Orig.X + 1, Orig.Y + i + 1, n + 6, Size.X, Attr);
		screen.ScrWrFrameLn(Orig.X + 1, Orig.Y + Size.Y, n + 3, Size.X, Attr);
		m = Size.X - 2;
		if (top.length() != 0) {
			std::string sTop = " " + top + " ";
			short l = min(sTop.length(), m);
			if (l > m) sTop = sTop.substr(0, m);
			screen.ScrWrStr(Col1() + (m - sTop.length()) / 2, Orig.Y + 1, sTop, Attr);
		}
		if (bottom.length() != 0) {
			std::string sBottom = " " + bottom + " ";
			short l = min(sBottom.length(), m);
			if (l > m) sBottom = sBottom.substr(0, m);
			screen.ScrWrStr(Col1() + (m - sBottom.length()) / 2, Row2() - 1, sBottom, Attr);
		}
	}
	else { 
		screen.ScrClr(Orig.X + 1, Orig.Y + 1, Size.X, Size.Y, ' ', Attr); 
	}
}

TWindow::~TWindow()
{
	if (SavedLLW != 0) PopW(SavedLLW);
	PopW(SavedW);
	if (WasCrsEnabled) screen.CrsShow();
}

void TWindow::Assign(BYTE C1, BYTE R1, BYTE C2, BYTE R2)
{
	WORD cols = 0, rows = 0, m = 0;
	m = 0;
	if (GetState(sfFramed)) m = 2;
	cols = C2 + m;
	if (C1 != 0) cols = C2 - C1 + 1;
	cols = MaxI(m + 1, MinI(cols, TxtCols));
	if (C1 == 0) Orig.X = (TxtCols - cols) / 2;
	else Orig.X = MinI(C1 - 1, TxtCols - cols);
	rows = R2 + m;
	if (R1 != 0) rows = R2 - R1 + 1;
	rows = MaxI(m + 1, MinI(rows, TxtRows));
	if (R1 == 0) Orig.Y = (TxtRows - rows) / 2;
	else Orig.Y = MinI(R1 - 1, TxtRows - rows);
	Size.X = cols;
	Size.Y = rows;
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

void TMenu::ClearHlp()
{
	if (HlpRdb != nullptr) ClearLL(screen.colors.uNorm);
}

bool TMenu::FindChar()
{
	pstring s(80);
	WORD i, j, k; char c2;
	bool result = false; i = iTxt;
	for (j = 1; j <= nTxt; j++) if (Enabled(j)) {
		s = GetText(j); 
		if (s.length() > 0) {
			k = s.first(0x17 /* CTRL+W */);
			if (k != 0) c2 = s[k + 1];
			else c2 = s[1];
			if (toupper(NoDiakr(c2)) == toupper(NoDiakr((char)KbdChar))) {
				iTxt = j; 
				WrText(i);
				return true;
			}
		}
	}
	return result;
}

void TMenu::HandleEvent()
{
	WORD i = 0; bool frst = false;
	WrText(iTxt);
	i = iTxt; frst = true;
	std::string hlp = GetHlpName();
	TestEvent();
label1:
	KbdChar = 0;
	//ReadKbd();
	/* !!! with Event do!!! */
	switch (Event.What) {
	case evMouseDown: {
		if (Contains(&Event.Where)) { KbdChar = _M_; LeadIn(&Event.Where); }
		else if (MouseInRect(0, TxtRows - 1, TxtCols, 1)) goto label2;
		else if (ParentsContain(&Event.Where)) { KbdChar = _ESC_; return; }}
	case evKeyDown: {
		if (Event.Pressed.Char() == '\0') {
			switch (Event.KeyCode) {
			case VK_HOME:
			case VK_PRIOR: {
				iTxt = 0;
				Next();
				WrText(i);
				break;
			}
			case VK_END:
			case VK_NEXT: {
				iTxt = nTxt + 1;
				Prev();
				WrText(i);
				break;
			}
			case VK_F1: {
			label2:
				ClrEvent();
				if (HlpRdb != nullptr) Help(HlpRdb, hlp, false);
				KbdChar = 0;
				break;
			}
			case VK_F10: {
				if (Event.Pressed.Alt()) {
					ClrEvent();
					Help(nullptr, "", false);
					KbdChar = 0;
				}
				break;
			}
			case VK_F2: {
				if (Event.Pressed.Alt() && IsTestRun && !IsBoxS) {
					ClrEvent();
					EditHelpOrCat(_AltF2_, 2, hlp);
					KbdChar = 0;
				}
				break;
			}
			default: {
				KbdChar = Event.KeyCode;
				break;
			}
			}
		}
		else {
			KbdChar = Event.KeyCode;
		}
		break;
	}
	default: {
		if (frst) {
			DisplLLHelp(HlpRdb, hlp, false);
			frst = false;
		}
		ClrEvent();
		WaitEvent(0);
		goto label1;
	}
	}
	ClrEvent();
}

bool TMenu::IsMenuBar()
{
	return Size.Y == 1;
}

void TMenu::LeadIn(TPoint* T)
{
	WORD i, j;
	TRect r{ {0,0},{0,0} };
	i = iTxt;
	for (j = 1; j <= nTxt; j++) {
		GetItemRect(j, &r);
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
	return (parent != nullptr) && (parent->IsMenuBar());
}

void TMenu::WrText(WORD I)
{
	WORD j = 0, posw = 0, x = 0, x2 = 0, y = 0;
	BYTE attr = 0;
	TRect r{ {0,0}, {0,0} };
	bool red = false, ena = false;

	pstring s = GetText(I);
	if (s.length() == 0) {  /* menubox only */
		screen.ScrWrFrameLn(Orig.X + 1, Orig.Y + I + 1, 18, Size.X, Palette[0]);
		return;
	}
	GetItemRect(I, &r);
	x = r.A.X; y = r.A.Y; x2 = x + r.Size.X;
	ena = Enabled(I);

	if (I == iTxt) attr = Palette[1];
	else if (ena) attr = Palette[0];
	else attr = Palette[3];

	posw = s.first(0x17);
	screen.ScrWrChar(x, y, ' ', attr); x++;
	red = false;
	for (j = 1; j <= s.length(); j++) {
		if ((s[j] == 0x17) || (posw == 0) && ((j == 1) || (j == 2)))
			if (ena && (I != iTxt)) {
				if (red) { attr = Palette[0]; red = false; }
				else { attr = Palette[2]; red = true; }
			}
		if (s[j] != 0x17) { screen.ScrWrChar(x, y, s[j], attr); x++; };
	}
	while (x < x2) { screen.ScrWrChar(x, y, ' ', attr); x++; }
}

void TMenu::SetPalette(Instr_menu* aPD)
{
	WORD i;
	/* !!! with aPD^ do!!! */
	Palette[0] = RunWordImpl(aPD->mAttr[0], screen.colors.mNorm);
	Palette[1] = RunWordImpl(aPD->mAttr[1], screen.colors.mHili);
	Palette[2] = RunWordImpl(aPD->mAttr[2], screen.colors.mFirst);
	Palette[3] = RunWordImpl(aPD->mAttr[3], screen.colors.mDisabled);
}

//TMenu::TMenu(WORD mx, WORD my)
//{
//	this.mx = mx;
//	this.my = my;
//}

TMenuBox::TMenuBox()
{
}

void TMenuBox::InitTMenuBox(WORD C1, WORD R1)
{
	WORD cols = 0, c2 = 0, r2 = 0, i = 0, l = 0;

	std::string hd = GetText(0);
	cols = hd.length();
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
	InitTWindow(C1, R1, c2, r2, Palette[0], hd, "", HlpRdb != nullptr);
	for (i = 1; i <= nTxt; i++) { WrText(i); }
}

WORD TMenuBox::Exec(WORD IStart)
{
	WORD i = 0, j = 0;
	TMenuBox* w = nullptr;
	if (nTxt == 0) { return 0; }
	j = 0;
	iTxt = IStart;
	if (iTxt == 0) iTxt = 1;
	Prev();
	Next();  /*get valid iTxt*/
label1:
	HandleEvent();
	i = iTxt;
	switch (KbdChar) {
	case VK_RETURN: goto label2; break;
	case VK_ESCAPE: { i = 0; goto label3; break; }
	case VK_UP: { Prev(); WrText(i); break; }
	case VK_DOWN: { Next(); WrText(i); break; }
	case VK_LEFT: { 
		if (UnderMenuBar()) { 
			j = 1; 
			goto label4; 
		} 
		break; 
	}
	case VK_RIGHT: { 
		if (UnderMenuBar()) { 
			j = 2; 
			goto label4; 
		} 
		break; 
	}
	default: {
		if (!FindChar()) goto label1;
		WrText(iTxt);
	label2:
		i = iTxt;
		MenuX = Orig.X + 4;
		MenuY = Orig.Y + i + 2;
	label3:
		ClearHlp();
		if (!ExecItem(i)) {
		label4:
			return (j << 8) + i;
		}
		break;
	}
	}
	goto label1;
}

void TMenuBox::GetItemRect(WORD I, TRect* R)
{
	R->A.X = Orig.X + 2 + 1;
	R->A.Y = Orig.Y + I + 1;
	R->Size.X = Size.X - 4;
	R->Size.Y = 1;
}

//TMenuBoxS::TMenuBoxS()
//{
//}

TMenuBoxS::TMenuBoxS(WORD C1, WORD R1, pstring Msg)
{
	MsgTxt = Msg;
	HlpRdb = (RdbDPtr)&HelpFD;
	IsBoxS = true;
	nTxt = CountDLines(&MsgTxt[1], MsgTxt.length(), '/') - 2;
	Move(&screen.colors.mNorm, Palette, 3);
	SetState(sfShadow, true);
	InitTMenuBox(C1, R1);
}

bool TMenuBoxS::Enabled(WORD I)
{
	return true;
}

bool TMenuBoxS::ExecItem(WORD& I)
{
	return false;
}

std::string TMenuBoxS::GetHlpName()
{
	pstring s;
	str(iTxt, s);
	return GetText(-1) + "_" + s.c_str();
}

std::string TMenuBoxS::GetText(integer I)
{
	/*helpname/head/text1/text2/...*/
	return GetDLine(&MsgTxt[1], MsgTxt.length(), '/', I + 2);
}

TMenuBoxP::TMenuBoxP(WORD C1, WORD R1, TMenu* aParent, Instr_menu* aPD)
{
	pstring s;
	PD = aPD;
	parent = aParent;
	s = RunShortStr(aPD->HdLine);
	s[0] = (char)MinI(s.length(), TxtCols - 6);
	HdTxt = s;
	HlpRdb = aPD->HelpRdb;
	CRoot = aPD->Choices;
	nTxt = CountNTxt(CRoot, false);
	SetPalette(aPD);
	if (aPD->X != nullptr)
	{
		C1 = RunInt(aPD->X);
		R1 = RunInt(aPD->Y);
	}
	else if (aPD->PullDown && (aParent == nullptr))
	{
		C1 = MenuX;
		R1 = MenuY;
	}
	if (aPD->Shdw) SetState(sfShadow, true);
	InitTMenuBox(C1, R1);
}

bool TMenuBoxP::Enabled(WORD I)
{
	return CI(CRoot, I)->Enabled;
}

bool TMenuBoxP::ExecItem(WORD& I)
{
	auto result = false;
	if (!PD->PullDown) return result;
	if (I == 0) {
		if ((Event.What == evMouseDown) || !PD->WasESCBranch) return result;
		RunInstr(PD->ESCInstr);
	}
	else {
		auto choice = CI(CRoot, I);
		RunInstr(choice->Instr);
	}
	if (ExitP) { I = 255; return result; }
	I = 0;
	if (PD->Loop) {
		if (BreakP) {
			BreakP = false;
			return result;
		}
		result = true;
	}
	else
		if (BreakP) {
			BreakP = false;
			I = 255;
		}
	return result;
}

std::string TMenuBoxP::GetHlpName()
{
	std::string* S = CI(CRoot, iTxt)->HelpName;
	if (S != nullptr) return *S;
	return "";
}

std::string TMenuBoxP::GetText(integer I)
{
	if (I == 0) return HdTxt;
	else return CI(CRoot, I)->Txt;
}

TMenuBar::TMenuBar()
{
}

TMenuBar::TMenuBar(WORD C1, WORD R1, WORD Cols)
{
	InitTMenuBar(C1, R1, Cols);
}

void TMenuBar::InitTMenuBar(WORD C1, WORD R1, WORD Cols)
{
	InitTMenu();
	WORD c2 = 0, r2 = 0, i = 0, l = 0;
	for (i = 1; i <= nTxt; i++) {
		l = l + LenStyleStr(GetText(i)) + 2;
	}
	if (l > TxtCols) RunError(636);
	Cols = MaxW(l, Cols);
	if (nTxt == 0) nBlks = 0;
	else nBlks = (Cols - l) / nTxt;
	while ((Cols - l - nBlks * nTxt) < nBlks) nBlks--;
	c2 = Cols;
	if (C1 != 0) c2 += (C1 - 1);
	r2 = 1;
	if (R1 != 0) r2 = R1;
	InitTWindow(C1, R1, c2, r2, Palette[0], "", "", HlpRdb != nullptr);
}

WORD TMenuBar::Exec()
{
	WORD i = 0;
	TMenuBox* w = nullptr;
	TRect r;
	if (nTxt == 0) { return 0; }
	bool down = false; iTxt = 1; 
	Prev(); 
	Next();  /*get valid iTxt*/
	for (i = 1; i <= nTxt; i++) WrText(i);
label1:
	HandleEvent();
	i = iTxt;
	bool enter = false;
	switch (KbdChar) {
	case VK_RETURN: { enter = true; goto label2; break; }
	case VK_ESCAPE: { i = 0; goto label4; break; }
	case VK_DOWN: goto label3; break;
	case VK_LEFT: { Prev(); WrText(i); if (down) goto label2; break; }
	case VK_RIGHT: { Next(); WrText(i); if (down) goto label2; break; }
	default: {
		if (!FindChar()) goto label1;
		enter = true;
	label2:
		WrText(iTxt);
	label3:
		GetItemRect(iTxt, &r); 
		MenuX = r.A.X; MenuY = r.A.Y + 1;
		if (GetDownMenu(&w)) {
			i = w->Exec(DownI[iTxt]);
			delete w;
			//ReleaseStore(w);
			DownI[iTxt] = Lo(i);
			enter = false; 
			down = true;
			if (Hi(i) == 1) { 
				i = iTxt; 
				Prev(); WrText(i); 
				goto label2; 
			}
			if (Hi(i) == 2) { 
				i = iTxt; 
				Next(); WrText(i); 
				goto label2;
			}
			down = false;
			if (i == 0) goto label1;
			return (iTxt << 8) + i;
		}
		if (enter) {
			i = iTxt;
		label4:
			ClearHlp();
			bool exI = ExecItem(i);
			if (!exI) return i << 8;
		}
	}
	}
	goto label1;
}

bool TMenuBar::GetDownMenu(TMenuBox** W)
{
	return false;
}

void TMenuBar::GetItemRect(WORD I, TRect* R)
{
	short x = Orig.X + nBlks;
	for (short j = 1; j <= I - 1; j++) {
		x += LenStyleStr(GetText(j)) + 2 + nBlks;
	}
	R->A.X = x + 1;
	R->A.Y = Orig.Y + 1;
	R->Size.X = LenStyleStr(GetText(I)) + 2;
	R->Size.Y = 1;
}

TMenuBarS::TMenuBarS()
{
}

TMenuBarS::TMenuBarS(WORD MsgNr)
{
	RdMsg(MsgNr);
	MsgTxt = StoreStr(MsgLine);
	HlpRdb = (RdbD*)&HelpFD;
	nTxt = (CountDLines(&MsgTxt[1], MsgTxt->length(), '/') - 1) / 2;
	Move(&screen.colors.mNorm, Palette, 3);
	InitTMenuBar(1, 1, TxtCols);
}

bool TMenuBarS::GetDownMenu(TMenuBox** W)
{
	pstring TNr(10); WORD n, err; TMenuBoxS* p;
	auto result = false;
	TNr = GetText(nTxt + iTxt);
	val(TNr, n, err);
	if ((TNr.length() == 0) || (err != 0)) return result;
	RdMsg(n);
	//New(p, Init(MenuX, MenuY, (pstring*)&MsgLine));
	p = new TMenuBoxS(MenuX, MenuY, MsgLine);
	p->parent = this; 
	*W = p;
	result = true;
	return result;
}

std::string TMenuBarS::GetHlpName()
{
	pstring s;
	str(iTxt, s);
	return GetText(0) + "_" + s.c_str();
}

std::string TMenuBarS::GetText(integer I)
{
	return GetDLine(&MsgTxt[1], MsgTxt->length(), '/', I + 1);
}

TMenuBarP::TMenuBarP()
{
}

TMenuBarP::TMenuBarP(Instr_menu* aPD)
{
	WORD x1 = 0, y1 = 0, l1 = 0;
	PD = aPD;
	HlpRdb = PD->HelpRdb;
	CRoot = PD->Choices;
	nTxt = CountNTxt(CRoot, true);
	SetPalette(PD);
	y1 = 1;
	if (PD->Y != nullptr) y1 = RunInt(PD->Y);
	x1 = 1;
	l1 = TxtCols;
	if (PD->X != nullptr) {
		x1 = RunInt(PD->X);
		l1 = RunInt(PD->XSz);
	}
	InitTMenuBar(x1, y1, l1);
}

bool TMenuBarP::Enabled(WORD I)
{
	auto choice = CI(CRoot, I);
	return choice->Enabled;
}

bool TMenuBarP::ExecItem(WORD& I)
{
	TRect r;
	auto result = false;
	if (I == 0) {
		if (!PD->WasESCBranch) return result;
		RunInstr(PD->ESCInstr);
	}
	else RunInstr(CI(CRoot, I)->Instr);
	I = 0;
	if (BreakP || ExitP) { result = false; return result; }
	result = true;
	return result;
}

bool TMenuBarP::GetDownMenu(TMenuBox** W)
{
	// Instr_menu* PD1; TMenuBoxP* p;
	auto result = false;
	auto PD1 = (Instr_menu*)CI(CRoot, iTxt)->Instr;
	if ((PD1 == nullptr) || (PD1->Chain != nullptr)
		|| (PD1->Kind != _menubox) || !PD1->PullDown) return result;
	//New(p, Init(MenuX, MenuY, this, PD1));
	auto p = new TMenuBoxP(MenuX, MenuY, this, PD1);
	*W = p;
	result = true;
	return result;
}

std::string TMenuBarP::GetHlpName()
{
	std::string* S = CI(CRoot, iTxt)->HelpName;
	if (S != nullptr) return *S;
	else return "";
}

std::string TMenuBarP::GetText(integer I)
{
	auto tmp = CI(CRoot, I);
	return tmp->Txt;
}

WORD Menu(WORD MsgNr, WORD IStart)
{
	TMenuBoxS* w = nullptr;
	void* p = nullptr;
	MarkStore(p);
	RdMsg(MsgNr);
	//New(w, Init(0, 0, (pstring*)&MsgLine));
	w = new TMenuBoxS(0, 0, MsgLine);
	auto result = w->Exec(IStart);
	delete w;
	ReleaseStore(p);
	return result;
}

bool PrinterMenu(WORD Msg)
{
	TMenuBoxS* w = nullptr; WORD i, j;
	void* p = nullptr;
	std::string nr;
	std::string nm, lpt;

	MarkStore(p); RdMsg(Msg); j = prCurr;
	for (prCurr = 0; prCurr <= prMax - 1; prCurr++) {
		i = printer[prCurr].Lpti;
		nr = std::to_string(i);
		nm = PrTab(prName);
		ReplaceChar(nm, '/', '-');
		lpt = "(LPT" + nr + ")";
		if (printer[prCurr].ToMgr) lpt = "";
		char buffer[10]{ 0 };
		snprintf(buffer, sizeof(buffer), "%*c", MaxI(0, 9 - nm.length()), ' ');
		MsgLine = MsgLine + '/' + nm + buffer + lpt;
	}
	prCurr = j;
	//New(w, Init(0, 0, (pstring*)&MsgLine));
	w = new TMenuBoxS(0, 0, MsgLine);
	i = w->Exec(prCurr + 1);
	if (i > 0) SetCurrPrinter(i - 1);
	delete w;
	ReleaseStore(p);
	return i > 0;
}

void MenuBoxProc(Instr_menu* PD)
{
	TMenuBoxP* w = nullptr; WORD i = 0;
	BYTE mx = 0, my = 0;
label1:
	w = new TMenuBoxP(0, 0, nullptr, PD);
	i = w->Exec(i);
	delete w;
	if (!PD->PullDown) {
		if (i == 0) {
			if (!PD->WasESCBranch) return;
			RunInstr(PD->ESCInstr);
		}
		else RunInstr(CI(PD->Choices, i)->Instr);
		if (BreakP || ExitP) {
			if (PD->Loop) BreakP = false;
		}
		else if (PD->Loop) goto label1;
	}
}

void MenuBarProc(Instr_menu* PD)
{
	TMenuBarP* w = nullptr;
	void* p = nullptr;
	MarkStore(p);
	w = new TMenuBarP(PD);
	w->Exec();
	delete w;
	ReleaseStore(p);
}

LongStr* GetHlpText(RdbD* R, std::string S, bool ByName, WORD& IRec)
{
	FieldDescr* NmF = nullptr; FieldDescr* TxtF = nullptr;
	LongStr* T = nullptr; std::string Nm;
	longint i = 0;
	TVideoFont fo; FileD* cf = nullptr;
	void* p = nullptr;
	LockMode md = NullMode;
	void* cr = CRecPtr;
	MarkStore2(p);
	if (ByName) {
		if (R == nullptr) goto label5;
		CFile = (FileD*)R;
		if (CFile == &HelpFD) {
			if (CFile->Handle == nullptr) goto label5;
		}
		else {
			CFile = R->HelpFD;
			if (CFile == nullptr) goto label5;
		}
		ConvToNoDiakr(&S[0], S.length(), fonts.VFont);
	}
label1:
	md = NewLMode(RdMode);
	if (CFile->Handle == nullptr) goto label5;
	CRecPtr = new BYTE[CFile->RecLen + 2];
	NmF = CFile->FldD.front();
	TxtF = (FieldDescr*)NmF->Chain;
	if (!ByName) {
		i = MaxW(1, MinW(IRec, CFile->NRecs));
		ReadRec(CFile, i, CRecPtr);
		goto label2;
	}
	for (i = 1; i <= CFile->NRecs; i++) {
		ReadRec(CFile, i, CRecPtr);
		Nm = TrailChar(' ', _ShortS(NmF));
		if (CFile == &HelpFD) fo = TVideoFont::foKamen;
		else fo = fonts.VFont;
		ConvToNoDiakr(&Nm[0], Nm.length(), fo);
		//printf("comp: %s <=> %s\n", S.c_str(), Nm.c_str());
		if (EqualsMask(&S[0], S.length(), Nm)) {
		label2:
			T = _LongS(TxtF);
			if (!ByName || (T->LL > 0) || (i == CFile->NRecs)) {
				if (CFile == &HelpFD) ConvKamenToCurr(T->A, T->LL);
				IRec = i;
				goto label3;
			}
			ReleaseStore(T);
			i++;
			ReadRec(CFile, i, CRecPtr);
			goto label2;
		}
	}
label3:
	OldLMode(md);
	ReleaseStore2(p);
	if ((T == nullptr) && (CFile != &HelpFD)) {
	label4:
		R = R->ChainBack;
		if (R != nullptr)
			if ((R->HelpFD != nullptr) && (R->HelpFD != CFile)) {
				CFile = R->HelpFD;
				goto label1;
			}
			else goto label4;
	}
label5:
	CRecPtr = cr;
	return T;

}

void DisplLLHelp(RdbD* R, std::string Name, bool R24)
{
	LongStr* s = nullptr;
	void* p = nullptr;
	size_t i = 0, y = 0; WORD iRec = 0; FileD* cf = nullptr;
	if ((R == nullptr) || (R != (RdbD*)&HelpFD) && (R->HelpFD == nullptr)) return;
	MarkStore(p);
	cf = CFile;
	if (!Name.empty()) {
		iRec = 0;
		s = GetHlpText(R, Name, true, iRec);
		if (s != nullptr) {
			std::string ss = std::string(s->A, s->LL);
			ss = GetNthLine(ss, 1, 1);
			MsgLine = ss;
			if (MsgLine[0] == '{') {
				MsgLine = MsgLine.substr(1, 255);
				i = MsgLine.find('}');
				if (i != std::string::npos) MsgLine.erase(i, 255);
			}
			MsgLine = MsgLine.substr(0, min(TxtCols, MsgLine.length()));
			goto label1;
		}
	}
	MsgLine = "";
label1:
	y = TxtRows - 1;
	if (R24) y--;
	screen.ScrWrStr(1, y + 1, MsgLine, screen.colors.nNorm);
	screen.ScrClr(MsgLine.length() + 1, y + 1, TxtCols - MsgLine.length(), 1, ' ', screen.colors.nNorm);
	CFile = cf;
	ReleaseStore(p);
}

void TMenu::InitTMenu()
{
	mx = MenuX; 
	my = MenuY;
}

TMenu::~TMenu()
{
	MenuX = mx;
	MenuY = my;
}
