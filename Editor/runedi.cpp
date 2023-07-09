#include "runedi.h"
#include <memory>
#include "OldEditor.h"
#include "EditorHelp.h"
#include "rdedit.h"
#include "../Core/ChkD.h"
#include "../Core/compile.h"
#include "../Core/EditOpt.h"
#include "../Core/FieldDescr.h"
#include "../Core/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/KeyFldD.h"
#include "../Core/oaccess.h"
#include "../Core/obase.h"
#include "../Core/obaseww.h"
#include "../Core/rdfildcl.h"
#include "../Core/rdrun.h"
#include "../Core/runproc.h"
#include "../Core/runproj.h"
#include "../fandio/files.h"
#include "../fandio/XKey.h"
#include "../fandio/XWKey.h"
#include "../Core/wwmenu.h"
#include "../Logging/Logging.h"
#include "../MergeReport/ReportGenerator.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"

bool TxtEdCtrlUBrk, TxtEdCtrlF4Brk;
EFldD* CFld;

//EditD* E = EditDRoot;
EFldD* FirstEmptyFld;
XKey* VK;
XWKey* WK;
int BaseRec;
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
	E = (EditD*)E->pChain;
	EditDRoot = E;
}

bool TestIsNewRec()
{
	return IsNewRec;
}

void SetSelectFalse()
{
	Select = false;
}

void DelBlk(BYTE& sLen, std::string s, WORD pos)
{
	while ((sLen > 0) && (s[sLen - 1] == ' ') && (pos <= sLen)) {
		sLen--;
	}
}

void WriteStr(WORD& pos, WORD& base, WORD& maxLen, WORD& maxCol, BYTE sLen, std::string s, bool star,
	WORD cx, WORD cy, WORD cx1, WORD cy1)
{
	CHAR_INFO Buffer[MaxTxtCols];
	if (pos <= base) base = pos - 1;
	else if (pos > base + maxCol) {
		base = pos - maxCol;
		if (pos > maxLen) base--;
	}
	if ((pos == base + 1) && (base > 0)) base--;
	DelBlk(sLen, s, pos);

	for (WORD i = 0; i < maxCol; i++) {
		Buffer[i].Attributes = TextAttr;
		if (base + i + 1 <= sLen) {
			if (star) Buffer[i].Char.AsciiChar = '*';
			else Buffer[i].Char.AsciiChar = s[base + i];
			if (Buffer[i].Char.AsciiChar >= '\0' && Buffer[i].Char.AsciiChar < ' ')
			{	// non-printable char ...
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

WORD EditTxt(std::string& text, WORD pos, WORD maxlen, WORD maxcol,
	FieldType typ, bool del, bool star, bool upd, bool ret, unsigned int Delta)
{
	WORD base = 0, cx = 0, cy = 0, cx1 = 0, cy1 = 0;
	int EndTime = 0; bool InsMode = false;
	InsMode = true; base = 0;
	bool delPreviousState = del;
	if (pos > maxlen + 1) pos = maxlen + 1;
	cx = screen.WhereX();
	cx1 = cx + WindMin.X - 1;
	cy = screen.WhereY();
	cy1 = cy + WindMin.Y - 1;
	screen.CrsNorm();
	WriteStr(pos, base, maxlen, maxcol, text.length(), text, star, cx, cy, cx1, cy1);
	WORD KbdChar;
label1:
	switch (WaitEvent(Delta)) {
	case 1/*flags*/: goto label1; break;
	case 2/*timer*/: {
		Event.Pressed.UpdateKey(__ESC);
		goto label6;
		break;
	}
	}

	switch (Event.What) {
	case evMouseDown: {
		if (MouseInRect(cx1, cy1, maxcol, 1)) {
			ClrEvent();
			Event.Pressed.UpdateKey(__ENTER);
			goto label6;
		}
		break;
	}
	case evKeyDown: {
		KbdChar = Event.Pressed.KeyCombination();
		ClrEvent();
		if (Event.Pressed.isChar()) {
			// printable character
			if (del) {
				pos = 1; text = "";
				WriteStr(pos, base, maxlen, maxcol, text.length(), text, star, cx, cy, cx1, cy1);
				del = false;
			}
			if (upd) {
				switch (typ) {
				case FieldType::NUMERIC: { if (KbdChar < '0' || KbdChar > '9') goto label7; }
				case FieldType::FIXED: {
					if (!((KbdChar >= '0' && KbdChar <= '9') || KbdChar == '.' || KbdChar == ','
						|| KbdChar == '-')) goto label7;
				}
				case FieldType::REAL: {
					if (!((KbdChar >= '0' && KbdChar <= '9') || KbdChar == '.' || KbdChar == ','
						|| KbdChar == '-' || KbdChar == '+' || KbdChar == 'e' || KbdChar == 'E'))
						goto label7;
				}
				}
			label5:
				if (pos > maxlen) { beep(); goto label7; }
				if (InsMode) {
					if (text.length() == maxlen) {
						if (text[text.length() - 1] == ' ') {
							text = text.substr(0, text.length() - 1);
						}
						else {
							beep();
							goto label7;
						}
					}
					char c = (char)(KbdChar & 0x00FF);
					text.insert(pos - 1, 1, c);
					pos++;
				}
				else {
					// overwrite
					text[pos - 1] = (char)(KbdChar & 0x00FF);
				}
			label7: {}
			}

			/*if (ret && ((KbdChar < 0x20) || (KbdChar >= 0x100))) {
				Event.What = evKeyDown;
				goto label8;
			}*/
		}
		else {
			delPreviousState = del;
			del = false;
			// non-printable
			switch (KbdChar) {
			case __INSERT:
			case _V_: InsMode = !InsMode; break;
			case _U_: if (TxtEdCtrlUBrk) goto label6; break;
			case __CTRL_F4: if (TxtEdCtrlF4Brk) goto label6; break;
			case __ESC:
			case __ENTER: {
			label6:
				text = TrailChar(text, ' ');
				screen.CrsHide();
				TxtEdCtrlUBrk = false;
				TxtEdCtrlF4Brk = false;
				return 0;
			}
			case __LEFT:
			case _S_: {
				if ((pos > 1)) pos--;
				break;
			}
			case __RIGHT:
			case _D_: {
				if (pos <= maxlen) {
					if ((pos > text.length()) && (text.length() < maxlen)) text += ' ';
					pos++;
				}
				break;
			}
			case _Q_: {
				if (ReadKbd() == _S_) goto label3;
				if (ReadKbd() == _D_) goto label4;
				break;
			}
			case __HOME: {
			label3:
				pos = 1;
				break;
			}
			case __END: {
			label4:
				pos = text.length() + 1;
				break;
			}
			case __BACK: {
				if (upd && (pos > 1)) {
					pos--;
					goto label2;
				}
				break;
			}
			case __DELETE:
			case _G_: {
				if (upd && (pos <= text.length())) {
				label2:
					if (text.length() >= pos) {
						text.erase(pos - 1, 1);
					}
				}
				break;
			}
			case _P_: {
				if (upd) {
					ReadKbd();
					if (KbdChar >= 0 && KbdChar <= 31) goto label5;
				}
				break;
			}
			case __F4: {
				if (upd && (typ == FieldType::ALFANUM) && (pos <= text.length())) {
					text[pos - 1] = ToggleCS(text[pos - 1]);
				}
				break;
			}
			default:
				del = delPreviousState; // not change 'del' state if unsupported key pressed
				break;
			}
		}
	}
	}
	WriteStr(pos, base, maxlen, maxcol, text.length(), text, star, cx, cy, cx1, cy1);
	ClrEvent();
	if (!ret) goto label1;

	return pos;
}

/// <summary>
/// Maska pro alfanum. retezec
/// # cislice, 9 cislice, @ pismeno, ? libovolny znak, $ pismeno s automatickou zmenou na velke (upcase)
/// ! libovolny znak s automatickou zmenou ve velke (upcase)
/// [....] volitelna skupina znaku
/// (...|...|...) alternativni skupiny znaku prip. ruzne delky
/// </summary>
/// <param name="S">vstupni retezec k overeni</param>
/// <param name="Mask">maska k porovnani</param>
/// <returns></returns>
bool TestMask(std::string& S, std::string Mask)
{
	WORD ii;
	auto result = true;
	if (Mask.empty()) return result;
	WORD v = 0; WORD i = 0;
	WORD ls = S.length();
	WORD j = 0;
	WORD lm = Mask.length();
label1:
	if (j == lm) {
		while (i < ls) {
			i++;
			if (S[i - 1] != ' ') goto label4;
		}
		return result;
	}
	j++;
	switch (Mask[j - 1]) {
	case ']':
	case ')': { v = 0; break; }
	case '[': { v = 1; ii = i; break; }
	case '(': { v = 2; ii = i; break; }
	case '|': { do { j++; } while (Mask[j - 1] != ')'); break; }
	default: {
		if (i == ls) goto label4; i++;
		char c = S[i - 1];
		switch (Mask[j - 1]) {
		case '#':
		case '9': if (!isdigit(c)) goto label3; break;
		case '@': if (!IsLetter(c)) goto label3; break;
		case '?':
		case '$': { if (!IsLetter(c)) goto label3;
				else goto label2; break; }
		case '!':
		label2:
			S[i - 1] = UpcCharTab[c]; break;
		default: { if (c != Mask[j - 1]) goto label3; break; }
		}
	}
	}
	goto label1;
label3:
	switch (v) {
	case 1: {
		do { j++; } while (Mask[j - 1] != ']');
		v = 0; i = ii;
		goto label1;
		break;
	}
	case 2: {
		do { j++; } while (!(Mask[j - 1] == '|' || Mask[j - 1] == ')'));
		i = ii;
		if (Mask[j - 1] == '|') goto label1;
		break; }
	}
label4:
	result = false;
	SetMsgPar(Mask);
	WrLLF10Msg(653);
	return result;
}

void SetWasUpdated(FandFile* fand_file, void* record)
{
	if (!WasUpdated) {
		if (EdRecVar) {
			fand_file->SetUpdFlag(record);
		}
		//Move(E->NewRecPtr, E->OldRecPtr, CFileRecSize(CFile->FF));
		memcpy(E->OldRecPtr, E->NewRecPtr, fand_file->RecordSize());
		WasUpdated = true;
	}
}

void AssignFld(FieldDescr* F, FrmlElem* Z)
{
	SetWasUpdated(CFile->FF, CRecPtr);
	AssgnFrml(CFile, CRecPtr, F, Z, false, false);
}

WORD FieldEdit(FieldDescr* F, FrmlElem* Impl, WORD LWw, WORD iPos, std::string& Txt,
	double& RR, bool del, bool upd, bool ret, unsigned int Delta)
{
	short Col = 0, Row = 0;
	char cc = '\0';
	pstring* Mask = nullptr;
	std::string Msk;
	pstring s;
	double r = 0;
	pstring T;
	bool b = false;
	WORD result = 0;
	pstring C999 = "999999999999999";
	Col = screen.WhereX();
	Row = screen.WhereY();
	WORD KbdChar = Event.Pressed.KeyCombination();
	if (F->field_type == FieldType::BOOL) {
		screen.GotoXY(Col, Row);
		if (Txt.empty()) screen.ScrWrStr(" ", TextAttr);
		else screen.ScrWrStr(Txt, TextAttr);
		screen.GotoXY(Col, Row);
		screen.CrsNorm();
	label0:
		GetEvent();
		switch (Event.What) {
		case evKeyDown: {
			KbdChar = Event.Pressed.KeyCombination();
			ClrEvent();
			if (KbdChar == __ESC) {
				screen.CrsHide();
				return result;
			}
			if (KbdChar == __ENTER) {
			label11:
				if ((Txt.length() > 0) && (Txt[0] == AbbrYes)) cc = AbbrYes;
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
				Event.Pressed.UpdateKey(__ENTER);
				goto label11;
			}
		}
		}
		ClrEvent();
		goto label0;
	label1:
		//printf("%c", cc);
		screen.ScrFormatWrText(Col, Row, "%c", cc);
		Txt = cc;
		screen.CrsHide();
		return 0;
	}
	WORD L = F->L;
	WORD M = F->M;
	//Mask = new pstring(FieldDMask(F));
	Mask = new pstring(F->Mask.c_str());
	if (((F->Flg & f_Mask) != 0) && (F->field_type == FieldType::ALFANUM)) {
		Msk = *Mask;
	}
	else {
		Msk = "";      /*!!!!*/
	}
label2:
	iPos = EditTxt(Txt, iPos, L, LWw, F->field_type, del, false, upd, (F->frml_type == 'S')
		&& ret, Delta);
	result = iPos;
	if (iPos != 0) return result;
	if ((KbdChar == VK_ESCAPE) || !upd) return result;
	del = true;
	iPos = 1;
	r = 0;
	if ((Txt.length() == 0) && (Impl != nullptr)) {
		AssignFld(F, Impl);
		Txt = DecodeField(CFile, F, L, CRecPtr);
	}
	switch (F->field_type) {
	case FieldType::FIXED:
	case FieldType::REAL: {
		T = LeadChar(' ', TrailChar(Txt, ' '));
		WORD I = T.first(',');
		if (I > 0) { T = copy(T, 1, I - 1) + "." + copy(T, I + 1, 255); }
		if (T.length() == 0) r = 0.0;
		else {
			val(T, r, I);
			if (F->field_type == FieldType::FIXED) {
				WORD N = L - 2 - M;
				if (M == 0) N++;
				if ((I != 0) || (abs(r) >= Power10[N])) {
					s = copy(C999, 1, N) + "." + copy(C999, 1, M);
					SetMsgPar(s, s);
					WrLLF10Msg(617);
					goto label4;
				}
			}
			else /*'R'*/ if (I != 0) { WrLLF10Msg(639); goto label4; }
		}
		if (F->field_type == FieldType::FIXED) {
			str(r, L, M, Txt);
			if ((F->Flg & f_Comma) != 0) {
				r = r * Power10[M];
				if (r >= 0) r = r + 0.5;
				else r = r - 0.5;
				r = (int)r;
			}
		}
		else /*'R'*/ str(r, L, 0, Txt);
		RR = r;
		break;
	}
	case FieldType::ALFANUM: {
		cc = ' ';
		goto label3;
		break;
	}
	case FieldType::NUMERIC: {
		cc = '0';
	label3:
		if (M == LeftJust) {
			while (Txt.length() < L) { Txt += cc; }
		}
		else {
			while (Txt.length() < L) { Txt = cc + Txt; }
		}
		if ((!Msk.empty()) && !TestMask(Txt, Msk)) goto label4;
		break;
	}
	case FieldType::DATE: {
		T = LeadChar(' ', TrailChar(Txt, ' '));
		if (T == "") r = 0;
		else {
			r = ValDate(T, *Mask);
			if ((r == 0.0) && (T != LeadChar(' ', OldTrailChar(' ', StrDate(r, *Mask)))))
			{
				SetMsgPar(*Mask);
				WrLLF10Msg(618);
			label4:
				screen.GotoXY(Col, Row);
				goto label2;
			}
		}
		Txt = StrDate(r, *Mask);
		RR = r;
		break;
	}
	}
	return result;
}


void WrPromptTxt(std::string& S, FrmlElem* Impl, FieldDescr* F, std::string& Txt, double& R)
{
	WORD x = 0, y = 0, d = 0, LWw = 0;
	std::string SS;
	std::string T;
	double RR = 0.0;
	bool BB = false;
	screen.WriteStyledStringToWindow(S, ProcAttr);
	x = screen.WhereX();
	y = screen.WhereY();
	d = WindMax.X - WindMin.X + 1;
	if (x + F->L - 1 > d) LWw = d - x;
	else LWw = F->L;
	TextAttr = screen.colors.dHili;
	if (Impl != nullptr) {
		switch (F->frml_type) {
		case 'R': RR = RunReal(CFile, Impl, CRecPtr); break;
		case 'S': SS = RunShortStr(CFile, Impl, CRecPtr); break;
		default: BB = RunBool(CFile, Impl, CRecPtr); break;
		}
		T = DecodeFieldRSB(F, F->L, RR, SS, BB);
	}
	screen.GotoXY(x, y);
	FieldEdit(F, nullptr, LWw, 1, T, R, true, true, false, 0);
	TextAttr = ProcAttr;
	if (Event.Pressed.KeyCombination() == __ESC) {
		EscPrompt = true;
		screen.GotoXY(1, y + 1);
	}
	else {
		EscPrompt = false;
		Txt = T.substr(0, LWw);
		screen.GotoXY(x, y);
		screen.ScrFormatWrText(x, y, "%s", T.c_str());
		screen.GotoXY(1, y + 1);
	}
}

bool PromptB(std::string& S, FrmlElem* Impl, FieldDescr* F)
{
	std::string Txt;
	double R = 0.0;
	WrPromptTxt(S, Impl, F, Txt, R);
	bool result = (Txt[0] == AbbrYes);
	if (Event.Pressed.KeyCombination() == __ESC) {
		if (Impl != nullptr) result = RunBool(CFile, Impl, CRecPtr);
		else result = false;
	}
	return result;
}

std::string PromptS(std::string& S, FrmlElem* Impl, FieldDescr* F)
{
	std::string Txt;
	double R = 0.0;
	WrPromptTxt(S, Impl, F, Txt, R);
	auto result = Txt;
	if (Event.Pressed.KeyCombination() == __ESC) {
		if (Impl != nullptr) result = RunShortStr(CFile, Impl, CRecPtr);
		else result = "";
	}
	return result;
}

double PromptR(std::string& S, FrmlElem* Impl, FieldDescr* F)
{
	std::string Txt;
	double R = 0.0;
	WrPromptTxt(S, Impl, F, Txt, R);
	auto result = R;
	if (Event.Pressed.KeyCombination() == __ESC) {
		if (Impl != nullptr) {
			result = RunReal(CFile, Impl, CRecPtr);
		}
		else {
			result = 0;
		}
	}
	return result;
}

int CRec()
{
	return BaseRec + IRec - 1;
	//return 0;
}

int CNRecs()
{
	int n = 0;
	if (EdRecVar) { return 1; }
	if (Subset) n = WK->NRecs();
	else {
		if (HasIndex) n = VK->NRecs();
		else n = CFile->FF->NRecs;
	}
	if (IsNewRec) n++;
	return n;
}

/// <summary>
/// Vraci poradi zaznamu v datovem souboru podle cisla indexu
/// </summary>
/// <param name="N">Cislo polozky v indexovem souboru (poradi)</param>
/// <returns>Cislo polozky v datovem souboru (poradi)</returns>
int AbsRecNr(int N)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "AbsRecNr(%i), CFile 0x%p", N, CFile->Handle);
	LockMode md;
	int result = 0;
	if (EdRecVar
#ifdef FandSQL
		|| CFile->IsSQLFile
#endif
		) {
		if (IsNewRec) result = 0;
		else result = 1;
		return result;
	}
	if (IsNewRec) {
		if ((N == CRec()) && (N == CNRecs())) {
			result = 0;
			return result;
		}
		if (N > CRec()) N--;
	}
	if (Subset) N = WK->NrToRecNr(CFile, N);
	else if (HasIndex) {
		md = CFile->NewLockMode(RdMode);
		CFile->FF->TestXFExist();
		N = VK->NrToRecNr(CFile, N);
		CFile->OldLockMode(md);
	}
	result = N;
	return result;
}

int LogRecNo(int N)
{
	LockMode md;
	int result = 0;
	if ((N <= 0) || (N > CFile->FF->NRecs)) return result;
	md = CFile->NewLockMode(RdMode);
	CFile->ReadRec(N, CRecPtr);
	if (!CFile->DeletedFlag(CRecPtr)) {
		if (Subset) result = WK->RecNrToNr(CFile, N, CRecPtr);
		else if (HasIndex) {
			CFile->FF->TestXFExist();
			result = VK->RecNrToNr(CFile, N, CRecPtr);
		}
		else result = N;
	}
	CFile->OldLockMode(md);
	return result;
}

bool IsSelectedRec(WORD I)
{
	XString x;
	auto result = false;
	if ((E->SelKey == nullptr) || (I == IRec) && IsNewRec) return result;
	int n = AbsRecNr(BaseRec + I - 1);
	void* cr = CRecPtr;
	if ((I == IRec) && WasUpdated) CRecPtr = E->OldRecPtr;
	result = E->SelKey->RecNrToPath(CFile, x, n, CRecPtr);
	CRecPtr = cr;
	return result;
}

bool EquOldNewRec()
{
	return (CompArea(CRecPtr, E->OldRecPtr, CFile->FF->RecLen) == _equ);
}

/// <summary>
/// Vycte X-ty zaznam (z DB nebo ze souboru v CFile)
/// Ulozi jej do CRecPtr
/// Nejedna se o fyzicke cislo zaznamu v souboru
/// </summary>
/// <param name="N">kolikaty zaznam</param>
void RdRec(int N)
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
		md = CFile->NewLockMode(RdMode);
		CFile->ReadRec(AbsRecNr(N), CRecPtr);
		CFile->OldLockMode(md);
	}
}

