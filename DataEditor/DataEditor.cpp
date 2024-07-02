#include "DataEditor.h"
#include <memory>
#include "../TextEditor/TextEditor.h"
#include "../TextEditor/EditorHelp.h"
#include "EditReader.h"
#include "../Core/ChkD.h"
#include "../Core/Compiler.h"
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
#include "../Drivers/constants.h"
#include "../Core/DateTime.h"


DataEditor::DataEditor()
{
	params_ = std::make_unique<DataEditorParams>();
}

DataEditor::DataEditor(EditD* edit)
{
	edit_ = edit;
	file_d_ = edit_->FD;
	original_record_ = edit_->FD->GetRecSpace();
	record_ = original_record_;
	params_ = std::make_unique<DataEditorParams>();
}

DataEditor::DataEditor(FileD* file_d)
{
	file_d_ = file_d;
	original_record_ = file_d->GetRecSpace();
	record_ = original_record_;
	params_ = std::make_unique<DataEditorParams>();
}

DataEditor::~DataEditor()
{
	if (original_record_ != nullptr) {
		delete[] original_record_;
		original_record_ = nullptr;
	}
}

FileD* DataEditor::GetFileD()
{
	return file_d_;
}

void DataEditor::SetEditD(EditD* edit)
{
	edit_ = edit;
	file_d_ = edit_->FD;
	original_record_ = edit_->FD->GetRecSpace();
	record_ = original_record_;
}

void DataEditor::SetFileD(FileD* file_d)
{
	//if (record_ != nullptr) {
	//	delete[] record_;
	//	record_ = nullptr;
	//}
	file_d_ = file_d;
	record_ = file_d->GetRecSpace();
}

uint8_t* DataEditor::GetRecord()
{
	return record_;
}

//void DataEditor::PopEdit()
//{
//	//E = edit_->pChain;
//	//EditDRoot = E;
//	v_edits.pop_back();
//}

EditD* DataEditor::GetEditD()
{
	return edit_;
}

bool DataEditor::TestIsNewRec()
{
	return IsNewRec;
}

void DataEditor::SetSelectFalse()
{
	params_->Select = false;
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
		else {
			Buffer[i].Char.AsciiChar = ' ';
		}
		//BuffLine[i] = *item;
	}
	screen.ScrWrCharInfoBuf(cx1, cy1, Buffer, maxCol);
	screen.GotoXY(cx + pos - base - 1, cy);
}

WORD DataEditor::EditTxt(std::string& text, WORD pos, WORD maxlen, WORD maxcol,
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
				if (pos > maxlen) { Beep(); goto label7; }
				if (InsMode) {
					if (text.length() == maxlen) {
						if (text[text.length() - 1] == ' ') {
							text = text.substr(0, text.length() - 1);
						}
						else {
							Beep();
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
			case 'V': {
				InsMode = !InsMode;
				break;
			}
			case 'U': {
				if (TxtEdCtrlUBrk) goto label6;
				break;
			}
			case __CTRL_F4: {
				if (TxtEdCtrlF4Brk) goto label6;
				break;
			}
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
			case 'S': {
				if ((pos > 1)) pos--;
				break;
			}
			case __RIGHT:
			case 'D': {
				if (pos <= maxlen) {
					if ((pos > text.length()) && (text.length() < maxlen)) text += ' ';
					pos++;
				}
				break;
			}
			case 'Q': {
				if (ReadKbd() == 'S') goto label3;
				if (ReadKbd() == 'D') goto label4;
				break;
			}
			case __HOME: {
			label3:
				pos = 1;
				break;
			}
			case __END: {
			label4:
				pos = (WORD)(TrailChar(text, ' ').length() + 1);
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
			case 'G': {
				if (upd && (pos <= text.length())) {
				label2:
					if (text.length() >= pos) {
						text.erase(pos - 1, 1);
					}
				}
				break;
			}
			case 'P': {
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
bool DataEditor::TestMask(std::string& S, std::string Mask)
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
		case '$': {
			if (!IsLetter(c)) goto label3;
			else goto label2; break;
		}
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
		break;
	}
	}
label4:
	result = false;
	SetMsgPar(Mask);
	WrLLF10Msg(653);
	return result;
}

void DataEditor::SetWasUpdated(FandFile* fand_file, void* record)
{
	if (!params_->WasUpdated) {
		if (params_->EdRecVar) {
			fand_file->SetUpdFlag(record);
		}
		memcpy(edit_->OldRecPtr, edit_->NewRecPtr, fand_file->RecordSize());
		params_->WasUpdated = true;
	}
}

void DataEditor::AssignFld(FieldDescr* F, FrmlElem* Z)
{
	SetWasUpdated(file_d_->FF, record_);
	AssgnFrml(file_d_, record_, F, Z, false, false);
}

WORD DataEditor::FieldEdit(FieldDescr* F, FrmlElem* Impl, WORD LWw, WORD iPos, std::string& Txt,
	double& RR, bool del, bool upd, bool ret, unsigned int Delta)
{
	short Col = 0, Row = 0;
	char cc = '\0';
	std::string Mask;
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
	Mask = F->Mask;
	if (((F->Flg & f_Mask) != 0) && (F->field_type == FieldType::ALFANUM)) {
		Msk = Mask;
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
		Txt = DecodeField(file_d_, F, L, record_);
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
			else /*'rdb'*/ if (I != 0) { WrLLF10Msg(639); goto label4; }
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
		else /*'rdb'*/ str(r, L, 0, Txt);
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
			r = ValDate(T, Mask);
			if ((r == 0.0) && (T != LeadChar(' ', OldTrailChar(' ', StrDate(r, Mask)))))
			{
				SetMsgPar(Mask);
				WrLLF10Msg(618);
			label4:
				screen.GotoXY(Col, Row);
				goto label2;
			}
		}
		Txt = StrDate(r, Mask);
		RR = r;
		break;
	}
	}
	return result;
}


void DataEditor::WrPromptTxt(std::string& S, FrmlElem* Impl, FieldDescr* F, std::string& Txt, double& R)
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
		case 'R': RR = RunReal(file_d_, Impl, record_); break;
		case 'S': SS = RunShortStr(file_d_, Impl, record_); break;
		default: BB = RunBool(file_d_, Impl, record_); break;
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

bool DataEditor::PromptB(std::string& S, FrmlElem* Impl, FieldDescr* F)
{
	std::string Txt;
	double R = 0.0;
	WrPromptTxt(S, Impl, F, Txt, R);
	bool result = (Txt[0] == AbbrYes);
	if (Event.Pressed.KeyCombination() == __ESC) {
		if (Impl != nullptr) result = RunBool(file_d_, Impl, record_);
		else result = false;
	}
	return result;
}

std::string DataEditor::PromptS(std::string& S, FrmlElem* Impl, FieldDescr* F)
{
	std::string Txt;
	double R = 0.0;
	WrPromptTxt(S, Impl, F, Txt, R);
	auto result = Txt;
	if (Event.Pressed.KeyCombination() == __ESC) {
		if (Impl != nullptr) result = RunShortStr(file_d_, Impl, record_);
		else result = "";
	}
	return result;
}

double DataEditor::PromptR(std::string& S, FrmlElem* Impl, FieldDescr* F)
{
	std::string Txt;
	double R = 0.0;
	WrPromptTxt(S, Impl, F, Txt, R);
	auto result = R;
	if (Event.Pressed.KeyCombination() == __ESC) {
		if (Impl != nullptr) {
			result = RunReal(file_d_, Impl, record_);
		}
		else {
			result = 0;
		}
	}
	return result;
}

int DataEditor::CRec()
{
	return BaseRec + IRec - 1;
	//return 0;
}

int DataEditor::CNRecs() const
{
	int n = 0;
	if (params_->EdRecVar) { return 1; }
	if (params_->Subset) n = WK->NRecs();
	else {
		if (HasIndex) n = VK->NRecs();
		else n = file_d_->FF->NRecs;
	}
	if (IsNewRec) n++;
	return n;
}

/// <summary>
/// Vraci poradi zaznamu v datovem souboru podle cisla indexu
/// </summary>
/// <param name="N">Cislo polozky v indexovem souboru (poradi)</param>
/// <returns>Cislo polozky v datovem souboru (poradi)</returns>
int DataEditor::AbsRecNr(int N)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "AbsRecNr(%i), file_d_ 0x%p", N, file_d_->Handle);
	LockMode md;
	int result = 0;
	if (params_->EdRecVar
#ifdef FandSQL
		|| file_d_->IsSQLFile
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
	if (params_->Subset) N = WK->NrToRecNr(file_d_, N);
	else if (HasIndex) {
		md = file_d_->NewLockMode(RdMode);
		file_d_->FF->TestXFExist();
		N = VK->NrToRecNr(file_d_, N);
		file_d_->OldLockMode(md);
	}
	result = N;
	return result;
}

int DataEditor::LogRecNo(int N)
{
	LockMode md;
	int result = 0;
	if ((N <= 0) || (N > file_d_->FF->NRecs)) return result;
	md = file_d_->NewLockMode(RdMode);
	file_d_->ReadRec(N, record_);
	if (!file_d_->DeletedFlag(record_)) {
		if (params_->Subset) result = WK->RecNrToNr(file_d_, N, record_);
		else if (HasIndex) {
			file_d_->FF->TestXFExist();
			result = VK->RecNrToNr(file_d_, N, record_);
		}
		else result = N;
	}
	file_d_->OldLockMode(md);
	return result;
}

bool DataEditor::IsSelectedRec(WORD I)
{
	XString x;
	auto result = false;
	if ((edit_->SelKey == nullptr) || (I == IRec) && IsNewRec) return result;
	int n = AbsRecNr(BaseRec + I - 1);

	/*void* cr = record_;
	if ((I == i_rec) && params_->WasUpdated) {
		record_ = edit_->OldRecPtr;
	}
	result = edit_->SelKey->RecNrToPath(file_d_, x, n, record_);
	record_ = cr;*/

	if ((I == IRec) && params_->WasUpdated) {
		result = edit_->SelKey->RecNrToPath(file_d_, x, n, edit_->OldRecPtr);
	}
	else
	{
		result = edit_->SelKey->RecNrToPath(file_d_, x, n, record_);
	}

	return result;
}

bool DataEditor::EquOldNewRec()
{
	return (CompArea(record_, edit_->OldRecPtr, file_d_->FF->RecLen) == _equ);
}

/// <summary>
/// Vycte X-ty zaznam (z DB nebo ze souboru v file_d_)
/// Ulozi jej do record_
/// Nejedna se o fyzicke cislo zaznamu v souboru
/// </summary>
/// <param name="N">kolikaty zaznam</param>
void DataEditor::RdRec(int N)
{
	LockMode md; XString x;
	if (params_->EdRecVar) return;
#ifdef FandSQL
	if (file_d_->IsSQLFile) {
		if (IsNewRec && (N > CRec)) dec(N); x.S = WK->NrToStr(N);
		Strm1->KeyAcc(WK, @x);
	}
	else
#endif
	{
		md = file_d_->NewLockMode(RdMode);
		file_d_->ReadRec(AbsRecNr(N), record_);
		file_d_->OldLockMode(md);
	}
}

bool DataEditor::CheckOwner(EditD* E)
{

	bool result = true;
	if (edit_->DownSet && (edit_->OwnerTyp != 'i')) {
		XString X, X1;
		X.PackKF(file_d_, edit_->DownKey->KFlds, record_);
		X1.PackKF(edit_->DownLD->ToFD, edit_->DownLD->ToKey->KFlds, edit_->DownRecPtr);
		X.S[0] = (char)(MinW(X.S.length(), X1.S.length()));
		if (X.S != X1.S) result = false;
	}
	return result;
}

bool DataEditor::CheckKeyIn(EditD* E)
{
	//KeyInD* k = edit_->KIRoot;
	XString X;
	//pstring* p1;
	//pstring* p2;
	auto result = true;
	if (edit_->KIRoot.empty()) return result;
	X.PackKF(file_d_, edit_->VK->KFlds, record_);
	//while (k != nullptr) {
	for (KeyInD* k : edit_->KIRoot) {
		//p1 = k->X1; p2 = k->X2;
		//if (p2 == nullptr) p2 = p1;
		//if ((p1->length() <= X.S.length()) && (X.S.length() <= p2->length() + 0xFF)) return result;
		if (k->X2.empty()) {
			if ((k->X1.length() <= X.S.length()) && (X.S.length() <= k->X1.length() + 0xFF)) return result;
		}
		else {
			if ((k->X1.length() <= X.S.length()) && (X.S.length() <= k->X2.length() + 0xFF)) return result;
		}
		//k = (KeyInD*)k->pChain;
	}
	result = false;
	return result;
}

bool DataEditor::ELockRec(EditD* E, int N, bool IsNewRec, bool Subset)
{
	LockMode md;
	auto result = true;
	if (edit_->IsLocked) return result;
	edit_->LockedRec = N;
	if (IsNewRec) return result;
	if (!edit_->params_->EdRecVar
#ifdef FandSQL
		&& !file_d_->IsSQLFile
#endif
		) {
		if (file_d_->FF->NotCached()) {
			if (!file_d_->Lock(N, 1/*withESC*/)) {
				result = false;
				return result;
			}
			md = file_d_->NewLockMode(RdMode);
			file_d_->ReadRec(N, record_);
			file_d_->OldLockMode(md);
			if (Subset && !
				((params_->NoCondCheck || RunBool(file_d_, edit_->Cond, record_) && CheckKeyIn(E)) && CheckOwner(E))) {
				WrLLF10Msg(150); goto label1;
			}
		}
		else if (file_d_->DeletedFlag(record_)) {
			WrLLF10Msg(148);
		label1:
			file_d_->Unlock(N);
			result = false;
			return result;
		}
	}
	edit_->IsLocked = true;
	return result;
}

WORD DataEditor::RecAttr(WORD I)
{
	bool b = (I != IRec) || !IsNewRec;
	if (!IsNewRec && file_d_->DeletedFlag(record_)) return edit_->dDel;
	else if (b && params_->Select && RunBool(file_d_, edit_->Bool, record_)) return edit_->dSubSet;
	else if (b && IsSelectedRec(I)) return edit_->dSelect;
	else return edit_->dNorm;
}

WORD DataEditor::FldRow(EFldD* D, WORD I)
{
	return edit_->FrstRow + edit_->NHdTxt + (I - 1) * RT->N + D->Ln - 1;
}

bool DataEditor::HasTTWw(FieldDescr* F)
{
	return (F->field_type == FieldType::TEXT) && (F->L > 1) && !edit_->IsUserForm;
}

void DataEditor::DisplEmptyFld(EFldD* D, WORD I)
{
	char c = '\0';
	screen.GotoXY(D->Col, FldRow(D, I));
	if ((D->FldD->Flg & f_Stored) != 0) c = '.';
	else c = ' ';
	//for (j = 1; j <= D->L; j++) printf("%c", c);
	printf("%s", std::string(D->L, c).c_str());
	if (HasTTWw(D->FldD)) printf("%*c", D->FldD->L - 1, ' ');
}

void DataEditor::Wr1Line(FieldDescr* F)
{
	auto X = screen.WhereX();
	auto Y = screen.WhereY();
	std::string ls = file_d_->loadS(F, record_);
	ls = GetNthLine(ls, 1, 1);
	WORD max = F->L - 2;
	ls = GetStyledStringOfLength(ls, 0, max);
	size_t chars = screen.WriteStyledStringToWindow(ls, edit_->dNorm);
	TextAttr = edit_->dNorm;
	if (chars < max) screen.ScrFormatWrStyledText(X + chars, Y, edit_->dNorm, "%*c", max - chars, ' ');
}

void DataEditor::DisplFld(EFldD* D, WORD I, BYTE Color)
{
	WORD r = FldRow(D, I);
	FieldDescr* F = D->FldD;
	screen.GotoXY(D->Col, r);
	std::string Txt = DecodeField(file_d_, F, D->L, record_);
	for (size_t j = 0; j < Txt.length(); j++)
		if ((unsigned char)Txt[j] < ' ') Txt[j] = Txt[j] + 0x40;
	screen.WriteStyledStringToWindow(Txt, Color);
	if (HasTTWw(F)) {
		screen.GotoXY(D->Col + 2, r);
		Wr1Line(F);
	}
}

// Display a form record
void DataEditor::DisplRec(WORD I)
{
	//EFldD* D = nullptr;
	bool NewFlds = false;
	BYTE a = edit_->dNorm;
	int N = BaseRec + I - 1;
	bool IsCurrNewRec = IsNewRec && (I == IRec);
	uint8_t* p = file_d_->GetRecSpace();

	if ((N > CNRecs()) && !IsCurrNewRec) {
		NewFlds = true;
	}
	else {
		if (I == IRec) {
			record_ = edit_->NewRecPtr;
		}
		else {
			record_ = p;
			RdRec(N);
		}
		NewFlds = false;
		if (!IsNewRec) {
			a = RecAttr(I);
		}
	}

	//std::vector<EFldD*>::iterator D = edit_->FirstFld;
	//while (D != nullptr) {
	for (EFldD* D : edit_->FirstFld) {
		if (IsCurrNewRec && D == FirstEmptyFld && D->Impl == nullptr) {
			NewFlds = true;
		}
		TextAttr = static_cast<uint8_t>(a);
		// Display an item of the record
		if (D->Page == CPage) {
			if (NewFlds) {
				DisplEmptyFld(D, I);
			}
			else {
				DisplFld(D, I, TextAttr);
			}
		}
		if (IsCurrNewRec && (D == FirstEmptyFld)) {
			NewFlds = true;
		}
		//D = D->pChain;
	}

	file_d_->ClearRecSpace(p);
	delete[] p; p = nullptr;
	record_ = edit_->NewRecPtr;
}

