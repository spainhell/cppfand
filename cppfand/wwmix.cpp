#include "wwmix.h"

#include "legacy.h"
#include "obaseww.h"
#include "rdedit.h"
#include "runedi.h"
#include "../FileSystem/directory.h"

SS ss;


struct Item : Chained
{
	//Item* Chain;
	char Tag;
	pstring S;
};

struct stSv : public Chained
{
	//Item* ItemRoot;
	void* markp;
	integer NItems, MaxItemLen, Tabs, TabSize, WwSize, Base, iItem;
} sv;

Item* GetItem(WORD N)
{
	Item* p; WORD i;
	p = (Item*)sv.Chain;
	for (i = 2; i <= N; i++) p = (Item*)p->Chain;
	return p;
}

wwmix::wwmix()
{
}

void wwmix::PutSelect(std::string s)
{
	Item* p = new Item(); // (Item*)GetStore(sizeof(*p) - 1 + l);
	WORD l = MinW(s.length(), 46);
	p->Tag = ' ';
	Move(&s[0], &p->S[1], l);
	p->S[0] = (char)l;
	if (ss.Empty) {
		FillChar(&sv, sizeof(sv), '\0');
		FillChar(&ss.Abcd, sizeof(ss) - 5, 0);
		sv.markp = p;
	}
	if (sv.Chain == nullptr) sv.Chain = p;
	else ChainLast(sv.Chain, p);
	sv.NItems++;
	sv.MaxItemLen = MaxW(l, sv.MaxItemLen);
}