bool CheckOwner(EditD* E)
{
	XString X, X1;
	auto result = true;
	if (E->DownSet && (E->OwnerTyp != 'i')) {
		X.PackKF(CFile, E->DownKey->KFlds, CRecPtr);
		CFile = E->DownLD->ToFD;
		CRecPtr = E->DownRecPtr;
		X1.PackKF(CFile, E->DownLD->ToKey->KFlds, CRecPtr);
		X.S[0] = (char)(MinW(X.S.length(), X1.S.length()));
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
	//pstring* p1;
	//pstring* p2;
	auto result = true;
	if (k == nullptr) return result;
	X.PackKF(CFile, E->VK->KFlds, CRecPtr);
	while (k != nullptr) {
		//p1 = k->X1; p2 = k->X2;
		//if (p2 == nullptr) p2 = p1;
		//if ((p1->length() <= X.S.length()) && (X.S.length() <= p2->length() + 0xFF)) return result;
		if (k->X2.empty()) {
			if ((k->X1.length() <= X.S.length()) && (X.S.length() <= k->X1.length() + 0xFF)) return result;
		}
		else {
			if ((k->X1.length() <= X.S.length()) && (X.S.length() <= k->X2.length() + 0xFF)) return result;
		}
		k = (KeyInD*)k->pChain;
	}
	result = false;
	return result;
}

bool ELockRec(EditD* E, int N, bool IsNewRec, bool Subset)
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
		if (CFile->FF->NotCached()) {
			if (!CFile->Lock(N, 1/*withESC*/)) {
				result = false;
				return result;
			}
			md = CFile->NewLockMode(RdMode);
			CFile->ReadRec(N, CRecPtr);
			CFile->OldLockMode(md);
			if (Subset && !
				((NoCondCheck || RunBool(CFile, E->Cond, CRecPtr) && CheckKeyIn(E)) && CheckOwner(E))) {
				WrLLF10Msg(150); goto label1;
			}
		}
		else if (CFile->DeletedFlag(CRecPtr)) {
			WrLLF10Msg(148);
		label1:
			CFile->Unlock(N);
			result = false;
			return result;
		}
	}
	E->IsLocked = true;
	return result;
}

WORD RecAttr(WORD I)
{
	bool b = (I != IRec) || !IsNewRec;
	if (!IsNewRec && CFile->DeletedFlag(CRecPtr)) return E->dDel;
	else if (b && Select && RunBool(CFile, E->Bool, CRecPtr)) return E->dSubSet;
	else if (b && IsSelectedRec(I)) return E->dSelect;
	else return E->dNorm;
}

WORD FldRow(EFldD* D, WORD I)
{
	return E->FrstRow + E->NHdTxt + (I - 1) * RT->N + D->Ln - 1;
}

bool HasTTWw(FieldDescr* F)
{
	return (F->field_type == FieldType::TEXT) && (F->L > 1) && !E->IsUserForm;
}

void DisplEmptyFld(EFldD* D, WORD I)
{
	char c = '\0';
	screen.GotoXY(D->Col, FldRow(D, I));
	if ((D->FldD->Flg & f_Stored) != 0) c = '.';
	else c = ' ';
	//for (j = 1; j <= D->L; j++) printf("%c", c);
	printf("%s", std::string(D->L, c).c_str());
	if (HasTTWw(D->FldD)) printf("%*c", D->FldD->L - 1, ' ');
}

void Wr1Line(FieldDescr* F)
{
	auto X = screen.WhereX();
	auto Y = screen.WhereY();
	std::string ls = CFile->loadS(F, CRecPtr);
	ls = GetNthLine(ls, 1, 1);
	WORD max = F->L - 2;
	ls = GetStyledStringOfLength(ls, 0, max);
	size_t chars = screen.WriteStyledStringToWindow(ls, E->dNorm);
	TextAttr = E->dNorm;
	if (chars < max) screen.ScrFormatWrStyledText(X + chars, Y, E->dNorm, "%*c", max - chars, ' ');

	/*pstring Txt;
	LongStr* s = CopyLine(loadLongS(F), 1, 1);
	WORD max = F->L - 2;
	WORD l = s->LL;
	if (l > 255) l = 255;
	Move(s->A, &Txt[1], l);
	Txt[0] = (char)l;
	l = LenStyleStr(Txt);
	if (l > max)
	{
		l = max;
		Txt[0] = (char)LogToAbsLenStyleStr(Txt, l);
	}
	//WrStyleStr(Txt, E->dNorm);
	screen.WriteStyledStringToWindow(Txt, E->dNorm);
	ReleaseStore(s);
	TextAttr = E->dNorm;
	if (l < max) printf("%*c", max - l, ' ');*/
}

void DisplFld(EFldD* D, WORD I, BYTE Color)
{
	WORD r = FldRow(D, I);
	auto F = D->FldD;
	screen.GotoXY(D->Col, r);
	std::string Txt = DecodeField(CFile, F, D->L, CRecPtr);
	for (size_t j = 0; j < Txt.length(); j++)
		if ((unsigned char)Txt[j] < ' ') Txt[j] = Txt[j] + 0x40;
	screen.WriteStyledStringToWindow(Txt, Color);
	if (HasTTWw(F)) {
		screen.GotoXY(D->Col + 2, r);
		Wr1Line(F);
	}
}

// Display a form record
void DisplRec(WORD I)
{
	EFldD* D = nullptr;
	bool NewFlds = false;
	WORD a = E->dNorm;
	int N = BaseRec + I - 1;
	bool IsCurrNewRec = IsNewRec && (I == IRec);
	void* p = CFile->GetRecSpace();
	if ((N > CNRecs()) && !IsCurrNewRec) {
		NewFlds = true;
		goto label1;
	}
	if (I == IRec) {
		CRecPtr = E->NewRecPtr;
	}
	else {
		CRecPtr = p;
		RdRec(N);
	}
	NewFlds = false;
	if (!IsNewRec) a = RecAttr(I);
label1:
	D = E->FirstFld;
	while (D != nullptr) {
		if (IsCurrNewRec && D == FirstEmptyFld && D->Impl == nullptr) NewFlds = true;
		TextAttr = a;
		// Display an item of the record
		if (D->Page == CPage) {
			if (NewFlds) {
				DisplEmptyFld(D, I);
			}
			else {
				DisplFld(D, I, TextAttr);
			}
		}
		if (IsCurrNewRec && (D == FirstEmptyFld)) NewFlds = true;
		D = D->pChain;
	}
	CFile->ClearRecSpace(p);
	ReleaseStore(&p);
	CRecPtr = E->NewRecPtr;
}

bool LockRec(bool Displ)
{

	bool result;
	if (E->IsLocked) {
		return true;
	}
	bool b = ELockRec(E, AbsRecNr(CRec()), IsNewRec, Subset);
	result = b;
	if (b && !IsNewRec && !EdRecVar && CFile->FF->NotCached() && Displ) {
		DisplRec(IRec);
	}
	return result;
}

void UnLockRec(EditD* E)
{
	if (E->FD->FF->IsShared() && E->IsLocked && !E->EdRecVar) {
		CFile->Unlock(E->LockedRec);
	}
	E->IsLocked = false;
}

void NewRecExit()
{
	for (auto& X : E->ExD) {
		if (X->AtNewRec) {
			EdBreak = 18;
			LastTxtPos = -1;
			StartExit(X, false);
		}
	}
}

void SetCPage(WORD* c_page, ERecTxtD** rt)
{
	*c_page = CFld->Page;
	*rt = E->RecTxt;
	for (WORD i = 1; i < *c_page; i++) {
		*rt = (*rt)->pChain;
	}
}


void DisplRecNr(int N)
{
	if (E->RecNrLen > 0) {
		screen.GotoXY(E->RecNrPos, 1);
		TextAttr = screen.colors.fNorm;
		screen.ScrFormatWrText(E->RecNrPos, 1, "%*i", E->RecNrLen, N);
	}
}

void AdjustCRec()
{
	if (CRec() <= CNRecs()) return;
	while (CRec() > CNRecs()) {
		if (IRec > 1) IRec--;
		else BaseRec--;
	}
	if (BaseRec == 0) {
		BaseRec = 1;
		if (!IsNewRec) {
			IsNewRec = true;
			Append = true;
			FirstEmptyFld = CFld;
			CFile->ZeroAllFlds(CRecPtr, false);
			SetWasUpdated(CFile->FF, CRecPtr);
			NewRecExit();
		}
		else {
			SetWasUpdated(CFile->FF, CRecPtr);
		}
		NewDisplLL = true;
	}
	UnLockRec(E);
	LockRec(false);
	DisplRecNr(CRec());
}

void WrEStatus()
{
	E->CFld = CFld;
	// Move(FirstEmptyFld, &E->FirstEmptyFld, uintptr_t(SelMode) - uintptr_t(FirstEmptyFld) + 1);
	E->FirstEmptyFld = FirstEmptyFld;
	E->VK = VK;	E->WK = WK; E->BaseRec = BaseRec; E->IRec = IRec;
	E->IsNewRec = IsNewRec; E->Append = Append; E->Select = Select;
	E->WasUpdated = WasUpdated; E->EdRecVar = EdRecVar;	E->AddSwitch = AddSwitch;
	E->ChkSwitch = ChkSwitch; E->WarnSwitch = WarnSwitch; E->SubSet = Subset;
	E->NoDelTFlds = NoDelTFlds; E->WasWK = WasWK; E->NoDelete = NoDelete;
	E->VerifyDelete = VerifyDelete; E->NoCreate = NoCreate; E->F1Mode = F1Mode;
	E->OnlyAppend = OnlyAppend; E->OnlySearch = OnlySearch;	E->Only1Record = Only1Record;
	E->OnlyTabs = OnlyTabs; E->NoESCPrompt = NoESCPrompt; E->MustESCPrompt = MustESCPrompt;
	E->Prompt158 = Prompt158; E->NoSrchMsg = NoSrchMsg; E->WithBoolDispl = WithBoolDispl;
	E->Mode24 = Mode24; E->NoCondCheck = NoCondCheck; E->F3LeadIn = F3LeadIn;
	E->LUpRDown = LUpRDown; E->MouseEnter = MouseEnter; E->TTExit = TTExit;
	E->MakeWorkX = MakeWorkX; E->NoShiftF7Msg = NoShiftF7Msg; E->MustAdd = MustAdd;
	E->MustCheck = MustCheck; E->SelMode = SelMode;
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
	CFld = E->CFld;
	if (CFile->FF->XF != nullptr) HasIndex = true;
	else HasIndex = false;
	if (CFile->FF->TF != nullptr) HasTF = true;
	else HasTF = false;
	SetCPage(&CPage, &RT);
}

void DuplFld(FileD* file_d1, FileD* file_d2, void* record1, void* record2, void* RPt, FieldDescr* field_d1, FieldDescr* field_d2)
{
	LongStr* ss;
	pstring s;
	double r = 0.0;
	bool b = false;

	switch (field_d1->frml_type) {
	case 'S': {
		if (field_d1->field_type == FieldType::TEXT) {
			ss = file_d1->loadLongS(field_d1, record1);
			if (RPt == nullptr) {
				file_d2->FF->DelTFld(field_d2, record2);
			}
			else {
				file_d2->FF->DelDifTFld(field_d2, record2, RPt);
			}
			file_d2->saveLongS(field_d2, ss, record2);
			delete ss; ss = nullptr;
		}
		else {
			s = file_d1->loadOldS(field_d1, record1);
			file_d2->saveS(field_d2, s, record2);
		}
		break;
	}
	case 'R': {
		r = file_d1->loadR(field_d1, record1);
		file_d2->saveR(field_d2, r, record2);
		break;
	}
	case 'B': {
		b = file_d1->loadB(field_d1, record1);
		file_d2->saveB(field_d2, b, record2);
		break;
	}
	}
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
	WORD TA = RecAttr(I);
	EFldD* D = E->FirstFld;
	while (D != nullptr) {
		if (D->Page == CPage) {
			SetFldAttr(D, I, TA);
		}
		D = (EFldD*)D->pChain;
	}
}

void DisplTabDupl()
{
	EFldD* D = E->FirstFld;
	TextAttr = E->dTab;
	while (D != nullptr) {
		if (D->Page == CPage) {
			const short Col = D->Col + D->L;
			const short Row = FldRow(D, 1);
			if (D->Tab) {
				if (D->Dupl) screen.ScrFormatWrText(Col, Row, "%c", 0x1F); // printf("%c", 0x1F);
				else screen.ScrFormatWrText(Col, Row, "%c", 0x11); // printf("%c", 0x11);
			}
			else if (D->Dupl) {
				screen.ScrFormatWrText(Col, Row, "%c", 0x19); // printf("%c", 0x19);
			}
			else {
				screen.ScrFormatWrText(Col, Row, "%c", ' '); // printf(" ");
			}
		}
		D = (EFldD*)D->pChain;
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
				for (j = 1; j <= m.length(); j++) x.Append(' ');
			}
		}
		else { x.Append(s[i]); i++; }
	if (x.length() > TxtCols) x[0] = (char)TxtCols;
	//printf("%s", x.c_str());
	screen.ScrWrText(1, 1, x.c_str());
	DisplRecNr(CRec());
}

void DisplBool()
{
	if (!WithBoolDispl) return;
	screen.GotoXY(1, 2);
	TextAttr = E->dSubSet;
	ClrEol();
	if (Select) {
		std::string s = E->BoolTxt;
		if (s.length() > TxtCols) s[0] = (char)TxtCols;
		screen.GotoXY((TxtCols - s.length()) / 2 + 1, 2);
		printf("%s", s.c_str());
	}
}

// zobrazi zaznamy v editoru
void DisplAllWwRecs()
{
	LockMode md = NullMode;
	WORD n = E->NRecs; // pocet zaznamu k zobrazeni (na strance)
	if ((n > 1) && !EdRecVar) md = CFile->NewLockMode(RdMode);
	AdjustCRec();
	if (!IsNewRec && !WasUpdated) RdRec(CRec());
	for (WORD i = 1; i <= n; i++) {
		DisplRec(i);
	}
	IVon();
	if ((n > 1) && !EdRecVar) CFile->OldLockMode(md);
}

void SetNewWwRecAttr()
{
	CRecPtr = CFile->GetRecSpace();
	for (WORD I = 1; I <= E->NRecs; I++) {
		if (BaseRec + I - 1 > CNRecs()) break;
		if (!IsNewRec || (I != IRec)) {
			RdRec(BaseRec + I - 1);
			SetRecAttr(I);
		}
	}
	IVon();
	CFile->ClearRecSpace(CRecPtr);
	ReleaseStore(&CRecPtr);
	CRecPtr = E->NewRecPtr;
}

void MoveDispl(WORD From, WORD Where, WORD Number)
{
	for (WORD i = 1; i <= Number; i++) {
		EFldD* D = E->FirstFld;
		while (D != nullptr) {
			WORD r1 = FldRow(D, From) - 1;
			WORD r2 = FldRow(D, Where) - 1;
			screen.ScrMove(D->Col - 1, r1, D->Col - 1, r2, D->L);
			if (HasTTWw(D->FldD))
				screen.ScrMove(D->Col + 1, r1, D->Col + 1, r2, D->FldD->L - 2);
			D = D->pChain;
		}
		if (From < Where) {
			From--;
			Where--;
		}
		else {
			From++;
			Where++;
		}
	}
}

void SetNewCRec(int N, bool withRead)
{
	int Max, I;
	Max = E->NRecs; I = N - BaseRec + 1;
	if (I > Max) { BaseRec += I - Max; IRec = Max; }
	else if (I <= 0) { BaseRec -= abs(I) + 1; IRec = 1; }
	else IRec = I;
	if (withRead) RdRec(CRec());
}

void WriteSL(StringListEl* SL)
{
	while (SL != nullptr) {
		WORD row = screen.WhereY();
		screen.WriteStyledStringToWindow(SL->S, E->Attr);
		screen.GotoXY(E->FrstCol, row + 1);
		SL = SL->pChain;
	}
}

void DisplRecTxt()
{
	screen.GotoXY(E->FrstCol, E->FrstRow + E->NHdTxt);
	for (WORD i = 1; i <= E->NRecs; i++) {
		WriteSL(RT->SL);
	}
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
		for (i = EV.R1; i <= EV.R2; i++) {
			screen.ScrColor(EV.C2, i, E->ShdwX, screen.colors.ShadowAttr);
		}
	screen.Window(EV.C1, EV.R1, EV.C2, EV.R2);
	TextAttr = E->Attr;
	ClrScr();

	WriteWFrame(E->WFlags, E->Top, "");
	screen.Window(1, 1, TxtCols, TxtRows);
	DisplSysLine();
	DisplBool();
	screen.GotoXY(E->FrstCol, E->FrstRow);
	WriteSL(E->HdTxt);
	DisplRecTxt(); // zobrazi prazdne formulare
	DisplTabDupl();
	NewDisplLL = true;
	DisplAllWwRecs(); // doplni do formularu data nebo tecky
}