bool DataEditor::LockRec(bool Displ)
{

	bool result;
	if (edit_->IsLocked) {
		return true;
	}
	bool b = ELockRec(edit_, AbsRecNr(CRec()), IsNewRec, params_->Subset);
	result = b;
	if (b && !IsNewRec && !params_->EdRecVar && file_d_->FF->NotCached() && Displ) {
		DisplRec(IRec);
	}
	return result;
}

void DataEditor::UnLockRec(EditD* edit)
{
	if (edit->FD->FF->IsShared() && edit->IsLocked && !edit->params_->EdRecVar) {
		file_d_->Unlock(edit_->LockedRec);
	}
	edit->IsLocked = false;
}

void DataEditor::NewRecExit()
{
	for (EdExitD* X : edit_->ExD) {
		if (X->AtNewRec) {
			EdBreak = 18;
			LastTxtPos = -1;
			StartExit(X, false);
		}
	}
}

void DataEditor::SetCPage(WORD& c_page, ERecTxtD** rt)
{
	c_page = (*CFld)->Page;
	*rt = edit_->RecTxt[0];
	for (WORD i = 1; i < c_page; i++) {
		//*rt = (*rt)->pChain;
		*rt = edit_->RecTxt[i];
	}
}


void DataEditor::DisplRecNr(int N)
{
	if (edit_->RecNrLen > 0) {
		screen.GotoXY(edit_->RecNrPos, 1);
		TextAttr = screen.colors.fNorm;
		screen.ScrFormatWrText(edit_->RecNrPos, 1, "%*i", edit_->RecNrLen, N);
	}
}

void DataEditor::AdjustCRec()
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
			params_->Append = true;
			FirstEmptyFld = *CFld;
			file_d_->ZeroAllFlds(record_, false);
			SetWasUpdated(file_d_->FF, record_);
			NewRecExit();
		}
		else {
			SetWasUpdated(file_d_->FF, record_);
		}
		NewDisplLL = true;
	}
	UnLockRec(edit_);
	LockRec(false);
	DisplRecNr(CRec());
}

//EditD* DataEditor::WriteParamsToE()
//{
//	EditD* edit = new EditD(TxtCols, TxtRows);
//	edit->FD = file_d_;
//	edit->NewRecPtr = record_;
//	edit->CFld = CFld;
//	edit->FirstEmptyFld = FirstEmptyFld;
//	edit->VK = VK;
//	edit->WK = WK;
//	edit->BaseRec = BaseRec;
//	edit->IRec = IRec;
//	edit->IsNewRec = IsNewRec;
//
//	DataEditorParams::CopyParams(params_.get(), edit->params_.get());
//	return edit;
//}

void DataEditor::ReadParamsFromE(const EditD* edit)
{
	FirstEmptyFld = edit->FirstEmptyFld;
	VK = edit->VK;
	WK = edit->WK;
	BaseRec = edit->BaseRec;
	IRec = edit->IRec;
	IsNewRec = edit->IsNewRec;

	DataEditorParams::CopyParams(edit->params_.get(), params_.get());

	if (VK == nullptr) params_->OnlySearch = false;

	file_d_ = edit->FD;
	record_ = edit->NewRecPtr;

	CFld = edit->CFld;

	if (file_d_->FF->XF != nullptr) HasIndex = true;
	else HasIndex = false;

	if (file_d_->FF->TF != nullptr) HasTF = true;
	else HasTF = false;

	SetCPage(CPage, &RT);
}

void DataEditor::DuplFld(FileD* file_d1, FileD* file_d2, void* record1, void* record2, void* RPt, FieldDescr* field_d1, FieldDescr* field_d2)
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
			s = file_d1->loadS(field_d1, record1);
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

bool DataEditor::IsFirstEmptyFld()
{
	return IsNewRec && (*CFld == FirstEmptyFld);
}

void DataEditor::SetFldAttr(EFldD* D, WORD I, WORD Attr)
{
	screen.ScrColor(D->Col - 1, FldRow(D, I) - 1, D->L, Attr);
}

void DataEditor::IVoff()
{
	SetFldAttr(*CFld, IRec, RecAttr(IRec));
}

void DataEditor::IVon()
{
	screen.ScrColor((*CFld)->Col - 1, FldRow(*CFld, IRec) - 1, (*CFld)->L, edit_->dHiLi);
}

void DataEditor::SetRecAttr(WORD I)
{
	WORD TA = RecAttr(I);
	//std::vector<EFldD*>::iterator D = edit_->FirstFld;
	//while (D != nullptr) {
	for (EFldD* D : edit_->FirstFld) {
		if (D->Page == CPage) {
			SetFldAttr(D, I, TA);
		}
		//D = (EFldD*)D->pChain;
	}
}

void DataEditor::DisplTabDupl()
{
	TextAttr = edit_->dTab;
	//std::vector<EFldD*>::iterator D = edit_->FirstFld;
	//while (D != nullptr) {
	for (EFldD* D : edit_->FirstFld) {
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
		//D = (EFldD*)D->pChain;
	}
}

void DataEditor::DisplSysLine()
{
	WORD i = 0, j = 0;
	pstring m, s, x, z;
	bool point = false;
	s = edit_->Head;
	if (s == "") return;
	screen.GotoXY(1, 1);
	TextAttr = screen.colors.fNorm;
	ClrEol(TextAttr);
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
				edit_->RecNrLen = m.length();
				edit_->RecNrPos = i - m.length();
				for (j = 1; j <= m.length(); j++) x.Append(' ');
			}
		}
		else { x.Append(s[i]); i++; }
	if (x.length() > TxtCols) x[0] = (char)TxtCols;
	//printf("%s", x.c_str());
	screen.ScrWrText(1, 1, x.c_str());
	DisplRecNr(CRec());
}

void DataEditor::DisplBool()
{
	if (!params_->WithBoolDispl) return;
	screen.GotoXY(1, 2);
	TextAttr = edit_->dSubSet;
	ClrEol(TextAttr);
	if (params_->Select) {
		std::string s = edit_->BoolTxt;
		if (s.length() > TxtCols) s[0] = (char)TxtCols;
		screen.GotoXY((TxtCols - s.length()) / 2 + 1, 2);
		printf("%s", s.c_str());
	}
}

// zobrazi zaznamy v editoru
void DataEditor::DisplAllWwRecs()
{
	LockMode md = NullMode;
	WORD n = edit_->NRecs; // pocet zaznamu k zobrazeni (na strance)
	if ((n > 1) && !params_->EdRecVar) md = file_d_->NewLockMode(RdMode);
	AdjustCRec();
	if (!IsNewRec && !params_->WasUpdated) RdRec(CRec());
	for (WORD i = 1; i <= n; i++) {
		DisplRec(i);
	}
	IVon();
	if ((n > 1) && !params_->EdRecVar) file_d_->OldLockMode(md);
}

void DataEditor::SetNewWwRecAttr()
{
	record_ = file_d_->GetRecSpace();
	for (WORD I = 1; I <= edit_->NRecs; I++) {
		if (BaseRec + I - 1 > CNRecs()) break;
		if (!IsNewRec || (I != IRec)) {
			RdRec(BaseRec + I - 1);
			SetRecAttr(I);
		}
	}
	IVon();
	file_d_->ClearRecSpace(record_);
	delete[] record_; record_ = nullptr;
	record_ = edit_->NewRecPtr;
}