void wwmix::SelectStr(integer C1, integer R1, WORD NMsg, std::string LowTxt)
{
	WORD cols = 0, MaxBase = 0;
	char schar = '\0';
	integer b = 0;
	Item* p = nullptr;
	integer i = 0, iOld = 0;
	/* !!! with sv do!!! */
	void* pw = PushScr(1, TxtRows, TxtCols, TxtRows);
	if (ss.Subset)
	{
		if (ss.AscDesc) WrLLMsg(135);
		else WrLLMsg(134);
	}
	else WrLLMsg(152);
	WORD rows = 5;
	if (TxtCols > 52) cols = 50;
	else cols = TxtCols - 2;
	RdMsg(NMsg);
	WORD c2 = cols;
	if (C1 != 0) c2 = C1 + cols + 1;
	WORD r2 = rows;
	if (R1 != 0) r2 = R1 + rows + 1;
	TextAttr = screen.colors.sNorm;
	longint w2 = PushWFramed(C1, R1, c2, r2, TextAttr, MsgLine, LowTxt,
	                         WHasFrame + WDoubleFrame + WShadow + WPushPixel);
	if (ss.Empty)
	{
		do { ReadKbd(); } while (KbdChar != VK_ESCAPE);
		goto label3;
	}
	sv.TabSize = sv.MaxItemLen + 2;
	if (ss.Subset) sv.TabSize++;
	sv.Tabs = cols / sv.TabSize;
	sv.WwSize = sv.Tabs * rows;
	MaxBase = 1;
	while (MaxBase + sv.WwSize <= sv.NItems) MaxBase += sv.Tabs;
	if (ss.Abcd) AbcdSort();
	if (ss.AscDesc) schar = '<';
	else schar = (char)p;
	SetFirstiItem();
	sv.Base = sv.iItem - (sv.iItem - 1) % sv.WwSize;
	DisplWw();
	iOld = 0;
label1:
	GetEvent();
	switch (Event.What) {
	case evMouseMove: {
		if ((iOld != 0) && MouseInItem(i) && (i != iOld)) {
			Switch(i, iOld);
			sv.iItem = i;
			DisplWw();
			iOld = i;
		}
		break;
	}
	case evMouseDown: {
		if (MouseInItem(i)) {
			if (ss.Subset) {
				p = GetItem(i);
				if (p->Tag == ' ') p->Tag = schar;
				else p->Tag = ' ';
				sv.iItem = i;
				iOld = i;
				DisplWw();
			}
			else goto label2;
		}
		else {
			if (ss.Subset && ((Event.Buttons & mbDoubleClick) != 0)) {
			label2:
				KbdChar = _M_;
				sv.iItem = i;
				goto label3;
			}
		}
		break;
	}
	case evMouseUp: {
		iOld = 0; break; }
	case evKeyDown: {
		KbdChar = Event.KeyCode;
		switch (KbdChar) {
		case _M_:
		case VK_ESCAPE: {
		label3:
			ClrEvent();
				PopW(w2);
			PopScr(pw, true);
				ReleaseStore(pw);
			if (ss.Empty) return;
			ss.Empty = true;
				ss.Pointto = nullptr;
			ss.Size = 0;
				p = (Item*)sv.Chain;
			while (p != nullptr) {
				if (p->Tag != ' ') ss.Size++;
				p = (Item*)p->Chain;
			}
			if (ss.Subset && ss.ImplAll && (ss.Size == 0)) {
				p = (Item*)sv.Chain;
				while (p != nullptr) {
					if (p->S[1] != SelMark)	{
						p->Tag = schar;
						ss.Size++;
					}
					p = (Item*)p->Chain;
				}
			}
			if (KbdChar == VK_ESCAPE) ReleaseStore(sv.markp);
			return;
			break;
		}
		case VK_LEFT: Left(); break;
		case VK_RIGHT: Right(); break;
		case VK_UP: Up(); break;
		case VK_DOWN: Down(); break;
		case VK_PRIOR: {
			if (sv.Base > 1)
			{
				IVOff();
				b = sv.Base - sv.WwSize;
				if (b < 1) b = 1;
				sv.iItem -= sv.Base - b;
				sv.Base = b;
				DisplWw();
				IVOn();
			}
			break;
		}
		case VK_NEXT: {
			if (sv.Base < MaxBase)
			{
				IVOff();
				b = sv.Base + sv.WwSize;
				if (b > MaxBase) b = MaxBase;
				sv.iItem += b - sv.Base;
				if (sv.iItem > sv.NItems) sv.iItem -= sv.Tabs;
				sv.Base = b;
				DisplWw();
				IVOn();
			}
			break;
		}
		case _Z_: {
			if (sv.Base < MaxBase) {
				IVOff();
				sv.Base = sv.Base + sv.Tabs;
				if (sv.iItem < sv.Base) sv.iItem = sv.iItem + sv.Tabs;
				if (sv.iItem > sv.NItems) sv.iItem = sv.NItems;
				DisplWw();
				IVOn();
			}
			break;
		}
		case _W_: {
			if (sv.Base > 1) {
				IVOff();
				sv.Base = sv.Base - sv.Tabs;
				if (sv.iItem >= sv.Base + sv.WwSize) sv.iItem = sv.iItem - sv.Tabs;
				DisplWw();
				IVOn();
			}
			break;
		}
		case _CtrlPgUp_:
		case _Home_: {
			if (sv.iItem > 1) {
				IVOff();
				sv.iItem = 1;
				if (sv.Base > 1) { sv.Base = 1; DisplWw(); }
				IVOn();
			}
			break;
		}
		case _CtrlPgDn_:
		case _End_: {
			if (sv.iItem < sv.NItems) {
				IVOff();
				sv.iItem = sv.NItems;
				if (sv.Base < MaxBase) {
					sv.Base = MaxBase;
					DisplWw();
				}
				IVOn();
			}
			break;
		}
		default: {
			if (ss.Subset) {
				switch (KbdChar) {
				case _F2_: SetTag(schar); break;
				case _CtrlF2_: SetAllTags(schar); break;
				case  62 /*>*/: if (ss.AscDesc) SetTag('>'); break;
				case _F3_: SetTag(' '); break;
				case _CtrlF3_: SetAllTags(' '); break;
				case _F9_: { ClrEvent(); GraspAndMove(schar); break; }
				}
			}
			break;
		}
		}
		ClrEvent();
		goto label1;
		break;
	}
	}
}

