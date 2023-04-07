#include "wwmix.h"

#include "compile.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "obaseww.h"
#include "../fandio/FandTFile.h"
#include "../Editor/runedi.h"
#include "../fandio/directory.h"
#include "../textfunc/textfunc.h"

SS ss;

struct Item
{
	char Tag = '\0';
	std::string S;

	WORD Compare(const Item& i) const { return CompLexStrings(this->S, i.S); }
	bool operator ==(const Item& i) const { return Compare(i) == 1; }
	bool operator <(const Item& i) const { return Compare(i) == 2; }
	bool operator >(const Item& i) const { return Compare(i) == 4; }
};

struct stSv
{
	std::vector<Item> items;
	short NItems = 0;
	short MaxItemLen = 0;
	short Tabs = 0;
	short TabSize = 0;
	short WwSize = 0;
	short Base = 0;
	short iItem = 0;

	void Reset() {
		NItems = 0;
		MaxItemLen = 0;
		Tabs = 0;
		TabSize = 0;
		WwSize = 0;
		Base = 0;
		iItem = 0;
		//for (auto& i : items) { delete i; }
		items.clear();
	}
} sv;

Item* GetItem(WORD N)
{
	if (sv.items.size() < N) return nullptr;
	return &sv.items[N - 1];
}

wwmix::wwmix()
{
}

void wwmix::PutSelect(std::string s)
{
	Item p;
	WORD l = MinW(s.length(), 46);
	p.Tag = ' ';
	p.S = s;
	if (ss.Empty) {
		sv.Reset();
		ss.Abcd = false;
		ss.AscDesc = false;
		ss.Subset = false;
		ss.ImplAll = false;
		ss.Empty = false;
		ss.Size = 0;
		ss.Tag = '\0';
	}
	sv.items.push_back(p);
	sv.NItems++;
	sv.MaxItemLen = MaxW(l, sv.MaxItemLen);
}

