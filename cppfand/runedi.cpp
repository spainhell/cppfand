#include "runedi.h"

#include "common.h"

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
	GotoXY(x, y); FieldEdit(F, nullptr, LWw, 1, T, R, true, true, false, 0);
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