void wwmix::WriteItem(WORD N)
{
	WORD l = 0;
	/* !!! with sv do!!! */
	WORD i = N - sv.Base;
	screen.GotoXY((i % sv.Tabs) * sv.TabSize + 2, i / sv.Tabs + 1);
	if (N > sv.NItems) l = sv.TabSize - 2;
	else {
		Item* p = GetItem(N);
		if (ss.Subset) {
			screen.ScrFormatWrStyledText(screen.WhereX(), screen.WhereY(), TextAttr, "%c", p->Tag);
			//printf("%c", p->Tag);
		}
		screen.ScrFormatWrStyledText(screen.WhereX(), screen.WhereY(), TextAttr, "%s", p->S.c_str());
		//printf("%s", p->S.c_str());
		l = sv.MaxItemLen - p->S.length();
	}
	if (l > 0) screen.ScrFormatWrStyledText(screen.WhereX(), screen.WhereY(), TextAttr, " "); // printf(" ");
}

void wwmix::SetAttr(WORD Attr)
{
	TextAttr = Attr;
	WriteItem(sv.iItem);
}

void wwmix::IVOn()
{
	TextAttr = screen.colors.sHili;
	WriteItem(sv.iItem);
}

void wwmix::IVOff()
{
	TextAttr = screen.colors.sNorm; WriteItem(sv.iItem);
}

void wwmix::DisplWw()
{
	char c;
	/* !!! with sv do!!! */
	TextAttr = screen.colors.sNorm;
	WORD max = sv.Base + sv.WwSize - 1;
	if (sv.Base > 1) c = ''; else c = ' ';
	screen.ScrWrChar(WindMin.X, WindMin.Y, c, TextAttr);
	if (max >= sv.NItems) c = ' '; else c = '';
	screen.ScrWrChar(WindMax.X, WindMax.Y, c, TextAttr);
	for (WORD i = sv.Base; i < max; i++) WriteItem(i);
	SetAttr(screen.colors.sHili);
}

void wwmix::Right()
{
	/* !!! with sv do!!! */
	IVOff();
	sv.iItem++;
	if (sv.iItem >= sv.Base + sv.WwSize)
	{
		sv.Base += sv.Tabs;
		DisplWw();
	}
	else IVOn();
}

void wwmix::Left()
{
	if (sv.iItem > 1)
	{
		IVOff(); sv.iItem--;
		if (sv.iItem < sv.Base)
		{
			sv.Base -= sv.Tabs;
			DisplWw();
		}
		else IVOn();
	};
}

void wwmix::Down()
{
	if (sv.iItem + sv.Tabs <= sv.NItems)
	{
		IVOff();
		sv.iItem += sv.Tabs;
		if (sv.iItem >= sv.Base + sv.WwSize)
		{
			sv.Base += sv.Tabs;
			DisplWw();
		}
		else IVOn();
	}
}

void wwmix::Up()
{
	if (sv.iItem > sv.Tabs)
	{
		IVOff();
		sv.iItem -= sv.Tabs;
		if (sv.iItem < sv.Base)
		{
			sv.Base -= sv.Tabs;
			DisplWw();
		}
		else IVOn();
	}
}

void wwmix::SetTag(char c)
{
	Item* p;
	p = GetItem(sv.iItem); p->Tag = c; TextAttr = screen.colors.sHili;
	WriteItem(sv.iItem); Right();
}

void wwmix::SetAllTags(char c)
{
	Item* p;
	p = (Item*)sv.Chain; while (p != nullptr) { p->Tag = c; p = (Item*)p->Chain; }
	DisplWw();
}