void DisplWwRecsOrPage(WORD* c_page, ERecTxtD** rt)
{
	if (*c_page != CFld->Page) {
		SetCPage(c_page, rt);
		TextAttr = E->Attr;
		Wind oldMin = WindMin;
		Wind oldMax = WindMax;
		screen.Window(E->FrstCol, E->FrstRow + E->NHdTxt, E->LastCol, E->FrstRow + E->Rows - 1);
		ClrScr();
		WindMin = oldMin;
		WindMax = oldMax;
		DisplRecTxt();
		DisplTabDupl();
	}
	DisplAllWwRecs();
	DisplRecNr(CRec());
}

void DuplOwnerKey()
{
	if (!E->DownSet || (E->OwnerTyp == 'i')) return;
	KeyFldD* KF = E->DownLD->ToKey->KFlds;
	//Arg = E->DownLD->Args;
	//while (Arg != nullptr) {
	for (auto& arg : E->DownLD->Args) {
		DuplFld(E->DownLD->ToFD, CFile, E->DownRecPtr, E->NewRecPtr, E->OldRecPtr,
			KF->FldD, arg->FldD);
		//Arg = Arg->pChain;
		KF = KF->pChain;
	}
}

bool TestDuplKey(FileD* file_d, XKey* K)
{
	XString x;
	int N = 0;
	x.PackKF(file_d, K->KFlds, CRecPtr);
	return K->Search(file_d, x, false, N) && (IsNewRec || (E->LockedRec != N));
}

void DuplKeyMsg(XKey* K)
{
	SetMsgPar(K->Alias);
	WrLLF10Msg(820);
}

void BuildWork()
{
	XScan* Scan = nullptr; XScan* Scan2 = nullptr;
	void* p = nullptr;
	XKey* K = nullptr;
	KeyFldD* KF = nullptr;
	XString xx;
	bool dupl = true, intvl = false, ok = false;
	XWKey* wk2 = nullptr; KeyInD* ki = nullptr;
	FrmlElem* boolP = nullptr;
	WORD l = 0;
	FieldDescr* f = nullptr;

	if (!CFile->Keys.empty()) KF = CFile->Keys[0]->KFlds;
	if (HasIndex) {
		K = VK;
		KF = K->KFlds;
		dupl = K->Duplic;
		intvl = K->IntervalTest;
	}
	WK->Open(CFile, KF, dupl, intvl);
	if (OnlyAppend) return;
	boolP = E->Cond;
	ki = E->KIRoot;
	wk2 = nullptr;
	MarkStore(p);
	ok = false;
	f = nullptr;
	//NewExit(Ovr(), er);
	//goto label1;
	try {
		if (E->DownSet) {
			Scan = new XScan(CFile, E->DownKey, nullptr, false);
			if (E->OwnerTyp == 'i') Scan->ResetOwnerIndex(E->DownLD, E->DownLV, boolP);
			else {
				CFile = E->DownLD->ToFD;
				CRecPtr = E->DownRecPtr;
				xx.PackKF(CFile, E->DownLD->ToKey->KFlds, CRecPtr);
				CFile = E->FD;
				CRecPtr = E->NewRecPtr;
				Scan->ResetOwner(&xx, boolP);
			}
			if (ki != nullptr) {
				wk2 = new XWKey(CFile);
				wk2->Open(CFile, KF, true, false);
				CFile->FF->CreateWIndex(Scan, wk2, 'W');
				Scan2 = new XScan(CFile, wk2, ki, false);
				Scan2->Reset(nullptr, false, CRecPtr);
				Scan = Scan2;
			}
		}
		else {
#ifdef FandSQL
			if (CFile->IsSQLFile && (boolP == nullptr)) {
				l = CFile->RecLen; f = CFile->FldD; OnlyKeyArgFlds(WK);
			}
#endif
			if (
#ifdef FandSQL
				CFile->IsSQLFile ||
#endif
				(boolP != nullptr))
				if ((K != nullptr) && !K->InWork && (ki == nullptr)) K = nullptr;
			Scan = new XScan(CFile, K, ki, false);
			Scan->Reset(boolP, E->SQLFilter, CRecPtr);
		}
		CFile->FF->CreateWIndex(Scan, WK, 'W');
		Scan->Close();
		if (wk2 != nullptr) wk2->Close(CFile);
		ok = true;
	}
	catch (std::exception& e) {
		// TODO: log error
	}

	if (f != nullptr) {
		CFile->FldD.clear();
		CFile->FldD.push_back(f);
		WK->KFlds = KF;
		CFile->FF->RecLen = l;
	}
	if (!ok) {
		GoExit();
	}
	ReleaseStore(&p);
}

void SetStartRec()
{
	int n = 0;
	KeyFldD* kf = nullptr;
	XKey* k = VK;
	if (Subset) k = WK;
	if (k != nullptr) kf = k->KFlds;
	if ((!E->StartRecKey.empty()) && (k != nullptr)) {
		if (k->FindNr(CFile, E->StartRecKey, n)) goto label1;
	}
	else if (E->StartRecNo > 0) {
		n = LogRecNo(E->StartRecNo);
	label1:
		n = MaxL(1, MinL(n, CNRecs()));
		IRec = MaxW(1, MinW(E->StartIRec, E->NRecs));
		BaseRec = n - IRec + 1;
		if (BaseRec <= 0) {
			IRec += BaseRec - 1;
			BaseRec = 1;
		}
	}
	if (Only1Record) {
		if (CNRecs() > 0) {
			RdRec(CRec());
			n = AbsRecNr(CRec());
		}
		else n = 0;
		if (Subset) WK->Close(CFile);
		Subset = true;
		if (n == 0) WK->Open(CFile, nullptr, true, false);
		else WK->OneRecIdx(CFile, kf, n, CRecPtr);
		BaseRec = 1;
		IRec = 1;
	}
}

bool OpenEditWw()
{
	LockMode md, md1, md2;
	int n = 0;
	auto result = false;
	CFile = E->Journal;
	if (CFile != nullptr) OpenCreateF(CFile, CPath, Shared);
	RdEStatus();
	if (EdRecVar) {
		if (OnlyAppend) {
			goto label2;
		}
		else {
			goto label3;
		}
	}
#ifdef FandSQL
	if (!CFile->IsSQLFile)
#endif
		OpenCreateF(CFile, CPath, Shared);
	E->OldMd = E->FD->FF->LMode;
	UpdCount = 0;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		if ((VK = nullptr) || !VK->InWork) Subset = true
	}
	else
#endif
	{
		if (HasIndex) {
			CFile->FF->TestXFExist();
		}
		md = NoDelMode;
		if (OnlyAppend || (E->Cond != nullptr) || (E->KIRoot != nullptr) || E->DownSet ||
			MakeWorkX && HasIndex && CFile->FF->NotCached() && !Only1Record)
		{
			Subset = true;
			if (HasIndex) {
				md = NoExclMode;
			}
			else {
				md = NoCrMode;
			}
		}
		else if ((VK != nullptr) && VK->InWork) {
			md = NoExclMode;
		}
	}
	if (Subset || Only1Record) {
		WK = new XWKey(CFile);
	}
	if (!CFile->TryLockMode(md, md1, 1)) {
		EdBreak = 15;
		goto label1;
	}
	md2 = CFile->NewLockMode(RdMode);
	if (E->DownSet && (E->OwnerTyp == 'F')) {
		CFile = E->DownLD->ToFD;
		CRecPtr = E->DownRecPtr;
		md1 = CFile->NewLockMode(RdMode);
		n = E->OwnerRecNo;
		if ((n == 0) || (n > CFile->FF->NRecs)) {
			CFile->RunErrorM(E->OldMd);
			RunError(611);
		}
		CFile->ReadRec(n, CRecPtr);
		CFile->OldLockMode(md1);
		CFile = E->FD;
		CRecPtr = E->NewRecPtr;
	}
	if (Subset) {
		BuildWork();
	}
	if (!Only1Record && HasIndex && VK->InWork) {
		if (!Subset) WK = (XWKey*)VK;
		if (!CFile->Keys.empty()) {
			VK = CFile->Keys[0];
		}
		else {
			VK = nullptr;
		}
		WasWK = true;
		Subset = true;
	}
#ifdef FandSQL
	if (CFile->IsSQLFile) Strm1->DefKeyAcc(WK);
#endif
	if (!OnlyAppend) SetStartRec();
	if (CNRecs() == 0)
		if (NoCreate) {
			if (Subset) {
				FileMsg(CFile, 107, '0');
			}
			else {
				FileMsg(CFile, 115, '0');
			}
			EdBreak = 13;
		label1:
			if (Subset && !WasWK) {
				WK->Close(CFile);
			}
			CFile->OldLockMode(E->OldMd);
			result = false;
			return result;
		}
		else {
		label2:
			IsNewRec = true;
			Append = true;
			LockRec(false);
			CFile->ZeroAllFlds(CRecPtr, false);
			DuplOwnerKey();
			SetWasUpdated(CFile->FF, CRecPtr);
		}
	else {
		RdRec(CRec());
	}
label3:
	MarkStore(E->AfterE);
	DisplEditWw();
	result = true;
	if (!EdRecVar) CFile->OldLockMode(md2);
	if (IsNewRec) NewRecExit();
	return result;
}

void RefreshSubset()
{
	LockMode md = CFile->NewLockMode(RdMode);
	if (Subset && !(OnlyAppend || Only1Record || WasWK)) {
		WK->Close(CFile);
		BuildWork();
	}
	DisplAllWwRecs();
	CFile->OldLockMode(md);
}

void GotoRecFld(int NewRec, EFldD* NewFld)
{
	int NewIRec = 0, NewBase = 0, D = 0, Delta = 0;
	WORD i = 0, Max = 0; LockMode md;
	IVoff();
	CFld = NewFld;
	if (NewRec == CRec()) {
		if (CPage != CFld->Page) {
			DisplWwRecsOrPage(&CPage, &RT);
		}
		else {
			IVon();
		}
		return;
	}
	if (!EdRecVar) md = CFile->NewLockMode(RdMode);
	if (NewRec > CNRecs()) NewRec = CNRecs();
	if (NewRec <= 0) NewRec = 1;
	if (Select) SetRecAttr(IRec);
	CFld = NewFld;
	Max = E->NRecs;
	Delta = NewRec - CRec();
	NewIRec = IRec + Delta;
	if ((NewIRec > 0) && (NewIRec <= Max)) {
		IRec = NewIRec;
		RdRec(CRec());
		goto label1;
	}
	NewBase = BaseRec + Delta;
	if (NewBase + Max - 1 > CNRecs()) NewBase = CNRecs() - pred(Max);
	if (NewBase <= 0) NewBase = 1;
	IRec = NewRec - NewBase + 1;
	D = NewBase - BaseRec;
	BaseRec = NewBase;
	RdRec(CRec());
	if (abs(D) >= Max) {
		DisplWwRecsOrPage(&CPage, &RT);
		goto label2;
	}
	if (D > 0) {
		MoveDispl(D + 1, 1, Max - D);
		for (i = Max - D + 1; i <= Max; i++) DisplRec(i);
	}
	else {
		D = -D;
		MoveDispl(Max - D, Max, Max - D);
		for (i = 1; i <= D; i++) DisplRec(i);
	}
label1:
	DisplRecNr(CRec());
	IVon();
label2:
	if (!EdRecVar) {
		CFile->OldLockMode(md);
	}
}

void UpdMemberRef(void* POld, void* PNew)
{
	XString x, xnew, xold;
	XScan* Scan = nullptr;
	FileD* cf = CFile;
	void* cr = CRecPtr; void* p = nullptr; void* p2 = nullptr;
	XKey* k = nullptr;
	KeyFldD* kf = nullptr, * kf1 = nullptr, * kf2 = nullptr; // , * Arg = nullptr;

	for (auto& LD : LinkDRoot) {
		if ((LD->MemberRef != 0) && (LD->ToFD == cf) && ((PNew != nullptr) || (LD->MemberRef != 2))) {
			CFile = cf;
			kf2 = LD->ToKey->KFlds;
			CRecPtr = POld;
			xold.PackKF(CFile, kf2, CRecPtr);
			if (PNew != nullptr) {
				CRecPtr = PNew;
				xnew.PackKF(CFile, kf2, CRecPtr);
				if (xnew.S == xold.S) continue;
			}
			CFile = LD->FromFD;
#ifdef FandSQL
			sql = CFile->IsSQLFile;
#endif
			k = GetFromKey(LD);
			kf1 = k->KFlds;
			p = CFile->GetRecSpace();
			CRecPtr = p;
			if (PNew != nullptr) {
				p2 = CFile->GetRecSpace();
			}
			Scan = new XScan(CFile, k, nullptr, true);
			Scan->ResetOwner(&xold, nullptr);
#ifdef FandSQL
			if (!sql)
#endif
				CFile->FF->ScanSubstWIndex(Scan, kf1, 'W');
		label1:
			CRecPtr = p;
			Scan->GetRec(CRecPtr);
			if (!Scan->eof) {
#ifdef FandSQL
				if (sql) x.PackKF(kf1);
#endif
				if (PNew == nullptr) {
					RunAddUpdate(CFile, '-', nullptr, false, nullptr, LD, CRecPtr);
					UpdMemberRef(p, nullptr);
#ifdef FandSQL
					if (sql) Strm1->DeleteXRec(k, @x, false);
					else
#endif
						CFile->FF->DeleteXRec(Scan->RecNr, true, CRecPtr);
				}
				else {
					Move(CRecPtr, p2, CFile->FF->RecLen);
					CRecPtr = p2;
					kf = kf2;
					//Arg = LD->Args;
					//while (kf != nullptr) {
					for (auto& arg : LD->Args) {
						DuplFld(cf, CFile, PNew, p2, nullptr, kf->FldD, arg->FldD);
						//Arg = Arg->pChain;
						kf = kf->pChain;
					}
					RunAddUpdate(CFile, 'd', p, false, nullptr, LD, CRecPtr);
					UpdMemberRef(p, p2);
#ifdef FandSQL
					if (sql) Strm1->UpdateXRec(k, @x, false) else
#endif
						CFile->FF->OverWrXRec(Scan->RecNr, p, p2, CRecPtr);
				}
				goto label1;
			}
			Scan->Close();
			CFile->ClearRecSpace(p);
			ReleaseStore(&p);
		}
	}
	CFile = cf; CRecPtr = cr;
}

void WrJournal(char Upd, void* RP, double Time)
{
	// Upd:
	// + new record; - deleted record; O old record data; N new record data

	size_t srcOffset = 0;
	if (E->Journal != nullptr) {
		WORD l = CFile->FF->RecLen;
		int n = AbsRecNr(CRec());
		if (CFile->FF->XF != nullptr) {
			srcOffset += 2;
			l--;
		}
		CFile = E->Journal;
		//CRecPtr = GetRecSpace();

		const auto newData = std::make_unique<BYTE[]>(CFile->FF->RecLen + 2);

		auto it = CFile->FldD.begin();

		CFile->saveS(*it++, std::string(1, Upd), newData.get());	// change type
		CFile->saveR(*it++, int(n), newData.get());						// record number
		CFile->saveR(*it++, int(UserCode), newData.get());				// user code
		CFile->saveR(*it++, Time, newData.get());							// timestamp

		char* src = (char*)RP;
		memcpy(&newData.get()[(*it)->Displ], &src[srcOffset], l);		// record data

		LockMode md = CFile->NewLockMode(CrMode);
		CFile->IncNRecs(1);
		CFile->WriteRec(CFile->FF->NRecs, newData.get());
		CFile->OldLockMode(md);
		CFile = E->FD;
		CRecPtr = E->NewRecPtr;
	}
	UpdCount++;
	if (UpdCount == E->SaveAfter) {
		SaveAndCloseAllFiles();
		UpdCount = 0;
	}
}

bool LockForMemb(FileD* FD, WORD Kind, LockMode NewMd, LockMode& md)
{
	LockMode md1; /*0-ExLMode,1-lock,2-unlock*/
	auto result = false;
	for (auto& ld : LinkDRoot) {
		if ((ld->ToFD == FD)
			&& ((NewMd != DelMode) && (ld->MemberRef != 0) || (ld->MemberRef == 1))
			&& (ld->FromFD != FD)) {
			CFile = ld->FromFD;
			switch (Kind) {
			case 0: {
				CFile->FF->TaLMode = CFile->FF->LMode;
				break;
			}
			case 1: {
				md = NewMd;
				if (!CFile->TryLockMode(NewMd, md1, 2)) return result;
				break;
			}
			case 2: {
				CFile->OldLockMode(CFile->FF->TaLMode);
				break;
			}
			}
			if (!LockForAdd(CFile, Kind, true, md)) return result;
			if (!LockForMemb(ld->FromFD, Kind, NewMd, md)) return result;
		}
	}
	result = true;
	return result;
}

bool LockWithDep(LockMode CfMd, LockMode MembMd, LockMode& OldMd)
{
	FileD* cf2 = nullptr;
	bool b = false;
	int w1 = 0;
	LockMode md;
	auto result = true;
	if (EdRecVar) return result;
	FileD* cf = CFile;
	int w = 0;
	LockForAdd(cf, 0, true, md);
	LockForMemb(cf, 0, MembMd, md);
label1:
	CFile = cf;
	if (!CFile->TryLockMode(CfMd, OldMd, 1)) {
		md = CfMd;
		goto label3;
	}
	if (!LockForAdd(cf, 1, true, md)) {
		cf2 = CFile;
		goto label2;
	}
	if (MembMd == NullMode) goto label4;
	if (!LockForMemb(cf, 1, MembMd, md)) {
		cf2 = CFile;
		LockForMemb(cf, 2, MembMd, md);
	label2:
		LockForAdd(cf, 2, true, md);
		CFile = cf;
		CFile->OldLockMode(OldMd);
		CFile = cf2;
	label3:
		SetPathAndVolume(CFile);
		SetMsgPar(CPath, LockModeTxt[md]);
		w1 = PushWrLLMsg(825, true);
		if (w == 0) w = w1;
		else TWork.Delete(w1);
		LockBeep();
		if (KbdTimer(spec.NetDelay, 1)) goto label1;
		result = false;
	}
label4:
	CFile = cf;
	if (w != 0) PopW(w);
	return result;
}

void UnLockWithDep(LockMode OldMd)
{
	LockMode md;
	if (EdRecVar) return;
	FileD* cf = CFile;
	CFile->OldLockMode(OldMd);
	LockForAdd(cf, 2, true, md);
	LockForMemb(cf, 2, md, md);
	CFile = cf;
}