void wwmix::SelectStr(short C1, short R1, WORD NMsg, std::string LowTxt)
{
	WORD cols = 0, MaxBase = 0;
	WORD key;
	char schar;
	short b = 0;
	Item* p = nullptr;
	short i = 0, iOld = 0;

	void* pw = PushScr(1, TxtRows, TxtCols, TxtRows);
	if (ss.Subset) {
		if (ss.AscDesc) WrLLMsg(135);
		else WrLLMsg(134);
	}
	else WrLLMsg(152);
	WORD rows = 5;
	if (TxtCols > 52) cols = 50;
	else cols = TxtCols - 2;
	ReadMessage(NMsg);
	WORD c2 = cols;
	if (C1 != 0) c2 = C1 + cols + 1;
	WORD r2 = rows;
	if (R1 != 0) r2 = R1 + rows + 1;
	TextAttr = screen.colors.sNorm;
	int w2 = PushWFramed(C1, R1, c2, r2, TextAttr, MsgLine, LowTxt,
		WHasFrame + WDoubleFrame + WShadow + WPushPixel);
	if (ss.Empty) {
		do {
			ReadKbd();
		} while (Event.Pressed.KeyCombination() != __ESC);
		goto label3;
	}
	sv.TabSize = sv.MaxItemLen + 2;
	if (ss.Subset) sv.TabSize++;
	sv.Tabs = cols / sv.TabSize;
	sv.WwSize = sv.Tabs * rows;
	MaxBase = 1;
	while (MaxBase + sv.WwSize <= sv.NItems) MaxBase += sv.Tabs;
	if (ss.Abcd) AbcdSort();
	if (ss.AscDesc) {
		schar = '<';
	}
	else {
		schar = 0x10;
	}
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
				Event.Pressed.Char = VK_RETURN;
				sv.iItem = i;
				goto label3;
			}
		}
		break;
	}
	case evMouseUp: {
		iOld = 0; break; }
	case evKeyDown: {
		key = Event.Pressed.KeyCombination();
		switch (key) {
		case __ENTER:
		case __ESC: {
		label3:
			ClrEvent();
			PopW(w2);
			PopScr(pw, true);
			ReleaseStore(pw);
			if (ss.Empty) return;
			ss.Empty = true;
			ss.Pointto = nullptr;
			ss.Size = 0;
			//p = (Item*)sv.pChain;
			for (auto& p : sv.items) { //while (p != nullptr) {
				if (p.Tag != ' ') ss.Size++;
				//p = (Item*)p->pChain;
			}
			if (ss.Subset && ss.ImplAll && (ss.Size == 0)) {
				// p = (Item*)sv.pChain;
				for (auto& p : sv.items) { //while (p != nullptr) {
					if (p.S[0] != SelMark) {
						p.Tag = schar;
						ss.Size++;
					}
					//p = (Item*)p->pChain;
				}
			}
			if (Event.Pressed.KeyCombination() == __ESC) {
				// TODO: clean (memory leaks)!
				//ReleaseStore(sv.markp);
			}
			return;
			break;
		}
		case __LEFT: Left(); break;
		case __RIGHT: Right(); break;
		case __UP: Up(); break;
		case __DOWN: Down(); break;
		case __PAGEUP: {
			if (sv.Base > 1) {
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
		case __PAGEDOWN: {
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
		case __CTRL_PAGEUP:
		case __HOME: {
			if (sv.iItem > 1) {
				IVOff();
				sv.iItem = 1;
				if (sv.Base > 1) { sv.Base = 1; DisplWw(); }
				IVOn();
			}
			break;
		}
		case __CTRL_PAGEDOWN:
		case __END: {
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
				switch (key) {
				case __F2: { SetTag(schar); break; }
				case __CTRL_F2: { SetAllTags(schar); break; }
				case '>': { if (ss.AscDesc) SetTag('>'); break; }
				case __F3: { SetTag(' '); break; }
				case __CTRL_F3: { SetAllTags(' '); break; }
				case __F9: { ClrEvent(); GraspAndMove(schar); break; }
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
	WORD i = N - sv.Base;
	screen.GotoXY((i % sv.Tabs) * sv.TabSize + 2, i / sv.Tabs + 1);
	if (N > sv.NItems) l = sv.TabSize - 2;
	else {
		Item* p = GetItem(N);
		if (ss.Subset) {
			screen.ScrFormatWrStyledText(screen.WhereX(), screen.WhereY(), TextAttr, "%c", p->Tag);
		}
		screen.ScrFormatWrStyledText(screen.WhereX(), screen.WhereY(), TextAttr, "%s", p->S.c_str());
		l = sv.MaxItemLen - p->S.length();
	}
	if (l > 0) screen.ScrFormatWrStyledText(screen.WhereX(), screen.WhereY(), TextAttr, "%*c", l, ' ');
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
	TextAttr = screen.colors.sNorm;
	WriteItem(sv.iItem);
}

void wwmix::DisplWw()
{
	char c;
	TextAttr = screen.colors.sNorm;
	WORD max = sv.Base + sv.WwSize - 1;
	if (sv.Base > 1) c = '';
	else c = ' ';
	screen.ScrWrChar(WindMin.X, WindMin.Y, c, TextAttr);
	if (max >= sv.NItems) c = ' ';
	else c = '';
	screen.ScrWrChar(WindMax.X, WindMax.Y, c, TextAttr);
	for (WORD i = sv.Base; i <= max; i++) {
		WriteItem(i);
	}
	SetAttr(screen.colors.sHili);
}

void wwmix::Right()
{
	IVOff();
	sv.iItem++;
	if (sv.iItem >= sv.Base + sv.WwSize) {
		sv.Base += sv.Tabs;
		DisplWw();
	}
	else IVOn();
}

void wwmix::Left()
{
	if (sv.iItem > 1) {
		IVOff(); sv.iItem--;
		if (sv.iItem < sv.Base) {
			sv.Base -= sv.Tabs;
			DisplWw();
		}
		else IVOn();
	}
}

void wwmix::Down()
{
	if (sv.iItem + sv.Tabs <= sv.NItems) {
		IVOff();
		sv.iItem += sv.Tabs;
		if (sv.iItem >= sv.Base + sv.WwSize) {
			sv.Base += sv.Tabs;
			DisplWw();
		}
		else IVOn();
	}
}

void wwmix::Up()
{
	if (sv.iItem > sv.Tabs) {
		IVOff();
		sv.iItem -= sv.Tabs;
		if (sv.iItem < sv.Base) {
			sv.Base -= sv.Tabs;
			DisplWw();
		}
		else IVOn();
	}
}

void wwmix::SetTag(char c)
{
	Item* p = GetItem(sv.iItem);
	p->Tag = c;
	TextAttr = screen.colors.sHili;
	WriteItem(sv.iItem);
	Right();
}

void wwmix::SetAllTags(char c)
{
	for (auto& p : sv.items) {
		p.Tag = c;
	}
	DisplWw();
}

void wwmix::Switch(WORD I1, WORD I2)
{
	Item tmp = sv.items[I2 - 1];
	sv.items[I2 - 1] = sv.items[I1 - 1];
	sv.items[I1 - 1] = tmp;


	/*Item* p1; Item* p2; Item* q1; Item* q2; Item* h;
	WORD i;

	p1 = (Item*)(&sv.pChain);
	for (i = 2; i <= I1; i++) p1 = (Item*)p1->pChain;
	q1 = (Item*)p1->pChain;
	p2 = (Item*)(&sv.pChain);
	for (i = 2; i <= I2; i++) p2 = (Item*)p2->pChain;
	q2 = (Item*)p2->pChain;
	h = (Item*)q1->pChain;
	p1->pChain = q2;
	q1->pChain = q2->pChain;

	if (p2 == q1) {
		q2->pChain = q1;
	}
	else {
		q2->pChain = h;
		p2->pChain = q1;
	}*/
}

void wwmix::GraspAndMove(char schar)
{
	Item* p = GetItem(sv.iItem);
	if (p->Tag == ' ') p->Tag = schar;
	SetAttr(screen.colors.sHili + 0x80);
	WORD A = screen.colors.sHili;
	screen.colors.sHili = screen.colors.sHili + 0x80;
label1:
	switch (ReadKbd()) {
	case __LEFT:
	{
		if (sv.iItem > 1) {
			Switch(sv.iItem - 1, sv.iItem);
			Left();
		}
		break;
	}
	case __RIGHT:
	{
		if (sv.iItem < sv.NItems) {
			Switch(sv.iItem, sv.iItem + 1);
			Right();
		}
		break;
	}
	case __DOWN: {
		if (sv.iItem + sv.Tabs <= sv.NItems) {
			Switch(sv.iItem, sv.iItem + sv.Tabs);
			Down();
		}
		break;
	}
	case __UP: {
		if (sv.iItem > sv.Tabs) {
			Switch(sv.iItem - sv.Tabs, sv.iItem);
			Up();
		}
		break;
	}
	case __F9:
	case __ESC: {
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
	std::sort(sv.items.begin(), sv.items.end());

	//Item* p; Item* q; Item* r;
	//bool sorted;
	//do {
	//	r = (Item*)(&sv.pChain);
	//	p = (Item*)sv.pChain;
	//	q = (Item*)p->pChain;
	//	sorted = true;
	//	while (q != nullptr) {
	//		if (CompLexStr(p->S, q->S) == _gt) {
	//			r->pChain = q;
	//			p->pChain = q->pChain;
	//			q->pChain = p;
	//			r = q;
	//			q = (Item*)p->pChain;
	//			sorted = false;
	//		}
	//		else {
	//			r = p; p = q;
	//			q = (Item*)q->pChain;
	//		}
	//	}
	//} while (!sorted);
}

void wwmix::SetFirstiItem()
{
	sv.iItem = 1;
	if (ss.Pointto == nullptr) return;
	//Item* p = (Item*)sv.pChain;
	for (auto& p : sv.items) { //while (p != nullptr) {
		if (p.S == *ss.Pointto) return;
		sv.iItem++;
		//p = (Item*)p->pChain;
	}
}

bool wwmix::MouseInItem(short& I)
{
	auto result = false;
	short x = Event.Where.X - WindMin.X - 1;
	if (x < 0) return result;
	short ix = x / sv.TabSize;
	if (ix >= sv.Tabs) return result;
	if ((Event.Where.Y < WindMin.Y) || (Event.Where.Y > WindMax.Y)) return result;
	I = (Event.Where.Y - WindMin.Y) * sv.Tabs + ix + sv.Base; if (I > sv.NItems) return result;
	result = true;
	return result;
}

std::string wwmix::GetSelect()
{
	Item* p = nullptr;
	pstring result;
	if (!ss.Subset) {
		p = GetItem(sv.iItem);
		result = p->S;
		// TODO: ReleaseStore(sv.markp);
		return result;
	}

	// temp vector with items with Tag == ' '
	for (size_t i = lastItemIndex; i < sv.items.size(); i++) {
		if (sv.items[i].Tag != ' ') {
			p = &sv.items[i];
			lastItemIndex++;
			break;
		}
		if (i == sv.items.size() - 1) {
			// on the last item with Tag == ' '
			p = nullptr;
		}
	}

	if (p == nullptr) {
		ss.Tag = ' ';
		result = "";
		return result;
	}
	ss.Tag = p->Tag;
	result = p->S;
	//p = (Item*)p->pChain;
	return result;
}

bool wwmix::SelFieldList(WORD Nmsg, bool ImplAll, FieldListEl** FLRoot)
{
	FieldDescr* F; FieldList FL;
	*FLRoot = nullptr;
	auto result = true;
	if (ss.Empty) return result;
	ss.Subset = true;
	ss.ImplAll = ImplAll;
	SelectStr(0, 0, Nmsg, CFile->Name);
	if (Event.Pressed.KeyCombination() == __ESC) { return false; }
label1:
	std::string s = GetSelect();
	if (!s.empty()) {
		F = CFile->FldD.front();
		if (s[0] == (char)SelMark) s = s.substr(1, 255); //copy(s, 2, 255);
		while (F != nullptr) {
			if (s == F->Name) {
				FL = new FieldListEl(); // (FieldListEl*)GetStore(sizeof(*FL));
				if (*FLRoot == nullptr) *FLRoot = FL;
				else ChainLast(*FLRoot, FL);
				FL->FldD = F;
				goto label1;
			}
			else {
				F = (FieldDescr*)F->pChain;
			}
		}
		goto label1;
	}
	return result;
}

bool wwmix::SelFieldList(WORD Nmsg, bool ImplAll, std::vector<FieldDescr*>& FLRoot)
{
	FLRoot.clear();
	auto result = true;
	if (ss.Empty) return result;
	ss.Subset = true;
	ss.ImplAll = ImplAll;
	SelectStr(0, 0, Nmsg, CFile->Name);
	if (Event.Pressed.KeyCombination() == __ESC) { return false; }
label1:
	std::string s = GetSelect();
	if (!s.empty()) {
		FieldDescr* F = CFile->FldD.front();
		if (s[0] == (char)SelMark) s = s.substr(1, 255);
		while (F != nullptr) {
			if (s == F->Name) {
				FLRoot.push_back(F);
				goto label1;
			}
			else {
				F = (FieldDescr*)F->pChain;
			}
		}
		goto label1;
	}
	return result;
}

std::string wwmix::SelectDiskFile(std::string Path, WORD HdMsg, bool OnFace)
{
	std::string mask, s;
	int w = 0; //SearchRec SR;
	BYTE sizeOfMask = 255;
	std::string p, d, n, ext, e, ne;

	std::string result;
	WORD c1 = 0; WORD r1 = 0; WORD c2 = 22; WORD r2 = 1; WORD c11 = 0; WORD r11 = 0;
	if (OnFace) {
		c1 = 43; r1 = 6; c2 = 67; r2 = 8; c11 = 28; r11 = 4;
	}
	if (Path.empty()) ext = ".*";
	else if (Path[0] == '.') {
		ext = Path;
	}
	else {
		FSplit(FExpand(Path), d, n, e);
		ne = n + e;
		if (ne.empty()) {
			ne = "*.*";
		}
		goto label3;
	}
	mask = "*" + ext;
label1:
	ReadMessage(HdMsg);
	w = PushWFramed(c1, r1, c2, r2, screen.colors.sMask, MsgLine, "", WHasFrame + WShadow + WPushPixel);
label2:
	screen.GotoXY(1, 1);
	//EditTxt(&mask, 1, sizeof(mask) - 1, 22, 'A', true, false, true, false, 0);
	EditTxt(mask, 1, sizeOfMask, 22, FieldType::ALFANUM, true, false, true, false, 0);
	if (Event.Pressed.KeyCombination() == __ESC) { PopW(w); return result; }
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
	bool dirExists = directoryExists(d);
	if (!dirExists) {
		SetMsgPar(p);
		mask = p;
		WrLLF10Msg(811);
		goto label1;
	}

	auto dirItems = directoryItems(d, ne);
	for (auto& item : dirItems) PutSelect(item);

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
	if (Event.Pressed.KeyCombination() == __ESC) return result;
	s = GetSelect();
	if (s[0] == '\\') {
		s.erase(0, 1);
		if (s == "..") {
			d = parentDirectory(d);
			// pokud jsme v korenovem adresari, vrati se s '\'
			if (d[d.length() - 1] != '\\') d += '\\';
		}
		else d = d + s + '\\';
		goto label3;
	}
	return d + s;
}

bool wwmix::PromptFilter(std::string Txt, FrmlElem** Bool, std::string* BoolTxt)
{
	void* p = nullptr;
	bool Del;
	FileD* cf = nullptr;
	MarkStore(p);
	size_t I = 1;

	Del = true;
	ResetCompilePars();
	cf = CFile;

	while (true) {
		try {
			PromptLL(113, Txt, I, Del);
			*Bool = nullptr;
			BoolTxt = nullptr;
			if (Event.Pressed.KeyCombination() == __ESC) {
				return false;
			}
			if (Txt.length() == 0) {
				return true;
			}
			SetInpStr(Txt);
			RdLex();
			*Bool = RdBool();
			if (Lexem != 0x1A) Error(21);
			BoolTxt = new std::string();
			*BoolTxt = Txt;
			return true;
		}
		catch (std::exception& e) {
			pstring Msg = MsgLine;
			I = CurrPos;
			SetMsgPar(Msg);
			WrLLF10Msg(110);
			IsCompileErr = false;
			ReleaseStore(p);
			CFile = cf;
			Del = false;
		}
	}
}

void wwmix::PromptLL(WORD N, std::string& Txt, WORD I, bool Del)
{
	int w = PushW(1, TxtRows, TxtCols, TxtRows);
	screen.GotoXY(1, TxtRows, ScrPosition::absolute);
	TextAttr = screen.colors.pTxt;
	ClrEol();
	ReadMessage(N);
	screen.ScrWrStr(1, TxtRows, MsgLine, screen.colors.pTxt);
	screen.GotoXY(MsgLine.length() + 1, TxtRows, ScrPosition::absolute);
	TextAttr = screen.colors.pNorm;
	EditTxt(Txt, I, 255, TxtCols - screen.WhereX(), FieldType::ALFANUM, Del, false, true, false, 0);
	PopW(w);
}

std::string wwmix::PassWord(bool TwoTimes)
{
	std::string Txt, Txt1;
	WORD col = (TxtCols - 21) >> 1;
	int w = PushW(col, TxtRows - 2, col + 21, TxtRows - 2);
	WORD MsgNr = 628;

	while (true) {
		TextAttr = screen.colors.pNorm | 0x80;
		ClrEol();
		ReadMessage(MsgNr);
		screen.ScrFormatWrText(1, 1, "%*s", (MsgLine.length() + 22) / 2, MsgLine.c_str());
		WORD pressed_key = ReadKbd(); // wait for 1st char
		keyboard.AddToFrontKeyBuf(pressed_key);
		TextAttr = screen.colors.pNorm;
		screen.GotoXY(2, 1);
		Txt = "";
		EditTxt(Txt, 1, 20, 20, FieldType::ALFANUM, true, true, true, false, 0);
		if (Event.Pressed.KeyCombination() == __ESC) {
			Txt = "";
			break;
		}
		if (TwoTimes) {
			if (MsgNr == 628) {
				MsgNr = 637;
				Txt1 = Txt;
				continue;
			}
			else {
				if (Txt != Txt1) {
					WrLLF10Msg(638);
					MsgNr = 628;
					continue;
				}
			}
		}
		break;
	}
	PopW(w);
	return Txt;
}

/// <summary>
/// Set password in FandTFile (set field PwCode or Pw2Code)
/// </summary>
/// <param name="FD">FileD pointer</param>
/// <param name="Nr">Password type</param>
/// <param name="Pw">Password</param>
void wwmix::SetPassWord(FileD* FD, WORD Nr, std::string Pw)
{
	if (Nr == 1) {
		FD->FF->TF->PwCode = Pw;
		FD->FF->TF->PwCode = AddTrailChars(FD->FF->TF->PwCode, '@', 20);
		Code(FD->FF->TF->PwCode);
	}
	else {
		FD->FF->TF->Pw2Code = Pw;
		FD->FF->TF->PwCode = AddTrailChars(FD->FF->TF->Pw2Code, '@', 20);
		Code(FD->FF->TF->Pw2Code);
	}
}

bool wwmix::HasPassWord(FileD* FD, WORD Nr, std::string Pw)
{
	std::string filePwd;
	if (Nr == 1) {
		filePwd = FD->FF->TF->PwCode;
		Code(filePwd);
	}
	else {
		filePwd = FD->FF->TF->Pw2Code;
		Code(filePwd);
	}
	return Pw == TrailChar(filePwd, '@');
}