void wwmix::Switch(WORD I1, WORD I2)
{
	Item* p1; Item* p2; Item* q1; Item* q2; Item* h;
	WORD i;

	p1 = (Item*)(&sv.Chain);
	for (i = 2; i <= I1; i++) p1 = (Item*)p1->Chain;
	q1 = (Item*)p1->Chain;
	p2 = (Item*)(&sv.Chain);
	for (i = 2; i <= I2; i++) p2 = (Item*)p2->Chain;
	q2 = (Item*)p2->Chain;
	h = (Item*)q1->Chain;
	p1->Chain = q2;
	q1->Chain = q2->Chain;

	if (p2 == q1) {
		q2->Chain = q1;
	}
	else {
		q2->Chain = h;
		p2->Chain = q1;
	}
}

void wwmix::GraspAndMove(char schar)
{
	WORD A; Item* p;
	/* !!! with sv do!!! */
	p = GetItem(sv.iItem);
	if (p->Tag == ' ') p->Tag = schar;
	SetAttr(screen.colors.sHili + 0x80);
	A = screen.colors.sHili;
	screen.colors.sHili = screen.colors.sHili + 0x80;
label1:
	switch (ReadKbd()) {
	case _left_:
	{
		if (sv.iItem > 1) {
			Switch(sv.iItem - 1, sv.iItem);
			Left();
		}
		break;
	}
	case _right_:
	{
		if (sv.iItem < sv.NItems) {
			Switch(sv.iItem, sv.iItem + 1);
			Right();
		}
		break;
	}
	case _down_: {
		if (sv.iItem + sv.Tabs <= sv.NItems) {
			Switch(sv.iItem, sv.iItem + sv.Tabs);
			Down();
		}
		break;
	}
	case _up_: {
		if (sv.iItem > sv.Tabs) {
			Switch(sv.iItem - sv.Tabs, sv.iItem);
			Up();
		}
		break;
	}
	case _F9_:
	case _ESC_: {
		screen.colors.sHili = A;
		SetAttr(A);
		return;
		break;
	}
	}
	goto label1;
}

void wwmix::AbcdSort()
{
	Item* p; Item* q; Item* r;
	bool sorted;
	do {
		r = (Item*)(&sv.Chain); p = (Item*)sv.Chain; q = (Item*)p->Chain;
		sorted = true;
		while (q != nullptr) {
			if (CompLexStr(p->S, q->S) == _gt) {
				r->Chain = q; p->Chain = q->Chain; q->Chain = p;
				r = q; q = (Item*)p->Chain; sorted = false;
			}
			else { r = p; p = q; q = (Item*)q->Chain; };
		}
	} while (!sorted);
}

void wwmix::SetFirstiItem()
{
	sv.iItem = 1;
	if (ss.Pointto == nullptr) return;
	Item* p = (Item*)sv.Chain;
	while (p != nullptr) {
		if (p->S == *ss.Pointto) return;
		sv.iItem++;
		p = (Item*)p->Chain;
	}
}

bool wwmix::MouseInItem(integer& I)
{
	auto result = false;
	integer x = Event.Where.X - WindMin.X - 1;
	if (x < 0) return result;
	integer ix = x / sv.TabSize;
	if (ix >= sv.Tabs) return result;
	if ((Event.Where.Y < WindMin.Y) || (Event.Where.Y > WindMax.Y)) return result;
	I = (Event.Where.Y - WindMin.Y) * sv.Tabs + ix + sv.Base; if (I > sv.NItems) return result;
	result = true;
	return result;
}

pstring wwmix::GetSelect()
{
	Item* p = (Item*)&sv;
	pstring result;
	if (!ss.Subset)
	{
		p = GetItem(sv.iItem); result = p->S; ReleaseStore(sv.markp); return result;
	}
	while ((p != nullptr) && (p->Tag == ' ')) p = (Item*)p->Chain;
	if (p == nullptr) { ss.Tag = ' '; result = ""; return result; }
	ss.Tag = p->Tag; result = p->S; p = (Item*)p->Chain;
	return result;
}