void UndoRecord()
{
	LockMode md;
	if (!IsNewRec && WasUpdated) {
		if (HasTF) {
			if (NoDelTFlds) {
				FieldDescr* f = CFile->FldD.front();
				while (f != nullptr) {
					if (((f->Flg & f_Stored) != 0) && (f->field_type == FieldType::TEXT))
						*(int*)((char*)(E->OldRecPtr) + f->Displ) = *(int*)(((char*)(CRecPtr)+f->Displ));
					f = f->pChain;
				}
			}
		}
		else { // je toto spravne zanorene???
			CFile->DelAllDifTFlds(E->NewRecPtr, E->OldRecPtr);
		}

		Move(E->OldRecPtr, E->NewRecPtr, CFile->FF->RecLen);
		WasUpdated = false; NoDelTFlds = false;
		UnLockRec(E);
		DisplRec(IRec);
		IVon();
	}
}

bool CleanUp()
{
	if (HasIndex && CFile->DeletedFlag(CRecPtr)) return false;
	for (auto& X : E->ExD) {
		if (X->AtWrRec) {
			EdBreak = 17;
			bool ok = EdOk;
			EdOk = true;
			LastTxtPos = -1;
			if (!StartExit(X, false) || !EdOk) {
				EdOk = ok;
				return false;
			}
			EdOk = ok;
			WasUpdated = false;
		}
	}
	if (AddSwitch) {
		for (auto& ld : LinkDRoot) {
			if ((ld->MemberRef == 2) && (ld->ToFD == CFile) && Owned(CFile, nullptr, nullptr, ld, CRecPtr) > 0) {
				WrLLF10Msg(662);
				return false;
			}
		}
		if (!RunAddUpdate(CFile, '-', nullptr, false, nullptr, nullptr, CRecPtr)) return false;
		UpdMemberRef(CRecPtr, nullptr);
	}
	if (!ChptDel()) return false;
	WrJournal('-', CRecPtr, Today() + CurrTime());
	return true;
}

bool DelIndRec(int I, int N)
{
	bool result = false;
	if (CleanUp()) {
		CFile->FF->DeleteXRec(N, true, CRecPtr);
		SetUpdHandle(CFile->FF->Handle); // navic
		SetUpdHandle(CFile->FF->XF->Handle); // navic
		if ((E->SelKey != nullptr) && E->SelKey->Delete(CFile, N, CRecPtr)) E->SelKey->NR--;
		if (Subset) WK->DeleteAtNr(CFile, I);
		result = true;
		E->EdUpdated = true;
	}
	return result;
}

bool DeleteRecProc()
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "DeleteRecProc() deleting item (CFile '%c')", CFile->Name.c_str());

	int I = 0, J = 0, N = 0, oBaseRec = 0;
	WORD oIRec = 0;
	bool Group = false, fail = false; LockMode OldMd;
	bool b = false;
	auto result = false; Group = false;
	if (Select) {
		F10SpecKey = VK_ESCAPE;
		Group = PromptYN(116);
		if (Event.Pressed.KeyCombination() == __ESC) return result;
	}
	if (!Group) {
		if (VerifyDelete && !PromptYN(109)) return result;
	}
	if (!LockWithDep(DelMode, DelMode, OldMd)) return result;
	UndoRecord();
	N = AbsRecNr(CRec());
	RdRec(CRec());
	oIRec = IRec;
	oBaseRec = BaseRec;    /* exit proc uses CRec for locking etc.*/
	if (HasIndex
#ifdef FandSQL
		|| CFile->IsSQLFile
#endif
		) {
		//log->log(loglevel::DEBUG, "... from file with index ...");
		CFile->FF->TestXFExist();
		if (Group) {
			IRec = 1; BaseRec = 1;
			while (BaseRec <= CNRecs()) {
				N = AbsRecNr(BaseRec);
				CFile->ClearDeletedFlag(CRecPtr); /*prevent err msg 148*/
				if (!ELockRec(E, N, false, Subset)) goto label1;
				RdRec(BaseRec);
				if (RunBool(CFile, E->Bool, CRecPtr)) b = DelIndRec(BaseRec, N);
				else {
					b = true;
					BaseRec++;
				}
				UnLockRec(E);
				if (!b) goto label1;
			}
		label1:
			{}
		}
		else {
			if (!ELockRec(E, N, false, Subset)) goto label1;
			DelIndRec(CRec(), N);
			UnLockRec(E);
		}
	}
	else if (Group) {
		J = 0;
		fail = false;
		BaseRec = 1;
		IRec = 1;
		E->EdUpdated = true;
		for (I = 1; I <= CFile->FF->NRecs; I++) {
			CFile->ReadRec(I, CRecPtr);
			if (fail) goto label2;
			if (Subset) {
				if ((BaseRec > WK->NRecs()) || (WK->NrToRecNr(CFile, BaseRec) != J + 1)) goto label2;
			}
			else BaseRec = I;
			if (RunBool(CFile, E->Bool, CRecPtr)) {
				if (!CleanUp()) {
					fail = true;
					goto label2;
				}
				if (Subset) {
					WK->DeleteAtNr(CFile, BaseRec);
					WK->AddToRecNr(CFile, J + 1, -1);
				}
				CFile->DelAllDifTFlds(CRecPtr, nullptr);
			}
			else {
				if (Subset) BaseRec++;
			label2:
				J++;
				CFile->WriteRec(J, CRecPtr);
			}
		}
		CFile->DecNRecs(CFile->FF->NRecs - J);
	}
	else if (CleanUp()) {
		E->EdUpdated = true;
		if (Subset) {
			WK->DeleteAtNr(CFile, CRec());
			WK->AddToRecNr(CFile, N, -1);
		}
		CFile->DeleteRec(N, CRecPtr);
	}
	CFld = E->FirstFld;
	IRec = (BYTE)oIRec;
	BaseRec = oBaseRec;
	CFile->ClearDeletedFlag(CRecPtr);
	AdjustCRec();
	if (IsNewRec) { DuplOwnerKey(); }
	else { RdRec(CRec()); }
	DisplWwRecsOrPage(&CPage, &RT);
	UnLockWithDep(OldMd);
	result = true;
	return result;
}

ChkD* CompChk(EFldD* D, char Typ)
{
	bool w = WarnSwitch && (Typ == 'W' || Typ == '?');
	bool f = (Typ == 'F' || Typ == '?');
	ChkD* C = D->Chk;
	ChkD* result = nullptr;
	while (C != nullptr) {
		if ((w && C->Warning || f && !C->Warning) && !RunBool(CFile, C->Bool, CRecPtr)) {
			result = C;
			return result;
		}
		C = C->pChain;
	}
	return result;
}

void FindExistTest(FrmlElem* Z, LinkD** LD)
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

bool ForNavigate(FileD* FD)
{
	auto result = true;
	if (UserCode == 0) return result;
	StringListEl* S = FD->ViewNames;
	while (S != nullptr) {
		if (TestAccRight(S)) return result;
		S = S->pChain;
	}
	result = false;
	return result;
}

std::string GetFileViewName(FileD* FD, StringListEl** SL)
{
	if (*SL == nullptr) { return FD->Name; }
	std::string result = "\x1"; // ^A
	while (!TestAccRight(*SL)) *SL = (*SL)->pChain;
	result += (*SL)->S;
	do { *SL = (*SL)->pChain; } while (!(SL == nullptr || TestAccRight(*SL)));
	return result;
}

void SetPointTo(LinkD* LD, std::string* s1, std::string* s2)
{
	//KeyFldD* KF = LD->Args;
	//while (KF != nullptr) {
	for (auto& arg : LD->Args) {
		if (arg->FldD == CFld->FldD) {
			s2 = s1;
			ss.Pointto = s2;
		}
		//KF = KF->pChain;
	}
}

void GetSel2S(std::string& s, std::string& s2, char C, WORD wh)
{
	wwmix ww;

	s = ww.GetSelect();
	s2 = "";
	size_t i = s.find(C);

	if (i != std::string::npos) {
		if (wh == 1) {
			std::string s1 = s.substr(i + 1, 255);
			s2 = s.substr(0, i);
			s = s1;
		}
		else {
			s2 = s.substr(i + 1, 255);
			s = s.substr(0, i);
		}
	}
}

bool EquRoleName(pstring S, LinkD* LD)
{
	if (S == "") {
		return LD->ToFD->Name == LD->RoleName;
	}
	else {
		return S == LD->RoleName;
	}
}

bool EquFileViewName(FileD* FD, std::string S, EditOpt** EO)
{
	auto result = true;
	FileD* cf = CFile;
	CFile = FD;
	if (S[0] == 0x01) { // ^A
		S = S.substr(1, 255);
		StringListEl* SL = CFile->ViewNames;
		while (SL != nullptr) {
			if (SL->S == S) {
				*EO = new EditOpt();
				(*EO)->UserSelFlds = true;
				RdUserView(S, *EO);

				CFile = cf;
				return result;
			}
			SL = SL->pChain;
		}
	}
	else if (S == std::string(CFile->Name)) {
		*EO = new EditOpt();
		(*EO)->UserSelFlds = true;
		(*EO)->Flds = AllFldsList(CFile, false);
		return result;
	}

	CFile = cf;

	result = false;
	return result;
}

void UpwEdit(LinkD* LkD)
{
	wwmix ww;

	void* p = nullptr;
	std::string s1, s2; XString x; XString* px = nullptr;
	FieldDescr* F = nullptr; KeyFldD* KF = nullptr;
	XKey* K = nullptr; EditOpt* EO = nullptr;
	WORD Brk;
	StringListEl* SL, * SL1;
	LinkD* LD;
	MarkStore(p);
	int w = PushW(1, 1, TxtCols, TxtRows, true, true);
	CFile->IRec = AbsRecNr(CRec());
	WrEStatus();

	if (LkD == nullptr) {
		for (auto& ld : LinkDRoot) {
			FileD* ToFD = ld->ToFD;
			if ((ld->FromFD == CFile) && ForNavigate(ToFD)) {
				std::string s;
				std::string rn = ld->RoleName;
				if (ToFD->Name != rn) { s = "." + ld->RoleName; }
				SL = ToFD->ViewNames;
				do {
					s1 = GetFileViewName(ToFD, &SL) + s;
					ww.PutSelect(s1);
					SetPointTo(ld, &s1, &s2);
				} while (SL != nullptr);
			}
		}
		ss.Abcd = true;
		ww.SelectStr(0, 0, 35, "");
		if (Event.Pressed.KeyCombination() == __ESC) goto label1;
		GetSel2S(s1, s2, '.', 2);
		//LD = LinkDRoot;
		/*while (LD != nullptr && !(LD->FromFD == CFile && EquRoleName(s2, LD) && EquFileViewName(LD->ToFD, s1, EO)))
			LD = LD->pChain;*/
		LD = nullptr;
		for (auto& ld : LinkDRoot) {
			if (ld->FromFD == CFile && EquRoleName(s2, ld) && EquFileViewName(ld->ToFD, s1, &EO)) {
				LD = ld;
				break;
			}
		}
	}
	else {
		LD = LkD;
		EO = new EditOpt(); // GetEditOpt();
		EO->UserSelFlds = false;
		CFile = LD->ToFD;
		SL = CFile->ViewNames;
		SL1 = nullptr;
		while (SL != nullptr) {
			if (TestAccRight(SL)) {
				SL1 = SL;
			}
			SL = SL->pChain;
		}
		if (SL1 == nullptr) {
			EO->Flds = AllFldsList(CFile, false);
		}
		else {
			RdUserView(SL1->S, EO);
		}
		EO->SetOnlyView = true;
	}
	CFile = E->FD;
	x.PackKF(CFile, LD->Args, CRecPtr);
	px = &x;
	K = LD->ToKey;
	CFile = LD->ToFD;

	if (EO->ViewKey == nullptr) {
		EO->ViewKey = K;
	}
	else if (&EO->ViewKey != &K) {
		px = nullptr;
	}

	if (SelFldsForEO(EO, nullptr)) {
		NewEditD(CFile, EO);
		E->ShiftF7LD = LkD;
		if (OpenEditWw()) {
			RunEdit(px, Brk);
		}
		SaveAndCloseAllFiles();
		PopEdit();
	}
label1:
	PopW(w);
	ReleaseStore(&p);
	RdEStatus();
	DisplEditWw();
}

void DisplChkErr(ChkD* C)
{
	LinkD* LD = nullptr;

	FindExistTest(C->Bool, &LD);
	if (!C->Warning && (LD != nullptr) && ForNavigate(LD->ToFD) && CFld->Ed(IsNewRec)) {
		FileD* cf = CFile;
		void* cr = CRecPtr;
		int n = 0;

		BYTE* rec = nullptr;
		bool b = LinkUpw(CFile, LD, n, false, CRecPtr, &rec);
		delete[] rec; rec = nullptr;
		CFile = cf; CRecPtr = cr;

		if (!b) {
			if (NoShiftF7Msg) {
				UpwEdit(LD);
				return;
			}
			else {
				F10SpecKey = __SHIFT_F7;
			}
		}
	}
	if (!C->HelpName.empty()) {
		if (F10SpecKey == __SHIFT_F7) F10SpecKey = 0xfffe;
		else F10SpecKey = __F1;
	}
	SetMsgPar(RunShortStr(CFile, C->TxtZ, CRecPtr));
	WrLLF10Msg(110);
	if (Event.Pressed.KeyCombination() == __F1) {
		Help(CFile->ChptPos.R, C->HelpName, false);
	}
	else if (Event.Pressed.KeyCombination() == __SHIFT_F7) {
		UpwEdit(LD);
	}
}

bool OldRecDiffers()
{
	XString x; FieldDescr* f = nullptr;
	auto result = false;
	if (IsCurrChpt() || (
#ifdef FandSQL
		!CFile->IsSQLFile &&
#endif 
		(!CFile->FF->NotCached()))) return result;
	CRecPtr = CFile->GetRecSpace();
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		x.S = WK->NrToStr(CRec); Strm1->KeyAcc(WK, @x); f = CFile->FldD;
		while (f != nullptr) {
			/* !!! with f^ do!!! */ if (Flg && f_Stored != 0) && (field_type != 'T') and
				(CompArea(Pchar(CRecPtr) + Displ, Pchar(E->OldRecPtr) + Displ, NBytes) != ord(_equ)) then
				goto label1;
			f = f->pChain;
		}
		goto label2;
	}
	else
#endif

		CFile->ReadRec(E->LockedRec, CRecPtr);
	if (CompArea(CRecPtr, E->OldRecPtr, CFile->FF->RecLen) != _equ) {
	label1:
		CFile->DelAllDifTFlds(E->NewRecPtr, E->OldRecPtr);
		Move(CRecPtr, E->NewRecPtr, CFile->FF->RecLen);
		WasUpdated = false;
		result = true;
	}
label2:
	CFile->ClearRecSpace(CRecPtr);
	ReleaseStore(&CRecPtr);
	CRecPtr = E->NewRecPtr;
	return result;
}

bool ExitCheck(bool MayDispl)
{
	auto result = false;
	for (auto& X : E->ExD) {
		if (X->AtWrRec) {
			EdBreak = 16;
			bool ok = EdOk;
			EdOk = true; LastTxtPos = -1;
			if (StartExit(X, MayDispl) && EdOk) {
				EdOk = ok;
			}
			else {
				EdOk = ok;
				return result;
			}
		}
	}
	result = true;
	return result;
}

int UpdateIndexes()
{
	int N = 0;
	XString x;
	int NNew = E->LockedRec;
	XWKey* KSel = E->SelKey;

	if (IsNewRec) {
		NNew = CFile->FF->NRecs + 1;
		CFile->FF->XF->NRecs++;
	}
	else if (KSel != nullptr) {
		CRecPtr = E->OldRecPtr;
		if (KSel->RecNrToPath(CFile, x, NNew, CRecPtr)) {
			KSel->DeleteOnPath(CFile);
			CRecPtr = E->NewRecPtr;
			KSel->Insert(CFile, NNew, false, CRecPtr);
		}
		CRecPtr = E->NewRecPtr;
	}

	if (VK->RecNrToPath(CFile, x, E->LockedRec, CRecPtr) && !WasWK) {
		if (IsNewRec) {
			VK->InsertOnPath(CFile, x, NNew);
			if (Subset) {
				WK->InsertAtNr(CFile, CRec(), NNew, CRecPtr);
			}
		}
		N = CRec();
	}
	else {
		if (!IsNewRec) {
			CRecPtr = E->OldRecPtr;
			VK->Delete(CFile, E->LockedRec, CRecPtr);
			if (Subset) {
				WK->DeleteAtNr(CFile, CRec());
			}
			CRecPtr = E->NewRecPtr;
			x.PackKF(CFile, VK->KFlds, CRecPtr);
			VK->Search(CFile, x, true, N);
		}
		N = VK->PathToNr(CFile);
		VK->InsertOnPath(CFile, x, NNew);
		if (VK->InWork) {
			VK->NR++;
		}
		if (Subset) {
			N = WK->InsertGetNr(CFile, NNew, CRecPtr);
		}
	}

	WORD result = N;
	for (size_t i = 0; i < CFile->Keys.size(); i++) {
		auto K = CFile->Keys[i];
		if (K != VK) {
			if (!IsNewRec) {
				CRecPtr = E->OldRecPtr;
				K->Delete(CFile, E->LockedRec, CRecPtr);
			}
			CRecPtr = E->NewRecPtr;
			K->Insert(CFile, NNew, true, CRecPtr);
		}
	}
	CRecPtr = E->NewRecPtr;
	return result;
}


bool WriteCRec(bool MayDispl, bool& Displ)
{
	int N = 0, CNew = 0;
	ImplD* ID = nullptr;
	double time = 0.0;
	LongStr* s = nullptr;
	EFldD* D = nullptr;
	ChkD* C = nullptr;
	LockMode OldMd = LockMode::NullMode;
	Displ = false;
	bool result = false;

	if (!WasUpdated || !IsNewRec && EquOldNewRec()) {
		IsNewRec = false; WasUpdated = false; result = true;
		UnLockRec(E);
		return result;
	}
	result = false;
	if (IsNewRec) {
		ID = E->Impl;
		while (ID != nullptr) {
			AssgnFrml(CFile, CRecPtr, ID->FldD, ID->Frml, true, false);
			ID = ID->pChain;
		}
	}
	if (MustCheck) {   /* repeat field checking */
		D = E->FirstFld;
		while (D != nullptr) {
			C = CompChk(D, 'F');
			if (C != nullptr) {
				if (MayDispl) GotoRecFld(CRec(), D);
				else CFld = D;
				DisplChkErr(C);
				return result;
			}
			D = (EFldD*)D->pChain;
		}
	}
	if (IsNewRec) {
		if (!LockWithDep(CrMode, NullMode, OldMd)) return result;
	}
	else if (!EdRecVar) {
		if (!LockWithDep(WrMode, WrMode, OldMd)) return result;
		if (OldRecDiffers()) {
			UnLockRec(E);
			UnLockWithDep(OldMd);
			WrLLF10Msg(149);
			DisplRec(CRec());
			IVon();
			return result;
		}
	}
	if (Subset && !(NoCondCheck || RunBool(CFile, E->Cond, CRecPtr) && CheckKeyIn(E))) {
		UnLockWithDep(OldMd);
		WrLLF10Msg(823);
		return result;
	}
	if (E->DownSet) {
		DuplOwnerKey();
		Displ = true;
	}
	if (!ExitCheck(MayDispl)) goto label1;
	if (EdRecVar) goto label2;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		if (UpdSQLFile) goto label2; else goto label1;
	}