void DataEditor::MoveDispl(WORD From, WORD Where, WORD Number)
{
	for (WORD i = 1; i <= Number; i++) {
		//std::vector<EFldD*>::iterator D = edit_->FirstFld;
		//while (D != nullptr) {
		for (EFldD* D : edit_->FirstFld) {
			WORD r1 = FldRow(D, From) - 1;
			WORD r2 = FldRow(D, Where) - 1;
			screen.ScrMove(D->Col - 1, r1, D->Col - 1, r2, D->L);
			if (HasTTWw(D->FldD))
				screen.ScrMove(D->Col + 1, r1, D->Col + 1, r2, D->FldD->L - 2);
			//D = D->pChain;
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

void DataEditor::SetNewCRec(int N, bool withRead)
{
	int Max, I;
	Max = edit_->NRecs; I = N - BaseRec + 1;
	if (I > Max) { BaseRec += I - Max; IRec = Max; }
	else if (I <= 0) { BaseRec -= abs(I) + 1; IRec = 1; }
	else IRec = I;
	if (withRead) RdRec(CRec());
}

void DataEditor::WriteSL(std::vector<std::string>& SL)
{
	//while (SL != nullptr) {
	//	WORD row = screen.WhereY();
	//	screen.WriteStyledStringToWindow(SL->S, edit_->Attr);
	//	screen.GotoXY(edit_->FrstCol, row + 1);
	//	SL = SL->pChain;
	//}
	for (std::string& s : SL) {
		WORD row = screen.WhereY();
		screen.WriteStyledStringToWindow(s, edit_->Attr);
		screen.GotoXY(edit_->FrstCol, row + 1);
	}
}

void DataEditor::DisplRecTxt()
{
	screen.GotoXY(edit_->FrstCol, edit_->FrstRow + edit_->NHdTxt);
	for (WORD i = 1; i <= edit_->NRecs; i++) {
		WriteSL(RT->SL);
	}
}

void DataEditor::DisplEditWw()
{
	WORD i = 0, x = 0, y = 0;
	auto EV = edit_->V;
	if (edit_->ShdwY == 1) {
		screen.ScrColor(EV.C1 + 1, EV.R2, EV.C2 - EV.C1 + edit_->ShdwX - 1, screen.colors.ShadowAttr);
	}
	if (edit_->ShdwX > 0)
		for (i = EV.R1; i <= EV.R2; i++) {
			screen.ScrColor(EV.C2, i, edit_->ShdwX, screen.colors.ShadowAttr);
		}
	screen.Window(EV.C1, EV.R1, EV.C2, EV.R2);
	TextAttr = edit_->Attr;
	ClrScr(TextAttr);

	WriteWFrame(edit_->WFlags, edit_->Top, "", TextAttr);
	screen.Window(1, 1, TxtCols, TxtRows);
	DisplSysLine();
	DisplBool();
	screen.GotoXY(edit_->FrstCol, edit_->FrstRow);
	WriteSL(edit_->HdTxt);
	DisplRecTxt(); // zobrazi prazdne formulare
	DisplTabDupl();
	NewDisplLL = true;
	DisplAllWwRecs(); // doplni do formularu data nebo tecky
}

void DataEditor::DisplWwRecsOrPage(WORD& c_page, ERecTxtD** rt)
{
	if (c_page != (*CFld)->Page) {
		SetCPage(c_page, rt);
		TextAttr = edit_->Attr;
		Wind oldMin = WindMin;
		Wind oldMax = WindMax;
		screen.Window(edit_->FrstCol, edit_->FrstRow + edit_->NHdTxt, edit_->LastCol, edit_->FrstRow + edit_->Rows - 1);
		ClrScr(TextAttr);
		WindMin = oldMin;
		WindMax = oldMax;
		DisplRecTxt();
		DisplTabDupl();
	}
	DisplAllWwRecs();
	DisplRecNr(CRec());
}

void DataEditor::DuplOwnerKey()
{
	if (!edit_->DownSet || (edit_->OwnerTyp == 'i')) return;
	//KeyFldD* KF = edit_->DownLD->ToKey->KFlds;
	for (KeyFldD* KF : edit_->DownLD->ToKey->KFlds) {
		for (auto& arg : edit_->DownLD->Args) {
			DuplFld(edit_->DownLD->ToFD, file_d_, edit_->DownRecPtr, edit_->NewRecPtr, edit_->OldRecPtr,
				KF->FldD, arg->FldD);
			//KF = KF->pChain;
		}
	}
}

bool DataEditor::TestDuplKey(FileD* file_d, XKey* K)
{
	XString x;
	int N = 0;
	x.PackKF(file_d, K->KFlds, record_);
	return K->Search(file_d, x, false, N) && (IsNewRec || (edit_->LockedRec != N));
}

void DataEditor::DuplKeyMsg(XKey* K)
{
	SetMsgPar(K->Alias);
	WrLLF10Msg(820);
}

void DataEditor::BuildWork()
{
	void* p = nullptr;
	XKey* K = nullptr;
	std::vector<KeyFldD*>* KF = nullptr;
	XString xx;
	bool dupl = true;
	bool intvl = false;

	if (!file_d_->Keys.empty()) {
		KF = &file_d_->Keys[0]->KFlds;
	}

	if (HasIndex) {
		K = VK;
		KF = &K->KFlds;
		dupl = K->Duplic;
		intvl = K->IntervalTest;
	}

	if (KF == nullptr) {
		KF = new std::vector<KeyFldD*>();
	}

	WK->Open(file_d_, *KF, dupl, intvl);
	if (params_->OnlyAppend) return;
	FrmlElem* boolP = edit_->Cond;
	//KeyInD* ki = edit_->KIRoot;
	XWKey* wk2 = nullptr;
	MarkStore(p);
	bool ok = false;
	FieldDescr* f = nullptr;
	WORD l = 0;
	//NewExit(Ovr(), er);
	//goto label1;
	try {
		XScan* Scan;
		if (edit_->DownSet) {
			std::vector<KeyInD*> empty;
			Scan = new XScan(file_d_, edit_->DownKey, empty, false);
			if (edit_->OwnerTyp == 'i') {
				Scan->ResetOwnerIndex(edit_->DownLD, edit_->DownLV, boolP);
			}
			else {
				xx.PackKF(edit_->DownLD->ToFD, edit_->DownLD->ToKey->KFlds, edit_->DownRecPtr);
				Scan->ResetOwner(&xx, boolP);
			}
			if (!edit_->KIRoot.empty()) {
				wk2 = new XWKey(file_d_);
				wk2->Open(file_d_, *KF, true, false);
				file_d_->FF->CreateWIndex(Scan, wk2, 'W');
				XScan* Scan2 = new XScan(file_d_, wk2, edit_->KIRoot, false);
				Scan2->Reset(nullptr, false, record_);
				Scan = Scan2;
			}
		}
		else {
#ifdef FandSQL
			if (file_d_->IsSQLFile && (boolP == nullptr)) {
				l = file_d_->FF->RecLen; f = file_d_->FldD[0]; OnlyKeyArgFlds(WK);
			}
#endif
			if (
#ifdef FandSQL
				file_d_->IsSQLFile ||
#endif
				(boolP != nullptr))
				if ((K != nullptr) && !K->InWork && (edit_->KIRoot.empty())) K = nullptr;
			Scan = new XScan(file_d_, K, edit_->KIRoot, false);
			Scan->Reset(boolP, edit_->SQLFilter, record_);
		}
		file_d_->FF->CreateWIndex(Scan, WK, 'W');
		Scan->Close();
		if (wk2 != nullptr) wk2->Close(file_d_);
		ok = true;
	}
	catch (std::exception& e) {
		// TODO: log error
	}

	if (f != nullptr) {
		file_d_->FldD.clear();
		file_d_->FldD.push_back(f);
		WK->KFlds = *KF;
		file_d_->FF->RecLen = l;
	}
	if (!ok) {
		GoExit();
	}
	ReleaseStore(&p);
}

void DataEditor::SetStartRec()
{
	int n = 0;
	//KeyFldD* kf = nullptr;
	XKey* k = VK;
	if (params_->Subset) {
		k = WK;
	}
	//if (k != nullptr) {
	//	kf = k->KFlds;
	//}
	if ((!edit_->StartRecKey.empty()) && (k != nullptr)) {
		if (k->FindNr(file_d_, edit_->StartRecKey, n)) {
			n = MaxL(1, MinL(n, CNRecs()));
			IRec = MaxW(1, MinW(edit_->StartIRec, edit_->NRecs));
			BaseRec = n - IRec + 1;
			if (BaseRec <= 0) {
				IRec += BaseRec - 1;
				BaseRec = 1;
			}
		}
	}
	else if (edit_->StartRecNo > 0) {
		n = LogRecNo(edit_->StartRecNo);
		n = MaxL(1, MinL(n, CNRecs()));
		IRec = MaxW(1, MinW(edit_->StartIRec, edit_->NRecs));
		BaseRec = n - IRec + 1;
		if (BaseRec <= 0) {
			IRec += BaseRec - 1;
			BaseRec = 1;
		}
	}
	if (params_->Only1Record) {
		if (CNRecs() > 0) {
			RdRec(CRec());
			n = AbsRecNr(CRec());
		}
		else n = 0;
		if (params_->Subset) {
			WK->Close(file_d_);
		}
		params_->Subset = true;
		if (n == 0) {
			std::vector<KeyFldD*> unused;
			WK->Open(file_d_, unused, true, false);
		}
		else {
			if (k != nullptr) {
				WK->OneRecIdx(file_d_, k->KFlds, n, record_);
			}
			else {
				std::vector<KeyFldD*> unused;
				WK->OneRecIdx(file_d_, unused, n, record_);
			}
		}
		BaseRec = 1;
		IRec = 1;
	}
}

bool DataEditor::OpenEditWw()
{
	LockMode md, md1, md2;
	int n = 0;
	bool result = false;

	// open journal file, if exists
	if (edit_->Journal != nullptr) {
		OpenCreateF(edit_->Journal, CPath, Shared);
	}

	ReadParamsFromE(edit_);
	if (params_->EdRecVar) {
		if (params_->OnlyAppend) {
			goto label2;
		}
		else {
			goto label3;
		}
	}
#ifdef FandSQL
	if (!file_d_->IsSQLFile)
#endif
		OpenCreateF(file_d_, CPath, Shared);
	edit_->OldMd = edit_->FD->FF->LMode;
	UpdCount = 0;
#ifdef FandSQL
	if (file_d_->IsSQLFile) {
		if ((VK = nullptr) || !VK->InWork) Subset = true
	}
	else
#endif
	{
		if (HasIndex) {
			file_d_->FF->TestXFExist();
		}
		md = NoDelMode;
		if (params_->OnlyAppend || (edit_->Cond != nullptr) || (!edit_->KIRoot.empty()) || edit_->DownSet ||
			params_->MakeWorkX && HasIndex && file_d_->FF->NotCached() && !params_->Only1Record)
		{
			params_->Subset = true;
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
	if (params_->Subset || params_->Only1Record) {
		WK = new XWKey(file_d_);
	}
	if (!file_d_->TryLockMode(md, md1, 1)) {
		EdBreak = 15;
		goto label1;
	}
	md2 = file_d_->NewLockMode(RdMode);
	if (edit_->DownSet && (edit_->OwnerTyp == 'F')) {
		md1 = edit_->DownLD->ToFD->NewLockMode(RdMode);
		n = edit_->OwnerRecNo;
		if ((n == 0) || (n > edit_->DownLD->ToFD->FF->NRecs)) {
			edit_->DownLD->ToFD->RunErrorM(edit_->OldMd);
			RunError(611);
		}
		edit_->DownLD->ToFD->ReadRec(n, edit_->DownRecPtr);
		edit_->DownLD->ToFD->OldLockMode(md1);
	}
	if (params_->Subset) {
		BuildWork();
	}
	if (!params_->Only1Record && HasIndex && VK->InWork) {
		if (!params_->Subset) WK = (XWKey*)VK;
		if (!file_d_->Keys.empty()) {
			VK = file_d_->Keys[0];
		}
		else {
			VK = nullptr;
		}
		params_->WasWK = true;
		params_->Subset = true;
	}
#ifdef FandSQL
	if (file_d_->IsSQLFile) Strm1->DefKeyAcc(WK);
#endif
	if (!params_->OnlyAppend) {
		SetStartRec();
	}
	if (CNRecs() == 0)
		if (params_->NoCreate) {
			if (params_->Subset) {
				FileMsg(file_d_, 107, '0');
			}
			else {
				FileMsg(file_d_, 115, '0');
			}
			EdBreak = 13;
		label1:
			if (params_->Subset && !params_->WasWK) {
				WK->Close(file_d_);
			}
			file_d_->OldLockMode(edit_->OldMd);
			result = false;
			return result;
		}
		else {
		label2:
			IsNewRec = true;
			params_->Append = true;
			LockRec(false);
			file_d_->ZeroAllFlds(record_, false);
			DuplOwnerKey();
			SetWasUpdated(file_d_->FF, record_);
		}
	else {
		RdRec(CRec());
	}
label3:
	MarkStore(edit_->AfterE);
	DisplEditWw();
	result = true;
	if (!params_->EdRecVar) file_d_->OldLockMode(md2);
	if (IsNewRec) NewRecExit();
	return result;
}

void DataEditor::RefreshSubset()
{
	LockMode md = file_d_->NewLockMode(RdMode);
	if (params_->Subset && !(params_->OnlyAppend || params_->Only1Record || params_->WasWK)) {
		WK->Close(file_d_);
		BuildWork();
	}
	DisplAllWwRecs();
	file_d_->OldLockMode(md);
}

void DataEditor::GotoPrevRecFld(int NewRec, std::vector<EFldD*>::iterator NewFld)
{
	--NewFld;
	GotoRecFld(NewRec, NewFld);
}

void DataEditor::GotoNextRecFld(int NewRec, std::vector<EFldD*>::iterator NewFld)
{
	++NewFld;
	GotoRecFld(NewRec, NewFld);
}

void DataEditor::GotoRecFld(int NewRec, std::vector<EFldD*>::iterator NewFld)
{
	int NewIRec = 0, NewBase = 0, D = 0, Delta = 0;
	WORD i = 0, Max = 0; LockMode md;
	IVoff();
	CFld = NewFld;
	if (NewRec == CRec()) {
		if (CPage != (*CFld)->Page) {
			DisplWwRecsOrPage(CPage, &RT);
		}
		else {
			IVon();
		}
		return;
	}
	if (!params_->EdRecVar) {
		md = file_d_->NewLockMode(RdMode);
	}
	if (NewRec > CNRecs()) {
		NewRec = CNRecs();
	}
	if (NewRec <= 0) {
		NewRec = 1;
	}
	if (params_->Select) {
		SetRecAttr(IRec);
	}
	CFld = NewFld;
	Max = edit_->NRecs;
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
		DisplWwRecsOrPage(CPage, &RT);
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
	if (!params_->EdRecVar) {
		file_d_->OldLockMode(md);
	}
}

void DataEditor::UpdMemberRef(void* POld, void* PNew)
{
	XString x, xnew, xold;
	XScan* Scan = nullptr;
	FileD* cf = file_d_;
	void* cr = record_;
	void* p = nullptr;
	void* p2 = nullptr;
	XKey* k = nullptr;
	//std::vector<KeyFldD*> *kf = nullptr;
	//std::vector<KeyFldD*> *kf1 = nullptr;
	//std::vector<KeyFldD*> *kf2 = nullptr;
	// , * Arg = nullptr;

	for (LinkD* LD : LinkDRoot) {
		if ((LD->MemberRef != 0) && (LD->ToFD == cf) && ((PNew != nullptr) || (LD->MemberRef != 2))) {
			//kf2 = &LD->ToKey->KFlds;
			xold.PackKF(cf, LD->ToKey->KFlds, POld);
			if (PNew != nullptr) {
				xnew.PackKF(cf, LD->ToKey->KFlds, PNew);
				if (xnew.S == xold.S) continue;
			}
#ifdef FandSQL
			sql = LD->FromFD->IsSQLFile;
#endif
			k = GetFromKey(LD);
			//kf1 = &;
			p = LD->FromFD->GetRecSpace();
			if (PNew != nullptr) {
				p2 = LD->FromFD->GetRecSpace();
			}
			std::vector<KeyInD*> empty;
			Scan = new XScan(LD->FromFD, k, empty, true);
			Scan->ResetOwner(&xold, nullptr);
#ifdef FandSQL
			if (!sql)
#endif
				LD->FromFD->FF->ScanSubstWIndex(Scan, k->KFlds, 'W');
		label1:
			Scan->GetRec(p);
			if (!Scan->eof) {
#ifdef FandSQL
				if (sql) x.PackKF(kf1);
#endif
				if (PNew == nullptr) {
					RunAddUpdate(LD->FromFD, '-', nullptr, false, nullptr, LD, p);
					UpdMemberRef(p, nullptr);
#ifdef FandSQL
					if (sql) Strm1->DeleteXRec(k, @x, false);
					else
#endif
						LD->FromFD->FF->DeleteXRec(Scan->RecNr, true, p);
				}
				else {
					Move(p, p2, LD->FromFD->FF->RecLen);
					//kf = &LD->ToKey->KFlds;
					for (size_t i = 0; i < LD->Args.size(); i++) {
						KeyFldD* arg = LD->Args[i];
						KeyFldD* k1 = LD->ToKey->KFlds[i];
						DuplFld(cf, LD->FromFD, PNew, p2, nullptr, k1->FldD, arg->FldD);
						//kf = kf->pChain;
					}
					RunAddUpdate(LD->FromFD, 'd', p, false, nullptr, LD, p2);
					UpdMemberRef(p, p2);
#ifdef FandSQL
					if (sql) Strm1->UpdateXRec(k, @x, false) else
#endif
						LD->FromFD->FF->OverWrXRec(Scan->RecNr, p, p2, p2);
				}
				goto label1;
			}
			Scan->Close();
			LD->FromFD->ClearRecSpace(p);
			ReleaseStore(&p);
		}
	} // for
}

void DataEditor::WrJournal(char Upd, void* RP, double Time)
{
	// Upd:
	// + new record; - deleted record; O old record data; N new record data

	size_t srcOffset = 0;
	if (edit_->Journal != nullptr) {
		WORD l = file_d_->FF->RecLen;
		int n = AbsRecNr(CRec());
		if (file_d_->FF->XF != nullptr) {
			srcOffset += 2;
			l--;
		}
		file_d_ = edit_->Journal;
		//record_ = GetRecSpace();

		const auto newData = std::make_unique<BYTE[]>(file_d_->FF->RecLen + 2);

		auto it = file_d_->FldD.begin();

		file_d_->saveS(*it++, std::string(1, Upd), newData.get());	// change type
		file_d_->saveR(*it++, n, newData.get());								// record number
		file_d_->saveR(*it++, UserCode, newData.get());						// user code
		file_d_->saveR(*it++, Time, newData.get());							// timestamp

		char* src = (char*)RP;
		memcpy(&newData.get()[(*it)->Displ], &src[srcOffset], l);					// record data

		LockMode md = file_d_->NewLockMode(CrMode);
		file_d_->IncNRecs(1);
		file_d_->WriteRec(file_d_->FF->NRecs, newData.get());
		file_d_->OldLockMode(md);
		file_d_ = edit_->FD;
		record_ = edit_->NewRecPtr;
	}
	UpdCount++;
	if (UpdCount == edit_->SaveAfter) {
		SaveFiles();
		UpdCount = 0;
	}
}

bool DataEditor::LockForMemb(FileD* FD, WORD Kind, LockMode NewMd, LockMode& md)
{
	LockMode md1; /*0-ExLMode,1-lock,2-unlock*/
	auto result = false;
	for (auto& ld : LinkDRoot) {
		if ((ld->ToFD == FD)
			&& ((NewMd != DelMode) && (ld->MemberRef != 0) || (ld->MemberRef == 1))
			&& (ld->FromFD != FD)) {
			file_d_ = ld->FromFD;
			switch (Kind) {
			case 0: {
				file_d_->FF->TaLMode = file_d_->FF->LMode;
				break;
			}
			case 1: {
				md = NewMd;
				if (!file_d_->TryLockMode(NewMd, md1, 2)) return result;
				break;
			}
			case 2: {
				file_d_->OldLockMode(file_d_->FF->TaLMode);
				break;
			}
			}
			if (!LockForAdd(file_d_, Kind, true, md)) return result;
			if (!LockForMemb(ld->FromFD, Kind, NewMd, md)) return result;
		}
	}
	result = true;
	return result;
}

bool DataEditor::LockWithDep(LockMode CfMd, LockMode MembMd, LockMode& OldMd)
{
	FileD* cf2 = nullptr;
	bool b = false;
	int w1 = 0;
	LockMode md;
	auto result = true;
	if (params_->EdRecVar) return result;
	FileD* cf = file_d_;
	int w = 0;
	LockForAdd(cf, 0, true, md);
	LockForMemb(cf, 0, MembMd, md);
label1:
	file_d_ = cf;
	if (!file_d_->TryLockMode(CfMd, OldMd, 1)) {
		md = CfMd;
		goto label3;
	}
	if (!LockForAdd(cf, 1, true, md)) {
		cf2 = file_d_;
		goto label2;
	}
	if (MembMd == NullMode) goto label4;
	if (!LockForMemb(cf, 1, MembMd, md)) {
		cf2 = file_d_;
		LockForMemb(cf, 2, MembMd, md);
	label2:
		LockForAdd(cf, 2, true, md);
		file_d_ = cf;
		file_d_->OldLockMode(OldMd);
		file_d_ = cf2;
	label3:
		SetPathAndVolume(file_d_);
		SetMsgPar(CPath, LockModeTxt[md]);
		w1 = PushWrLLMsg(825, true);
		if (w == 0) w = w1;
		else TWork.Delete(w1);
		LockBeep();
		if (KbdTimer(spec.NetDelay, 1)) goto label1;
		result = false;
	}
label4:
	file_d_ = cf;
	if (w != 0) PopW(w);
	return result;
}

void DataEditor::UnLockWithDep(LockMode OldMd)
{
	LockMode md;
	if (params_->EdRecVar) return;
	FileD* cf = file_d_;
	file_d_->OldLockMode(OldMd);
	LockForAdd(cf, 2, true, md);
	LockForMemb(cf, 2, md, md);
	file_d_ = cf;
}

void DataEditor::UndoRecord()
{
	LockMode md;
	if (!IsNewRec && params_->WasUpdated) {
		if (HasTF) {
			if (params_->NoDelTFlds) {
				//FieldDescr* f = file_d_->FldD.front();
				//while (f != nullptr) {
				for (FieldDescr* f : file_d_->FldD) {
					if (((f->Flg & f_Stored) != 0) && (f->field_type == FieldType::TEXT))
						*(int*)((char*)(edit_->OldRecPtr) + f->Displ) = *(int*)(((char*)(record_)+f->Displ));
					//f = f->pChain;
				}
			}
		}
		else { // je toto spravne zanorene???
			file_d_->DelAllDifTFlds(edit_->NewRecPtr, edit_->OldRecPtr);
		}

		Move(edit_->OldRecPtr, edit_->NewRecPtr, file_d_->FF->RecLen);
		params_->WasUpdated = false; params_->NoDelTFlds = false;
		UnLockRec(edit_);
		DisplRec(IRec);
		IVon();
	}
}

bool DataEditor::CleanUp()
{
	if (HasIndex && file_d_->DeletedFlag(record_)) return false;
	for (auto& X : edit_->ExD) {
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
			params_->WasUpdated = false;
		}
	}
	if (params_->AddSwitch) {
		for (auto& ld : LinkDRoot) {
			if ((ld->MemberRef == 2) && (ld->ToFD == file_d_) && Owned(file_d_, nullptr, nullptr, ld, record_) > 0) {
				WrLLF10Msg(662);
				return false;
			}
		}
		if (!RunAddUpdate(file_d_, '-', nullptr, false, nullptr, nullptr, record_)) return false;
		UpdMemberRef(record_, nullptr);
	}
	if (!ChptDel(file_d_, edit_)) {
		return false;
	}
	WrJournal('-', record_, Today() + CurrTime());
	return true;
}

bool DataEditor::DelIndRec(int I, int N)
{
	bool result = false;
	if (CleanUp()) {
		file_d_->FF->DeleteXRec(N, true, record_);
		SetUpdHandle(file_d_->FF->Handle); // navic
		SetUpdHandle(file_d_->FF->XF->Handle); // navic
		if ((edit_->SelKey != nullptr) && edit_->SelKey->Delete(file_d_, N, record_)) edit_->SelKey->NR--;
		if (params_->Subset) WK->DeleteAtNr(file_d_, I);
		result = true;
		edit_->EdUpdated = true;
	}
	return result;
}

bool DataEditor::DeleteRecProc()
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "DeleteRecProc() deleting item (file_d_ '%c')", file_d_->Name.c_str());

	int I = 0, J = 0, N = 0, oBaseRec = 0;
	WORD oIRec = 0;
	bool Group = false, fail = false; LockMode OldMd;
	bool b = false;
	auto result = false; Group = false;
	if (params_->Select) {
		F10SpecKey = VK_ESCAPE;
		Group = PromptYN(116);
		if (Event.Pressed.KeyCombination() == __ESC) return result;
	}
	if (!Group) {
		if (params_->VerifyDelete && !PromptYN(109)) return result;
	}
	if (!LockWithDep(DelMode, DelMode, OldMd)) return result;
	UndoRecord();
	N = AbsRecNr(CRec());
	RdRec(CRec());
	oIRec = IRec;
	oBaseRec = BaseRec;    /* exit proc uses CRec for locking etc.*/
	if (HasIndex
#ifdef FandSQL
		|| file_d_->IsSQLFile
#endif
		) {
		//log->log(loglevel::DEBUG, "... from file with index ...");
		file_d_->FF->TestXFExist();
		if (Group) {
			IRec = 1; BaseRec = 1;
			while (BaseRec <= CNRecs()) {
				N = AbsRecNr(BaseRec);
				file_d_->ClearDeletedFlag(record_); /*prevent err msg 148*/
				if (!ELockRec(edit_, N, false, params_->Subset)) goto label1;
				RdRec(BaseRec);
				if (RunBool(file_d_, edit_->Bool, record_)) {
					b = DelIndRec(BaseRec, N);
				}
				else {
					b = true;
					BaseRec++;
				}
				UnLockRec(edit_);
				if (!b) goto label1;
			}
		label1:
			{}
		}
		else {
			if (!ELockRec(edit_, N, false, params_->Subset)) goto label1;
			DelIndRec(CRec(), N);
			UnLockRec(edit_);
		}
	}
	else if (Group) {
		J = 0;
		fail = false;
		BaseRec = 1;
		IRec = 1;
		edit_->EdUpdated = true;
		for (I = 1; I <= file_d_->FF->NRecs; I++) {
			file_d_->ReadRec(I, record_);
			if (fail) goto label2;
			if (params_->Subset) {
				if ((BaseRec > WK->NRecs()) || (WK->NrToRecNr(file_d_, BaseRec) != J + 1)) goto label2;
			}
			else BaseRec = I;
			if (RunBool(file_d_, edit_->Bool, record_)) {
				if (!CleanUp()) {
					fail = true;
					goto label2;
				}
				if (params_->Subset) {
					WK->DeleteAtNr(file_d_, BaseRec);
					WK->AddToRecNr(file_d_, J + 1, -1);
				}
				file_d_->DelAllDifTFlds(record_, nullptr);
			}
			else {
				if (params_->Subset) BaseRec++;
			label2:
				J++;
				file_d_->WriteRec(J, record_);
			}
		}
		file_d_->DecNRecs(file_d_->FF->NRecs - J);
	}
	else if (CleanUp()) {
		edit_->EdUpdated = true;
		if (params_->Subset) {
			WK->DeleteAtNr(file_d_, CRec());
			WK->AddToRecNr(file_d_, N, -1);
		}
		file_d_->DeleteRec(N, record_);
	}
	CFld = edit_->FirstFld.begin();
	IRec = (BYTE)oIRec;
	BaseRec = oBaseRec;
	file_d_->ClearDeletedFlag(record_);
	AdjustCRec();
	if (IsNewRec) { DuplOwnerKey(); }
	else { RdRec(CRec()); }
	DisplWwRecsOrPage(CPage, &RT);
	UnLockWithDep(OldMd);
	result = true;
	return result;
}

ChkD* DataEditor::CompChk(EFldD* D, char Typ)
{
	bool w = params_->WarnSwitch && (Typ == 'W' || Typ == '?');
	bool f = (Typ == 'F' || Typ == '?');
	//ChkD* C = D->Chk;
	ChkD* result = nullptr;
	for (ChkD* C : D->Chk) { //while (C != nullptr) {
		if ((w && C->Warning || f && !C->Warning) && !RunBool(file_d_, C->Bool, record_)) {
			result = C;
			break;
		}
		//C = C->pChain;
	}
	return result;
}

void DataEditor::FindExistTest(FrmlElem* Z, LinkD** LD)
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
		auto iZ = (FrmlElemFunction*)Z;
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

bool DataEditor::TestAccRight(std::string& S)
{
	if (UserCode == 0) { return true; }
	//TODO: return OverlapByteStr((void*)(uintptr_t(S) + 5 + S->S.length()), &AccRight);
	return false;
}

bool DataEditor::ForNavigate(FileD* FD)
{
	auto result = true;
	if (UserCode == 0) return result;

	//StringListEl* S = FD->ViewNames;
	//while (S != nullptr) {
	//	if (TestAccRight(S)) return result;
	//	S = S->pChain;
	//}
	for (std::string& S : FD->ViewNames) {
		if (TestAccRight(S)) return result;
	}


	result = false;
	return result;
}

std::string DataEditor::GetFileViewName(FileD* FD, std::vector<std::string>& SL, size_t index_from)
{
	//if (*SL == nullptr) { return FD->Name; }
	//std::string result = "\x1"; // ^A
	//while (!TestAccRight(*SL)) *SL = (*SL)->pChain;
	//result += (*SL)->S;
	//do { *SL = (*SL)->pChain; } while (!(SL == nullptr || TestAccRight(*SL)));

	if (index_from >= SL.size()) { return FD->Name; }

	std::string result = "\x1"; // ^A
	for (size_t i = index_from; i <= SL.size(); i++) {
		if (TestAccRight(SL[i])) {
			result += SL[i];
			break;
		}
	}

	return result;
}

void DataEditor::SetPointTo(LinkD* LD, std::string* s1, std::string* s2)
{
	//KeyFldD* KF = LD->Args;
	//while (KF != nullptr) {
	for (KeyFldD* arg : LD->Args) {
		if (arg->FldD == (*CFld)->FldD) {
			s2 = s1;
			ss.Pointto = s2;
		}
		//KF = KF->pChain;
	}
}

void DataEditor::GetSel2S(std::string& s, std::string& s2, char C, WORD wh)
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

bool DataEditor::EquRoleName(pstring S, LinkD* LD)
{
	if (S == "") {
		return LD->ToFD->Name == LD->RoleName;
	}
	else {
		return S == LD->RoleName;
	}
}

bool DataEditor::EquFileViewName(FileD* FD, std::string S, EditOpt** EO)
{
	auto result = true;
	if (S[0] == 0x01) { // ^A
		S = S.substr(1, 255);
		//StringListEl* SL = FD->ViewNames;
		//while (SL != nullptr) {
		//	if (SL->S == S) {
		//		*EO = new EditOpt();
		//		(*EO)->UserSelFlds = true;
		//		RdUserView(FD, S, *EO);
		//		return result;
		//	}
		//	SL = SL->pChain;
		//}
		for (std::string& s : FD->ViewNames) {
			if (s == S) {
				*EO = new EditOpt();
				(*EO)->UserSelFlds = true;
				RdUserView(FD, S, *EO);
				return result;
			}
		}
	}
	else if (S == FD->Name) {
		*EO = new EditOpt();
		(*EO)->UserSelFlds = true;
		(*EO)->Flds = g_compiler->AllFldsList(FD, false);
		return result;
	}

	result = false;
	return result;
}

void DataEditor::UpwEdit(LinkD* LkD)
{
	wwmix ww;

	void* p = nullptr;
	std::string s1, s2;
	XString x;
	XString* px = nullptr;
	FieldDescr* F = nullptr;
	KeyFldD* KF = nullptr;
	XKey* K = nullptr;
	EditOpt* EO = nullptr;
	WORD Brk;
	LinkD* LD;
	MarkStore(p);
	int w = PushW(1, 1, TxtCols, TxtRows, true, true);
	file_d_->IRec = AbsRecNr(CRec());

	//EditD* EE = WriteParamsToE();
	std::unique_ptr<DataEditor> data_editor2 = std::make_unique<DataEditor>();

	if (LkD == nullptr) {
		for (LinkD* ld : LinkDRoot) {
			FileD* ToFD = ld->ToFD;
			if ((ld->FromFD == file_d_) && data_editor2->ForNavigate(ToFD)) {
				std::string s;
				std::string rn = ld->RoleName;
				if (ToFD->Name != rn) { s = "." + ld->RoleName; }
				/*SL = ToFD->ViewNames;
				do {
					s1 = data_editor2->GetFileViewName(ToFD, &SL) + s;
					ww.PutSelect(s1);
					data_editor2->CFld = this->CFld;
					data_editor2->SetPointTo(ld, &s1, &s2);
				} while (SL != nullptr);*/

				s1 = data_editor2->GetFileViewName(ToFD, ToFD->ViewNames, 0) + s;
				ww.PutSelect(s1);
				data_editor2->CFld = this->CFld;
				data_editor2->SetPointTo(ld, &s1, &s2);

				for (size_t i = 0; i < ToFD->ViewNames.size(); i++) {
					s1 = data_editor2->GetFileViewName(ToFD, ToFD->ViewNames, i) + s;
					ww.PutSelect(s1);
					data_editor2->CFld = this->CFld;
					data_editor2->SetPointTo(ld, &s1, &s2);
				}

			}
		}
		ss.Abcd = true;
		ww.SelectStr(0, 0, 35, "");
		if (Event.Pressed.KeyCombination() == __ESC) {
			PopW(w);
			ReleaseStore(&p);
			//ReadParamsFromE(EE);
			DisplEditWw();
			return;
		}

		data_editor2->GetSel2S(s1, s2, '.', 2);

		LD = nullptr;
		for (auto& ld : LinkDRoot) {
			if (ld->FromFD == this->file_d_
				&& data_editor2->EquRoleName(s2, ld)
				&& data_editor2->EquFileViewName(ld->ToFD, s1, &EO)) {
				LD = ld;
				data_editor2->SetFileD(ld->ToFD);
				break;
			}
		}

	}
	else {
		data_editor2->SetFileD(LkD->ToFD);
		LD = LkD;
		EO = new EditOpt();
		EO->UserSelFlds = false;
		std::string sl1;

		//std::vector<std::string> SL = LD->ToFD->ViewNames;
		//while (SL != nullptr) {
		//	if (data_editor2->TestAccRight(SL)) {
		//		SL1 = SL;
		//	}
		//	SL = SL->pChain;
		//}
		for (std::string& s : LD->ToFD->ViewNames) {
			if (data_editor2->TestAccRight(s)) {
				sl1 = s;
			}
		}

		if (sl1.empty()) {
			EO->Flds = g_compiler->AllFldsList(LD->ToFD, false);
		}
		else {
			RdUserView(LD->ToFD, sl1, EO);
		}
		EO->SetOnlyView = true;
	}

	// prepare DB key from current item
	x.PackKF(file_d_, LD->Args, record_);
	px = &x;
	K = LD->ToKey;

	if (EO->ViewKey == nullptr) {
		EO->ViewKey = K;
	}
	else if (&EO->ViewKey != &K) {
		px = nullptr;
	}

	if (data_editor2->SelFldsForEO(EO, nullptr)) {
		EditReader* reader = new EditReader();
		reader->NewEditD(LD->ToFD, EO, data_editor2->record_);
		data_editor2->edit_ = reader->GetEditD();
		data_editor2->edit_->ShiftF7_link = LkD;
		data_editor2->edit_->ShiftF7_caller = edit_;
		if (data_editor2->OpenEditWw()) {
			data_editor2->RunEdit(px, Brk);
		}
		SaveFiles();
		//PopEdit();
		delete reader; reader = nullptr;
	}

	PopW(w);
	ReleaseStore(&p);
	//ReadParamsFromE(EE);
	DisplEditWw();
}

void DataEditor::DisplChkErr(ChkD* C)
{
	LinkD* LD = nullptr;

	FindExistTest(C->Bool, &LD);
	if (!C->Warning && (LD != nullptr) && ForNavigate(LD->ToFD) && (*CFld)->Ed(IsNewRec)) {
		FileD* cf = file_d_;
		uint8_t* cr = record_;

		int n = 0;

		BYTE* rec = nullptr;
		bool b = LinkUpw(file_d_, LD, n, false, record_, &rec);

		delete[] rec; rec = nullptr;
		file_d_ = cf; record_ = cr;

		if (!b) {
			if (params_->NoShiftF7Msg) {
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
	SetMsgPar(RunShortStr(file_d_, C->TxtZ, record_));
	WrLLF10Msg(110);
	if (Event.Pressed.KeyCombination() == __F1) {
		Help(file_d_->ChptPos.rdb, C->HelpName, false);
	}
	else if (Event.Pressed.KeyCombination() == __SHIFT_F7) {
		UpwEdit(LD);
	}
}

bool DataEditor::OldRecDiffers()
{
	XString x; FieldDescr* f = nullptr;
	auto result = false;
	if (IsCurrChpt(file_d_) || (
#ifdef FandSQL
		!file_d_->IsSQLFile &&
#endif 
		(!file_d_->FF->NotCached()))) return result;
	uint8_t* rec = file_d_->GetRecSpace();
#ifdef FandSQL
	if (file_d_->IsSQLFile) {
		x.S = WK->NrToStr(CRec); Strm1->KeyAcc(WK, @x); f = file_d_->FldD;
		while (f != nullptr) {
			/* !!! with f^ do!!! */ if (Flg && f_Stored != 0) && (field_type != 'T') and
				(CompArea(Pchar(rec) + Displ, Pchar(edit_->OldRecPtr) + Displ, NBytes) != ord(_equ)) then
				goto label1;
			f = f->pChain;
		}
		goto label2;
	}
	else
#endif

		file_d_->ReadRec(edit_->LockedRec, rec);
	if (CompArea(rec, edit_->OldRecPtr, file_d_->FF->RecLen) != _equ) {
	label1:
		file_d_->DelAllDifTFlds(edit_->NewRecPtr, edit_->OldRecPtr);
		Move(rec, edit_->NewRecPtr, file_d_->FF->RecLen);
		params_->WasUpdated = false;
		result = true;
	}
label2:
	file_d_->ClearRecSpace(rec);
	delete[] rec; rec = nullptr;

	return result;
}

bool DataEditor::ExitCheck(bool MayDispl)
{
	auto result = false;
	for (auto& X : edit_->ExD) {
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

int DataEditor::UpdateIndexes()
{
	int N = 0;
	XString x;
	int NNew = edit_->LockedRec;
	XWKey* KSel = edit_->SelKey;

	if (IsNewRec) {
		NNew = file_d_->FF->NRecs + 1;
		file_d_->FF->XF->NRecs++;
	}
	else if (KSel != nullptr) {
		record_ = edit_->OldRecPtr;
		if (KSel->RecNrToPath(file_d_, x, NNew, record_)) {
			KSel->DeleteOnPath(file_d_);
			record_ = edit_->NewRecPtr;
			KSel->Insert(file_d_, NNew, false, record_);
		}
		record_ = edit_->NewRecPtr;
	}

	if (VK->RecNrToPath(file_d_, x, edit_->LockedRec, record_) && !params_->WasWK) {
		if (IsNewRec) {
			VK->InsertOnPath(file_d_, x, NNew);
			if (params_->Subset) {
				WK->InsertAtNr(file_d_, CRec(), NNew, record_);
			}
		}
		N = CRec();
	}
	else {
		if (!IsNewRec) {
			record_ = edit_->OldRecPtr;
			VK->Delete(file_d_, edit_->LockedRec, record_);
			if (params_->Subset) {
				WK->DeleteAtNr(file_d_, CRec());
			}
			record_ = edit_->NewRecPtr;
			x.PackKF(file_d_, VK->KFlds, record_);
			VK->Search(file_d_, x, true, N);
		}
		N = VK->PathToNr(file_d_);
		VK->InsertOnPath(file_d_, x, NNew);
		if (VK->InWork) {
			VK->NR++;
		}
		if (params_->Subset) {
			N = WK->InsertGetNr(file_d_, NNew, record_);
		}
	}

	WORD result = N;
	for (size_t i = 0; i < file_d_->Keys.size(); i++) {
		auto K = file_d_->Keys[i];
		if (K != VK) {
			if (!IsNewRec) {
				record_ = edit_->OldRecPtr;
				K->Delete(file_d_, edit_->LockedRec, record_);
			}
			record_ = edit_->NewRecPtr;
			K->Insert(file_d_, NNew, true, record_);
		}
	}
	record_ = edit_->NewRecPtr;
	return result;
}


bool DataEditor::WriteCRec(bool MayDispl, bool& Displ)
{
	int N = 0, CNew = 0;
	ImplD* ID = nullptr;
	double time = 0.0;
	LongStr* s = nullptr;
	ChkD* C = nullptr;
	LockMode OldMd = LockMode::NullMode;
	Displ = false;
	bool result = false;

	if (!params_->WasUpdated || !IsNewRec && EquOldNewRec()) {
		IsNewRec = false; params_->WasUpdated = false; result = true;
		UnLockRec(edit_);
		return result;
	}
	result = false;
	if (IsNewRec) {
		//ID = edit_->Impl;
		//while (ID != nullptr) {
		//	AssgnFrml(file_d_, record_, ID->FldD, ID->Frml, true, false);
		//	ID = ID->pChain;
		//}
		for (ImplD* id : edit_->Impl) {
			AssgnFrml(file_d_, record_, id->FldD, id->Frml, true, false);
		}
	}
	if (params_->MustCheck) {   /* repeat field checking */
		std::vector<EFldD*>::iterator D = edit_->FirstFld.begin();
		while (D != edit_->FirstFld.end()) {
			C = CompChk(*D, 'F');
			if (C != nullptr) {
				if (MayDispl) GotoRecFld(CRec(), D);
				else CFld = D;
				DisplChkErr(C);
				return result;
			}
			++D; // = (EFldD*)D->pChain;
		}
	}
	if (IsNewRec) {
		if (!LockWithDep(CrMode, NullMode, OldMd)) return result;
	}
	else if (!params_->EdRecVar) {
		if (!LockWithDep(WrMode, WrMode, OldMd)) return result;
		if (OldRecDiffers()) {
			UnLockRec(edit_);
			UnLockWithDep(OldMd);
			WrLLF10Msg(149);
			DisplRec(CRec());
			IVon();
			return result;
		}
	}
	if (params_->Subset
		&& !(params_->NoCondCheck || RunBool(file_d_, edit_->Cond, record_)
			&& CheckKeyIn(edit_))) {
		UnLockWithDep(OldMd);
		WrLLF10Msg(823);
		return result;
	}
	if (edit_->DownSet) {
		DuplOwnerKey();
		Displ = true;
	}
	if (!ExitCheck(MayDispl)) goto label1;
	if (params_->EdRecVar) goto label2;
#ifdef FandSQL
	if (file_d_->IsSQLFile) {
		if (UpdSQLFile) goto label2; else goto label1;
	}
#endif
	if (HasIndex) {   /* test duplicate keys */
		for (XKey* K : file_d_->Keys) {
			if (!K->Duplic && TestDuplKey(file_d_, K)) {
				UnLockWithDep(OldMd);
				DuplKeyMsg(K);
				return result;
			}
		}
	}
	file_d_->ClearDeletedFlag(record_);
	if (HasIndex) {
		file_d_->FF->TestXFExist();
		if (IsNewRec) {
			if (params_->AddSwitch
				&& !RunAddUpdate(file_d_, '+', nullptr, false, nullptr, nullptr, record_)) {
				goto label1;
			}
			CNew = UpdateIndexes();
			file_d_->CreateRec(file_d_->FF->NRecs + 1, record_);
		}
		else {
			if (params_->AddSwitch) {
				if (!RunAddUpdate(file_d_, 'd', edit_->OldRecPtr, false, nullptr, nullptr, record_)) goto label1;
				UpdMemberRef(edit_->OldRecPtr, record_);
			}
			CNew = UpdateIndexes();
			file_d_->WriteRec(edit_->LockedRec, record_);
		}
		if (CNew != CRec()) {
			SetNewCRec(CNew, true);
			if (edit_->NRecs > 1) Displ = true;
		}
	}
	else if (IsNewRec) {
		N = edit_->LockedRec;
		if (N == 0) {
			N = CRec();
			if (N == CNRecs()) N = file_d_->FF->NRecs + 1;
			else if (params_->Subset) N = WK->NrToRecNr(file_d_, N);
		}
		if (params_->AddSwitch && !RunAddUpdate(file_d_, '+', nullptr, false, nullptr, nullptr, record_)) goto label1;
		if (ChptWriteCRec(this, edit_) != 0) goto label1;
		file_d_->CreateRec(N, record_);
		if (params_->Subset) {
			WK->AddToRecNr(file_d_, N, 1);
			WK->InsertAtNr(file_d_, CRec(), N, record_);
		}
	}
	else {
		if (params_->AddSwitch) {
			if (!RunAddUpdate(file_d_, 'd', edit_->OldRecPtr, false, nullptr, nullptr, record_)) goto label1;
			UpdMemberRef(edit_->OldRecPtr, record_);
		}
		WORD chptWrite = ChptWriteCRec(this, edit_);
		switch (chptWrite) {
		case 1: {
			goto label1;
			break;
		}
		case 2: {
			// are old and new text positions same?
			if ((*(int*)((char*)edit_->OldRecPtr + ChptTxt->Displ) == *(int*)((char*)record_ + ChptTxt->Displ)) && PromptYN(157)) {
				s = file_d_->loadLongS(ChptTxt, record_);
				TWork.Delete(ClpBdPos);
				ClpBdPos = TWork.Store(s->A, s->LL);
				delete s; s = nullptr;
			}
			UndoRecord();
			goto label1;
		}
		}
		file_d_->WriteRec(edit_->LockedRec, record_);
	}
	time = Today() + CurrTime();
	if (IsNewRec) WrJournal('+', record_, time);
	else {
		WrJournal('O', edit_->OldRecPtr, time);
		WrJournal('N', record_, time);
	}
label2:
	if (!IsNewRec && !params_->NoDelTFlds) {
		file_d_->DelAllDifTFlds(edit_->OldRecPtr, edit_->NewRecPtr);
	}
	edit_->EdUpdated = true;
	params_->NoDelTFlds = false;
	IsNewRec = false;
	params_->WasUpdated = false;
	result = true;
	UnLockRec(edit_);
label1:
	UnLockWithDep(OldMd);
	return result;
}

void DataEditor::DuplFromPrevRec()
{
	if ((*CFld)->Ed(IsNewRec)) {
		FieldDescr* F = (*CFld)->FldD;
		LockMode md = RdMode;
		if (F->field_type == FieldType::TEXT) md = WrMode;
		md = file_d_->NewLockMode(md);
		SetWasUpdated(file_d_->FF, record_);

		uint8_t* rec = file_d_->GetRecSpace();
		RdRec(CRec() - 1);
		DuplFld(file_d_, file_d_, rec, edit_->NewRecPtr, edit_->OldRecPtr, F, F);
		file_d_->ClearRecSpace(rec);
		delete[] rec; rec = nullptr;

		file_d_->OldLockMode(md);
	}
}

void DataEditor::InsertRecProc(void* RP)
{
	GotoRecFld(CRec(), edit_->FirstFld.begin());
	IsNewRec = true;
	LockRec(false);
	if (RP != nullptr) {
		Move(RP, record_, file_d_->FF->RecLen);
	}
	else {
		file_d_->ZeroAllFlds(record_, false);
	}
	DuplOwnerKey();
	SetWasUpdated(file_d_->FF, record_);
	IVoff();
	MoveDispl(edit_->NRecs - 1, edit_->NRecs, edit_->NRecs - IRec);
	FirstEmptyFld = *CFld;
	DisplRec(IRec);
	IVon();
	NewDisplLL = true;
	NewRecExit();
}

void DataEditor::AppendRecord(void* RP)
{
	IVoff();
	IsNewRec = true;
	WORD Max = edit_->NRecs;
	CFld = edit_->FirstFld.begin();
	FirstEmptyFld = *CFld;
	if (IRec < Max) {
		IRec++;
		MoveDispl(Max - 1, Max, Max - IRec);
		DisplRec(IRec);
		IVon();
	}
	else if (Max == 1) {
		BaseRec++;
		DisplWwRecsOrPage(CPage, &RT);
	}
	else {
		BaseRec += Max - 1;
		IRec = 2;
		DisplAllWwRecs();
	}
	if (RP != nullptr) {
		Move(RP, record_, file_d_->FF->RecLen);
	}
	else {
		file_d_->ZeroAllFlds(record_, false);
	}
	DuplOwnerKey();
	DisplRecNr(CRec());
	SetWasUpdated(file_d_->FF, record_);
	LockRec(false);
	NewRecExit();
}

bool DataEditor::GotoXRec(XString* PX, int& N)
{
	bool result = false;
	LockMode md = file_d_->NewLockMode(RdMode);
	XKey* k = VK;
	if (params_->Subset) k = WK;
	if (params_->Subset || HasIndex) {
		result = k->SearchInterval(file_d_, *PX, false, N);
		N = k->PathToNr(file_d_);
	}
	else {
		result = file_d_->SearchKey(*PX, k, N, record_);
	}
	RdRec(CRec());
	GotoRecFld(N, CFld);
	file_d_->OldLockMode(md);
	return result;
}

std::vector<EFldD*>::iterator DataEditor::FindEFld(FieldDescr* F)
{
	std::vector<EFldD*>::iterator D = edit_->FirstFld.begin();
	while (D != edit_->FirstFld.end()) {
		if ((*D)->FldD == F) {
			break;
		}
		++D; // = D->pChain;
	}

	return D;
}

void DataEditor::CreateOrErr(bool create, void* RP, int N)
{
	if (create) {
		if (N > CNRecs()) {
			AppendRecord(RP);
		}
		else {
			InsertRecProc(RP);
		}
	}
	else if (!params_->NoSrchMsg) {
		WrLLF10Msg(118);
	}
}

bool DataEditor::PromptSearch(bool create)
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
	FileD* FD = file_d_;
	XKey* K = VK;
	if (params_->Subset) K = WK;
	std::vector<KeyFldD*>::iterator KF = K->KFlds.begin();

	void* RP = file_d_->GetRecSpace();
	//record_ = RP;

	file_d_->ZeroAllFlds(record_, false);
	x.Clear();
	bool li = params_->F3LeadIn && !IsNewRec;
	int w = PushW(1, TxtRows, TxtCols, TxtRows, true, false);
	if (KF == K->KFlds.end()) {
		result = true;
		//record_ = edit_->NewRecPtr;
		PopW(w);
		ReleaseStore(&RP);
		return result;
	}
	if (HasIndex && edit_->DownSet && (VK == edit_->DownKey)) {
		FileD* FD2 = edit_->DownLD->ToFD;
		void* RP2 = edit_->DownRecPtr;
		std::vector<KeyFldD*>::iterator KF2 = edit_->DownLD->ToKey->KFlds.begin();

		while (KF2 != edit_->DownLD->ToKey->KFlds.end()) {
			F = (*KF)->FldD;
			FieldDescr* F2 = (*KF2)->FldD;
			switch (F->frml_type) {
			case 'S': {
				s = FD2->loadS(F2, record_);
				x.StoreStr(s, *KF);
				file_d_->saveS(F, s, RP);
				break;
			}
			case 'R': {
				r = FD2->loadR(F2, record_);
				x.StoreReal(r, *KF);
				file_d_->saveR(F, r, RP);
				break;
			}
			case 'B': {
				b = FD2->loadB(F2, record_);
				x.StoreBool(b, *KF);
				file_d_->saveB(F, b, RP);
				break;
			}
			}
			++KF2; // = KF2->pChain;
			++KF;  // = KF->pChain;
		}
	}

	if (KF == K->KFlds.end()) {
		result = true;
		PopW(w);
		ReleaseStore(&RP);
		return result;
	}

	while (KF != K->KFlds.end()) {
		F = (*KF)->FldD;
		if (li) {
			std::vector<EFldD*>::iterator D = FindEFld(F);
			if (D != edit_->FirstFld.end()) {
				GotoRecFld(CRec(), D);
			}
		}
		TextAttr = screen.colors.pTxt;
		screen.ScrWrStr(1, TxtRows, F->Name + ":", TextAttr);
		screen.GotoXY((WORD)F->Name.length() + 2, TxtRows);
		ClrEol(TextAttr);
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
			pos = FieldEdit(F, nullptr, LWw, pos, s, r, false, true, li, edit_->WatchDelay);
			const XString x_old = x;
			if (Event.Pressed.KeyCombination() == __ESC || (Event.What == evKeyDown)) {
				PopW(w);
				ReleaseStore(&RP);
				return result;
			}
			switch (F->frml_type) {
			case 'S': {
				x.StoreStr(s, *KF);
				file_d_->saveS(F, s, RP);
				break;
			}
			case 'R': {
				x.StoreReal(r, *KF);
				file_d_->saveR(F, r, RP);
				break;
			}
			case 'B': {
				b = s[0] = AbbrYes;
				x.StoreBool(b, *KF);
				file_d_->saveB(F, b, RP);
				break;
			}
			}
			if (li) {
				found = GotoXRec(&x, n);
				if ((pos == 0) && (F->frml_type == 'S')) {
					x = x_old;
					x.StoreStr(file_d_->loadS(F, edit_->NewRecPtr), *KF);
				}
				if (pos != 0) {
					x = x_old;
					continue;
				}
			}
			break;
		}
		++KF; // = KF->pChain;
	}

	if (li) {
		if (!found) CreateOrErr(create, RP, n);
	}
	else if (IsNewRec) {
		Move(RP, record_, file_d_->FF->RecLen);
	}
	else if (!GotoXRec(&x, n)) {
		CreateOrErr(create, RP, n);
	}
	result = true;

	PopW(w);
	ReleaseStore(&RP);
	return result;
}

bool DataEditor::PromptAndSearch(bool create)
{
	bool result = false;
	if (VK == nullptr) {
		WrLLF10Msg(111);
		return result;
	}
	result = PromptSearch(create);
	GotoRecFld(CRec(), edit_->FirstFld.begin());
	return result;
}

void DataEditor::PromptGotoRecNr()
{
	wwmix ww;

	int n;
	WORD i = 1;
	std::string text;
	bool del = true;
	do {
		ww.PromptLL(122, text, i, del, false, false);
		if (Event.Pressed.KeyCombination() == __ESC) return;
		val(text, n, i);
		del = false;
	} while (i != 0);

	GotoRecFld(n, CFld);
}

void DataEditor::CheckFromHere()
{
	std::vector<EFldD*>::iterator D = CFld;
	int N = CRec();
	LockMode md = file_d_->NewLockMode(RdMode);

	while (true) {
		if (!file_d_->DeletedFlag(record_))
			while (D != edit_->FirstFld.end()) {
				ChkD* C = CompChk(*D, '?');
				if (C != nullptr) {
					if (BaseRec + edit_->NRecs - 1 < N) {
						BaseRec = N;
					}
					IRec = N - BaseRec + 1;
					CFld = D;
					DisplWwRecsOrPage(CPage, &RT);
					file_d_->OldLockMode(md);
					DisplChkErr(C);
					return;
				}
				++D; // D = D->pChain;
			}
		if (N < CNRecs()) {
			N++;
			DisplRecNr(N);
			RdRec(N);
			D = edit_->FirstFld.begin();
			continue;
		}
		break;
	}

	RdRec(CRec());
	DisplRecNr(CRec());
	file_d_->OldLockMode(md);
	WrLLF10Msg(120);
}

void DataEditor::Sorting()
{
	std::vector<KeyFldD*> SKRoot;
	void* p = nullptr;
	LockMode md;
	SaveFiles();
	MarkStore(p);

	if (!g_compiler->PromptSortKeys(edit_->FD, edit_->Flds, SKRoot) || (SKRoot.empty())) {
		ReleaseStore(&p);
		record_ = edit_->NewRecPtr;
		DisplAllWwRecs();
		return;
	}

	if (!file_d_->TryLockMode(ExclMode, md, 1)) {
		ReleaseStore(&p);
		record_ = edit_->NewRecPtr;
		DisplAllWwRecs();
		return;
	}

	try {
		file_d_->FF->SortAndSubst(SKRoot);
		edit_->EdUpdated = true;
	}
	catch (std::exception&) {
		file_d_ = edit_->FD;
		file_d_->OldLockMode(md);
	}

	ReleaseStore(&p);
	record_ = edit_->NewRecPtr;
	DisplAllWwRecs();
}

void DataEditor::AutoReport()
{
	void* p = nullptr; RprtOpt* RO = nullptr;
	FileUseMode UM = Closed;
	MarkStore(p); RO = g_compiler->GetRprtOpt();
	RO->FDL.FD = file_d_;
	RO->Flds = edit_->Flds;
	if (params_->Select) {
		RO->FDL.Cond = edit_->Bool;
		RO->CondTxt = edit_->BoolTxt;
	}
	if (params_->Subset) {
		RO->FDL.ViewKey = WK;
	}
	else if (HasIndex) {
		RO->FDL.ViewKey = VK;
	}
	PrintView = false;
	const std::unique_ptr auto_report = std::make_unique<ReportGenerator>();
	if (auto_report->SelForAutoRprt(RO)) {
		SpecFDNameAllowed = IsCurrChpt(file_d_);
		auto_report->RunAutoReport(RO);
		SpecFDNameAllowed = false;
	}
	ReleaseStore(&p);
	std::unique_ptr<TextEditor> text_editor = std::make_unique<TextEditor>();
	text_editor->ViewPrinterTxt();
	record_ = edit_->NewRecPtr;
}

void DataEditor::AutoGraph()
{
#ifdef FandGraph
	FrmlElem* Bool = nullptr;
	if (params_->Select) Bool = edit_->Bool;
	XKey* K = nullptr;
	if (params_->Subset) K = WK;
	else if (HasIndex) K = VK;
	RunAutoGraph(edit_->Flds, K, Bool);
#endif
	file_d_ = edit_->FD;
	record_ = edit_->NewRecPtr;
}

bool DataEditor::IsDependItem()
{
	if (!IsNewRec && (edit_->NEdSet == 0)) return false;
	//DepD* Dp = CFld->Dep;
	//while (Dp != nullptr) {
	//	if (RunBool(Dp->Bool)) {
	//		return true;
	//	}
	//	Dp = Dp->pChain;
	//}

	for (const DepD* dep : (*CFld)->Dep) {
		if (RunBool(file_d_, dep->Bool, record_)) {
			return true;
		}
	}

	return false;
}

void DataEditor::SetDependItem()
{
	for (const DepD* dep : (*CFld)->Dep) {
		if (RunBool(file_d_, dep->Bool, record_)) {
			AssignFld((*CFld)->FldD, dep->Frml);
			return;
		}
	}
}

void DataEditor::SwitchToAppend()
{
	GotoRecFld(CNRecs(), CFld);
	params_->Append = true;
	AppendRecord(nullptr);
	NewDisplLL = true;
}

bool DataEditor::CheckForExit(bool& Quit)
{
	auto result = false;
	for (auto& X : edit_->ExD) {
		bool b = FieldInList((*CFld)->FldD, X->Flds);
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

bool DataEditor::FldInModeF3Key(FieldDescr* F)
{
	auto result = false;
	if ((F->Flg & f_Stored) == 0) return result;

	//KeyFldD* KF = VK->KFlds;
	//while (KF != nullptr) {
	for (KeyFldD* KF : VK->KFlds) {
		if (KF->FldD == F) {
			result = true;
			return result;
		}
		//KF = KF->pChain;
	}

	return result;
}

bool DataEditor::IsSkipFld(EFldD* D)
{
	return !D->Tab &&
		(edit_->NTabsSet > 0 || (D->FldD->Flg & f_Stored) == 0 || params_->OnlySearch && FldInModeF3Key(D->FldD));
}

bool DataEditor::ExNotSkipFld()
{
	bool result = false;
	if (edit_->NFlds == 1) return result;

	std::vector<EFldD*>::iterator D = edit_->FirstFld.begin();
	while (D != edit_->FirstFld.end()) {
		if ((D != CFld) && !IsSkipFld(*D)) {
			result = true;
			break;
		}
		++D; // D = D->pChain;
	}

	return result;
}

bool DataEditor::CtrlMProc(WORD Mode)
{
	int i = 0;
	bool b = false;
	ChkD* C = nullptr;
	EdExitD* X = nullptr;
	WORD Brk = 0, NR = 0;
	bool displ = false, skip = false, Quit = false, WasNewRec = false;
	LockMode md;
	char Typ = '\0';

	int OldCRec = CRec();
	std::vector<EFldD*>::iterator OldCFld = CFld;

	bool result = true;
	if (Mode == 0 /*only bypass unrelevant fields*/) {
		goto label2;
	}
label1:
	if (IsFirstEmptyFld()) {
		//FirstEmptyFld =	FirstEmptyFld->pChain;
		++FirstEmptyFld;
	}
	Quit = false;
	if (!CheckForExit(Quit)) return result;
	TextAttr = edit_->dHiLi;
	DisplFld(*CFld, IRec, TextAttr);
	if (params_->ChkSwitch) {
		if (Mode == 1 || Mode == 3) Typ = '?';
		else Typ = 'F';
		C = CompChk(*CFld, Typ);
		if (C != nullptr) {
			DisplChkErr(C);
			if (!C->Warning) return result;
		}
	}
	if (params_->WasUpdated && !params_->EdRecVar && HasIndex) {
		//KL = CFld->KL;
		//while (KL != nullptr) {
		for (XKey* key : (*CFld)->KL) {
			md = file_d_->NewLockMode(RdMode);
			b = TestDuplKey(file_d_, key);
			file_d_->OldLockMode(md);
			if (b) {
				DuplKeyMsg(key);
				return result;
			}
			//KL = KL->pChain;
		}
	}
	if (Quit && !IsNewRec && (Mode == 1 || Mode == 3)) {
		EdBreak = 12;
		result = false;
		return result;
	}

	++CFld;
	if (CFld != edit_->FirstFld.end()) {
		--CFld;
		GotoNextRecFld(CRec(), CFld);
		if (Mode == 1 || Mode == 3) Mode = 0;
	}
	else {
		--CFld;
		WasNewRec = IsNewRec;
		Mode = 0; NR++;
		if (!WriteCRec(true, displ)) return result;
		if (displ) DisplAllWwRecs();
		else SetRecAttr(IRec);
		if (params_->Only1Record)
			if (params_->NoESCPrompt) {
				EdBreak = 0;
				return false;
			}
			else {
				params_->Append = false;
				goto label3;
			}
		if (params_->OnlySearch) {
			params_->Append = false;
			goto label3;
		}
		if (params_->Append) AppendRecord(nullptr);
		else {
			if (WasNewRec) NewDisplLL = true;
			if (CRec() < CNRecs())
				if (params_->Select) {
					for (i = CRec() + 1; i <= CNRecs(); i++) {
						if (KeyPressed() && (ReadKey() != 'M') && PromptYN(23)) goto label4;
						RdRec(i);
						DisplRecNr(i);
						if (!file_d_->DeletedFlag(record_) && RunBool(file_d_, edit_->Bool, record_)) {
							RdRec(CRec());
							GotoRecFld(i, edit_->FirstFld.begin());
							goto label2;
						}
					}
				label4:
					RdRec(CRec());
					DisplRecNr(CRec());
					GotoRecFld(OldCRec, OldCFld);
					Beep(); Beep();
					return result;
				}
				else GotoRecFld(CRec() + 1, edit_->FirstFld.begin());
			else {
			label3:
				GotoRecFld(CRec(), OldCFld);
				Beep(); Beep();
				return result;
			}
		}
	}
label2:
	skip = false;
	displ = false;
	if (IsFirstEmptyFld()) {
		if (((*CFld)->Impl != nullptr) && LockRec(true)) {
			AssignFld((*CFld)->FldD, (*CFld)->Impl);
			displ = true;
		}
		if ((*CFld)->Dupl && (CRec() > 1) && LockRec(true)) {
			DuplFromPrevRec();
			displ = true; skip = true;
		}
	}
	if (IsDependItem() && LockRec(true)) {
		SetDependItem();
		displ = true; skip = true;
	}
	if (IsSkipFld(*CFld)) skip = true;
	if ((*CFld)->Tab) skip = false;
	if (displ) {
		TextAttr = edit_->dHiLi;
		DisplFld(*CFld, IRec, TextAttr);
	}
	if (Mode == 2 /*bypass all remaining fields of the record */) goto label1;
	if (skip && ExNotSkipFld() && (NR <= 1)) goto label1;
	return result;
}

bool DataEditor::GoPrevNextRec(short Delta, bool Displ)
{
	int i = 0; LockMode md; WORD w = 0, Max = 0;
	int OldBaseRec = 0;
	auto result = false;
	if (params_->EdRecVar) return result;
	md = file_d_->NewLockMode(RdMode);
	i = CRec();
	if (Displ) IVoff();
label0:
	i += Delta;
	if ((i > 0) && (i <= CNRecs())) {
		RdRec(i);
		if (Displ) DisplRecNr(i); // zobrazi cislo zaznamu v hlavicce
		if (!params_->Select || !file_d_->DeletedFlag(record_) && RunBool(file_d_, edit_->Bool, record_)) goto label2;
		if (KeyPressed()) {
			w = ReadKey();
			if (((Delta > 0) && (w != __DOWN) && (w != __CTRL_END) && (w != __PAGEDOWN)
				|| (Delta < 0) && (w != __UP) && (w != __CTRL_HOME) && (w != __PAGEUP))
				&& PromptYN(23)) goto label1;
		}
		goto label0;
	}
	if (params_->Select) WrLLF10Msg(16);
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
		Max = edit_->NRecs;
		int D = BaseRec - OldBaseRec;
		if (abs(D) > 0) {
			DisplWwRecsOrPage(CPage, &RT);
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
	file_d_->OldLockMode(md);
	return result;
}

bool DataEditor::GetChpt(pstring Heslo, int& NN)
{
	pstring s(12);

	for (int j = 1; j <= file_d_->FF->NRecs; j++) {
		file_d_->ReadRec(j, record_);
		if (IsCurrChpt(file_d_)) {
			s = OldTrailChar(' ', file_d_->loadS(ChptName, record_));
			short i = s.first('.');
			if (i > 0) s.Delete(i, 255);
			if (EquUpCase(Heslo, s)) {
				NN = j;
				return true;
			}
		}
		else {
			s = OldTrailChar(' ', file_d_->loadS(file_d_->FldD.front(), record_));
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

void DataEditor::SetCRec(int I)
{
	if (I > BaseRec + edit_->NRecs - 1) BaseRec = I - edit_->NRecs + 1;
	else if (I < BaseRec) BaseRec = I;
	IRec = I - BaseRec + 1;
	RdRec(CRec());
}

void DataEditor::UpdateEdTFld(LongStr* S)
{
	LockMode md;
	if (!params_->EdRecVar) md = file_d_->NewLockMode(WrMode);
	SetWasUpdated(file_d_->FF, edit_->NewRecPtr);
	file_d_->FF->DelDifTFld((*CFld)->FldD, edit_->NewRecPtr, edit_->OldRecPtr);
	file_d_->saveLongS((*CFld)->FldD, S, edit_->NewRecPtr);
	if (!params_->EdRecVar) {
		file_d_->OldLockMode(md);
	}
}

void DataEditor::UpdateTxtPos(WORD TxtPos)
{
	LockMode md;
	if (IsCurrChpt(file_d_)) {
		md = file_d_->NewLockMode(WrMode);
		SetWasUpdated(file_d_->FF, record_);
		file_d_->saveR(ChptTxtPos, (short)TxtPos, record_);
		file_d_->OldLockMode(md);
	}
}

bool DataEditor::EditFreeTxt(FieldDescr* F, std::string ErrMsg, bool Ed, WORD& Brk)
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
	WORD R1 = 0, OldTxtPos = 0;
	WORD TxtPos = 0, CtrlMsgNr = 0;
	WORD C = 0, LastLen = 0;
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
	if (edit_->Head.empty()) w = PushW(1, 1, TxtCols, 1);
	if (edit_->params_->TTExit) {
		TxtMsgS.Head = "";
		TxtMsgS.Last = edit_->Last;
		TxtMsgS.CtrlLast = edit_->CtrlLast;
		TxtMsgS.AltLast = edit_->AltLast;
		TxtMsgS.ShiftLast = edit_->ShiftLast;
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
	if (IsCurrChpt(file_d_)) {
		HdTxt = file_d_->loadS(ChptTyp, record_) + ':' + file_d_->loadS(ChptName, record_) + HdTxt;
		TxtPos = trunc(file_d_->loadR(ChptTxtPos, record_));
		Breaks = BreakKeys2;
		CtrlMsgNr = 131;
	}
	else {
		CtrlMsgNr = 151;
		if (file_d_ == CRdb->help_file) {
			Breaks = BreakKeys1;
		}
		else {
			Breaks = BreakKeys;
		}
	}
	R1 = edit_->FrstRow;
	if ((R1 == 3) && params_->WithBoolDispl) R1 = 2;
	screen.Window(edit_->FrstCol, R1, edit_->LastCol, edit_->LastRow);
	TextAttr = screen.colors.tNorm;
	Kind = 'V';
	OldTxtPos = TxtPos;
	if (Ed) LockRec(false);
	if ((F->Flg & f_Stored) != 0) {
		S = file_d_->loadLongS(F, record_);
		if (Ed) Kind = 'T';
	}
	else {
		std::string std_s = RunStdStr(file_d_, F->Frml, record_);
		S = new LongStr(std_s.length());
		S->LL = std_s.length();
		memcpy(S->A, std_s.c_str(), S->LL);
	}
label2:
	if (params_->TTExit) {
		X = edit_->ExD;
	}
	else {
		X.clear();
	}

	Upd = false;
	std::unique_ptr<TextEditor> editor = std::make_unique<TextEditor>();
	result =
		editor->EditText(Kind, MemoT, HdTxt, ErrMsg, S, MaxLStrLen, TxtPos, TxtXY, Breaks, X,
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

	if (C == __ALT_EQUAL) {
		C = __ESC;
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
	case 'U': {
		delete S; S = nullptr;
		TxtXY = 0;
		goto label1;
		break;
	}
	}
	screen.Window(1, 1, TxtCols, TxtRows);

	if (WasUpd) UpdateEdTFld(S);
	if ((OldTxtPos != TxtPos) && !Srch) UpdateTxtPos(TxtPos);
	delete S; S = nullptr;

	if (Ed && !params_->WasUpdated) {
		UnLockRec(edit_);
	}
	if (Srch) {
		if (WriteCRec(false, Displ)) {
			goto label31;
		}
	}

	switch (C) {
	case __F9: {
		if (WriteCRec(false, Displ)) {
			SaveFiles();
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
		if (IsCurrChpt(file_d_) || (file_d_ == CRdb->help_file)) {
			if ((iStk < maxStk) && WriteCRec(false, Displ) && GetChpt(heslo, i)) {
				params_->Append = false;
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
			params_->Append = false;
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
		break;
	}
	case __ALT_F1: {
		heslo = file_d_->loadS(ChptTyp, record_);
	label3:
		Help((RdbD*)HelpFD, heslo, false);
		goto label4;
		break;
	}
	} // switch end

	if ((C > 0xFF) && WriteCRec(false, Displ)) {
		params_->Append = false;
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
		//WriteParamsToE();
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

bool DataEditor::EditItemProc(bool del, bool ed, WORD& Brk)
{
	std::string Txt;
	double R = 0; bool b = false; ChkD* C = nullptr;
	FieldDescr* F = (*CFld)->FldD;
	auto result = true;
	if (F->field_type == FieldType::TEXT) {
		if (!EditFreeTxt(F, "", ed, Brk)) {
			return false;
		}
	}
	else {
		TextAttr = edit_->dHiLi;
		Txt = DecodeField(file_d_, F, (*CFld)->FldD->L, record_);
		screen.GotoXY((*CFld)->Col, FldRow(*CFld, IRec));
		unsigned int wd = 0;
		if (file_d_->FF->NotCached()) {
			wd = edit_->WatchDelay;
		}
		FieldEdit(F, (*CFld)->Impl, (*CFld)->L, 1, Txt, R, del, ed, false, wd);
		if (Event.Pressed.KeyCombination() == __ESC || !ed) {
			DisplFld(*CFld, IRec, TextAttr);
			if (ed && !params_->WasUpdated) UnLockRec(edit_);
			return result;
		}
		SetWasUpdated(file_d_->FF, record_);
		switch (F->frml_type) {
		case 'B': file_d_->saveB(F, toupper(Txt[0]) == AbbrYes, record_); break;
		case 'S': file_d_->saveS(F, Txt, record_); break;
		case 'R': file_d_->saveR(F, R, record_); break;
		}
	}
	if (Brk == 0) result = CtrlMProc(1);
	return result;
}

void DataEditor::SetSwitchProc()
{
	bool B; WORD N, iMsg;
	iMsg = 104; if (params_->EdRecVar) goto label1; iMsg = 101;
	if (params_->MustCheck) if (params_->MustAdd) goto label1;
	else { iMsg = 102; goto label1; }
	iMsg = 103;
	if (params_->MustAdd) goto label1;
	iMsg = 100;
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
		if (params_->Select) params_->Select = false;
		else if (edit_->Bool != nullptr) params_->Select = true;
		DisplBool();
		NewDisplLL = true;
		SetNewWwRecAttr();
		break;
	}
	case 2: {
		if (((*CFld)->FldD->Flg & f_Stored) != 0) {
			B = (*CFld)->Dupl;
			(*CFld)->Dupl = !B;
			DisplTabDupl();
			if (B) edit_->NDuplSet--;
			else edit_->NDuplSet++;
		}
		break;
	}
	case 3: {
		B = (*CFld)->Tab;
		(*CFld)->Tab = !B;
		DisplTabDupl();
		if (B) edit_->NTabsSet--;
		else edit_->NTabsSet++;
		break;
	}
	case 4: {
		params_->AddSwitch = !params_->AddSwitch;
		NewDisplLL = true;
		break;
	}
	case 5: {
		if (!params_->MustCheck) {
			params_->ChkSwitch = !params_->ChkSwitch;
			NewDisplLL = true;
		}
		break;
	}
	case 6: {
		params_->WarnSwitch = !params_->WarnSwitch;
		NewDisplLL = true;
		break;
	}
	}
}

void DataEditor::PromptSelect()
{
	wwmix ww;
	std::string Txt;
	if (params_->Select) {
		Txt = edit_->BoolTxt;
	}
	else {
		Txt = "";
	}
	if (IsCurrChpt(file_d_)) {
		ReleaseFilesAndLinksAfterChapter(edit_);
	}
	ReleaseStore(&edit_->AfterE);
	ww.PromptFilter(Txt, &edit_->Bool, &edit_->BoolTxt);
	if (edit_->Bool == nullptr) {
		params_->Select = false;
	}
	else {
		params_->Select = true;
	}
	DisplBool();
	SetNewWwRecAttr();
	NewDisplLL = true;
}

void DataEditor::SwitchRecs(short Delta)
{
	LockMode md; int n1, n2;
	uint8_t* p1; uint8_t* p2;
	XString x1, x2;
#ifdef FandSQL
	if (file_d_->IsSQLFile) return;
#endif
	if (params_->NoCreate && params_->NoDelete || params_->WasWK) return;
	if (!file_d_->TryLockMode(WrMode, md, 1)) return;
	p1 = file_d_->GetRecSpace();
	p2 = file_d_->GetRecSpace();

	n1 = AbsRecNr(CRec());
	file_d_->ReadRec(n1, p1);
	if (HasIndex) x1.PackKF(file_d_, VK->KFlds, p1);
	n2 = AbsRecNr(CRec() + Delta);
	file_d_->ReadRec(n2, p2);
	if (HasIndex) {
		x2.PackKF(file_d_, VK->KFlds, p2);
		if (x1.S != x2.S) {
			goto label1;
		}
	}
	file_d_->WriteRec(n1, p2);
	file_d_->WriteRec(n2, p1);
	if (HasIndex) {
		for (XKey* k : file_d_->Keys) {
			if (k != VK) {
				k->Delete(file_d_, n1, p1);
				k->Delete(file_d_, n2, p2);
				k->Insert(file_d_, n2, true, p1);
				k->Insert(file_d_, n1, true, p2);
			}
		}
	}
	SetNewCRec(CRec() + Delta, true);
	DisplAllWwRecs();
	DisplRecNr(CRec());
	edit_->EdUpdated = true;
	if (IsCurrChpt(file_d_)) SetCompileAll();
label1:
	file_d_->OldLockMode(md);
	delete[] p1; p1 = nullptr;
}

bool DataEditor::FinArgs(LinkD* LD, FieldDescr* F)
{
	auto result = true;
	for (auto& arg : LD->Args) {
		if (arg->FldD == F) return result;
	}
	result = false;
	return result;
}

bool DataEditor::SelFldsForEO(EditOpt* EO, LinkD* LD)
{
	// TODO: this method is bad, need to investigate what happens here ...

	wwmix ww;

	void* p = nullptr;
	bool result = true;
	if (EO->Flds.empty()) return result;
	//FieldListEl* FL = options->Flds;
	if (!EO->UserSelFlds) {
		if (LD != nullptr) {
			//FieldListEl* FL1 = FieldList(options->Flds);
			//while (FL != nullptr) {
			for (FieldDescr* FL : EO->Flds) {
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
	for (FieldDescr* F : EO->Flds) {
		if ((LD == nullptr) || !FinArgs(LD, F)) {
			if (!F->isStored()) {
				ww.PutSelect(static_cast<char>(SelMark) + F->Name);
			}
			else {
				ww.PutSelect(F->Name);
			}
		}
		//FL = FL->pChain;
	}
	if (EO->Flds.empty()) {
		WrLLF10Msg(156);
	}
	else {
		ww.SelFieldList(file_d_, 36, true, EO->Flds);
	}
	if (EO->Flds.empty()) {
		ReleaseStore(&p);
		result = false;
	}
	return result;
}

void DataEditor::ImbeddEdit()
{
	wwmix ww;

	void* p = nullptr;
	std::string s1, s2;
	WORD Brk;
	//std::vector<std::string> SL;
	EditOpt* EO = nullptr;
	RdbD* R = nullptr; int w = 0;

	MarkStore(p);
	w = PushW(1, 1, TxtCols, TxtRows, true, true);
	file_d_->IRec = AbsRecNr(CRec());

	//EditD* EE = WriteParamsToE();
	std::unique_ptr<DataEditor> data_editor2 = std::make_unique<DataEditor>();

	R = CRdb;
	while (R != nullptr) {
		//FD = R->v_files;
		//while (FD != nullptr) {
		for (size_t i = 1; i < R->v_files.size(); i++) {
			FileD* f = R->v_files[i];
			if (data_editor2->ForNavigate(f)) {
				//SL = FD->ViewNames;
				//do {
				//	std::string s = data_editor2->GetFileViewName(FD, &SL);
				//	if (R != CRdb) {
				//		s = R->v_files->Name + "." + s;
				//	}
				//	ww.PutSelect(s);
				//} while (SL != nullptr);
				for (size_t i = 0; i < f->ViewNames.size(); i++) {
					std::string s = data_editor2->GetFileViewName(f, f->ViewNames, i);
					if (R != CRdb) {
						s = R->v_files[0]->Name + "." + s;
					}
					ww.PutSelect(s);
				}
			}
			//FD = FD->pChain;
		}
		R = R->ChainBack;
	}
	ss.Abcd = true;
	ww.SelectStr(0, 0, 35, "");
	if (Event.Pressed.KeyCombination() == __ESC) {
		// do nothing
	}
	else {
		data_editor2->GetSel2S(s1, s2, '.', 1);
		R = CRdb;
		if (!s2.empty()) {
			std::string ss2 = s2;
			do {
				R = R->ChainBack;
			} while (R->v_files[0]->Name != ss2);
		}

		//data_editor2->file_d_ = R->v_files[0];
		//while (!data_editor2->EquFileViewName(file_d_, s1, &EO)) {
		//	data_editor2->file_d_ = data_editor2->file_d_->pChain;
		//}
		for (auto& v_file : R->v_files) {
			if (data_editor2->EquFileViewName(v_file, s1, &EO)) {
				data_editor2->file_d_ = v_file;
			}
		}

		if (data_editor2->SelFldsForEO(EO, nullptr)) {
			EditReader* reader = new EditReader();
			reader->NewEditD(data_editor2->file_d_, EO, data_editor2->record_);
			data_editor2->edit_ = reader->GetEditD();
			if (data_editor2->OpenEditWw()) {
				data_editor2->RunEdit(nullptr, Brk);
			}
			SaveFiles();
			//PopEdit();
			delete reader; reader = nullptr;
		}
	}

	PopW(w);
	ReleaseStore(&p);
	//ReadParamsFromE(EE);
	DisplEditWw();
}

void DataEditor::DownEdit()
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
	file_d_->IRec = AbsRecNr(CRec());

	//EditD* EE = WriteParamsToE();
	std::unique_ptr<DataEditor> data_editor2 = std::make_unique<DataEditor>();

	for (LinkD* ld : LinkDRoot) {
		FileD* FD = ld->FromFD;
		if ((ld->ToFD == file_d_) && data_editor2->ForNavigate(FD) && (ld->IndexRoot != 0)) {
			/*own key with equal beginning*/
			XKey* K = GetFromKey(ld);

			std::string s = data_editor2->GetFileViewName(FD, FD->ViewNames, 0);
			std::string kali = K->Alias;
			if (!K->Alias.empty()) {
				s += "/" + kali;
			}
			ww.PutSelect(s);

			for (size_t i = 0; i < FD->ViewNames.size(); i++) {
				std::string s = data_editor2->GetFileViewName(FD, FD->ViewNames, i);
				std::string kali = K->Alias;
				if (!K->Alias.empty()) {
					s += "/" + kali;
				}
				ww.PutSelect(s);
			}
		}
	}
	ss.Abcd = true;
	ww.SelectStr(0, 0, 35, "");

	if (Event.Pressed.KeyCombination() == __ESC) {
		// do nothing;
	}
	else {
		LinkD* LD = *LinkDRoot.begin();
		data_editor2->GetSel2S(s1, s2, '/', 2);
		ali = GetFromKey(LD)->Alias;

		for (LinkD* ld : LinkDRoot) {
			if ((ld->ToFD != file_d_)
				|| (ld->IndexRoot == 0)
				|| (s2 != ali)
				|| !data_editor2->EquFileViewName(ld->FromFD, s1, &EO)) {
				continue;
			}
			else {
				LD = ld;
			}
		}

		data_editor2->file_d_ = LD->FromFD;
		if (data_editor2->SelFldsForEO(EO, LD)) {
			EO->DownLD = LD;
			EO->DownRecPtr = record_;
			EditReader* reader = new EditReader();
			reader->NewEditD(data_editor2->file_d_, EO, data_editor2->record_);
			data_editor2->edit_ = reader->GetEditD();
			if (data_editor2->OpenEditWw()) {
				data_editor2->RunEdit(nullptr, Brk);
			}
			SaveFiles();
			//PopEdit();
			delete reader; reader = nullptr;
		}
	}

	PopW(w);
	ReleaseStore(&p);
	//ReadParamsFromE(EE);
	DisplEditWw();
}

void DataEditor::ShiftF7Proc()
{
	/* find last (first decl.) foreign key link with CFld as an argument */
	FieldDescr* F = (*CFld)->FldD;
	LinkD* LD1 = nullptr;
	for (LinkD* ld : LinkDRoot) { //while (LD != nullptr) {
		for (KeyFldD* arg : ld->Args) {
			//KeyFldD* KF = ld->Args;
			//while (KF != nullptr) {
			if ((arg->FldD == F) && ForNavigate(ld->ToFD)) LD1 = ld;
			//KF = KF->pChain;
		}
	}
	if (LD1 != nullptr) UpwEdit(LD1);
}

bool DataEditor::ShiftF7Duplicate()
{
	bool result = false;

	EditD* ee = edit_->ShiftF7_caller;
	file_d_ = ee->FD;
	record_ = ee->NewRecPtr;

	if (!ELockRec(ee, file_d_->IRec, ee->IsNewRec, ee->params_->Subset)) {
		return result;
	}

	if (!params_->WasUpdated) {
		Move(record_, ee->OldRecPtr, file_d_->FF->RecLen);
		params_->WasUpdated = true;
	}

	//KeyFldD* kf2 = edit_->ShiftF7_link->ToKey->KFlds;
	for (KeyFldD* kf2 : edit_->ShiftF7_link->ToKey->KFlds) {
		for (KeyFldD* arg : edit_->ShiftF7_link->Args) {
			DuplFld(edit_->FD, file_d_, edit_->NewRecPtr, record_, ee->OldRecPtr, kf2->FldD, arg->FldD);
			//kf2 = kf2->pChain;
		}
	}

	file_d_->SetUpdFlag(record_);
	file_d_ = edit_->FD;
	record_ = edit_->NewRecPtr;
	result = true;

	return result;
}

bool DataEditor::DuplToPrevEdit()
{
	LockMode md;
	bool result = false;
	EditD* ee = edit_->pChain;
	if (ee == nullptr) return result;
	FieldDescr* f1 = (*CFld)->FldD;

	/* !!! with ee^ do!!! */
	FieldDescr* f2 = (*CFld)->FldD;
	if ((f2->Flg && f_Stored == 0) || (f1->field_type != f2->field_type) || (f1->L != f2->L)
		|| (f1->M != f2->M) || !(*CFld)->Ed(IsNewRec)) {
		WrLLF10Msg(140);
		return result;
	}
	file_d_ = ee->FD;
	record_ = ee->NewRecPtr;
	if (!ELockRec(ee, file_d_->IRec, ee->IsNewRec, ee->params_->Subset)) {
		return result;
	}
	if (!params_->WasUpdated) {
		Move(record_, ee->OldRecPtr, file_d_->FF->RecLen);
		params_->WasUpdated = true;
	}
	DuplFld(edit_->FD, file_d_, edit_->NewRecPtr, record_, ee->OldRecPtr, f1, f2);
	file_d_->SetUpdFlag(record_);

	file_d_ = edit_->FD; record_ = edit_->NewRecPtr;
	result = true;

	keyboard.AddToFrontKeyBuf(0x0D); // ^M .. \r .. #13
	//pstring oldKbdBuffer = KbdBuffer;
	//KbdBuffer = 0x0D; // ^M
	//KbdBuffer += oldKbdBuffer;
	return result;
}

void DataEditor::Calculate2()
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
		ww.PromptLL(114, txt, I, Del, false, false);
		if (Event.Pressed.KeyCombination() == 'U') goto label0;
		if (Event.Pressed.KeyCombination() == __ESC || (txt.length() == 0)) goto label3;
		CalcTxt = txt;
		g_compiler->SetInpStr(txt);
		g_compiler->RdLex();
		Z = g_compiler->RdFrml(FTyp, nullptr);
		if (Lexem != 0x1A) g_compiler->Error(21);
		if (Event.Pressed.KeyCombination() == __CTRL_F4) {
			F = (*CFld)->FldD;
			if ((*CFld)->Ed(IsNewRec) && (F->frml_type == FTyp)) {
				if (LockRec(true)) {
					if ((F->field_type == FieldType::FIXED) && ((F->Flg & f_Comma) != 0)) {
						auto iZ0 = (FrmlElemFunction*)Z;
						auto iZ02 = (FrmlElemNumber*)iZ0->P1;
						if ((Z->Op = _const)) R = ((FrmlElemNumber*)Z)->R;
						else if ((Z->Op == _unminus) && (iZ02->Op == _const)) R = -iZ02->R;
						else goto label5;
						SetWasUpdated(file_d_->FF, record_);
						file_d_->saveR(F, R * Power10[F->M], record_);
					}
					else
						label5:
					AssignFld(F, Z);
					DisplFld(*CFld, IRec, TextAttr);
					IVon();
					goto label3;
				}
			}
			else WrLLF10Msg(140);
		}
		switch (FTyp) {
		case 'R': {
			R = RunReal(file_d_, Z, record_);
			str(R, 30, 10, txt);
			txt = LeadChar(' ', TrailChar(txt, '0'));
			if (txt[txt.length() - 1] == '.') {
				txt = txt.substr(0, txt.length() - 1);
			}
			break;
		}
		case 'S': {
			/* wie RdMode fuer T ??*/
			txt = RunShortStr(file_d_, Z, record_);
			break;
		}
		case 'B': {
			if (RunBool(file_d_, Z, record_)) txt = AbbrYes;
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
		file_d_ = edit_->FD;
		ReleaseStore(&p);
		// TODO: goto label1;
	}
label3:
	//ReleaseStore(p);
	{}
}

void DataEditor::DelNewRec()
{
	LockMode md;
	file_d_->DelAllDifTFlds(record_, nullptr);
	if (CNRecs() == 1) return;
	IsNewRec = false;
	params_->Append = false;
	params_->WasUpdated = false;
	CFld = edit_->FirstFld.begin();
	if (CRec() > CNRecs()) { // pozor! uspodarani IF a ELSE neni jasne !!!
		if (IRec > 1) IRec--;
		else BaseRec--;
	}
	RdRec(CRec());
	NewDisplLL = true;
	DisplWwRecsOrPage(CPage, &RT);
}

std::vector<EFldD*>::iterator DataEditor::FrstFldOnPage(WORD Page)
{
	std::vector<EFldD*>::iterator D = edit_->FirstFld.begin();
	while ((*D)->Page < Page) {
		++D; //D = D->pChain;
	}
	return D;
}

void DataEditor::F6Proc()
{
	WORD iMsg = 105;
	if (params_->Subset || HasIndex || params_->NoCreate || params_->NoDelete
#ifdef FandSQL
		|| file_d_->IsSQLFile
#endif
		) iMsg = 106;
	switch (Menu(iMsg, 1)) {
	case 1: AutoReport(); break;
	case 2: CheckFromHere(); break;
	case 3: PromptSelect(); break;
	case 4: AutoGraph(); break;
	case 5: Sorting(); break;
	default:;
	}
}

int DataEditor::GetEdRecNo()
{
	if (IsNewRec) return 0;
	if (edit_->IsLocked) return edit_->LockedRec;
	return AbsRecNr(CRec());
}

void DataEditor::SetEdRecNoEtc(int RNr)
{
	XString x;
	x.S = EdRecKey;
	EdField = (*CFld)->FldD->Name;
	EdIRec = IRec;
	EdRecKey = "";
	EdKey = "";
	EdRecNo = RNr;
	if (RNr == 0) EdRecNo = GetEdRecNo();
	if (VK == nullptr) return;
	if (!params_->WasWK && !VK->Alias.empty()) {
		EdKey = VK->Alias;
		if (EdKey == "") EdKey = "@";
	}
	if (!IsNewRec) {
		void* cr = record_;
		XKey* k = VK;
		if (params_->Subset) {
			k = WK;
		}
		if (params_->WasUpdated) {
			x.PackKF(file_d_, k->KFlds, edit_->OldRecPtr);
		}
		else {
			x.PackKF(file_d_, k->KFlds, record_);
		}
	}
	EdRecKey = x.S;
}

bool DataEditor::StartProc(Instr_proc* ExitProc, bool Displ)
{
	bool upd = false;
	bool b = false, b2 = false, lkd = false;
	char* p = nullptr;
	WORD d = 0; LockMode md; /*float t;*/

	auto result = false;
	file_d_->FF->WasWrRec = false;
	if (HasTF) {
		p = (char*)file_d_->GetRecSpace();
		Move(record_, p, file_d_->FF->RecLen);
	}
	SetEdRecNoEtc(0);
	lkd = edit_->IsLocked;
	if (!lkd && !LockRec(false)) return result;
	b = params_->WasUpdated;
	EdUpdated = b;
	b2 = file_d_->HasUpdFlag(record_);
	SetWasUpdated(file_d_->FF, record_);
	file_d_->ClearUpdFlag(record_);

	// upravime argumenty exit procedury
	ExitProc->TArg[ExitProc->N - 1].FD = file_d_;
	ExitProc->TArg[ExitProc->N - 1].RecPtr = record_;

	md = file_d_->FF->LMode;

	//EditD* EE = WriteParamsToE();                            /*t = currtime;*/

	CallProcedure(ExitProc);

	//ReadParamsFromE(EE);

	file_d_->NewLockMode(md);
	upd = file_d_->FF->WasWrRec;      /*writeln(strdate(currtime-t,"ss mm.ttt"));wait;*/
	if (file_d_->HasUpdFlag(record_)) {
		b = true;
		upd = true;
	}
	params_->WasUpdated = b;
	if (b2) file_d_->SetUpdFlag(record_);
	if (!params_->WasUpdated && !lkd) UnLockRec(edit_);
	if (Displ && upd) DisplAllWwRecs();
	if (Displ) NewDisplLL = true;
	result = true;
	if (HasTF) {
		for (FieldDescr* f : file_d_->FldD) {
			if ((f->field_type == FieldType::TEXT) && ((f->Flg & f_Stored) != 0) &&
				(*(int*)(p + f->Displ) == *(int*)(edit_->OldRecPtr) + f->Displ))
				params_->NoDelTFlds = true;
		}
		delete[] p; p = nullptr;
	}
	return result;
}

void DataEditor::StartRprt(RprtOpt* RO)
{
	bool displ = false;
	if (IsNewRec || params_->EdRecVar || (EdBreak == 16) || !WriteCRec(true, displ)) {
		return;
	}
	if (displ) {
		DisplAllWwRecs();
	}
	XWKey* k = new XWKey(file_d_);

	if (VK != nullptr) {
		k->OneRecIdx(file_d_, VK->KFlds, AbsRecNr(CRec()), record_);
	}
	else {
		std::vector<KeyFldD*> unused;
		k->OneRecIdx(file_d_, unused, AbsRecNr(CRec()), record_);
	}

	RO->FDL.FD = file_d_;
	RO->FDL.ViewKey = k;
	ReportProc(RO, false);
	file_d_ = edit_->FD;
	record_ = edit_->NewRecPtr;
}


bool DataEditor::StartExit(EdExitD* X, bool Displ)
{
	auto result = true;
	switch (X->Typ) {
	case 'P': result = StartProc(X->Proc, Displ); break;
	case 'R': StartRprt((RprtOpt*)X->RO); break;
	}
	return result;
}

WORD DataEditor::ExitKeyProc()
{
	WORD w = 0;
	WORD c = Event.Pressed.KeyCombination();
	for (auto& X : edit_->ExD) {
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
	if (((w == 0) || (w == 3)) && (c == __SHIFT_F7) && (*CFld)->Ed(IsNewRec)) {
		ShiftF7Proc();
		w = 2;
	}
	//Event.Pressed.UpdateKey(c);
	return w;
}

void DataEditor::FieldHelp()
{
	Help(file_d_->ChptPos.rdb, file_d_->Name + '.' + (*CFld)->FldD->Name, false);
}

void DataEditor::DisplLASwitches()
{
	if (!params_->ChkSwitch) screen.ScrWrStr(0, TxtRows - 1, "L", screen.colors.lSwitch);
	if (!params_->WarnSwitch) screen.ScrWrStr(2, TxtRows - 1, "?", screen.colors.lSwitch);
	if (!params_->EdRecVar && !params_->AddSwitch) screen.ScrWrStr(3, TxtRows - 1, "A", screen.colors.lSwitch);
	if (!params_->WithBoolDispl && params_->Select) screen.ScrWrStr(5, TxtRows - 1, "\x12", screen.colors.lSwitch);
}

void DataEditor::DisplLL()
{
	WORD n;
	if (!edit_->Last.empty()) {
		MsgLine = edit_->Last;
		if (MsgLine.length() > 0) {
			WrLLMsgTxt();
			DisplLASwitches();
		}
		return;
	}

	if (edit_->ShiftF7_link != nullptr) {
		n = 144;
	}
	else if (params_->NoCreate || params_->Only1Record) {
		if (IsNewRec) n = 129;
		else if (params_->EdRecVar) n = 130;
		else n = 128;
	}
	else if (IsNewRec) {
		n = 123;
	}
	else {
		n = 124;
	}

	if (!params_->F1Mode || params_->Mode24) {
		WrLLMsg(n);
		DisplLASwitches();
	}
}

void DataEditor::DisplCtrlAltLL(WORD Flags)
{
	if ((Flags & 0x04) != 0) {        /* Ctrl */
		if (!edit_->CtrlLast.empty()) {
			MsgLine = edit_->CtrlLast;
			WrLLMsgTxt();
		}
		else if (IsCurrChpt(file_d_)) WrLLMsg(125);
		else if (params_->EdRecVar) WrLLMsg(154);
		else WrLLMsg(127);
	}
	else if ((Flags & 0x03) != 0) {         /* Shift */
		if (!edit_->ShiftLast.empty()) {
			MsgLine = edit_->ShiftLast;
			WrLLMsgTxt();
		}
		else DisplLL();
	}
	else if ((Flags & 0x08) != 0) {         /* Alt */
		if (!edit_->AltLast.empty()) {
			MsgLine = edit_->AltLast;
			WrLLMsgTxt();
		}
		else DisplLL();
	}
}

// po nacteni editoru se smycka drzi tady a ceka na stisknuti klavesy
void DataEditor::CtrlReadKbd()
{
	BYTE flgs = 0;
	uint64_t TimeBeg = getMillisecondsNow();
	unsigned int D = 0;

	if (params_->F1Mode && params_->Mode24 && CRdb->help_file != nullptr) {
		DisplayLastLineHelp(file_d_->ChptPos.rdb, file_d_->Name + "." + (*CFld)->FldD->Name, params_->Mode24);
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

	if (file_d_->FF->NotCached()) {
		if (!edit_->params_->EdRecVar && (spec.ScreenDelay == 0 || edit_->RefreshDelay < spec.ScreenDelay)) {
			D = edit_->RefreshDelay;
		}
		if (edit_->WatchDelay != 0) {
			if (D == 0) {
				D = edit_->WatchDelay;
			}
			else {
				D = min(D, edit_->WatchDelay);
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
			if (params_->F1Mode && !params_->Mode24 && CRdb->help_file != nullptr) {
				DisplayLastLineHelp(file_d_->ChptPos.rdb, file_d_->Name + "." + (*CFld)->FldD->Name, params_->Mode24);
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

void DataEditor::MouseProc()
{
	bool Displ;
	for (WORD i = 1; i <= edit_->NRecs; i++) {
		int n = BaseRec + i - 1;
		if (n > CNRecs()) goto label1;
		std::vector<EFldD*>::iterator D = edit_->FirstFld.begin();
		while (D != edit_->FirstFld.end()) {
			if (IsNewRec && (i == IRec) && (*D == FirstEmptyFld)) goto label1;
			if (((*D)->Page == CPage) && MouseInRect((*D)->Col - 1, FldRow(*D, i) - 1, (*D)->L, 1)) {
				if ((i != IRec) && (IsNewRec || !WriteCRec(true, Displ))) goto label1;
				GotoRecFld(n, D);
				if ((Event.Buttons & mbDoubleClick) != 0) {
					if (params_->MouseEnter) Event.Pressed.UpdateKey('M');
					else Event.Pressed.UpdateKey(__INSERT);
					Event.What = evKeyDown;
					return;
				}
				else ClrEvent();
				return;
			}
			++D; //D = D->pChain;
		}
	}
label1:
	ClrEvent();
}

void DataEditor::ToggleSelectRec()
{
	XString x; LockMode md;
	XWKey* k = edit_->SelKey;
	int n = AbsRecNr(CRec());
	if (k->RecNrToPath(file_d_, x, n, record_)) {
		k->NR--;
		k->DeleteOnPath(file_d_);
	}
	else {
		k->NR++;
		k->Insert(file_d_, n, false, record_);
	}
	SetRecAttr(IRec);
	IVon();
}

void DataEditor::ToggleSelectAll()
{
	XWKey* k = edit_->SelKey;
	if (k == nullptr) return;
	if (k->NR > 0) {
		k->Release(file_d_);
	}
	else if (params_->Subset) {
		file_d_->FF->CopyIndex(k, WK);
	}
	else {
		file_d_->FF->CopyIndex(k, VK);
	}
	DisplAllWwRecs();
}

void DataEditor::GoStartFld(EFldD* SFld)
{
	std::vector<EFldD*>::iterator nextCFld = CFld;
	++nextCFld;

	while ((*CFld != SFld) && (nextCFld != edit_->FirstFld.end())) {
		if (IsFirstEmptyFld()) {
			if (((*CFld)->Impl != nullptr) && LockRec(true)) {
				AssignFld((*CFld)->FldD, (*CFld)->Impl);
			}
			++FirstEmptyFld; // = FirstEmptyFld->pChain;
			DisplFld(*CFld, IRec, TextAttr);
		}
		GotoRecFld(CRec(), nextCFld);
		nextCFld = CFld;
		++nextCFld;
	}
}

void DataEditor::RunEdit(XString* PX, WORD& Brk)
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
	if (params_->OnlySearch) goto label2;
	if (!IsNewRec && (PX != nullptr)) {
		GotoXRec(PX, n);
	}
	if (params_->Select && !RunBool(file_d_, edit_->Bool, record_)) {
		GoPrevNextRec(+1, true);
	}
	//if (/*edit_->StartFld != nullptr*/ true) { GoStartFld(&edit_->StartFld); goto label1; }
	if (edit_->StartFld != nullptr) {
		GoStartFld(edit_->StartFld);
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
	if (file_d_->FF->NotCached()) {
		if (!params_->EdRecVar && (edit_->RefreshDelay > 0) && (OldTimeR + edit_->RefreshDelay < getMillisecondsNow())) {
			DisplAllWwRecs();
		}
		if (Event.What == 0) {
			if ((edit_->WatchDelay > 0) && (OldTimeW + edit_->WatchDelay < getMillisecondsNow()))
				if (LongBeep < 3) {
					for (i = 1; i <= 4; i++) {
						Beep();
					}
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
		if (params_->F1Mode && (CRdb->help_file != nullptr)
			&& (params_->Mode24 && (Event.Where.Y == TxtRows - 2) || !params_->Mode24 && (Event.Where.Y == TxtRows - 1))) {
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
			if ((*CFld)->Ed(IsNewRec) && (((*CFld)->FldD->field_type != FieldType::TEXT) || (file_d_->loadT((*CFld)->FldD, record_) == 0))
				&& LockRec(true)) {
				//keyboard.AddToFrontKeyBuf(KbdChar); // vrati znak znovu do bufferu
				const bool res = !EditItemProc(true, true, Brk);
				if (res) {
					goto label7;
				}
				if (Brk != 0) {
					SetEdRecNoEtc(0);
					goto label71;
				}
			}
		}
		else {
			// klavesa je funkcni
			ClrEvent();
			switch (Event.Pressed.KeyCombination()) {
			case __F1: {
				// index napovedy
				ReadMessage(7);
				Help((RdbD*)HelpFD, MsgLine, false);
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
				if (params_->OnlySearch) {
					if (IsNewRec) {
						if (CNRecs() > 1) {
							DelNewRec();
						}
						else {
							EdBreak = 0;
							goto label7;
						}
					}
					else
						if (!WriteCRec(true, Displ)) {
							goto label1;
						}
				label2:
					if (PromptAndSearch(!params_->NoCreate)) {
						goto label0;
					}
				}
				EdBreak = 0;
			label7:
				if (IsNewRec && !EquOldNewRec()) {
					if (!params_->Prompt158 || PromptYN(158)) {
						SetEdRecNoEtc(0);
						goto label71;
					}
					else {
						goto label1;
					}
				}
				EdBr = EdBreak;
				n = GetEdRecNo();
				if (((IsNewRec
					|| WriteCRec(true, Displ)) && ((EdBreak == 11))
					|| params_->NoESCPrompt
					|| (!spec.ESCverify && !params_->MustESCPrompt)
					|| PromptYN(137))) {
					//if ((IsNewRec || WriteCRec(true, Displ)) && ((EdBreak == 11) || NoESCPrompt || !spec.ESCverify && !MustESCPrompt || PromptYN(137))) {
					EdBreak = EdBr;
					SetEdRecNoEtc(n);
				label71:
					if (IsNewRec && !params_->EdRecVar) DelNewRec();
					IVoff();
					EdUpdated = edit_->EdUpdated;
					if (!params_->EdRecVar) file_d_->ClearRecSpace(edit_->NewRecPtr);
					if (params_->Subset && !params_->WasWK) WK->Close(file_d_);
					if (!params_->EdRecVar) {
#ifdef FandSQL
						if (file_d_->IsSQLFile) Strm1->EndKeyAcc(WK);
#endif
						file_d_->OldLockMode(edit_->OldMd);
					}
					return;
				}
				break;
			}
			case __ALT_EQUAL: {
				// ukonceni editace bez ulozeni zmen
				UndoRecord();
				EdBreak = 0;
				SetEdRecNoEtc(0);
				goto label71;
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
				if (!params_->EdRecVar) {
					if (IsNewRec) {
						if ((CNRecs() > 1) && (!params_->Prompt158 || EquOldNewRec() || PromptYN(158))) {
							DelNewRec();
						}
					}
					else if (!params_->NoCreate && !params_->Only1Record && WriteCRec(true, Displ)) {
						if (Displ) {
							DisplAllWwRecs();
						}
						SwitchToAppend();
					}
				}
				goto label0;
				break;
			}
			case __UP: {
				if (params_->LUpRDown) {
					if (CFld != edit_->FirstFld.begin()) {
						GotoPrevRecFld(CRec(), CFld);
					}
				}
				else {
					goto defaultCaseLabel;
				}
				break;
			}
			case __DOWN: {
				if (params_->LUpRDown) {
					if ((*CFld != edit_->FirstFld.back()) && !IsFirstEmptyFld())
						GotoNextRecFld(CRec(), CFld);
				}
				else {
					goto defaultCaseLabel;
				}
				break;
			}
			case __LEFT:
			case 'S': {
				if (CFld != edit_->FirstFld.begin()) {
					GotoPrevRecFld(CRec(), CFld);
				}
				break;
			}
			case __RIGHT:
			case 'D': {
				if ((*CFld != edit_->FirstFld.back()) && !IsFirstEmptyFld())
					GotoNextRecFld(CRec(), CFld);
				break;
			}
			case __HOME:
			label3:
				GotoRecFld(CRec(), edit_->FirstFld.begin()); break;
			case __END: {
			label4:
				if (IsNewRec && (FirstEmptyFld != nullptr)) {
					GotoRecFld(CRec(), edit_->GetEFldIter(FirstEmptyFld));
				}
				else {
					GotoRecFld(CRec(), edit_->GetEFldIter(edit_->LastFld));
				}
				break;
			}
			case __ENTER: {
				if (params_->SelMode && (edit_->SelKey != nullptr) && !IsNewRec) {
					if (WriteCRec(true, Displ)) {
						if ((edit_->SelKey != nullptr) && (edit_->SelKey->NRecs() == 0)) {
							ToggleSelectRec();
						}
						EdBreak = 12;
						SetEdRecNoEtc(0);
						goto label71;
					}
				}
				else {
					if ((edit_->ShiftF7_link != nullptr) && !IsNewRec) {
						if (ShiftF7Duplicate()) {
							// TODO: probably don't continue, caller should manage this
							EdBreak = 0;
							keyboard.AddToFrontKeyBuf(__ENTER);
							//goto label7;
							return;
						}
					}
					else {
						if (!CtrlMProc(3)) {
							goto label7;
						}
					}
				}
				break;
			}
			case __INSERT: {
				// zahajeni opravy udaje
				if ((*CFld)->Ed(IsNewRec) && LockRec(true)) {
					b = true;
				}
				else {
					b = false;
				}

				if (!EditItemProc(false, b, Brk)) {
					goto label7;
				}

				if (Brk != 0) {
					SetEdRecNoEtc(0);
					goto label71;
				}

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
			defaultCaseLabel:
				if (!IsNewRec) {
					if (KbdChar == __CTRL_Y) {
						if (!params_->NoDelete) {
							if (DeleteRecProc()) {
								ClearKeyBuf();
								b = true;
							label14:
								if (((CNRecs() == 0) || (CNRecs() == 1) && IsNewRec) && params_->NoCreate) {
									WrLLF10Msg(112);
									EdBreak = 13;
									SetEdRecNoEtc(0);
									goto label71;
								}
								if (b && !CtrlMProc(0)) {
									goto label7;
								}
							}
						}
					}
					else if (WriteCRec(true, Displ)) {
						if (Displ) DisplAllWwRecs();
						//Event.Pressed.UpdateKey(w);       /*only in edit mode*/
						switch (KbdChar) {
						case __F9: {
							// uloz
							SaveFiles();
							UpdCount = 0;
							break;
						}
						case __CTRL_N: {
							// vloz novy radek pred aktualni
							if (!params_->NoCreate && !params_->Only1Record) {
								InsertRecProc(nullptr);
								goto label0;
							}
							break;
						}
						case __UP:
						case 'E': {
							// predchozi radek
							if (edit_->NRecs > 1) {
								GoPrevNextRec(-1, true);
							}
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
							if (edit_->NRecs > 1) {
								GoPrevNextRec(+1, true);
							}
							break;
						}
						case __CTRL_END: {
							// nasledujici veta
							GoPrevNextRec(+1, true);
							break;
						}
						case __PAGEUP:
						case 'R': {
							// o obrazovku vzad
							if (edit_->NPages == 1) {
								if (edit_->NRecs == 1) {
									GoPrevNextRec(-1, true);
								}
								else {
									GotoRecFld(CRec() - edit_->NRecs, CFld);
								}
							}
							else if (CPage > 1) {
								GotoRecFld(CRec(), FrstFldOnPage(CPage - 1));
							}
							break;
						}
						case __PAGEDOWN:
						case 'C': {
							// o obrazovku vpred
							if (edit_->NPages == 1)
								if (edit_->NRecs == 1) GoPrevNextRec(+1, true);
								else GotoRecFld(CRec() + edit_->NRecs, CFld);
							else if (CPage < edit_->NPages) GotoRecFld(CRec(), FrstFldOnPage(CPage + 1));
							break;
						}
						case 'Q':
							switch (ReadKbd())
							{
							case 'S': goto label3;
							case 'D': goto label4;
							case 'R': goto label5;
							case 'C': goto label6;
							}
							break;
						case __CTRL_PAGEUP:
						label5:
							GotoRecFld(1, edit_->FirstFld.begin()); break;
						case __CTRL_PAGEDOWN:
						label6:
							GotoRecFld(CNRecs(), edit_->GetEFldIter(edit_->LastFld)); break;
						case __CTRL_LEFT:
							if (CRec() > 1) SwitchRecs(-1); break;
						case __CTRL_RIGHT:
							if (CRec() < CNRecs()) SwitchRecs(+1); break;
						case __CTRL_F2: {
							if (!params_->EdRecVar) RefreshSubset();
							b = false;
							goto label14;
							break;
						}
						case __ALT_F2:
						case __ALT_F3:
							if (IsCurrChpt(file_d_)) {
								if (KbdChar == __ALT_F3) {
									ForAllFDs(ForAllFilesOperation::close_passive_fd);
									EditHelpOrCat(KbdChar, 0, "");
								}
								else {
									Brk = 2;
									SetEdRecNoEtc(0);
									goto label71;
								}
							}
							else if (IsTestRun && (file_d_ != catalog->GetCatalogFile()) && (KbdChar == __ALT_F2)) {
								EditHelpOrCat(KbdChar, 1, file_d_->Name + "." + (*CFld)->FldD->Name);
							}
							break;
						case __F6: if (!params_->EdRecVar) F6Proc(); break;
						case __F4: {
							if (DuplToPrevEdit()) {
								EdBreak = 14;
								SetEdRecNoEtc(0);
								goto label71;
							}
							break;
						}
						case __CTRL_F7: DownEdit(); break;
						case __F8: {
							if (edit_->SelKey != nullptr) {
								ToggleSelectRec(); GoPrevNextRec(+1, true);
							}
							break;
						}
						case __F3: {
							// najdi vetu podle klic. udaje
							if (!params_->EdRecVar)
								if (file_d_ == CRdb->help_file) {
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
							if (!params_->EdRecVar) PromptGotoRecNr();
							break;
						}
						case __SHIFT_F8: ToggleSelectAll(); break;
						case __CTRL_F8:
						case __CTRL_F9:
						case __CTRL_F10:
						case __ALT_F9:
							if (IsCurrChpt(file_d_)) {
								Brk = 2;
								SetEdRecNoEtc(0);
								goto label71;
							}
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

void DataEditor::EditDataFile(FileD* FD, EditOpt* EO)
{
	void* p = nullptr;
	int w1 = 0, w2 = 0, w3 = 0;
	WORD Brk = 0, r1 = 0, r2 = 0;
	bool pix = false;
	EditReader* reader = new EditReader();

	MarkStore(p);
	if (EO->SyntxChk) {
		IsCompileErr = false;

		try {
			reader->NewEditD(FD, EO, record_);
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
		//PopEdit();
		return;
	}
	reader->NewEditD(FD, EO, record_);

	delete edit_;
	edit_ = reader->GetEditD();
	w2 = 0;
	w3 = 0;
	pix = (edit_->WFlags & WPushPixel) != 0;
	if (edit_->WwPart) {
		r1 = TxtRows;
		r2 = edit_->params_->WithBoolDispl ? 2 : 1;
		if (edit_->params_->Mode24) {
			r1--;
		}
		w1 = PushW(1, 1, TxtCols, r2, pix, true);
		w2 = PushW(1, r1, TxtCols, TxtRows, pix, true);
		if ((edit_->WFlags & WNoPop) == 0) {
			w3 = PushW(edit_->V.C1, edit_->V.R1, edit_->V.C2 + edit_->ShdwX, edit_->V.R2 + edit_->ShdwY, pix, true);
		}
	}
	else {
		w1 = PushW(1, 1, TxtCols, TxtRows, pix, true);
	}
	if (OpenEditWw()) {
		if (params_->OnlyAppend && !params_->Append) {
			SwitchToAppend();
		}
		RunEdit(nullptr, Brk);
	}

	if (w3 != 0) PopW(w3);
	if (w2 != 0) PopW(w2);
	PopW(w1);
	//PopEdit();
	ReleaseStore(&p);
}