bool wwmix::SelFieldList(WORD Nmsg, bool ImplAll, FieldList FLRoot)
{
	FieldDPtr F; FieldList FL; pstring s;
	FLRoot = nullptr;
	auto result = true;
	if (ss.Empty) return true;
	ss.Subset = true; ss.ImplAll = ImplAll; SelectStr(0, 0, Nmsg, CFile->Name);
	if (KbdChar == _ESC_) { return false; }
label1:
	s = GetSelect();
	if (s != "")
	{
		F = CFile->FldD; if (s[1] = SelMark) s = copy(s, 2, 255);
		while (F != nullptr)
			if (s == F->Name)
			{
				FL = (FieldListEl*)GetStore(sizeof(*FL));
				ChainLast(FLRoot, FL);
				FL->FldD = F;
				goto label1;
			}
			else F = (FieldDescr*)F->Chain;
		goto label1;
	}
	return result;
}

std::string wwmix::SelectDiskFile(std::string Path, WORD HdMsg, bool OnFace)
{
	std::string mask, s;
	longint w = 0; //SearchRec SR;
	BYTE sizeOfMask = 255;
	std::string p, d, n, ext, e, ne;

	std::string result;
	WORD c1 = 0; WORD r1 = 0; WORD c2 = 22; WORD r2 = 1; WORD c11 = 0; WORD r11 = 0;
	if (OnFace) {
		c1 = 43; r1 = 6; c2 = 67; r2 = 8; c11 = 28; r11 = 4;
	}
	if (Path.empty()) ext = ".*";
	else if (Path[0] == '.') ext = Path;
	else {
		FSplit(FExpand(Path), d, n, e);
		ne = n + e;
		if (ne == "") ne = "*.*";
		goto label3;
	}
	mask = pstring("*") + ext;
label1:
	RdMsg(HdMsg);
	w = PushWFramed(c1, r1, c2, r2, screen.colors.sMask, MsgLine, "", WHasFrame + WShadow + WPushPixel);
label2:
	screen.GotoXY(1, 1);
	//EditTxt(&mask, 1, sizeof(mask) - 1, 22, 'A', true, false, true, false, 0);
	EditTxt(mask, 1, sizeOfMask, 22, 'A', true, false, true, false, 0);
	if (KbdChar == VK_ESCAPE) { PopW(w); return result; }
	if (mask.find(' ') != std::string::npos) {
		WrLLF10Msg(60);
		goto label2;
	}
	FSplit(FExpand(mask), d, n, e);
	if (e.empty()) e = ext;
	else if ((ext == ".RDB") && (e != ".RDB")) {
		WrLLF10Msg(005);
		goto label2;
	}
	PopW(w);
	if (n.empty()) n = "*";
	ne = n + e;
	if (ne.find('*') == std::string::npos && ne.find('?') == std::string::npos) {
		result = d + ne;
		return result;
	}
label3:
	p = d + ne;
	auto dirItems = directoryItems(Path);
	//FindFirst(p + 00, 0, SR);
	/*if (!(DosError() == 0 || DosError() == 18)) {
		SetMsgPar(p);
		mask = p;
		WrLLF10Msg(811);
		goto label1;
	}
	while (DosError() == 0) {
		PutSelect(SR.name);
		FindNext(SR);
	}
	FindFirst(d + "*.*" + 00, Directory, SR);
	while (DosError() == 0) {
		if (((SR.Attr && Directory) != 0)
			&& (d.first('\\') != d.length() || (SR.name != ".."))
			&& (SR.name != '.')) {
			PutSelect('\\' + SR.name);
		}
		FindNext(SR);
	}*/
	ss.Abcd = true;
	SelectStr(c11, r11, HdMsg, p);
	if (KbdChar == _ESC_) return result;
	s = GetSelect();
	if (s[1] == '\\') {
		s.erase(0, 1);
		if (s == "..") do { d[0]--; } while (!(d[d.length()] == '\\'));
		else d = d + s + '\\';
		goto label3;
	}
	return d + s;
}