#endif
	if (HasIndex) {   /* test duplicate keys */
		//K = CFile->Keys;
		//while (K != nullptr) {
		for (auto& K : CFile->Keys) {
			if (!K->Duplic && TestDuplKey(CFile, K)) {
				UnLockWithDep(OldMd);
				DuplKeyMsg(K);
				return result;
			}
			//K = K->Chain;
		}
	}
	CFile->ClearDeletedFlag(CRecPtr);
	if (HasIndex) {
		CFile->FF->TestXFExist();
		if (IsNewRec) {
			if (AddSwitch && !RunAddUpdate(CFile, '+', nullptr, false, nullptr, nullptr, CRecPtr)) goto label1;
			CNew = UpdateIndexes();
			CFile->CreateRec(CFile->FF->NRecs + 1, CRecPtr);
		}
		else {
			if (AddSwitch) {
				if (!RunAddUpdate(CFile, 'd', E->OldRecPtr, false, nullptr, nullptr, CRecPtr)) goto label1;
				UpdMemberRef(E->OldRecPtr, CRecPtr);
			}
			CNew = UpdateIndexes();
			CFile->WriteRec(E->LockedRec, CRecPtr);
		}
		if (CNew != CRec()) {
			SetNewCRec(CNew, true);
			if (E->NRecs > 1) Displ = true;
		}
	}
	else if (IsNewRec) {
		N = E->LockedRec;
		if (N == 0) {
			N = CRec();
			if (N == CNRecs()) N = CFile->FF->NRecs + 1;
			else if (Subset) N = WK->NrToRecNr(CFile, N);
		}
		if (AddSwitch && !RunAddUpdate(CFile, '+', nullptr, false, nullptr, nullptr, CRecPtr)) goto label1;
		if (ChptWriteCRec() != 0) goto label1;
		CFile->CreateRec(N, CRecPtr);
		if (Subset) {
			WK->AddToRecNr(CFile, N, 1);
			WK->InsertAtNr(CFile, CRec(), N, CRecPtr);
		}
	}
	else {
		if (AddSwitch) {
			if (!RunAddUpdate(CFile, 'd', E->OldRecPtr, false, nullptr, nullptr, CRecPtr)) goto label1;
			UpdMemberRef(E->OldRecPtr, CRecPtr);
		}
		WORD chptWrite = ChptWriteCRec();
		switch (chptWrite) {
		case 1: {
			goto label1;
			break;
		}
		case 2: {
			// are old and new text positions same?
			if ((*(int*)((char*)E->OldRecPtr + ChptTxt->Displ) == *(int*)((char*)CRecPtr + ChptTxt->Displ)) && PromptYN(157)) {
				s = CFile->loadLongS(ChptTxt, CRecPtr);
				TWork.Delete(ClpBdPos);
				ClpBdPos = TWork.Store(s->A, s->LL);
				delete s; s = nullptr;
			}
			UndoRecord();
			goto label1;
		}
		}
		CFile->WriteRec(E->LockedRec, CRecPtr);
	}
	time = Today() + CurrTime();
	if (IsNewRec) WrJournal('+', CRecPtr, time);
	else {
		WrJournal('O', E->OldRecPtr, time);
		WrJournal('N', CRecPtr, time);
	}
label2:
	if (!IsNewRec && !NoDelTFlds) {
		CFile->DelAllDifTFlds(E->OldRecPtr, E->NewRecPtr);
	}
	E->EdUpdated = true;
	NoDelTFlds = false;
	IsNewRec = false;
	WasUpdated = false;
	result = true;
	UnLockRec(E);
label1:
	UnLockWithDep(OldMd);
	return result;
}

void DuplFromPrevRec()
{
	if (CFld->Ed(IsNewRec)) {
		FieldDescr* F = CFld->FldD;
		LockMode md = RdMode;
		if (F->field_type == FieldType::TEXT) md = WrMode;
		md = CFile->NewLockMode(md);
		SetWasUpdated(CFile->FF, CRecPtr);
		void* cr = CRecPtr;
		CRecPtr = CFile->GetRecSpace();
		RdRec(CRec() - 1);
		DuplFld(CFile, CFile, CRecPtr, E->NewRecPtr, E->OldRecPtr, F, F);
		CFile->ClearRecSpace(CRecPtr);
		ReleaseStore(&CRecPtr);
		CRecPtr = cr;
		CFile->OldLockMode(md);
	}
}

void InsertRecProc(void* RP)
{
	GotoRecFld(CRec(), E->FirstFld);
	IsNewRec = true;
	LockRec(false);
	if (RP != nullptr) {
		Move(RP, CRecPtr, CFile->FF->RecLen);
	}
	else {
		CFile->ZeroAllFlds(CRecPtr, false);
	}
	DuplOwnerKey();
	SetWasUpdated(CFile->FF, CRecPtr);
	IVoff();
	MoveDispl(E->NRecs - 1, E->NRecs, E->NRecs - IRec);
	FirstEmptyFld = CFld;
	DisplRec(IRec);
	IVon();
	NewDisplLL = true;
	NewRecExit();
}

void AppendRecord(void* RP)
{
	WORD Max;
	IVoff();
	IsNewRec = true;
	Max = E->NRecs;
	CFld = E->FirstFld;
	FirstEmptyFld = CFld;
	if (IRec < Max) {
		IRec++;
		MoveDispl(Max - 1, Max, Max - IRec);
		DisplRec(IRec);
		IVon();
	}
	else if (Max == 1) {
		BaseRec++;
		DisplWwRecsOrPage(&CPage, &RT);
	}
	else {
		BaseRec += Max - 1;
		IRec = 2;
		DisplAllWwRecs();
	}
	if (RP != nullptr) {
		Move(RP, CRecPtr, CFile->FF->RecLen);
	}
	else {
		CFile->ZeroAllFlds(CRecPtr, false);
	}
	DuplOwnerKey();
	DisplRecNr(CRec());
	SetWasUpdated(CFile->FF, CRecPtr);
	LockRec(false);
	NewRecExit();
}

bool GotoXRec(XString* PX, int& N)
{
	bool result = false;
	LockMode md = CFile->NewLockMode(RdMode);
	XKey* k = VK;
	if (Subset) k = WK;
	if (Subset || HasIndex) {
		result = k->SearchInterval(CFile, *PX, false, N);
		N = k->PathToNr(CFile);
	}
	else {
		result = CFile->SearchKey(*PX, k, N, CRecPtr);
	}
	RdRec(CRec());
	GotoRecFld(N, CFld);
	CFile->OldLockMode(md);
	return result;
}

EFldD* FindEFld(FieldDescr* F)
{

	EFldD* D = E->FirstFld;
	while (D != nullptr) {
		if (D->FldD == F) {
			break;
		}
		D = D->pChain;
	}
	return D;
}

void CreateOrErr(bool create, void* RP, int N)
{
	if (create) {
		if (N > CNRecs()) {
			AppendRecord(RP);
		}
		else {
			InsertRecProc(RP);
		}
	}
	else if (!NoSrchMsg) {
		WrLLF10Msg(118);
	}
}

bool PromptSearch(bool create)
{
	auto result = false;
	FieldDescr* F = nullptr;
	int n = 0;
	std::string s;
	double r = 0.0;
	bool b = false;
	bool found = false;
	LockMode md;
	XString x;
	WORD Col = 0, LWw = 0, pos = 0;
	FileD* FD = CFile;
	XKey* K = VK;
	if (Subset) K = WK;
	KeyFldD* KF = K->KFlds;
	void* RP = CFile->GetRecSpace();
	CRecPtr = RP;
	CFile->ZeroAllFlds(CRecPtr, false);
	x.Clear();
	bool li = F3LeadIn && !IsNewRec;
	int w = PushW(1, TxtRows, TxtCols, TxtRows, true, false);
	if (KF == nullptr) {
		result = true;
		CRecPtr = E->NewRecPtr;
		PopW(w);
		ReleaseStore(&RP);
		return result;
	}
	if (HasIndex && E->DownSet && (VK == E->DownKey)) {
		FileD* FD2 = E->DownLD->ToFD;
		void* RP2 = E->DownRecPtr;
		KeyFldD* KF2 = E->DownLD->ToKey->KFlds;
		CFile = FD2;
		CRecPtr = RP2;
		while (KF2 != nullptr) {
			CFile = FD2;
			CRecPtr = RP2;
			F = KF->FldD;
			FieldDescr* F2 = KF2->FldD;
			switch (F->frml_type) {
			case 'S': {
				s = CFile->loadOldS(F2, CRecPtr);
				x.StoreStr(s, KF);
				CFile = FD;
				CRecPtr = RP;
				CFile->saveS(F, s, CRecPtr);
				break;
			}
			case 'R': {
				r = CFile->loadR(F2, CRecPtr);
				x.StoreReal(r, KF);
				CFile = FD;
				CRecPtr = RP;
				CFile->saveR(F, r, CRecPtr);
				break;
			}
			case 'B': {
				b = CFile->loadB(F2, CRecPtr);
				x.StoreBool(b, KF);
				CFile = FD;
				CRecPtr = RP;
				CFile->saveB(F, b, CRecPtr);
				break;
			}
			}
			KF2 = KF2->pChain;
			KF = KF->pChain;
		}
	}
	if (KF == nullptr) {
		result = true;
		CRecPtr = E->NewRecPtr;
		PopW(w);
		ReleaseStore(&RP);
		return result;
	}
	while (KF != nullptr) {
		F = KF->FldD;
		if (li) {
			EFldD* D = FindEFld(F);
			if (D != nullptr) {
				GotoRecFld(CRec(), D);
			}
		}
		TextAttr = screen.colors.pTxt;
		screen.ScrWrStr(1, TxtRows, F->Name + ":", TextAttr);
		screen.GotoXY((WORD)F->Name.length() + 2, TxtRows);
		ClrEol();
		s = "";
		pos = 1;
		Col = screen.WhereX();
		if (Col + F->L > TxtCols) {
			LWw = TxtCols - Col;
		}
		else {
			LWw = F->L;
		}
		while (true) {
			TextAttr = screen.colors.pNorm;
			screen.GotoXY(Col, TxtRows);
			pos = FieldEdit(F, nullptr, LWw, pos, s, r, false, true, li, E->WatchDelay);
			const XString x_old = x;
			if (Event.Pressed.KeyCombination() == __ESC || (Event.What == evKeyDown)) {
				CRecPtr = E->NewRecPtr;
				PopW(w);
				ReleaseStore(&RP);
				return result;
			}
			switch (F->frml_type) {
			case 'S': {
				x.StoreStr(s, KF);
				CFile->saveS(F, s, CRecPtr);
				break;
			}
			case 'R': {
				x.StoreReal(r, KF);
				CFile->saveR(F, r, CRecPtr);
				break;
			}
			case 'B': {
				b = s[0] = AbbrYes;
				x.StoreBool(b, KF);
				CFile->saveB(F, b, CRecPtr);
				break;
			}
			}
			if (li) {
				CRecPtr = E->NewRecPtr;
				found = GotoXRec(&x, n);
				if ((pos == 0) && (F->frml_type == 'S')) {
					x = x_old;
					x.StoreStr(CFile->loadOldS(F, CRecPtr), KF);
				}
				CRecPtr = RP;
				if (pos != 0) {
					x = x_old;
					continue;
				}
			}
			break;
		}
		KF = KF->pChain;
	}
	CRecPtr = E->NewRecPtr;
	if (li) {
		if (!found) CreateOrErr(create, RP, n);
	}
	else if (IsNewRec) {
		Move(RP, CRecPtr, CFile->FF->RecLen);
	}
	else if (!GotoXRec(&x, n)) {
		CreateOrErr(create, RP, n);
	}
	result = true;

	PopW(w);
	ReleaseStore(&RP);
	return result;
}

bool PromptAndSearch(bool create)
{
	bool result = false;
	if (VK == nullptr) {
		WrLLF10Msg(111);
		return result;
	}
	result = PromptSearch(create);
	GotoRecFld(CRec(), E->FirstFld);
	return result;
}

void PromptGotoRecNr()
{
	wwmix ww;

	int n;
	WORD i = 1;
	std::string text;
	bool del = true;
	do {
		ww.PromptLL(122, text, i, del);
		if (Event.Pressed.KeyCombination() == __ESC) return;
		val(text, n, i);
		del = false;
	} while (i != 0);

	GotoRecFld(n, CFld);
}

void CheckFromHere()
{
	EFldD* D = CFld;
	int N = CRec();
	LockMode md = CFile->NewLockMode(RdMode);

	while (true) {
		if (!CFile->DeletedFlag(CRecPtr))
			while (D != nullptr) {
				ChkD* C = CompChk(D, '?');
				if (C != nullptr) {
					if (BaseRec + E->NRecs - 1 < N) {
						BaseRec = N;
					}
					IRec = N - BaseRec + 1;
					CFld = D;
					DisplWwRecsOrPage(&CPage, &RT);
					CFile->OldLockMode(md);
					DisplChkErr(C);
					return;
				}
				D = D->pChain;
			}
		if (N < CNRecs()) {
			N++;
			DisplRecNr(N);
			RdRec(N);
			D = E->FirstFld;
			continue;
		}
		break;
	}

	RdRec(CRec());
	DisplRecNr(CRec());
	CFile->OldLockMode(md);
	WrLLF10Msg(120);
}

void Sorting()
{
	KeyFldD* SKRoot = nullptr;
	void* p = nullptr;
	LockMode md;
	SaveAndCloseAllFiles();
	MarkStore(p);

	if (!PromptSortKeys(E->Flds, SKRoot) || (SKRoot == nullptr)) {
		ReleaseStore(&p);
		CRecPtr = E->NewRecPtr;
		DisplAllWwRecs();
		return;
	}

	if (!CFile->TryLockMode(ExclMode, md, 1)) {
		ReleaseStore(&p);
		CRecPtr = E->NewRecPtr;
		DisplAllWwRecs();
		return;
	}

	try {
		CFile->FF->SortAndSubst(SKRoot);
		E->EdUpdated = true;
	}
	catch (std::exception&) {
		CFile = E->FD;
		CFile->OldLockMode(md);
	}

	ReleaseStore(&p);
	CRecPtr = E->NewRecPtr;
	DisplAllWwRecs();
}

void AutoReport()
{
	void* p = nullptr; RprtOpt* RO = nullptr;
	FileUseMode UM = Closed;
	MarkStore(p); RO = GetRprtOpt();
	RO->FDL.FD = CFile;
	RO->Flds = E->Flds;
	if (Select) {
		RO->FDL.Cond = E->Bool;
		RO->CondTxt = E->BoolTxt;
	}
	if (Subset) {
		RO->FDL.ViewKey = WK;
	}
	else if (HasIndex) {
		RO->FDL.ViewKey = VK;
	}
	PrintView = false;
	const std::unique_ptr auto_report = std::make_unique<ReportGenerator>();
	if (auto_report->SelForAutoRprt(RO)) {
		SpecFDNameAllowed = IsCurrChpt();
		auto_report->RunAutoReport(RO);
		SpecFDNameAllowed = false;
	}
	ReleaseStore(&p);
	ViewPrinterTxt();
	CRecPtr = E->NewRecPtr;
}

void AutoGraph()
{
#ifdef FandGraph
	FrmlElem* Bool = nullptr;
	if (Select) Bool = E->Bool;
	XKey* K = nullptr;
	if (Subset) K = WK;
	else if (HasIndex) K = VK;
	RunAutoGraph(E->Flds, K, Bool);
#endif
	CFile = E->FD;
	CRecPtr = E->NewRecPtr;
}

bool IsDependItem()
{
	if (!IsNewRec && (E->NEdSet == 0)) return false;
	//DepD* Dp = CFld->Dep;
	//while (Dp != nullptr) {
	//	if (RunBool(Dp->Bool)) {
	//		return true;
	//	}
	//	Dp = Dp->pChain;
	//}

	for (const DepD* dep : CFld->Dep) {
		if (RunBool(CFile, dep->Bool, CRecPtr)) {
			return true;
		}
	}

	return false;
}

