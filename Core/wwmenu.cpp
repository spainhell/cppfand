#include "wwmenu.h"
#include <memory>
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "obaseww.h"
#include "runfrml.h"
#include "runproc.h"
#include "runproj.h"
#include "../Editor/OldEditor.h"
#include "../Editor/EditorHelp.h"
#include "../Common/codePages.h"
#include "../Common/textfunc.h"


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

//TWindow::TWindow(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, pstring top, pstring bottom, bool SaveLL)
//{
//	//InitTWindow(C1, R1, C2, R2, Attr, top, bottom, SaveLL);
//}

void TWindow::InitTWindow(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, const std::string& top, const std::string& bottom, bool SaveLL)
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
	SavedW = PushW(Orig.X + 1, Orig.Y + 1, Orig.X + Size.X + Shadow.X, Orig.Y + Size.Y + Shadow.Y, true, false);
	if (SaveLL) {
		SavedLLW = PushW(1, TxtRows, TxtCols, TxtRows, true, false);
	}
	else {
		SavedLLW = 0;
	}
	if (Shadow.Y == 1) screen.ScrColor(Orig.X + 2, Row2(), Size.X + Shadow.X - 2, screen.colors.ShadowAttr);
	if (Shadow.X > 0) {
		for (i = Row1(); i <= Row2(); i++) {
			screen.ScrColor(Col2(), i, Shadow.X, screen.colors.ShadowAttr);
		}
	}
	if (GetState(sfFramed)) {
		n = 0;
		if (GetState(sfFrDouble)) {
			n = 9;
		}
		screen.ScrWrFrameLn(Orig.X + 1, Orig.Y + 1, n, Size.X, Attr);
		for (i = 1; i <= Size.Y - 2; i++) {
			screen.ScrWrFrameLn(Orig.X + 1, Orig.Y + i + 1, n + 6, Size.X, Attr);
		}
		screen.ScrWrFrameLn(Orig.X + 1, Orig.Y + Size.Y, n + 3, Size.X, Attr);
		m = Size.X - 2;
		if (top.length() != 0) {
			std::string sTop = " " + top + " ";
			short l = min(sTop.length(), m);
			if (l > m) sTop = sTop.substr(0, m);
			screen.ScrWrStr(Col1() + (m - sTop.length()) / 2 + 1, Orig.Y + 1, sTop, Attr);
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

TMenu::TMenu() : TWindow()
{
}

void TMenu::ClearHlp()
{
	if (HlpRdb != nullptr) ClearLL(screen.colors.uNorm);
}

bool TMenu::FindChar(char c1)
{
	bool result = false;
	WORD i = iTxt;
	for (size_t j = 1; j <= nTxt; j++) {
		if (Enabled(j)) {
			std::string s = GetText(j);
			if (s.length() > 0) {
				size_t k = s.find_first_of(0x17); /* CTRL+W */
				char c2;
				if (k != std::string::npos) c2 = s[k + 1];
				else c2 = s[0];
				if (toupper(NoDiakr(c2, fonts.VFont)) == toupper(NoDiakr(c1, fonts.VFont))) {
					iTxt = j;
					WrText(i);
					return true;
				}
			}
		}
	}
	return result;
}

void TMenu::HandleEvent()
{
	WORD i = 0; bool frst = true;
	WrText(iTxt);
	i = iTxt;
	std::string hlp = GetHlpName();
	TestEvent();

	while (true) {
		//ReadKbd();
		/* !!! with Event do!!! */
		switch (Event.What) {
		case evMouseDown: {
			if (Contains(&Event.Where)) {
				Event.Pressed.UpdateKey(__ENTER);
				LeadIn(&Event.Where);
			}
			else if (MouseInRect(0, TxtRows - 1, TxtCols, 1)) {
				//goto label2;
				ClrEvent();
				if (HlpRdb != nullptr) Help(HlpRdb, hlp, false);
			}
			else if (ParentsContain(&Event.Where)) {
				Event.Pressed.UpdateKey(__ESC);
				return;
			}
		}
		case evKeyDown: {
			if (Event.Pressed.Char == '\0') {
				WORD KbdChar = Event.Pressed.KeyCombination();
				switch (KbdChar) {
				case __HOME:
				case __PAGEUP: {
					iTxt = 0;
					Next();
					WrText(i);
					break;
				}
				case __END:
				case __PAGEDOWN: {
					iTxt = nTxt + 1;
					Prev();
					WrText(i);
					break;
				}
				case __F1: {
					ClrEvent();
					if (HlpRdb != nullptr) Help(HlpRdb, hlp, false);
					//KbdChar = 0;
					break;
				}
				case __ALT_F10: {
					ClrEvent();
					Help(nullptr, "", false);
					//KbdChar = 0;
					break;
				}
				case __ALT_F2: {
					if (IsTestRun && !IsBoxS) {
						ClrEvent();
						EditHelpOrCat(__ALT_F2, 2, hlp);
						//KbdChar = 0;
					}
					break;
				}
				default: {
					//KbdChar = Event.Pressed.KeyCombination();
					break;
				}
				}
			}
			else {
				//KbdChar = Event.Pressed.KeyCombination();
			}
			break;
		}
		default: {
			if (frst) {
				//DisplayLastLineHelp(HlpRdb, hlp, false);
				frst = false;
			}
			ClrEvent();
			WaitEvent(0);
			continue;
		}
		}
		ClrEvent();
		break;
	}
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
	Event.Pressed.UpdateKey(0);
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
	Palette[0] = RunWordImpl(CFile, aPD->mAttr[0], screen.colors.mNorm, CRecPtr);
	Palette[1] = RunWordImpl(CFile, aPD->mAttr[1], screen.colors.mHili, CRecPtr);
	Palette[2] = RunWordImpl(CFile, aPD->mAttr[2], screen.colors.mFirst, CRecPtr);
	Palette[3] = RunWordImpl(CFile, aPD->mAttr[3], screen.colors.mDisabled, CRecPtr);
}

ChoiceD* TMenu::getChoice(size_t order)
{
	if (this->filteredChoices.size() < order) return nullptr;
	return this->filteredChoices[order - 1];
}

void TMenu::insertChoices(std::vector<ChoiceD*>& choices, bool isMenuBar)
{
	this->choices = choices;
	this->countChoices(isMenuBar);
	for (auto& c : this->choices) {
		if (c->Displ) {
			this->filteredChoices.push_back(c);
		}
	}
}

void TMenu::countChoices(bool isMenuBar)
{
	nTxt = 0;
	WORD nValid = 0;
	std::string s;
	bool b = false;
	for (auto& C : choices) {
		b = RunBool(CFile, C->Condition, CRecPtr);
		C->Displ = false;
		if (b || C->DisplEver) {
			C->Displ = true;
			nTxt++;
			s = RunShortStr(CFile, C->TxtFrml, CRecPtr);
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
			else if (isMenuBar) s = " ";
			C->Txt = s;
			if (C->Txt.empty()) b = false;
			if (b) nValid++;
		}
		C->Enabled = b;
	}
	if (nValid == 0) nTxt = 0;
}

TMenuBox::TMenuBox() : TMenu()
{
}

void TMenuBox::InitTMenuBox(WORD C1, WORD R1)
{
	WORD l = 0;

	std::string hd = GetText(0);
	WORD cols = hd.length();
	for (WORD i = 1; i <= nTxt; i++) {
		std::string s = GetText(i);
		l = GetLengthOfStyledString(s);
		if (l > cols) cols = l;
	}
	cols += 4;
	if ((cols + 2 > TxtCols) || (nTxt + 2 > TxtRows)) {
		RunError(636);
	}
	WORD c2 = cols;
	WORD r2 = nTxt;
	if (C1 != 0) c2 += C1 + 1;
	if (R1 != 0) r2 += R1 + 1;
	SetState(sfFramed, true);
	InitTWindow(C1, R1, c2, r2, Palette[0], hd, "", HlpRdb != nullptr);
	for (WORD i = 1; i <= nTxt; i++) {
		WrText(i);
	}
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

	while (true) {
		DisplayLastLineHelp(HlpRdb, GetHlpName(), false);
		HandleEvent();
		i = iTxt;
		const WORD KbdChar = Event.Pressed.KeyCombination();
		ClrEvent();
		switch (KbdChar) {
		case __ENTER: {
			i = iTxt;
			MenuX = Orig.X + 4;
			MenuY = Orig.Y + i + 2;
			ClearHlp();
			if (!ExecItem(i)) {
				return (j << 8) + i;
			}
			break;
		}
		case __ESC: {
			i = 0;
			ClearHlp();
			return 0;
			break;
		}
		case __UP: {
			Prev();
			WrText(i);
			break;
		}
		case __DOWN: {
			Next();
			WrText(i);
			break;
		}
		case __LEFT: {
			if (UnderMenuBar()) {
				j = 1;
				return (j << 8) + i;
			}
			break;
		}
		case __RIGHT: {
			if (UnderMenuBar()) {
				j = 2;
				return (j << 8) + i;
			}
			break;
		}
		default: {
			if (Event.Pressed.isChar() && FindChar(Event.Pressed.Char)) {
				// item char shortcut
				WrText(iTxt);
				i = iTxt;
				MenuX = Orig.X + 4;
				MenuY = Orig.Y + i + 2;
				ClearHlp();
				if (!ExecItem(i)) {
					return (j << 8) + i;
				}
			}
			else {
				//WrText(iTxt);
				//i = iTxt;
				//MenuX = Orig.X + 4;
				//MenuY = Orig.Y + i + 2;
				//ClearHlp();
				//if (!ExecItem(i)) {
				//	return (j << 8) + i;
				//}
			}
			break;
		}
		}
	}
}

void TMenuBox::GetItemRect(WORD I, TRect* R)
{
	R->A.X = Orig.X + 2 + 1;
	R->A.Y = Orig.Y + I + 1;
	R->Size.X = Size.X - 4;
	R->Size.Y = 1;
}

TMenuBoxS::TMenuBoxS(WORD C1, WORD R1, pstring Msg) : TMenuBox()
{
	MsgTxt = Msg;

	HlpRdb = new RdbD(); HlpRdb->HelpFD = HelpFD;
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
	return GetText(-1) + "_" + std::to_string(iTxt);
}

std::string TMenuBoxS::GetText(short I)
{
	// format: 'helpname/head/text1/text2/...'
	std::string s = MsgTxt;
	return GetNthLine(s, I + 2, 1, '/');
}

TMenuBoxP::TMenuBoxP(WORD C1, WORD R1, TMenu* aParent, Instr_menu* aPD)
{
	PD = aPD;
	parent = aParent;
	pstring s = RunShortStr(CFile, aPD->HdLine, CRecPtr);
	s[0] = (char)MinI(s.length(), TxtCols - 6);
	HdTxt = s;
	HlpRdb = aPD->HelpRdb;
	this->insertChoices(aPD->Choices, false);
	SetPalette(aPD);
	if (aPD->X != nullptr) {
		C1 = RunInt(CFile, aPD->X, CRecPtr);
		R1 = RunInt(CFile, aPD->Y, CRecPtr);
	}
	else if (aPD->PullDown && aParent == nullptr) {
		C1 = MenuX;
		R1 = MenuY;
	}
	if (aPD->Shdw) SetState(sfShadow, true);
	InitTMenuBox(C1, R1);
}

bool TMenuBoxP::Enabled(WORD I)
{
	return getChoice(I)->Enabled;
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
		auto choice = getChoice(I);
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
	std::string helpName = getChoice(iTxt)->HelpName;
	return helpName;
}

std::string TMenuBoxP::GetText(short I)
{
	if (I == 0) {
		return HdTxt;
	}
	else {
		return getChoice(I)->Txt;
	}
}

void TMenuBoxP::call()
{
	WORD i = 0;
	BYTE mx = 0, my = 0;
label1:
	i = this->Exec(i);
	if (!PD->PullDown) {
		if (i == 0) {
			if (!PD->WasESCBranch) return;
			RunInstr(PD->ESCInstr);
		}
		else {
			RunInstr(getChoice(i)->Instr);
		}
		if (BreakP || ExitP) {
			if (PD->Loop) BreakP = false;
		}
		else if (PD->Loop) goto label1;
	}
}

TMenuBar::TMenuBar(WORD C1, WORD R1, WORD Cols)
{
	InitTMenuBar(C1, R1, Cols);
}

void TMenuBar::InitTMenuBar(WORD C1, WORD R1, WORD Cols)
{
	InitTMenu();
	WORD l = 0;
	for (WORD i = 1; i <= nTxt; i++) {
		std::string s = GetText(i);
		l = l + GetLengthOfStyledString(s) + 2;
	}
	if (l > TxtCols) RunError(636);
	Cols = MaxW(l, Cols);
	if (nTxt == 0) nBlks = 0;
	else nBlks = (Cols - l) / nTxt;
	while ((Cols - l - nBlks * nTxt) < nBlks) nBlks--;
	WORD c2 = Cols;
	if (C1 != 0) c2 += (C1 - 1);
	WORD r2 = 1;
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

	while (true) {//label1:
		DisplayLastLineHelp(HlpRdb, GetHlpName(), false);
		HandleEvent();
		i = iTxt;
		bool enter = false;
		switch (Event.Pressed.KeyCombination()) {
		case __ENTER: { enter = true; goto label2; break; }
		case __ESC: { i = 0; goto label4; break; }
		case __DOWN: { goto label3; break; }
		case __LEFT: {
			Prev();
			WrText(i);
			if (down) goto label2;
			break;
		}
		case __RIGHT: {
			Next();
			WrText(i);
			if (down) goto label2;
			break;
		}
		default: {
			if (Event.Pressed.isChar() && !FindChar(Event.Pressed.Char)) continue;
			enter = true;
		label2:
			WrText(iTxt);
		label3:
			GetItemRect(iTxt, &r);
			MenuX = r.A.X; MenuY = r.A.Y + 1;
			if (GetDownMenu(&w)) {
				i = w->Exec(DownI[iTxt]);
				delete w;
				DownI[iTxt] = Lo(i);
				enter = false;
				down = true;
				if (Hi(i) == 1) {
					i = iTxt;
					Prev();
					WrText(i);
					goto label2;
				}
				if (Hi(i) == 2) {
					i = iTxt;
					Next();
					WrText(i);
					goto label2;
				}
				down = false;
				if (i == 0) continue;
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
	}
}

bool TMenuBar::GetDownMenu(TMenuBox** W)
{
	return false;
}

void TMenuBar::GetItemRect(WORD I, TRect* R)
{
	short x = Orig.X + nBlks;
	for (short j = 1; j <= I - 1; j++) {
		std::string s = GetText(j);
		x += GetLengthOfStyledString(s) + 2 + nBlks;
	}
	R->A.X = x + 1;
	R->A.Y = Orig.Y + 1;
	std::string s = GetText(I);
	R->Size.X = GetLengthOfStyledString(s) + 2;
	R->Size.Y = 1;
}

TMenuBarS::TMenuBarS()
{
}

TMenuBarS::TMenuBarS(WORD MsgNr)
{
	ReadMessage(MsgNr);
	MsgTxt = StoreStr(MsgLine);
	HlpRdb = (RdbD*)HelpFD;
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
	ReadMessage(n);
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

std::string TMenuBarS::GetText(short I)
{
	std::string s = *MsgTxt;
	return GetNthLine(s, I + 1, 1, '/');
	//return GetDLine(&MsgTxt[1], MsgTxt->length(), '/', I + 1);
}

TMenuBarP::TMenuBarP()
{
}

TMenuBarP::TMenuBarP(Instr_menu* aPD)
{
	WORD x1 = 0, y1 = 0, l1 = 0;
	PD = aPD;
	HlpRdb = PD->HelpRdb;
	//nTxt = CountNTxt(choices, true);
	this->insertChoices(aPD->Choices, true);
	SetPalette(PD);
	y1 = 1;
	if (PD->Y != nullptr) y1 = RunInt(CFile, PD->Y, CRecPtr);
	x1 = 1;
	l1 = TxtCols;
	if (PD->X != nullptr) {
		x1 = RunInt(CFile, PD->X, CRecPtr);
		l1 = RunInt(CFile, PD->XSz, CRecPtr);
	}
	InitTMenuBar(x1, y1, l1);
}

bool TMenuBarP::Enabled(WORD I)
{
	auto choice = getChoice(I);
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
	else RunInstr(getChoice(I)->Instr);
	I = 0;
	if (BreakP || ExitP) { result = false; return result; }
	result = true;
	return result;
}

bool TMenuBarP::GetDownMenu(TMenuBox** W)
{
	auto result = false;
	auto PD1 = (Instr_menu*)getChoice(iTxt)->Instr;
	if ((PD1 == nullptr) || (PD1->Chain != nullptr)
		|| (PD1->Kind != _menubox) || !PD1->PullDown) return result;
	auto p = new TMenuBoxP(MenuX, MenuY, this, PD1);
	*W = p;
	result = true;
	return result;
}

std::string TMenuBarP::GetHlpName()
{
	std::string helpName = getChoice(iTxt)->HelpName;
	return helpName;
}

std::string TMenuBarP::GetText(short I)
{
	ChoiceD* choice = getChoice(I);
	return choice->Txt;
}

WORD Menu(WORD MsgNr, WORD IStart)
{
	TMenuBoxS* w = nullptr;
	void* p = nullptr;
	MarkStore(p);
	ReadMessage(MsgNr);
	w = new TMenuBoxS(0, 0, MsgLine);
	auto result = w->Exec(IStart);
	delete w;
	ReleaseStore(&p);
	return result;
}

bool PrinterMenu(WORD Msg)
{
	ReadMessage(Msg);
	for (WORD j = 0; j < prMax; j++) {
		Printer* pr = &printer[j];
		std::string nm = PrTab(j, prName);
		std::replace(nm.begin(), nm.end(), '/', '-');
		std::string lpt = (pr->ToMgr) ? "" : "(LPT" + std::to_string(pr->Lpti) + ")";
		char buffer[10]{ 0 };
		snprintf(buffer, sizeof(buffer), "%*c", MaxI(0, 9 - nm.length()), ' ');
		MsgLine = MsgLine + '/' + nm + buffer + lpt;
	}
	std::unique_ptr<TMenuBoxS> w = std::make_unique<TMenuBoxS>(0, 0, MsgLine);
	WORD i = w->Exec(prCurr + 1);
	if (i > 0) SetCurrPrinter(i - 1);
	return i > 0;
}

void MenuBarProc(Instr_menu* PD)
{
	TMenuBarP* w = nullptr;
	void* p = nullptr;
	MarkStore(p);
	w = new TMenuBarP(PD);
	w->Exec();
	delete w;
	ReleaseStore(&p);
}

std::string GetHlpText(RdbD* R, std::string S, bool ByName, WORD& IRec)
{
	FieldDescr* NmF = nullptr;
	FieldDescr* TxtF = nullptr;
	std::string Nm;
	int i = 0;
	TVideoFont fo;
	//FileD* cf = nullptr;
	void* p = nullptr;
	LockMode md = LockMode::NullMode;
	void* cr = CRecPtr;
	std::string result;

	if (ByName) {
		if (R == nullptr) goto label5;
		//CFile = (FileD*)R;  // TODO: toto je nesmysl
		CFile = R->HelpFD;
		if (CFile == HelpFD) {
			if (CFile->FF->Handle == nullptr) goto label5;
		}
		else {
			// CFile = R->HelpFD;
			if (CFile == nullptr) goto label5;
		}
		ConvToNoDiakr(&S[0], S.length(), fonts.VFont);
	}
label1:
	md = CFile->NewLockMode(RdMode);
	if (CFile->FF->Handle == nullptr) goto label5;
	CRecPtr = new BYTE[CFile->FF->RecLen + 2]{ '\0' };
	NmF = CFile->FldD.front();
	TxtF = NmF->pChain;
	if (!ByName) {
		i = MaxW(1, MinW(IRec, CFile->FF->NRecs));
		CFile->ReadRec(i, CRecPtr);
		goto label2;
	}
	for (i = 1; i <= CFile->FF->NRecs; i++) {
		CFile->ReadRec(i, CRecPtr);
		Nm = OldTrailChar(' ', CFile->loadOldS(NmF, CRecPtr));
		if (CFile == HelpFD) fo = TVideoFont::foKamen;
		else fo = fonts.VFont;
		ConvToNoDiakr(&Nm[0], Nm.length(), fo);
		if (EqualsMask(S, Nm)) {
		label2:
			result = CFile->loadS(TxtF, CRecPtr);
			if (!ByName || (result.length() > 0) || (i == CFile->FF->NRecs)) {
				if (CFile == HelpFD) {
					ConvKamenToCurr(result, !fonts.NoDiakrSupported);
				}
				IRec = i;
				goto label3;
			}
			i++;
			CFile->ReadRec(i, CRecPtr);
			goto label2;
		}
	}
label3:
	CFile->OldLockMode(md);
	if ((result.empty()) && (CFile != HelpFD)) {
	label4:
		R = R->ChainBack;
		if (R != nullptr)
			if ((R->HelpFD != nullptr) && (R->HelpFD != CFile)) {
				CFile = R->HelpFD;
				goto label1;
			}
			else {
				goto label4;
			}
	}
label5:
	CRecPtr = cr;
	return result;
}

void DisplayLastLineHelp(RdbD* R, std::string Name, bool R24)
{
	size_t i = 0, y = 0; WORD iRec = 0;

	if ((R == nullptr) || (R->HelpFD != HelpFD) && (R->HelpFD == nullptr)) return;

	FileD* cf = CFile;
	if (!Name.empty()) {
		iRec = 0;
		std::string sHelp = GetHlpText(R, Name, true, iRec);
		if (!sHelp.empty()) {
			sHelp = GetNthLine(sHelp, 1, 1);
			MsgLine = sHelp;
			if (MsgLine[0] == '{') {
				MsgLine = MsgLine.substr(1, 255);
				i = MsgLine.find('}');
				if (i != std::string::npos) MsgLine.erase(i, 255);
			}
			MsgLine = MsgLine.substr(0, min(TxtCols, MsgLine.length()));
		}
		else {
			MsgLine = "";
		}
	}
	else {
		MsgLine = "";
	}

	y = TxtRows - 1;
	if (R24) y--;
	screen.ScrWrStr(1, y + 1, MsgLine, screen.colors.nNorm);
	screen.ScrClr(MsgLine.length() + 1, y + 1, TxtCols - MsgLine.length(), 1, ' ', screen.colors.nNorm);
	CFile = cf;
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