bool wwmix::PromptFilter(pstring Txt, FrmlPtr Bool, pstring* BoolTxt)
{
	void* p = nullptr;
	ExitRecord er = {};
	bool Del;
	FileDPtr cf = nullptr;
	MarkStore(p);
	WORD I = 1;
	auto result = true;
	//NewExit(Ovr(), er);
	goto label3;
	Del = true;
	ResetCompilePars();
	cf = CFile;
label1:
	PromptLL(113, &Txt, I, Del);
	Bool = nullptr;
	BoolTxt = nullptr;
	if (KbdChar == _ESC_) { result = false; goto label2; }
	if (Txt.length() == 0) goto label2;
	SetInpStr(Txt);
	RdLex();
	Bool = RdBool();
	if (Lexem != 0x1A) Error(21);
	BoolTxt = (pstring*)GetStore(Txt.length() + 1);
	Move(&Txt, BoolTxt, Txt.length() + 1);
label2:
	RestoreExit(er);
	return result;
label3:
	pstring Msg = MsgLine;
	I = CurrPos;
	SetMsgPar(Msg);
	WrLLF10Msg(110);
	IsCompileErr = false;
	ReleaseStore(p);
	CFile = cf;
	Del = false;
	goto label1;
}

void wwmix::PromptLL(WORD N, pstring* Txt, WORD I, bool Del)
{
	longint w = PushW(1, TxtRows, TxtCols, TxtRows);
	screen.GotoXY(1, TxtRows);
	TextAttr = screen.colors.pTxt;
	ClrEol();
	RdMsg(N);
	printf("%s", MsgLine.c_str());
	TextAttr = screen.colors.pNorm;
	EditTxt(Txt, I, 255, TxtCols - screen.WhereX(), 'A', Del, false, true, false, 0);
	PopW(w);
}

pstring wwmix::PassWord(bool TwoTimes)
{
	longint w; pstring Txt, Txt1;  WORD MsgNr, col;
	col = (TxtCols - 21) >> 1;
	w = PushW(col, TxtRows - 2, col + 21, TxtRows - 2);
	MsgNr = 628;
label1:
	TextAttr = screen.colors.pNorm | 0x80;
	screen.GotoXY(1, 1); ClrEol(); RdMsg(MsgNr);
	printf("%*s", (MsgLine.length() + 22) / 2, MsgLine.c_str());
	pstring tmpStr = char(ReadKbd);
	KbdBuffer = tmpStr + KbdBuffer;
	TextAttr = screen.colors.pNorm;
	screen.GotoXY(2, 1);
	Txt = "";
	EditTxt(&Txt, 1, 20, 20, 'A', true, true, true, false, 0);
	if (KbdChar == _ESC_) { Txt = ""; goto label2; }
	if (TwoTimes)
		if (MsgNr == 628) { MsgNr = 637; Txt1 = Txt; goto label1; }
		else if (Txt != Txt1) { WrLLF10Msg(638); MsgNr = 628; goto label1; }
label2:
	PopW(w);
	return Txt;
}

void wwmix::SetPassWord(FileDPtr FD, WORD Nr, pstring Pw)
{
	void* p;
	if (Nr == 1) p = FD->TF->PwCode;
	else p = FD->TF->Pw2Code;
	FillChar(p, 20, '@');
	Move(&Pw[1], p, Pw.length());
	Code(p, 20);
}

bool wwmix::HasPassWord(FileDPtr FD, WORD Nr, pstring Pw)
{
	PwCodeArr* X = nullptr;
	/* !!! with FD->TF^ do!!! */
	if (Nr == 1) X = &FD->TF->PwCode;
	else X = &FD->TF->Pw2Code;
	Code(X, 20);
	/*for (int i = 0; i < 20; i++)
	{
		if (*X[i] != '@') return true;
	}
	return false;*/
	pstring tmp(*X, 20);
	return Pw == TrailChar('@', tmp);
}