void SetDependItem()
{
	//DepD* Dp = CFld->Dep;
	//while (Dp != nullptr) {
	//	if (RunBool(Dp->Bool)) {
	//		AssignFld(CFld->FldD, Dp->Frml);
	//		return;
	//	}
	//	Dp = Dp->pChain;
	//}

	for (const DepD* dep : CFld->Dep) {
		if (RunBool(CFile, dep->Bool, CRecPtr)) {
			AssignFld(CFld->FldD, dep->Frml);
			return;
		}
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
	for (auto& X : E->ExD) {
		bool b = FieldInList(CFld->FldD, X->Flds);
		if (X->NegFlds) b = !b;
		if (b) {
			if (X->Typ == 'Q') Quit = true;
			else {
				EdBreak = 12;
				LastTxtPos = -1;
				if (!StartExit(X, true)) {
					return result;
				}
			}
		}
	}
	result = true;
	return result;
}

bool FldInModeF3Key(FieldDescr* F)
{
	auto result = false;
	if ((F->Flg & f_Stored) == 0) return result;
	KeyFldD* KF = VK->KFlds;
	while (KF != nullptr) {
		if (KF->FldD == F) {
			result = true;
			return result;
		}
		KF = KF->pChain;
	}
	return result;
}

bool IsSkipFld(EFldD* D)
{
	return !D->Tab &&
		(E->NTabsSet > 0 || (D->FldD->Flg & f_Stored) == 0 || OnlySearch && FldInModeF3Key(D->FldD));
}

bool ExNotSkipFld()
{
	auto result = false;
	if (E->NFlds == 1) return result;
	EFldD* D = E->FirstFld;
	while (D != nullptr) {
		if ((D != CFld) && !IsSkipFld(D)) {
			result = true;
			return result;
		}
		D = D->pChain;
	}
	return result;
}

bool CtrlMProc(WORD Mode)
{
	int i = 0;
	bool b = false;
	ChkD* C = nullptr;
	EdExitD* X = nullptr;
	WORD Brk = 0, NR = 0;
	KeyListEl* KL = nullptr;
	bool displ = false, skip = false, Quit = false, WasNewRec = false;
	LockMode md;
	char Typ = '\0';

	int OldCRec = CRec();
	EFldD* OldCFld = CFld;

	auto result = true;
	if (Mode == 0 /*only bypass unrelevant fields*/) goto label2;
label1:
	if (IsFirstEmptyFld()) FirstEmptyFld = (EFldD*)FirstEmptyFld->pChain;
	Quit = false;
	if (!CheckForExit(Quit)) return result;
	TextAttr = E->dHiLi;
	DisplFld(CFld, IRec, TextAttr);
	if (ChkSwitch) {
		if (Mode == 1 || Mode == 3) Typ = '?';
		else Typ = 'F';
		C = CompChk(CFld, Typ);
		if (C != nullptr) {
			DisplChkErr(C);
			if (!C->Warning) return result;
		}
	}
	if (WasUpdated && !EdRecVar && HasIndex) {
		KL = CFld->KL;
		while (KL != nullptr) {
			md = CFile->NewLockMode(RdMode);
			b = TestDuplKey(CFile, KL->Key);
			CFile->OldLockMode(md);
			if (b) {
				DuplKeyMsg(KL->Key);
				return result;
			}
			KL = KL->pChain;
		}
	}
	if (Quit && !IsNewRec && (Mode == 1 || Mode == 3)) {
		EdBreak = 12;
		result = false;
		return result;
	}
	if (CFld->pChain != nullptr) {
		GotoRecFld(CRec(), CFld->pChain);
		if (Mode == 1 || Mode == 3) Mode = 0;
	}
	else {
		WasNewRec = IsNewRec;
		Mode = 0; NR++;
		if (!WriteCRec(true, displ)) return result;
		if (displ) DisplAllWwRecs();
		else SetRecAttr(IRec);
		if (Only1Record)
			if (NoESCPrompt) {
				EdBreak = 0;
				return false;
			}
			else {
				Append = false;
				goto label3;
			}
		if (OnlySearch) {
			Append = false;
			goto label3;
		}
		if (Append) AppendRecord(nullptr);
		else {
			if (WasNewRec) NewDisplLL = true;
			if (CRec() < CNRecs())
				if (Select) {
					for (i = CRec() + 1; i <= CNRecs(); i++) {
						if (KeyPressed() && (ReadKey() != _M_) && PromptYN(23)) goto label4;
						RdRec(i);
						DisplRecNr(i);
						if (!CFile->DeletedFlag(CRecPtr) && RunBool(CFile, E->Bool, CRecPtr)) {
							RdRec(CRec());
							GotoRecFld(i, E->FirstFld);
							goto label2;
						}
					}
				label4:
					RdRec(CRec());
					DisplRecNr(CRec());
					GotoRecFld(OldCRec, OldCFld);
					beep(); beep();
					return result;
				}
				else GotoRecFld(CRec() + 1, E->FirstFld);
			else {
			label3:
				GotoRecFld(CRec(), OldCFld);
				beep(); beep();
				return result;
			}
		}
	}
label2:
	skip = false;
	displ = false;
	if (IsFirstEmptyFld()) {
		if ((CFld->Impl != nullptr) && LockRec(true)) {
			AssignFld(CFld->FldD, CFld->Impl);
			displ = true;
		}
		if (CFld->Dupl && (CRec() > 1) && LockRec(true)) {
			DuplFromPrevRec();
			displ = true; skip = true;
		}
	}
	if (IsDependItem() && LockRec(true)) {
		SetDependItem();
		displ = true; skip = true;
	}
	if (IsSkipFld(CFld)) skip = true;
	if (CFld->Tab) skip = false;
	if (displ) {
		TextAttr = E->dHiLi;
		DisplFld(CFld, IRec, TextAttr);
	}
	if (Mode == 2 /*bypass all remaining fields of the record */) goto label1;
	if (skip && ExNotSkipFld() && (NR <= 1)) goto label1;
	return result;
}

bool GoPrevNextRec(short Delta, bool Displ)
{
	int i = 0; LockMode md; WORD w = 0, Max = 0;
	int OldBaseRec = 0;
	auto result = false;
	if (EdRecVar) return result;
	md = CFile->NewLockMode(RdMode);
	i = CRec();
	if (Displ) IVoff();
label0:
	i += Delta;
	if ((i > 0) && (i <= CNRecs())) {
		RdRec(i);
		if (Displ) DisplRecNr(i); // zobrazi cislo zaznamu v hlavicce
		if (!Select || !CFile->DeletedFlag(CRecPtr) && RunBool(CFile, E->Bool, CRecPtr)) goto label2;
		if (KeyPressed()) {
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
	if (Displ) {
		DisplRecNr(CRec());
		IVon();
	}
	goto label4;
label2:
	result = true;
	OldBaseRec = BaseRec;
	SetNewCRec(i, false);
	if (Displ) {
		Max = E->NRecs;
		int D = BaseRec - OldBaseRec;
		if (abs(D) > 0) {
			DisplWwRecsOrPage(&CPage, &RT);
			goto label3;
		}
		if (D > 0) {
			MoveDispl(D + 1, 1, Max - D);
			for (i = Max - D + 1; i <= Max; i++) DisplRec(i);
		}
		else if (D < 0) {
			D = -D;
			MoveDispl(Max - D, Max, Max - D);
			for (i = 1; i <= D; i++) DisplRec(i);
		}
	}
label3:
	if (Displ)IVon();
label4:
	CFile->OldLockMode(md);
	return result;
}

bool GetChpt(pstring Heslo, int& NN)
{
	pstring s(12);

	for (int j = 1; j <= CFile->FF->NRecs; j++) {
		CFile->ReadRec(j, CRecPtr);
		if (IsCurrChpt()) {
			s = OldTrailChar(' ', CFile->loadOldS(ChptName, CRecPtr));
			short i = s.first('.');
			if (i > 0) s.Delete(i, 255);
			if (EquUpCase(Heslo, s)) {
				NN = j;
				return true;
			}
		}
		else {
			s = OldTrailChar(' ', CFile->loadOldS(CFile->FldD.front(), CRecPtr));
			ConvToNoDiakr((WORD*)s[1], s.length(), fonts.VFont);
			if (EqualsMask(&Heslo[1], Heslo.length(), s)) {
				NN = j;
				return true;
			}
		}
	}
	RdRec(CRec());

	return false;
}

void SetCRec(int I)
{
	if (I > BaseRec + E->NRecs - 1) BaseRec = I - E->NRecs + 1;
	else if (I < BaseRec) BaseRec = I;
	IRec = I - BaseRec + 1;
	RdRec(CRec());
}

void UpdateEdTFld(LongStr* S)
{
	LockMode md;
	if (!EdRecVar) md = CFile->NewLockMode(WrMode);
	SetWasUpdated(CFile->FF, E->NewRecPtr);
	CFile->FF->DelDifTFld(CFld->FldD, E->NewRecPtr, E->OldRecPtr);
	CFile->saveLongS(CFld->FldD, S, E->NewRecPtr);
	if (!EdRecVar) {
		CFile->OldLockMode(md);
	}
}

void UpdateTxtPos(WORD TxtPos)
{
	LockMode md;
	if (IsCurrChpt()) {
		md = CFile->NewLockMode(WrMode);
		SetWasUpdated(CFile->FF, CRecPtr);
		CFile->saveR(ChptTxtPos, (short)TxtPos, CRecPtr);
		CFile->OldLockMode(md);
	}
}

bool EditFreeTxt(FieldDescr* F, std::string ErrMsg, bool Ed, WORD& Brk)
{
	std::vector<WORD> BreakKeys;
#ifndef FandRunV
	BreakKeys.push_back(__CTRL_F1);
#endif
	BreakKeys.push_back(__F1);
	BreakKeys.push_back(__F9);
	BreakKeys.push_back(__CTRL_HOME);
	BreakKeys.push_back(__CTRL_END);

	std::vector<WORD> BreakKeys1 = { __CTRL_F1, __F1, __CTRL_HOME, __CTRL_END, __F9, __ALT_F10, __SHIFT_F1, __F10 };
	std::vector<WORD> BreakKeys2 = { __F1, __CTRL_HOME, __CTRL_END, __F9, __F10, __ALT_F10,
									__CTRL_F1, __ALT_F1, __SHIFT_F1, __ALT_F2, __ALT_F3, __CTRL_F8, __CTRL_F9, __ALT_F9 };

	std::vector<WORD> Breaks;

	const BYTE maxStk = 10;

	bool Srch = false, Upd = false, WasUpd = false, Displ = false, quit;
	std::string HdTxt;
	MsgStr TxtMsgS;
	MsgStr* PTxtMsgS;
	int TxtXY = 0;
	WORD R1 = 0, OldTxtPos = 0, TxtPos = 0, CtrlMsgNr = 0, C = 0, LastLen = 0;
	LongStr* S = nullptr;
	char Kind = '\0'; LockMode md; void* p = nullptr; int i = 0, w = 0;
	std::vector<EdExitD*> X;
	WORD iStk = 0;
	struct { int N = 0; int I = 0; } Stk[maxStk];
	std::string heslo;

	MarkStore(p);
	Srch = false; Brk = 0; TxtPos = 1; iStk = 0; TxtXY = 0;
	auto result = true;
	w = 0;
	if (E->Head.empty()) w = PushW(1, 1, TxtCols, 1);
	if (E->TTExit) {
		TxtMsgS.Head = nullptr;
		TxtMsgS.Last = E->Last;
		TxtMsgS.CtrlLast = E->CtrlLast;
		TxtMsgS.AltLast = E->AltLast;
		TxtMsgS.ShiftLast = E->ShiftLast;
		PTxtMsgS = &TxtMsgS;
	}
	else {
		PTxtMsgS = nullptr;
	}
label1:
	HdTxt = "    ";
	WasUpd = false;
	if (CRec() > 1) {
		HdTxt[2] = 0x18; // ^X
	}
	if (CRec() < CNRecs()) {
		HdTxt[3] = 0x19; // ^Y
	}
	if (IsCurrChpt()) {
		HdTxt = CFile->loadS(ChptTyp, CRecPtr) + ':' + CFile->loadS(ChptName, CRecPtr) + HdTxt;
		TxtPos = trunc(CFile->loadR(ChptTxtPos, CRecPtr));
		Breaks = BreakKeys2;
		CtrlMsgNr = 131;
	}
	else {
		CtrlMsgNr = 151;
		if (CFile == CRdb->HelpFD) {
			Breaks = BreakKeys1;
		}
		else {
			Breaks = BreakKeys;
		}
	}
	R1 = E->FrstRow;
	if ((R1 == 3) && WithBoolDispl) R1 = 2;
	screen.Window(E->FrstCol, R1, E->LastCol, E->LastRow);
	TextAttr = screen.colors.tNorm;
	Kind = 'V';
	OldTxtPos = TxtPos;
	if (Ed) LockRec(false);
	if ((F->Flg & f_Stored) != 0) {
		S = CFile->loadLongS(F, CRecPtr);
		if (Ed) Kind = 'T';
	}
	else {
		S = RunLongStr(CFile, F->Frml, CRecPtr);
	}
label2:
	if (TTExit) {
		X = E->ExD;
	}
	else {
		X.clear();
	}

	Upd = false;
	result =
		EditText(Kind, MemoT, HdTxt, ErrMsg, S, MaxLStrLen, TxtPos, TxtXY, Breaks, X,
			Srch, Upd, 141, CtrlMsgNr, PTxtMsgS);
	ErrMsg = "";
	heslo = LexWord;
	LastLen = S->LL;
	if (EdBreak == 0xffff) {
		C = Event.Pressed.KeyCombination();
	}
	else {
		C = 0;
	}

	if (C == _AltEqual_) {
		C = _ESC_;
	}
	else {
		WasUpd = WasUpd || Upd;
	}

	switch (C) {
	case __ALT_F3: {
		EditHelpOrCat(C, 0, "");
		goto label2;
		break;
	}
	case _U_: {
		delete S; S = nullptr;
		TxtXY = 0;
		goto label1;
		break; }
	}
	screen.Window(1, 1, TxtCols, TxtRows);

	if (WasUpd) UpdateEdTFld(S);
	if ((OldTxtPos != TxtPos) && !Srch) UpdateTxtPos(TxtPos);
	delete S; S = nullptr;

	if (Ed && !WasUpdated) {
		UnLockRec(E);
	}
	if (Srch) {
		if (WriteCRec(false, Displ)) {
			goto label31;
		}
	}

	switch (C) {
	case __F9: {
		if (WriteCRec(false, Displ)) {
			SaveAndCloseAllFiles();
			UpdCount = 0;
		}
		goto label4;
		break;
	}
	case __F1: {
		ReadMessage(6);
		heslo = MsgLine;
		goto label3;
		break;
	}
	case __CTRL_F1: {
		goto label3;
		break;
	}
	case __SHIFT_F1:
		if (IsCurrChpt() || (CFile == CRdb->HelpFD)) {
			if ((iStk < maxStk) && WriteCRec(false, Displ) && GetChpt(heslo, i)) {
				Append = false;
				iStk++;
				Stk[iStk].N = CRec();
				Stk[iStk].I = TxtPos;
				SetCRec(i);
			}
			TxtXY = 0;
			goto label4;
			break;
		}
	case __F10: {
		if ((iStk > 0) && WriteCRec(false, Displ)) {
			Append = false;
			SetCRec(Stk[iStk].N);
			TxtPos = Stk[iStk].I;
			iStk--;
		}
		TxtXY = 0;
		goto label4;
		break;
	}
	case __ALT_F10: {
		Help(nullptr, "", false);
		goto label4;
		break; }
	case __ALT_F1: {
		heslo = CFile->loadOldS(ChptTyp, CRecPtr);
	label3:
		Help((RdbD*)&HelpFD, heslo, false);
		goto label4;
		break;
	}
	} // switch end

	if ((C > 0xFF) && WriteCRec(false, Displ)) {
		Append = false;
		if (C == __CTRL_HOME) {
			GoPrevNextRec(-1, false);
			TxtXY = 0;
			goto label4;
		}
		if (C == __CTRL_END) {
		label31:
			if (!GoPrevNextRec(+1, false) && Srch) {
				UpdateTxtPos(LastLen);
				Srch = false;
			}
			TxtXY = 0;
		label4:
			if (!Ed || LockRec(false)) {
				goto label1;
			}
			else {
				goto label5;
			}
		}
		WrEStatus();
		Brk = 1;
		Event.Pressed.UpdateKey(C);
		goto label6;

	}
label5:
	ReleaseStore(&p);
	DisplEditWw();
label6:
	if (w != 0) PopW(w);
	return result;
}

bool EditItemProc(bool del, bool ed, WORD& Brk)
{
	std::string Txt;
	double R = 0; bool b = false; ChkD* C = nullptr;
	FieldDescr* F = CFld->FldD;
	auto result = true;
	if (F->field_type == FieldType::TEXT) {
		if (!EditFreeTxt(F, "", ed, Brk)) {
			return false;
		}
	}
	else {
		TextAttr = E->dHiLi;
		Txt = DecodeField(CFile, F, CFld->FldD->L, CRecPtr);
		screen.GotoXY(CFld->Col, FldRow(CFld, IRec));
		unsigned int wd = 0;
		if (CFile->FF->NotCached()) {
			wd = E->WatchDelay;
		}
		FieldEdit(F, CFld->Impl, CFld->L, 1, Txt, R, del, ed, false, wd);
		if (Event.Pressed.KeyCombination() == __ESC || !ed) {
			DisplFld(CFld, IRec, TextAttr);
			if (ed && !WasUpdated) UnLockRec(E);
			return result;
		}
		SetWasUpdated(CFile->FF, CRecPtr);
		switch (F->frml_type) {
		case 'B': CFile->saveB(F, toupper(Txt[0]) == AbbrYes, CRecPtr); break;
		case 'S': CFile->saveS(F, Txt, CRecPtr); break;
		case 'R': CFile->saveR(F, R, CRecPtr); break;
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
	case 1: {
		if (Select) Select = false;
		else if (E->Bool != nullptr) Select = true;
		DisplBool();
		NewDisplLL = true;
		SetNewWwRecAttr();
		break;
	}
	case 2: {
		if (CFld->FldD->Flg && f_Stored != 0) {
			B = CFld->Dupl;
			CFld->Dupl = !B;
			DisplTabDupl();
			if (B) E->NDuplSet--;
			else E->NDuplSet++;
		}
		break;
	}
	case 3: {
		B = CFld->Tab;
		CFld->Tab = !B;
		DisplTabDupl();
		if (B) E->NTabsSet--;
		else E->NTabsSet++;
		break;
	}
	case 4: {
		AddSwitch = !AddSwitch;
		NewDisplLL = true;
		break;
	}
	case 5: {
		if (!MustCheck) {
			ChkSwitch = !ChkSwitch;
			NewDisplLL = true;
		}
		break;
	}
	case 6: {
		WarnSwitch = !WarnSwitch;
		NewDisplLL = true;
		break;
	}
	}
}

void PromptSelect()
{
	wwmix ww;
	std::string Txt;
	if (Select) Txt = E->BoolTxt;
	else Txt = "";
	if (IsCurrChpt()) ReleaseFilesAndLinksAfterChapter();
	ReleaseStore(&E->AfterE);
	ww.PromptFilter(Txt, &E->Bool, &E->BoolTxt);
	if (E->Bool == nullptr) Select = false; else Select = true;
	DisplBool();
	SetNewWwRecAttr();
	NewDisplLL = true;
}

void SwitchRecs(short Delta)
{
	LockMode md; int n1, n2; void* p1; void* p2; XString x1, x2;
#ifdef FandSQL
	if (CFile->IsSQLFile) return;
#endif
	if (NoCreate && NoDelete || WasWK) return;
	if (!CFile->TryLockMode(WrMode, md, 1)) return;
	p1 = CFile->GetRecSpace();
	p2 = CFile->GetRecSpace();
	CRecPtr = p1; n1 = AbsRecNr(CRec());
	CFile->ReadRec(n1, CRecPtr);
	if (HasIndex) x1.PackKF(CFile, VK->KFlds, CRecPtr);
	CRecPtr = p2; n2 = AbsRecNr(CRec() + Delta);
	CFile->ReadRec(n2, CRecPtr);
	if (HasIndex) { x2.PackKF(CFile, VK->KFlds, CRecPtr); if (x1.S != x2.S) goto label1; }
	CFile->WriteRec(n1, CRecPtr);
	CRecPtr = p1;
	CFile->WriteRec(n2, CRecPtr);
	if (HasIndex) {
		for (auto& k : CFile->Keys) {
			if (k != VK) {
				CRecPtr = p1; k->Delete(CFile, n1, CRecPtr);
				CRecPtr = p2; k->Delete(CFile, n2, CRecPtr);
				CRecPtr = p1; k->Insert(CFile, n2, true, CRecPtr);
				CRecPtr = p2; k->Insert(CFile, n1, true, CRecPtr);
			}
		}
	}
	SetNewCRec(CRec() + Delta, true);
	DisplAllWwRecs();
	DisplRecNr(CRec());
	E->EdUpdated = true;
	if (IsCurrChpt()) SetCompileAll();
label1:
	CFile->OldLockMode(md);
	ReleaseStore(&p1);
	CRecPtr = E->NewRecPtr;
}

bool FinArgs(LinkD* LD, FieldDescr* F)
{
	auto result = true;
	//KeyFldD* KF = LD->Args;
	//while (KF != nullptr) {
	for (auto& arg : LD->Args) {
		if (arg->FldD == F) return result;
		//KF = KF->pChain;
	}
	result = false;
	return result;
}

bool SelFldsForEO(EditOpt* EO, LinkD* LD)
{
	// TODO: this method is bad, need to investigate what happens here ...

	wwmix ww;

	void* p = nullptr;
	auto result = true;
	if (EO->Flds.empty()) return result;
	//FieldListEl* FL = EO->Flds;
	if (!EO->UserSelFlds) {
		if (LD != nullptr) {
			//FieldListEl* FL1 = FieldList(EO->Flds);
			//while (FL != nullptr) {
			for (auto& FL : EO->Flds) {
				if (FinArgs(LD, FL)) {
					//FL1->pChain = FL;
					//FL1 = FL;
				}
				//FL = FL->pChain;
			}
			//FL1->pChain = nullptr;
		}
		return result;
	}
	MarkStore(p);
	//while (FL != nullptr) {
	for (auto& F : EO->Flds) {
		if ((LD == nullptr) || !FinArgs(LD, F)) {
			pstring s = F->Name;
			if ((F->Flg & f_Stored) == 0) {
				pstring olds = s;
				s = SelMark;
				s += olds;
			}
			ww.PutSelect(s);
		}
		//FL = FL->pChain;
	}
	if (EO->Flds.empty()) WrLLF10Msg(156);
	else {
		ww.SelFieldList(36, true, EO->Flds);
	}
	if (EO->Flds.empty()) {
		ReleaseStore(&p);
		result = false;
	}
	return result;
}

void ImbeddEdit()
{
	wwmix ww;

	void* p = nullptr;
	std::string s1, s2;
	WORD Brk; StringListEl* SL = nullptr;
	EditOpt* EO = nullptr;
	FileD* FD = nullptr;
	RdbD* R = nullptr; int w = 0;

	MarkStore(p);
	w = PushW(1, 1, TxtCols, TxtRows, true, true);
	CFile->IRec = AbsRecNr(CRec());
	WrEStatus();
	R = CRdb;
	while (R != nullptr) {
		FD = R->FD->pChain;
		while (FD != nullptr) {
			if (ForNavigate(FD)) {
				SL = FD->ViewNames;
				do {
					std::string s = GetFileViewName(FD, &SL);
					if (R != CRdb) {
						s = R->FD->Name + "." + s;
					}
					ww.PutSelect(s);
				} while (SL != nullptr);
			}
			FD = FD->pChain;
		}
		R = R->ChainBack;
	}
	ss.Abcd = true; ww.SelectStr(0, 0, 35, "");
	if (Event.Pressed.KeyCombination() == __ESC) {
		// do nothing
	}
	else {
		GetSel2S(s1, s2, '.', 1);
		R = CRdb;
		if (!s2.empty()) {
			std::string ss2 = s2;
			do {
				R = R->ChainBack;
			} while (R->FD->Name != ss2);
		}
		CFile = R->FD;
		while (!EquFileViewName(CFile, s1, &EO)) {
			CFile = CFile->pChain;
		}
		if (SelFldsForEO(EO, nullptr)) {
			NewEditD(CFile, EO);
			if (OpenEditWw()) {
				RunEdit(nullptr, Brk);
			}
			SaveAndCloseAllFiles();
			PopEdit();
		}
	}

	PopW(w);
	ReleaseStore(&p);
	RdEStatus();
	DisplEditWw();
}

void DownEdit()
{
	wwmix ww;

	EditOpt* EO = nullptr;
	WORD Brk;
	void* p = nullptr;
	std::string s1, s2;
	std::string ali;
	//LinkD* LD = LinkDRoot;
	MarkStore(p);

	int w = PushW(1, 1, TxtCols, TxtRows, true, true);
	CFile->IRec = AbsRecNr(CRec());

	WrEStatus();

	for (auto& ld : LinkDRoot) { //while (LD != nullptr) {
		FileD* FD = ld->FromFD;
		StringListEl* SL;
		if ((ld->ToFD == CFile) && ForNavigate(FD) && (ld->IndexRoot != 0)) {
			/*own key with equal beginning*/
			SL = FD->ViewNames;
			XKey* K = GetFromKey(ld);
			do {
				std::string s = GetFileViewName(FD, &SL);
				std::string kali = K->Alias;
				if (!K->Alias.empty()) {
					s += "/" + kali;
				}
				ww.PutSelect(s);
			} while (SL != nullptr);
		}
		//ld = ld->pChain;
	}
	ss.Abcd = true;
	ww.SelectStr(0, 0, 35, "");

	if (Event.Pressed.KeyCombination() == __ESC) {
		// do nothing;
	}
	else {
		LinkD* LD = *LinkDRoot.begin();
		GetSel2S(s1, s2, '/', 2);
		ali = GetFromKey(LD)->Alias;
		//while ((LD->ToFD != E->FD) || (LD->IndexRoot == 0) || (s2 != ali)
		//	|| !EquFileViewName(LD->FromFD, s1, EO)) LD = LD->pChain;
		for (auto& ld : LinkDRoot) {
			if ((ld->ToFD != E->FD) || (ld->IndexRoot == 0) || (s2 != ali) || !EquFileViewName(ld->FromFD, s1, &EO)) {
				continue;
			}
			else {
				LD = ld;
			}
		}

		CFile = LD->FromFD;
		if (SelFldsForEO(EO, LD)) {
			EO->DownLD = LD;
			EO->DownRecPtr = CRecPtr;
			NewEditD(CFile, EO);
			if (OpenEditWw()) {
				RunEdit(nullptr, Brk);
			}
			SaveAndCloseAllFiles();
			PopEdit();
		}
	}

	PopW(w);
	ReleaseStore(&p);
	RdEStatus();
	DisplEditWw();
}

void ShiftF7Proc()
{
	/* find last (first decl.) foreign key link with CFld as an argument */
	FieldDescr* F = CFld->FldD;
	LinkD* LD1 = nullptr;
	for (auto& ld : LinkDRoot) { //while (LD != nullptr) {
		for (auto& arg : ld->Args) {
			//KeyFldD* KF = ld->Args;
			//while (KF != nullptr) {
			if ((arg->FldD == F) && ForNavigate(ld->ToFD)) LD1 = ld;
			//KF = KF->pChain;
		}
	}
	if (LD1 != nullptr) UpwEdit(LD1);
}

bool ShiftF7Duplicate()
{
	auto result = false;
	EditD* ee = E->pChain;

	CFile = ee->FD;
	CRecPtr = ee->NewRecPtr;
	if (!ELockRec(ee, CFile->IRec, ee->IsNewRec, ee->SubSet)) return result;
	if (!WasUpdated) {
		Move(CRecPtr, ee->OldRecPtr, CFile->FF->RecLen);
		WasUpdated = true;
	}
	//KeyFldD* kf = E->ShiftF7LD->Args;
	KeyFldD* kf2 = E->ShiftF7LD->ToKey->KFlds;
	//while (kf != nullptr) {
	for (auto& arg : E->ShiftF7LD->Args) {
		DuplFld(E->FD, CFile, E->NewRecPtr, CRecPtr, ee->OldRecPtr, kf2->FldD, arg->FldD);
		//kf = kf->pChain;
		kf2 = kf2->pChain;
	}

	CFile->SetUpdFlag(CRecPtr);
	CFile = E->FD;
	CRecPtr = E->NewRecPtr;
	result = true;

	keyboard.AddToFrontKeyBuf(0x0D); // ^M .. \r .. #13
	//pstring oldKbdBuffer = KbdBuffer;
	//KbdBuffer = 0x0D; // ^M
	//KbdBuffer += oldKbdBuffer;
	return result;
}

bool DuplToPrevEdit()
{
	LockMode md;
	auto result = false;
	EditD* ee = E->pChain;
	if (ee == nullptr) return result;
	FieldDescr* f1 = CFld->FldD;

	/* !!! with ee^ do!!! */
	FieldDescr* f2 = CFld->FldD;
	if ((f2->Flg && f_Stored == 0) || (f1->field_type != f2->field_type) || (f1->L != f2->L)
		|| (f1->M != f2->M) || !CFld->Ed(IsNewRec)) {
		WrLLF10Msg(140);
		return result;
	}
	CFile = ee->FD;
	CRecPtr = ee->NewRecPtr;
	if (!ELockRec(ee, CFile->IRec, ee->IsNewRec, ee->SubSet)) return result;
	if (!WasUpdated) {
		Move(CRecPtr, ee->OldRecPtr, CFile->FF->RecLen);
		WasUpdated = true;
	}
	DuplFld(E->FD, CFile, E->NewRecPtr, CRecPtr, ee->OldRecPtr, f1, f2);
	CFile->SetUpdFlag(CRecPtr);

	CFile = E->FD; CRecPtr = E->NewRecPtr;
	result = true;

	keyboard.AddToFrontKeyBuf(0x0D); // ^M .. \r .. #13
	//pstring oldKbdBuffer = KbdBuffer;
	//KbdBuffer = 0x0D; // ^M
	//KbdBuffer += oldKbdBuffer;
	return result;
}

void Calculate2()
{
	wwmix ww;

	FrmlElem* Z; std::string txt; WORD I; pstring Msg;
	void* p = nullptr; char FTyp; double R; FieldDescr* F; bool Del;
	//MarkStore(p);
	//NewExit(Ovr(), er);
	//goto label2;
	try {
		ResetCompilePars();
	label0:
		txt = CalcTxt;
	label4:
		I = 1;
		Del = true;
	label1:
		TxtEdCtrlUBrk = true;
		TxtEdCtrlF4Brk = true;
		ww.PromptLL(114, txt, I, Del);
		if (Event.Pressed.KeyCombination() == 'U') goto label0;
		if (Event.Pressed.KeyCombination() == __ESC || (txt.length() == 0)) goto label3;
		CalcTxt = txt;
		SetInpStr(txt);
		RdLex();
		Z = RdFrml(FTyp, nullptr);
		if (Lexem != 0x1A) Error(21);
		if (Event.Pressed.KeyCombination() == __CTRL_F4) {
			F = CFld->FldD;
			if (CFld->Ed(IsNewRec) && (F->frml_type == FTyp)) {
				if (LockRec(true)) {
					if ((F->field_type == FieldType::FIXED) && ((F->Flg & f_Comma) != 0)) {
						auto iZ0 = (FrmlElem0*)Z;
						auto iZ02 = (FrmlElem2*)iZ0->P1;
						if ((Z->Op = _const)) R = ((FrmlElem2*)Z)->R;
						else if ((Z->Op == _unminus) && (iZ02->Op == _const)) R = -iZ02->R;
						else goto label5;
						SetWasUpdated(CFile->FF, CRecPtr);
						CFile->saveR(F, R * Power10[F->M], CRecPtr);
					}
					else
						label5:
					AssignFld(F, Z);
					DisplFld(CFld, IRec, TextAttr);
					IVon();
					goto label3;
				}
			}
			else WrLLF10Msg(140);
		}
		switch (FTyp) {
		case 'R': {
			R = RunReal(CFile, Z, CRecPtr);
			str(R, 30, 10, txt);
			txt = LeadChar(' ', TrailChar(txt, '0'));
			if (txt[txt.length() - 1] == '.') {
				txt = txt.substr(0, txt.length() - 1);
			}
			break;
		}
		case 'S': {
			/* wie RdMode fuer T ??*/
			txt = RunShortStr(CFile, Z, CRecPtr);
			break;
		}
		case 'B': {
			if (RunBool(CFile, Z, CRecPtr)) txt = AbbrYes;
			else txt = AbbrNo;
			break;
		}
		}
		goto label4;
	}
	catch (std::exception& e) {
		//label2:
		Msg = MsgLine;
		I = CurrPos;
		SetMsgPar(Msg);
		WrLLF10Msg(110);
		IsCompileErr = false;
		Del = false;
		CFile = E->FD;
		ReleaseStore(&p);
		// TODO: goto label1;
	}
label3:
	//ReleaseStore(p);
	{}
}

void DelNewRec()
{
	LockMode md;
	CFile->DelAllDifTFlds(CRecPtr, nullptr);
	if (CNRecs() == 1) return;
	IsNewRec = false; Append = false;
	WasUpdated = false; CFld = E->FirstFld;
	if (CRec() > CNRecs()) { // pozor! uspodarani IF a ELSE neni jasne !!!
		if (IRec > 1) IRec--;
		else BaseRec--;
	}
	RdRec(CRec());
	NewDisplLL = true;
	DisplWwRecsOrPage(&CPage, &RT);
}

EFldD* FrstFldOnPage(WORD Page)
{
	EFldD* D = E->FirstFld;
	while (D->Page < Page) D = (EFldD*)D->pChain;
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

int GetEdRecNo()
{
	if (IsNewRec) return 0;
	if (E->IsLocked) return E->LockedRec;
	return AbsRecNr(CRec());
}

void SetEdRecNoEtc(int RNr)
{
	XString x;
	x.S = EdRecKey;
	EdField = CFld->FldD->Name;
	EdIRec = IRec;
	EdRecKey = "";
	EdKey = "";
	EdRecNo = RNr;
	if (RNr == 0) EdRecNo = GetEdRecNo();
	if (VK == nullptr) return;
	if (!WasWK && !VK->Alias.empty()) {
		EdKey = VK->Alias;
		if (EdKey == "") EdKey = "@";
	}
	if (!IsNewRec) {
		void* cr = CRecPtr;
		if (WasUpdated) CRecPtr = E->OldRecPtr;
		XKey* k = VK;
		if (Subset) k = WK;
		x.PackKF(CFile, k->KFlds, CRecPtr);
		CRecPtr = cr;
	}
	EdRecKey = x.S;
}

bool StartProc(Instr_proc* ExitProc, bool Displ)
{
	bool upd = false;
	bool b = false, b2 = false, lkd = false;
	char* p = nullptr;
	WORD d = 0; LockMode md; /*float t;*/

	auto result = false;
	CFile->FF->WasWrRec = false;
	if (HasTF) {
		p = (char*)CFile->GetRecSpace();
		Move(CRecPtr, p, CFile->FF->RecLen);
	}
	SetEdRecNoEtc(0);
	lkd = E->IsLocked;
	if (!lkd && !LockRec(false)) return result;
	b = WasUpdated;
	EdUpdated = b;
	b2 = CFile->HasUpdFlag(CRecPtr);
	SetWasUpdated(CFile->FF, CRecPtr);
	CFile->ClearUpdFlag(CRecPtr);

	// upravime argumenty exit procedury
	ExitProc->TArg[ExitProc->N - 1].FD = CFile;
	ExitProc->TArg[ExitProc->N - 1].RecPtr = CRecPtr;

	md = CFile->FF->LMode;
	WrEStatus();                            /*t = currtime;*/
	CallProcedure(ExitProc);
	RdEStatus();
	CFile->NewLockMode(md);
	upd = CFile->FF->WasWrRec;      /*writeln(strdate(currtime-t,"ss mm.ttt"));wait;*/
	if (CFile->HasUpdFlag(CRecPtr)) { b = true; upd = true; }
	WasUpdated = b;
	if (b2) CFile->SetUpdFlag(CRecPtr);
	if (!WasUpdated && !lkd) UnLockRec(E);
	if (Displ && upd) DisplAllWwRecs();
	if (Displ) NewDisplLL = true;
	result = true;
	if (HasTF) {
		for (auto& f : CFile->FldD) {
			if ((f->field_type == FieldType::TEXT) && ((f->Flg & f_Stored) != 0) &&
				(*(int*)(p + f->Displ) == *(int*)(E->OldRecPtr) + f->Displ))
				NoDelTFlds = true;
		}
		delete[] p; p = nullptr;
	}
	return result;
}

void StartRprt(RprtOpt* RO)
{
	bool displ = false;
	XWKey* k = nullptr; KeyFldD* kf = nullptr;
	if (IsNewRec || EdRecVar || (EdBreak == 16) || !WriteCRec(true, displ)) return;
	if (displ) DisplAllWwRecs();
	kf = nullptr;
	if (VK != nullptr) kf = VK->KFlds;
	k = new XWKey(CFile);
	k->OneRecIdx(CFile, kf, AbsRecNr(CRec()), CRecPtr);
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
	WORD c = Event.Pressed.KeyCombination();
	for (auto& X : E->ExD) {
		if (TestExitKey(c, X)) {
			ClrEvent();
			LastTxtPos = -1;
			if (X->Typ == 'Q') {
				w = 1;
			}
			else {
				bool ok = EdOk;
				EdOk = false;
				StartExit(X, true);
				if (EdOk) w = 3;
				else w = 2;
				EdOk = ok;
			}
		}
	}
	if (((w == 0) || (w == 3)) && (c == _ShiftF7_) && CFld->Ed(IsNewRec)) {
		ShiftF7Proc();
		w = 2;
	}
	//Event.Pressed.UpdateKey(c);
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
	if (!E->Last.empty()) {
		MsgLine = E->Last;
		if (MsgLine.length() > 0) {
			WrLLMsgTxt();
			DisplLASwitches();
		}
		return;
	}

	if (E->ShiftF7LD != nullptr) {
		n = 144;
	}
	else if (NoCreate || Only1Record) {
		if (IsNewRec) n = 129;
		else if (EdRecVar) n = 130;
		else n = 128;
	}
	else if (IsNewRec) {
		n = 123;
	}
	else {
		n = 124;
	}

	if (!F1Mode || Mode24) {
		WrLLMsg(n);
		DisplLASwitches();
	}
}

void DisplCtrlAltLL(WORD Flags)
{
	if ((Flags & 0x04) != 0) {        /* Ctrl */
		if (!E->CtrlLast.empty()) {
			MsgLine = E->CtrlLast;
			WrLLMsgTxt();
		}
		else if (IsCurrChpt()) WrLLMsg(125);
		else if (EdRecVar) WrLLMsg(154);
		else WrLLMsg(127);
	}
	else if ((Flags & 0x03) != 0) {         /* Shift */
		if (!E->ShiftLast.empty()) {
			MsgLine = E->ShiftLast;
			WrLLMsgTxt();
		}
		else DisplLL();
	}
	else if ((Flags & 0x08) != 0) {         /* Alt */
		if (!E->AltLast.empty()) {
			MsgLine = E->AltLast;
			WrLLMsgTxt();
		}
		else DisplLL();
	}
}

// po nacteni editoru se smycka drzi tady a ceka na stisknuti klavesy
void CtrlReadKbd()
{
	BYTE flgs = 0;
	uint64_t TimeBeg = getMillisecondsNow();
	unsigned int D = 0;

	if (F1Mode && Mode24 && CRdb->HelpFD != nullptr) {
		DisplayLastLineHelp(CFile->ChptPos.R, CFile->Name + "." + CFld->FldD->Name, Mode24);
	}

	TestEvent();

	if (Event.What == evKeyDown || Event.What == evMouseDown) {
		if (flgs != 0) {
			LLKeyFlags = 0;
			DisplLL();
			AddCtrlAltShift(flgs);
		}
		return;
	}
	ClrEvent();

	if (NewDisplLL) {
		DisplLL();
		NewDisplLL = false;
	}

	if (CFile->FF->NotCached()) {
		if (!E->EdRecVar && (spec.ScreenDelay == 0 || E->RefreshDelay < spec.ScreenDelay)) {
			D = E->RefreshDelay;
		}
		if (E->WatchDelay != 0) {
			if (D == 0) {
				D = E->WatchDelay;
			}
			else {
				D = min(D, E->WatchDelay);
			}
		}
	}

	while (true) {
		if (LLKeyFlags != 0) {
			flgs = LLKeyFlags;
			//goto label11;
			DisplCtrlAltLL(flgs);
		}
		else if ((KbdFlgs & 0x0F) != 0) {
			flgs = KbdFlgs;
			//label11:
			DisplCtrlAltLL(flgs);
		}
		else {
			DisplLL();
			flgs = 0;
			if (F1Mode && !Mode24 && CRdb->HelpFD != nullptr) {
				DisplayLastLineHelp(CFile->ChptPos.R, CFile->Name + "." + CFld->FldD->Name, Mode24);
			}
		}

		if (D > 0) {
			if (getMillisecondsNow() >= TimeBeg + D) {
				//goto label2;
				break;
			}
			else {
				WaitEvent(TimeBeg + D - getMillisecondsNow());
			}
		}
		else {
			WaitEvent(0);
		}

		if (!(Event.What == evKeyDown || Event.What == evMouseDown)) {
			ClrEvent();
			//goto label1;
			continue;
		}

		break;
	}
	//label2:
	if (flgs != 0) {
		LLKeyFlags = 0;
		DisplLL();
		AddCtrlAltShift(flgs);
	}
}

void MouseProc()
{
	WORD i; int n; EFldD* D; bool Displ;
	for (i = 1; i <= E->NRecs; i++) {
		n = BaseRec + i - 1; if (n > CNRecs()) goto label1;
		D = E->FirstFld; while (D != nullptr) {
			if (IsNewRec && (i == IRec) && (D == FirstEmptyFld)) goto label1;
			if ((D->Page == CPage) && MouseInRect(D->Col - 1, FldRow(D, i) - 1, D->L, 1)) {
				if ((i != IRec) && (IsNewRec || !WriteCRec(true, Displ))) goto label1;
				GotoRecFld(n, D);
				if ((Event.Buttons & mbDoubleClick) != 0) {
					if (MouseEnter) Event.Pressed.UpdateKey(_M_);
					else Event.Pressed.UpdateKey(_Ins_);
					Event.What = evKeyDown;
					return;
				}
				else ClrEvent();
				return;
			}
			D = (EFldD*)D->pChain;
		}
	}
label1:
	ClrEvent();
}

void ToggleSelectRec()
{
	XString x; LockMode md;
	XWKey* k = E->SelKey;
	int n = AbsRecNr(CRec());
	if (k->RecNrToPath(CFile, x, n, CRecPtr)) {
		k->NR--;
		k->DeleteOnPath(CFile);
	}
	else {
		k->NR++;
		k->Insert(CFile, n, false, CRecPtr);
	}
	SetRecAttr(IRec);
	IVon();
}

void ToggleSelectAll()
{
	XWKey* k = E->SelKey;
	if (k == nullptr) return;
	if (k->NR > 0) {
		k->Release(CFile);
	}
	else if (Subset) {
		CFile->FF->CopyIndex(k, WK);
	}
	else {
		CFile->FF->CopyIndex(k, VK);
	}
	DisplAllWwRecs();
}

void GoStartFld(EFldD* SFld)
{
	while ((CFld != SFld) && (CFld->pChain != nullptr)) {
		if (IsFirstEmptyFld()) {
			if ((CFld->Impl != nullptr) && LockRec(true)) AssignFld(CFld->FldD, CFld->Impl);
			FirstEmptyFld = (EFldD*)FirstEmptyFld->pChain;
			DisplFld(CFld, IRec, TextAttr);
		}
		GotoRecFld(CRec(), (EFldD*)CFld->pChain);
	}
}

void RunEdit(XString* PX, WORD& Brk)
{
	WORD i = 0, LongBeep = 0;
	bool Displ = false, b = false;
	EdExitD* X = nullptr;
	uint64_t OldTimeW = 0;
	uint64_t OldTimeR = 0;
	int n = 0;
	BYTE EdBr = 0;
	WORD KbdChar;

	Brk = 0;
	DisplLL();
	if (OnlySearch) goto label2;
	if (!IsNewRec && (PX != nullptr)) {
		GotoXRec(PX, n);
	}
	if (Select && !RunBool(CFile, E->Bool, CRecPtr)) {
		GoPrevNextRec(+1, true);
	}
	//if (/*E->StartFld != nullptr*/ true) { GoStartFld(&E->StartFld); goto label1; }
	if (E->StartFld != nullptr) {
		GoStartFld(E->StartFld);
		goto label1;
	}
label0:
	if (!CtrlMProc(0)) {
		goto label7;
	}
label1:
	LongBeep = 0;
	OldTimeW = getMillisecondsNow();
label81:
	OldTimeR = getMillisecondsNow();
	CtrlReadKbd();
	if (CFile->FF->NotCached()) {
		if (!EdRecVar && (E->RefreshDelay > 0) && (OldTimeR + E->RefreshDelay < getMillisecondsNow())) {
			DisplAllWwRecs();
		}
		if (Event.What == 0) {
			if ((E->WatchDelay > 0) && (OldTimeW + E->WatchDelay < getMillisecondsNow()))
				if (LongBeep < 3) {
					for (i = 1; i <= 4; i++) beep();
					LongBeep++;
					OldTimeW = getMillisecondsNow();
					goto label81;
				}
				else {
					UndoRecord();
					EdBreak = 11;
					goto label7;
				}
			else {
				goto label81;
			}
		}
	}
	switch (Event.What) {
	case evMouseDown: {
		if (F1Mode && (CRdb->HelpFD != nullptr)
			&& (Mode24 && (Event.Where.Y == TxtRows - 2) || !Mode24 && (Event.Where.Y == TxtRows - 1))) {
			ClrEvent();
			FieldHelp();
		}
		else MouseProc();
		break;
	}
	case evKeyDown: {
		KbdChar = Event.Pressed.KeyCombination();
		switch (ExitKeyProc())
		{
		case 1:/*quit*/ goto label7; break;
		case 2:/*exit*/ goto label1; break;
		}
		if (Event.Pressed.isChar()) {
			// jedna se o tisknutelny znak
			if (CFld->Ed(IsNewRec) && ((CFld->FldD->field_type != FieldType::TEXT) || (CFile->loadT(CFld->FldD, CRecPtr) == 0))
				&& LockRec(true)) {
				//keyboard.AddToFrontKeyBuf(KbdChar); // vrati znak znovu do bufferu
				const bool res = !EditItemProc(true, true, Brk);
				if (res) goto label7;
				if (Brk != 0) goto fin;
			}
		}
		else {
			// klavesa je funkcni
			ClrEvent();
			switch (Event.Pressed.KeyCombination()) {
			case __F1: {
				// index napovedy
				ReadMessage(7);
				Help((RdbD*)&HelpFD, MsgLine, false);
				break;
			}
			case __CTRL_F1: {
				// napoveda k aktualnimu udaji
				FieldHelp();
				break;
			}
			case __ALT_F10: {
				// posledne vyvolana napoveda
				Help(nullptr, "", false);
				break;
			}
			case __ESC: {
				// ukonceni editace bez ulozeni zmen
				if (OnlySearch) {
					if (IsNewRec) {
						if (CNRecs() > 1) DelNewRec();
						else goto label9;
					}
					else
						if (!WriteCRec(true, Displ)) goto label1;
				label2:
					if (PromptAndSearch(!NoCreate)) goto label0;
				}
			label9:
				EdBreak = 0;
			label7:
				if (IsNewRec && !EquOldNewRec()) {
					if (!Prompt158 || PromptYN(158)) goto fin;
					else goto label1;
				}
				EdBr = EdBreak;
				n = GetEdRecNo();
				if (((IsNewRec || WriteCRec(true, Displ)) && ((EdBreak == 11)) || NoESCPrompt || (!spec.ESCverify && !MustESCPrompt) || PromptYN(137))) {
					//if ((IsNewRec || WriteCRec(true, Displ)) && ((EdBreak == 11) || NoESCPrompt || !spec.ESCverify && !MustESCPrompt || PromptYN(137))) {
					EdBreak = EdBr;
					SetEdRecNoEtc(n);
					goto label71;
				fin:
					SetEdRecNoEtc(0);
				label71:
					if (IsNewRec && !EdRecVar) DelNewRec();
					IVoff();
					EdUpdated = E->EdUpdated;
					if (!EdRecVar) CFile->ClearRecSpace(E->NewRecPtr);
					if (Subset && !WasWK) WK->Close(CFile);
					if (!EdRecVar) {
#ifdef FandSQL
						if (CFile->IsSQLFile) Strm1->EndKeyAcc(WK);
#endif
						CFile->OldLockMode(E->OldMd);
				}
					return;
			}
				break;
			}
			case __ALT_EQUAL: {
				// ukonceni editace bez ulozeni zmen
				UndoRecord();
				EdBreak = 0;
				goto fin;
			}
			case 'U' + CTRL: {
				// obnoveni puvodniho stavu
				if (PromptYN(108)) UndoRecord();
				break;
			}
			case VK_OEM_102 + CTRL: // klavesa '\' na RT 102 keyb
			case VK_OEM_5 + CTRL: { // klavesa '\|' na US keyb
				// na zacatek dalsi vety
				if (!CtrlMProc(2)) goto label7;
				break;
			}
			case __F2: {
				// F2 - novy zaznam, porizeni nove vety
				if (!EdRecVar) {
					if (IsNewRec) {
						if ((CNRecs() > 1) && (!Prompt158 || EquOldNewRec() || PromptYN(158))) DelNewRec();
					}
					else if (!NoCreate && !Only1Record && WriteCRec(true, Displ))
					{
						if (Displ) DisplAllWwRecs();
						SwitchToAppend();
					}
				}
				goto label0;
				break;
			}
			case __UP: {
				if (LUpRDown) goto label11;
				else goto label13;
				break;
			}
			case __DOWN: {
				if (LUpRDown) goto label12;
				else goto label13;
				break;
			}
			case __LEFT:
			case 'S':
			{
			label11:
				if (CFld->ChainBack != nullptr) GotoRecFld(CRec(), CFld->ChainBack);
				break;
			}
			case __RIGHT:
			case 'D': {
			label12:
				if ((CFld->pChain != nullptr) && !IsFirstEmptyFld())
					GotoRecFld(CRec(), CFld->pChain);
				break;
			}
			case __HOME:
			label3:
				GotoRecFld(CRec(), E->FirstFld); break;
			case __END: {
			label4:
				if (IsNewRec && (FirstEmptyFld != nullptr))
					GotoRecFld(CRec(), FirstEmptyFld);
				else GotoRecFld(CRec(), E->LastFld);
				break;
			}
			case __ENTER: {
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
			}
			case __INSERT: {
				// zahajeni opravy udaje
				b = false;
				if (CFld->Ed(IsNewRec) && LockRec(true)) b = true;
				if (!EditItemProc(false, b, Brk)) goto label7;
				if (Brk != 0) goto fin;
				break;
			}
			case __F4: {
				// dopln diakriticke znamenko
				if ((CRec() > 1) && (IsFirstEmptyFld() || PromptYN(121)) && LockRec(true)) {
					DuplFromPrevRec();
					if (!CtrlMProc(1)) goto label7;
				}
				break;
			}
			case __F5: {
				// zapni / vypni prepinace
				SetSwitchProc(); break;
			}
			case __F7: {
				// navigace nahoru (nadrizeny soubor)
				UpwEdit(nullptr); break;
			}
			case __CTRL_F5: {
				// kalkulacka na poslednim radku
				Calculate2(); break;
			}
			default: {
				//if (KbdChar >= 0x20 && KbdChar <= 0xFE)
				//{
			label13:
				if (!IsNewRec) {
					if (KbdChar == __CTRL_Y) {
						if (!NoDelete) if (DeleteRecProc()) {
							ClearKeyBuf();
							b = true;
						label14:
							if (((CNRecs() == 0) || (CNRecs() == 1) && IsNewRec) && NoCreate) {
								WrLLF10Msg(112);
								EdBreak = 13;
								goto fin;
							}
							if (b && !CtrlMProc(0)) goto label7;
						}
					}
					else if (WriteCRec(true, Displ)) {
						if (Displ) DisplAllWwRecs();
						//Event.Pressed.UpdateKey(w);       /*only in edit mode*/
						switch (KbdChar) {
						case __F9: {
							// uloz
							SaveAndCloseAllFiles();
							UpdCount = 0;
							break;
						}
						case __CTRL_N: {
							// vloz novou vetu pred aktualni
							if (!NoCreate && !Only1Record) {
								InsertRecProc(nullptr);
								goto label0;
							}
							break;
						}
						case __UP:
						case 'E': {
							// predchozi radek
							if (E->NRecs > 1) GoPrevNextRec(-1, true);
							break;
						}
						case __CTRL_HOME: {
							// predchozi volny text
							GoPrevNextRec(-1, true);
							break;
						}
						case __DOWN:
						case 'X': {
							// nasledujici radek
							if (E->NRecs > 1) GoPrevNextRec(+1, true); break;
						}
						case __CTRL_END: {
							// nasledujici veta
							GoPrevNextRec(+1, true);
							break;
						}
						case __PAGEUP:
						case _R_: {
							// o obrazovku vzad
							if (E->NPages == 1)
								if (E->NRecs == 1) GoPrevNextRec(-1, true);
								else GotoRecFld(CRec() - E->NRecs, CFld);
							else if (CPage > 1) GotoRecFld(CRec(), FrstFldOnPage(CPage - 1));
							break;
						}
						case __PAGEDOWN:
						case _C_: {
							// o obrazovku vpred
							if (E->NPages == 1)
								if (E->NRecs == 1) GoPrevNextRec(+1, true);
								else GotoRecFld(CRec() + E->NRecs, CFld);
							else if (CPage < E->NPages) GotoRecFld(CRec(), FrstFldOnPage(CPage + 1));
							break;
						}
						case _Q_:
							switch (ReadKbd())
							{
							case _S_: goto label3;
							case _D_: goto label4;
							case _R_: goto label5;
							case _C_: goto label6;
							}
							break;
						case __CTRL_PAGEUP:
						label5:
							GotoRecFld(1, E->FirstFld); break;
						case __CTRL_PAGEDOWN:
						label6:
							GotoRecFld(CNRecs(), E->LastFld); break;
						case __CTRL_LEFT:
							if (CRec() > 1) SwitchRecs(-1); break;
						case __CTRL_RIGHT:
							if (CRec() < CNRecs()) SwitchRecs(+1); break;
						case __CTRL_F2: {
							if (!EdRecVar) RefreshSubset();
							b = false;
							goto label14;
							break;
						}
						case __ALT_F2:
						case __ALT_F3:
							if (IsCurrChpt()) {
								if (KbdChar == __ALT_F3) {
									ForAllFDs(ForAllFilesOperation::close_passive_fd);
									EditHelpOrCat(KbdChar, 0, "");
								}
								else {
									Brk = 2;
									goto fin;
								}
							}
							else if (IsTestRun && (CFile != CatFD->GetCatalogFile()) && (KbdChar == _AltF2)) {
								EditHelpOrCat(KbdChar, 1, CFile->Name + "." + CFld->FldD->Name);
							}
							break;
						case __F6: if (!EdRecVar) F6Proc(); break;
						case __F4: if (DuplToPrevEdit()) { EdBreak = 14; goto fin; } break;
						case __CTRL_F7: DownEdit(); break;
						case __F8: {
							if (E->SelKey != nullptr) {
								ToggleSelectRec(); GoPrevNextRec(+1, true);
							}
							break;
						}
						case __F3: {
							// najdi vetu podle klic. udaje
							if (!EdRecVar)
								if (CFile == CRdb->HelpFD) {
									if (PromptHelpName(i)) {
										GotoRecFld(i, CFld);
										goto label1;
									}
								}
								else {
									PromptAndSearch(false);
									goto label0;
								}
							break;
						}
						case __CTRL_F3: {
							// najdi vetu podle jejiho cisla
							if (!EdRecVar) PromptGotoRecNr();
							break;
						}
						case __SHIFT_F8: ToggleSelectAll(); break;
						case __CTRL_F8:
						case __CTRL_F9:
						case __CTRL_F10:
						case __ALT_F9:
							if (IsCurrChpt()) { Brk = 2; goto fin; }
							break;
						case __ALT_F7:
							ImbeddEdit();
							break;
						}
					}
				}
				//}
			}
		}
			break;
	}
		break;
	}
	default: {
		// nejedna se o udalost z klavesnice ani mysi
		ClrEvent();
		break;
	}
}
	Event.What = evNothing;
	goto label1;
}

void EditDataFile(FileD* FD, EditOpt* EO)
{
	void* p = nullptr;
	int w1 = 0, w2 = 0, w3 = 0;
	WORD Brk = 0, r1 = 0, r2 = 0;
	bool pix = false;
	MarkStore(p);
	if (EO->SyntxChk) {
		IsCompileErr = false;

		try {
			NewEditD(FD, EO);
		}
		catch (std::exception& e) {
			// TODO: log error
		}

		if (IsCompileErr) {
			EdRecKey = MsgLine;
			LastExitCode = CurrPos + 1;
			IsCompileErr = false;
		}
		else {
			LastExitCode = 0;
		}
		PopEdit();
		return;
	}
	NewEditD(FD, EO);
	w2 = 0;
	w3 = 0;
	pix = (E->WFlags & WPushPixel) != 0;
	if (E->WwPart) {
		r1 = TxtRows;
		r2 = E->WithBoolDispl ? 2 : 1;
		if (E->Mode24) {
			r1--;
		}
		w1 = PushW(1, 1, TxtCols, r2, pix, true);
		w2 = PushW(1, r1, TxtCols, TxtRows, pix, true);
		if ((E->WFlags & WNoPop) == 0) {
			w3 = PushW(E->V.C1, E->V.R1, E->V.C2 + E->ShdwX, E->V.R2 + E->ShdwY, pix, true);
		}
	}
	else {
		w1 = PushW(1, 1, TxtCols, TxtRows, pix, true);
	}
	if (OpenEditWw()) {
		if (OnlyAppend && !Append) {
			SwitchToAppend();
		}
		RunEdit(nullptr, Brk);
	}
	if (w3 != 0) PopW(w3);
	if (w2 != 0) PopW(w2);
	PopW(w1);
	PopEdit();
	ReleaseStore(&p);
}

void ViewPrinterTxt()
{
	WRect V = { 1, 2, 80, 24 };
	if (!PrintView) return;
	SetPrintTxtPath();
	V.C2 = TxtCols;
	V.R2 = TxtRows - 1;
	std::string ErrMsg;
	std::vector<EdExitD*> emptyExitD;
	EditTxtFile(nullptr, 'T', ErrMsg, emptyExitD, 1, 0, &V, 0, "", WPushPixel, nullptr);
}