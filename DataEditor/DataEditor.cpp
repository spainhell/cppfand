#include "DataEditor.h"
#include <memory>
#include <regex>

#include "Dependency.h"
#include "EditReader.h"
#include "EditableField.h"
#include "../TextEditor/TextEditor.h"
#include "../TextEditor/EditorHelp.h"
#include "../Core/LogicControl.h"
#include "../Core/Compiler.h"
#include "../Core/EditOpt.h"
#include "../fandio/FieldDescr.h"
#include "../Common/Record.h"
#include "../Common/FileD.h"
#include "../Common/CommonVariables.h"
#include "../Core/GlobalVariables.h"
#include "../fandio/KeyFldD.h"
#include "../Common/LinkD.h"
#include "../Core/oaccess.h"
#include "../Core/obase.h"
#include "../Core/obaseww.h"
#include "../Core/rdfildcl.h"
#include "../Core/rdrun.h"
#include "../Core/runproc.h"
#include "../Core/runproj.h"
#include "../fandio/XKey.h"
#include "../fandio/XWKey.h"
#include "../Core/wwmenu.h"
#include "../Logging/Logging.h"
#include "../MergeReport/ReportGenerator.h"
#include "../Common/textfunc.h"
#include "../Common/exprcmp.h"
#include "../Common/compare.h"
#include "../Drivers/constants.h"
#include "../Common/DateTime.h"
#include "../Core/RunMessage.h"


DataEditor::DataEditor()
{
	params_ = std::make_unique<DataEditorParams>();
	runner_ = std::make_unique<ProjectRunner>();
	current_rec_ = new Record();
}

DataEditor::DataEditor(EditD* edit)
{
	edit_ = edit;
	file_d_ = edit_->FD;
	original_rec_ = new Record(edit_->FD);
	current_rec_ = original_rec_->Clone();
	params_ = std::make_unique<DataEditorParams>();
	runner_ = std::make_unique<ProjectRunner>();
}

DataEditor::DataEditor(FileD* file_d)
{
	file_d_ = file_d;
	original_rec_ = new Record(file_d);
	current_rec_ = original_rec_->Clone();
	params_ = std::make_unique<DataEditorParams>();
	runner_ = std::make_unique<ProjectRunner>();
}

DataEditor::~DataEditor()
{
	delete original_rec_;
	if (!current_rec_is_ref) {
		delete current_rec_;
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

	delete original_rec_; original_rec_ = nullptr;
	delete current_rec_; current_rec_ = nullptr;

	original_rec_ = new Record(edit_->FD);
	current_rec_ = original_rec_->Clone();
}

void DataEditor::SetFileD(FileD* file_d)
{
	//if (record_ != nullptr) {
	//	delete[] record_;
	//	record_ = nullptr;
	//}
	file_d_ = file_d;

	delete original_rec_; original_rec_ = nullptr;
	delete current_rec_; current_rec_ = nullptr;

	original_rec_ = new Record(file_d);
	current_rec_ = original_rec_->Clone();
}

Record* DataEditor::GetRecord() const
{
	return current_rec_;
}

Record* DataEditor::GetOriginalRecord() const
{
	return original_rec_;
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

void DelBlk(uint8_t& sLen, std::string s, WORD pos)
{
	while ((sLen > 0) && (s[sLen - 1] == ' ') && (pos <= sLen)) {
		sLen--;
	}
}

void WriteStr(WORD& pos, WORD& base, WORD& maxLen, WORD& maxCol, uint8_t sLen, std::string s, bool star,
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
	case 1/*flags*/: {
		goto label1;
		break;
	}
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
				case FieldType::NUMERIC: {
					if (KbdChar < '0' || KbdChar > '9') {
						goto label7;
					}
					break;
				}
				case FieldType::FIXED: {
					if (!((KbdChar >= '0' && KbdChar <= '9')
						|| KbdChar == '.' || KbdChar == ',' || KbdChar == '-')
						) {
						goto label7;
					}
					break;
				}
				case FieldType::REAL: {
					if (!((KbdChar >= '0' && KbdChar <= '9') || KbdChar == '.' || KbdChar == ','
						|| KbdChar == '-' || KbdChar == '+' || KbdChar == 'e' || KbdChar == 'E')) {
						goto label7;
					}
					break;
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
				// dopln diakriticke znamenko
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

void DataEditor::SetWasUpdated()
{
	if (!params_->WasUpdated) {
		params_->WasUpdated = true;
		current_rec_->SetUpdated();

		// TODO:
		// previously there was also:
		//memcpy(original_rec_->GetRecord(), current_rec_->GetRecord(), file_d->GetRecordSize());
	}
}

void DataEditor::AssignFld(FieldDescr* F, FrmlElem* Z)
{
	SetWasUpdated();
	AssgnFrml(current_rec_, F, Z, false);
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
	if ((Txt.empty()) && (Impl != nullptr)) {
		AssignFld(F, Impl);
		Txt = decodeField(F, L, current_rec_);
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
		if ((!Msk.empty()) && !TestMask(Txt, Msk)) {
			goto label4;
		}
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

// can be moved from DataEditor
void DataEditor::WrPromptTxt(std::string& S, FrmlElem* Impl, FieldDescr* field, std::string& Txt, double& R)
{
	WORD LWw = 0;
	std::string s;
	std::string T;
	double r = 0.0;
	bool b = false;
	screen.WriteStyledStringToWindow(S, ProcAttr);
	int16_t x = screen.WhereX();
	int16_t y = screen.WhereY();
	int16_t d = WindMax.X - WindMin.X + 1;
	
	if (x + field->L - 1 > d) LWw = d - x;
	else LWw = field->L;
	
	TextAttr = screen.colors.dHili;
	
	if (Impl != nullptr) {
		switch (field->frml_type) {
		case 'R': r = RunReal(file_d_, Impl, current_rec_); break;
		case 'S': s = RunString(file_d_, Impl, current_rec_); break;
		default: b = RunBool(file_d_, Impl, current_rec_); break;
		}
		T = decodeFieldRSB(field, field->L, r, s, b);
	}
	
	screen.GotoXY(x, y);
	FieldEdit(field, nullptr, LWw, 1, T, R, true, true, false, 0);
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

// can be moved from DataEditor
bool DataEditor::PromptB(std::string& S, FrmlElem* Impl, FieldDescr* F)
{
	std::string Txt;
	double R = 0.0;
	WrPromptTxt(S, Impl, F, Txt, R);
	bool result = (Txt[0] == AbbrYes);
	if (Event.Pressed.KeyCombination() == __ESC) {
		if (Impl != nullptr) result = RunBool(file_d_, Impl, current_rec_);
		else result = false;
	}
	return result;
}

// can be moved from DataEditor
std::string DataEditor::PromptS(std::string& S, FrmlElem* Impl, FieldDescr* F)
{
	std::string Txt;
	double R = 0.0;
	WrPromptTxt(S, Impl, F, Txt, R);
	auto result = Txt;
	if (Event.Pressed.KeyCombination() == __ESC) {
		if (Impl != nullptr) result = RunString(file_d_, Impl, current_rec_);
		else result = "";
	}
	return result;
}

// can be moved from DataEditor
double DataEditor::PromptR(std::string& S, FrmlElem* Impl, FieldDescr* F)
{
	std::string Txt;
	double R = 0.0;
	WrPromptTxt(S, Impl, F, Txt, R);
	auto result = R;
	if (Event.Pressed.KeyCombination() == __ESC) {
		if (Impl != nullptr) {
			result = RunReal(file_d_, Impl, current_rec_);
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
}

int DataEditor::CNRecs() const
{
	int n = 0;
	if (params_->EdRecVar) { return 1; }
	if (params_->Subset) n = WK->NRecs();
	else {
		if (HasIndex) n = VK->NRecs();
		else n = file_d_->GetNRecs();
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
		if (N > CRec()) {
			N--;
		}
	}

	if (params_->Subset) {
		N = WK->NrToRecNr(file_d_, N);
	}
	else if (HasIndex) {
		LockMode md = file_d_->NewLockMode(RdMode);
		file_d_->FF->TestXFExist();
		N = VK->NrToRecNr(file_d_, N);
		file_d_->OldLockMode(md);
	}

	result = N;
	return result;
}

int DataEditor::LogRecNo(int N)
{
	int result = 0;
	if ((N <= 0) || (N > file_d_->GetNRecs())) return result;

	LockMode md = file_d_->NewLockMode(RdMode);
	file_d_->ReadRec(N, current_rec_);
	if (!current_rec_->IsDeleted()) {
		if (params_->Subset) {
			result = WK->RecNrToNr(file_d_, N, current_rec_);
		}
		else if (HasIndex) {
			file_d_->FF->TestXFExist();
			result = VK->RecNrToNr(file_d_, N, current_rec_);
		}
		else {
			result = N;
		}
	}
	file_d_->OldLockMode(md);
	return result;
}

bool DataEditor::IsSelectedRec(WORD I, Record* record)
{
	XString x;
	auto result = false;
	if ((edit_->SelKey == nullptr) || (I == IRec) && IsNewRec) return result;
	int n = AbsRecNr(BaseRec + I - 1);

	if ((I == IRec) && params_->WasUpdated) {
		result = edit_->SelKey->RecNrToPath(file_d_, x, n, original_rec_);
	}
	else
	{
		result = edit_->SelKey->RecNrToPath(file_d_, x, n, record);
	}

	return result;
}

bool DataEditor::EquOldNewRec()
{
	return Record::Compare(current_rec_, original_rec_) == _equ;
}

/// <summary>
/// Vycte X-ty zaznam (z DB nebo ze souboru v file_d_)
/// Ulozi jej do Record
/// Nejedna se o fyzicke cislo zaznamu v souboru
/// </summary>
/// <param name="nr">kolikaty zaznam</param>
/// <param name="record">pointer to Record object</param>
void DataEditor::RdRec(int nr, Record* record)
{
	LockMode md;
	if (params_->EdRecVar) return;
	// TODO: FandSQL condition removed
	md = file_d_->NewLockMode(RdMode);
	file_d_->ReadRec(AbsRecNr(nr), record);
	file_d_->OldLockMode(md);
}

bool DataEditor::CheckOwner(EditD* E)
{

	bool result = true;
	if (edit_->DownSet && (edit_->OwnerTyp != 'i')) {
		XString X, X1;
		X.PackKF(file_d_, edit_->DownKey->KFlds, current_rec_);
		X1.PackKF(edit_->DownLD->ToFD, edit_->DownLD->ToKey->KFlds, edit_->DownRecord);
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
	X.PackKF(file_d_, edit_->VK->KFlds, current_rec_);
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
		if (file_d_->NotCached()) {
			if (!file_d_->Lock(N, 1/*withESC*/)) {
				result = false;
				return result;
			}
			md = file_d_->NewLockMode(RdMode);
			file_d_->ReadRec(N, current_rec_);
			file_d_->OldLockMode(md);
			if (Subset && !
				((params_->NoCondCheck || RunBool(file_d_, edit_->Cond, current_rec_) && CheckKeyIn(E)) && CheckOwner(E))) {
				WrLLF10Msg(150); goto label1;
			}
		}
		else if (current_rec_->IsDeleted()) {
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

WORD DataEditor::RecAttr(WORD I, Record* record)
{
	bool b = (I != IRec) || !IsNewRec;
	if (!IsNewRec && record->IsDeleted()) {
		return edit_->dDel;
	}
	else if (b && params_->Select && RunBool(file_d_, edit_->Bool, record)) {
		return edit_->dSubSet;
	}
	else if (b && IsSelectedRec(I, record)) {
		return edit_->dSelect;
	}
	else {
		return edit_->dNorm;
	}
}

WORD DataEditor::FldRow(EditableField* D, WORD I)
{
	return edit_->FrstRow + edit_->NHdTxt + (I - 1) * RT->N + D->Ln - 1;
}

bool DataEditor::HasTTWw(FieldDescr* F)
{
	return (F->field_type == FieldType::TEXT) && (F->L > 1) && !edit_->IsUserForm;
}

void DataEditor::DisplEmptyFld(EditableField* D, WORD I)
{
	char c = '\0';

	WORD row = FldRow(D, I);
	screen.GotoXY(D->Col, row);

	if (D->FldD->isStored()) c = '.';
	else c = ' ';

	// print dots for an empty field
	screen.ScrWrStr(D->Col, row, std::string(D->L, c), screen.colors.tNorm);
	if (HasTTWw(D->FldD)) {
		// print spaces for a 'T' empty field (text preview)
		screen.ScrWrStr(D->Col + D->L, row, std::string(D->FldD->L - 1, ' '), screen.colors.tNorm);
	}
}

/// <summary>
/// Prints 1st line of a text field
/// </summary>
/// <param name="field"></param>
/// <param name="record"></param>
void DataEditor::Wr1Line(FieldDescr* field, const Record* record) const
{
	auto X = screen.WhereX();
	auto Y = screen.WhereY();
	//std::string ls = file_d_->loadS(field, record->GetRecord());
	std::string ls = record->LoadS(field);
	ls = GetNthLine(ls, 1, 1);
	WORD max = field->L - 2;
	ls = GetStyledStringOfLength(ls, 0, max);
	size_t chars = screen.WriteStyledStringToWindow(ls, edit_->dNorm);
	TextAttr = edit_->dNorm;
	if (chars < max) {
		screen.ScrFormatWrStyledText(X + chars, Y, edit_->dNorm, "%*c", max - chars, ' ');
	}
}

void DataEditor::DisplFld(EditableField* D, uint16_t I, uint8_t Color, Record* record)
{
	WORD r = FldRow(D, I);
	FieldDescr* field = D->FldD;
	screen.GotoXY(D->Col, r);
	std::string Txt = decodeField(field, D->L, record);
	for (size_t j = 0; j < Txt.length(); j++) {
		if ((uint8_t)Txt[j] < ' ') {
			Txt[j] = Txt[j] + 0x40;
		}
	}
	screen.WriteStyledStringToWindow(Txt, Color);
	if (HasTTWw(field)) {
		screen.GotoXY(D->Col + 2, r);
		Wr1Line(field, record);
	}
}

// Display a form record
void DataEditor::DisplayRecord(uint16_t screen_data_row_nr)
{
	//EditableField* D = nullptr;
	bool NewFlds = false;
	uint8_t a = edit_->dNorm;
	int N = BaseRec + screen_data_row_nr - 1;
	bool is_curr_new_rec = IsNewRec && (screen_data_row_nr == IRec);

	Record* display_rec = nullptr;

	if ((N > CNRecs()) && !is_curr_new_rec) {
		NewFlds = true;
	}
	else {
		if (screen_data_row_nr == IRec) {
			display_rec = current_rec_; // print current record
		}
		else {
			display_rec = new Record(file_d_);
			RdRec(N, display_rec);  // read different record
		}

		NewFlds = false;

		if (!IsNewRec) {
			a = RecAttr(screen_data_row_nr, display_rec);
		}
	}

	for (EditableField* D : edit_->FirstFld) {
		if (is_curr_new_rec && D == *FirstEmptyFld && D->Impl == nullptr) {
			NewFlds = true;
		}

		TextAttr = a;

		// Display an item of the record
		if (D->Page == CPage) {
			if (NewFlds) {
				DisplEmptyFld(D, screen_data_row_nr);
			}
			else {
				DisplFld(D, screen_data_row_nr, TextAttr, display_rec);
			}
		}

		if (is_curr_new_rec && (D == *FirstEmptyFld)) {
			NewFlds = true;
		}
	}

	// delete temporary record if it was created
	if (display_rec != current_rec_) {
		delete display_rec;
		display_rec = nullptr;
	}
}

bool DataEditor::LockRec(bool Displ)
{
	bool result = true;

	if (edit_->IsLocked) {
		return result;
	}

	result = ELockRec(edit_, AbsRecNr(CRec()), IsNewRec, params_->Subset);

	if (result && !IsNewRec && !params_->EdRecVar && file_d_->NotCached() && Displ) {
		DisplayRecord(IRec);
	}
	return result;
}

void DataEditor::UnLockRec(EditD* edit)
{
	if (edit->FD->IsShared() && edit->IsLocked && !edit->params_->EdRecVar) {
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

/// <summary>
/// Print the record number to the system line (1st line of the screen)
/// </summary>
/// <param name="N">record number</param>
void DataEditor::DisplRecNr(int N)
{
	if (edit_->RecNrLen > 0) {
		//screen.GotoXY(edit_->RecNrPos, 1);
		TextAttr = screen.colors.fNorm;
		screen.ScrFormatWrText(edit_->RecNrPos + 1, 1, "%*i", edit_->RecNrLen, N);
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
			FirstEmptyFld = CFld;
			current_rec_->Reset(); //file_d_->Reset(current_rec_, false);
			SetWasUpdated();
			NewRecExit();
		}
		else {
			SetWasUpdated();
		}

		NewDisplLL = true;
	}

	UnLockRec(edit_);
	LockRec(false);
	DisplRecNr(CRec());
}

void DataEditor::ReadParamsFromE(const EditD* edit)
{
	FirstEmptyFld = edit_->GetEFldIter(edit->FirstEmptyFld);
	VK = edit->VK;
	WK = edit->WK;
	BaseRec = edit->BaseRec;
	IRec = edit->IRec;
	IsNewRec = edit->IsNewRec;

	DataEditorParams::CopyParams(edit->params_.get(), params_.get());

	if (VK == nullptr) params_->OnlySearch = false;

	file_d_ = edit->FD;

	delete current_rec_; current_rec_ = nullptr;
	if (edit->LvRec != nullptr) {
		current_rec_ = edit->LvRec;
		current_rec_is_ref = true;
	}
	else {
		current_rec_ = new Record(file_d_);
	}

	delete original_rec_;
	original_rec_ = current_rec_->Clone();

	CFld = edit->CFld;

	HasIndex = file_d_->HasIndexFile();
	HasTF = file_d_->HasTextFile();

	SetCPage(CPage, &RT);
}

void DataEditor::DuplicateField(Record* src_record, FieldDescr* src_field, Record* dst_record, FieldDescr* dst_field)
{
	switch (src_field->frml_type) {
	case 'S': {
		std::string s = src_record->LoadS(src_field);
		dst_record->SaveS(dst_field, s);
		break;
	}
	case 'R': {
		double r = src_record->LoadR(src_field);
		dst_record->SaveR(dst_field, r);
		break;
	}
	case 'B': {
		bool b = src_record->LoadB(src_field);
		dst_record->SaveB(dst_field, b);
		break;
	}
	}
}

bool DataEditor::IsFirstEmptyFld()
{
	return IsNewRec && (CFld == FirstEmptyFld);
}

void DataEditor::SetFldAttr(EditableField* D, WORD I, WORD Attr)
{
	screen.ScrColor(D->Col - 1, FldRow(D, I) - 1, D->L, Attr);
}

void DataEditor::HighLightOff()
{
	SetFldAttr(*CFld, IRec, RecAttr(IRec, current_rec_));
}

void DataEditor::HighLightOn()
{
	screen.ScrColor((*CFld)->Col - 1, FldRow(*CFld, IRec) - 1, (*CFld)->L, edit_->dHiLi);
}

void DataEditor::SetRecAttr(WORD I)
{
	WORD TA = RecAttr(I, current_rec_);

	for (EditableField* D : edit_->FirstFld) {
		if (D->Page == CPage) {
			SetFldAttr(D, I, TA);
		}
	}
}

void DataEditor::DisplTabDupl()
{
	TextAttr = edit_->dTab;

	for (EditableField* D : edit_->FirstFld) {
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
	}
}

void DataEditor::DisplaySystemLine()
{
	if (edit_->Head.empty()) return;

	screen.GotoXY(1, 1);
	TextAttr = screen.colors.fNorm;
	ClrEol(TextAttr);

	std::string sys_line = edit_->Head;

	std::regex regex_full_date("(__\\.__\\.____)", std::regex_constants::icase);
	std::regex regex_short_date("(__\\.__\\.__)", std::regex_constants::icase);
	std::regex regex_rec_nr("(_{2,})", std::regex_constants::icase);
	std::smatch sm;

	sys_line = std::regex_replace(sys_line, regex_full_date, StrDate(Today(), "DD.MM.YYYY"));
	sys_line = std::regex_replace(sys_line, regex_short_date, StrDate(Today(), "DD.MM.YY"));
	if (std::regex_search(sys_line, sm, regex_rec_nr)) {
		edit_->RecNrLen = (uint8_t)sm[0].length();
		edit_->RecNrPos = (uint8_t)sm.prefix().length();
		sys_line = sm.prefix().str() + std::string(sm[0].length(), ' ') + sm.suffix().str();
	}

	if (sys_line.length() > TxtCols) {
		sys_line = sys_line.substr(0, TxtCols);
	}

	screen.ScrWrText(1, 1, sys_line.c_str());
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
	WORD data_rows_count_on_screen = edit_->NRecs;

	if ((data_rows_count_on_screen > 1) && !params_->EdRecVar) {
		md = file_d_->NewLockMode(RdMode);
	}

	AdjustCRec();

	if (!IsNewRec && !params_->WasUpdated) {
		// not new record & not updated
		// RdRec(CRec(), current_rec_);
	}

	for (WORD i = 1; i <= data_rows_count_on_screen; i++) {
		DisplayRecord(i);
	}

	HighLightOn();

	if ((data_rows_count_on_screen > 1) && !params_->EdRecVar) {
		file_d_->OldLockMode(md);
	}
}

void DataEditor::SetNewWwRecAttr()
{
	Record* prev_record = this->current_rec_;
	this->current_rec_ = new Record(file_d_);

	for (WORD I = 1; I <= edit_->NRecs; I++) {
		if (BaseRec + I - 1 > CNRecs()) {
			break;
		}
		if (!IsNewRec || (I != IRec)) {
			RdRec(BaseRec + I - 1, current_rec_);
			SetRecAttr(I);
		}
	}
	HighLightOn();
	// TODO: is there TWork? file_d_->ClearRecSpace(current_rec_->GetRecord());
	delete[] current_rec_; current_rec_ = prev_record;
}

void DataEditor::MoveDispl(WORD From, WORD Where, WORD Number)
{
	for (WORD i = 1; i <= Number; i++) {
		//std::vector<EditableField*>::iterator D = edit_->FirstFld;
		//while (D != nullptr) {
		for (EditableField* D : edit_->FirstFld) {
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
	int Max = edit_->NRecs;
	int I = N - BaseRec + 1;

	if (I > Max) { BaseRec += I - Max; IRec = Max; }
	else if (I <= 0) { BaseRec -= abs(I) + 1; IRec = 1; }
	else IRec = I;

	if (withRead) {
		RdRec(CRec(), current_rec_);
	}
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
	DisplaySystemLine();
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

	for (KeyFldD* KF : edit_->DownLD->ToKey->KFlds) {
		for (KeyFldD* arg : edit_->DownLD->Args) {
			DuplicateField(edit_->DownRecord, KF->FldD, current_rec_, arg->FldD);
		}
	}
}

bool DataEditor::TestDuplKey(FileD* file_d, XKey* K)
{
	XString x;
	int N = 0;
	x.PackKF(file_d, K->KFlds, current_rec_);
	return K->Search(file_d, x, false, N) && (IsNewRec || (edit_->LockedRec != N));
}

void DataEditor::DuplKeyMsg(XKey* K)
{
	SetMsgPar(K->Alias);
	WrLLF10Msg(820);
}

void DataEditor::BuildWork()
{
	uint8_t* p = nullptr;
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
				int32_t err_no = Scan->ResetOwnerIndex(edit_->DownLD, edit_->DownLV, boolP);
				if (err_no != 0) {
					RunError(err_no);
				}
			}
			else {
				xx.PackKF(edit_->DownLD->ToFD, edit_->DownLD->ToKey->KFlds, edit_->DownRecord);
				Scan->ResetOwner(&xx, boolP);
			}
			if (!edit_->KIRoot.empty()) {
				wk2 = new XWKey(file_d_);
				wk2->Open(file_d_, *KF, true, false);
				file_d_->FF->CreateWIndex(Scan, wk2, OperationType::Work);
				XScan* Scan2 = new XScan(file_d_, wk2, edit_->KIRoot, false);
				Scan2->Reset(nullptr, false, current_rec_);
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
			Scan->Reset(boolP, edit_->SQLFilter, current_rec_);
		}
		file_d_->FF->CreateWIndex(Scan, WK, OperationType::Work);
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
		file_d_->SetRecLen(l);
	}
	if (!ok) {
		GoExit(MsgLine);
	}
	ReleaseStore(&p);
}

void DataEditor::SetStartRec()
{
	int n = 0;

	XKey* k = VK;
	if (params_->Subset) {
		k = WK;
	}

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
			RdRec(CRec(), current_rec_);
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
				WK->OneRecIdx(file_d_, k->KFlds, n, current_rec_);
			}
			else {
				std::vector<KeyFldD*> unused;
				WK->OneRecIdx(file_d_, unused, n, current_rec_);
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
		edit_->Journal->OpenCreateF(CPath, Shared, false);
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
		file_d_->OpenCreateF(CPath, Shared, false);
	edit_->OldMd = edit_->FD->GetLMode();
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
			params_->MakeWorkX && HasIndex && file_d_->NotCached() && !params_->Only1Record)
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

		if ((n == 0) || (n > edit_->DownLD->ToFD->GetNRecs())) {
			edit_->DownLD->ToFD->RunErrorM(edit_->OldMd);
			RunError(611);
		}

		edit_->DownLD->ToFD->ReadRec(n, edit_->DownRecord);
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
			current_rec_->Reset(); // file_d_->ZeroAllFlds(current_rec_, false);
			DuplOwnerKey();
			SetWasUpdated();
		}
	else {
		RdRec(CRec(), current_rec_);
	}
label3:
	MarkStore(edit_->AfterE);
	DisplEditWw();
	result = true;

	if (!params_->EdRecVar) {
		file_d_->OldLockMode(md2);
	}

	if (IsNewRec) {
		NewRecExit();
	}

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

void DataEditor::GotoPrevRecFld(int NewRec, std::vector<EditableField*>::iterator NewFld)
{
	--NewFld;
	GotoRecFld(NewRec, NewFld);
}

void DataEditor::GotoNextRecFld(int NewRec, std::vector<EditableField*>::iterator NewFld)
{
	++NewFld;
	GotoRecFld(NewRec, NewFld);
}

void DataEditor::GotoRecFld(int NewRec, const std::vector<EditableField*>::iterator& NewFld)
{
	int NewIRec = 0, NewBase = 0, D = 0, Delta = 0;
	WORD i = 0, Max = 0; LockMode md;
	HighLightOff();
	CFld = NewFld;

	if (NewRec == CRec()) {
		if (CPage != (*CFld)->Page) {
			DisplWwRecsOrPage(CPage, &RT);
		}
		else {
			HighLightOn();
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
		RdRec(CRec(), current_rec_);
		goto label1;
	}

	NewBase = BaseRec + Delta;

	if (NewBase + Max - 1 > CNRecs()) {
		NewBase = CNRecs() - pred(Max);
	}

	if (NewBase <= 0) {
		NewBase = 1;
	}

	IRec = NewRec - NewBase + 1;
	D = NewBase - BaseRec;
	BaseRec = NewBase;
	RdRec(CRec(), current_rec_);

	if (abs(D) >= Max) {
		DisplWwRecsOrPage(CPage, &RT);
		goto label2;
	}

	if (D > 0) {
		MoveDispl(D + 1, 1, Max - D);
		for (i = Max - D + 1; i <= Max; i++) DisplayRecord(i);
	}
	else {
		D = -D;
		MoveDispl(Max - D, Max, Max - D);
		for (i = 1; i <= D; i++) DisplayRecord(i);
	}
label1:
	DisplRecNr(CRec());
	HighLightOn();
label2:
	if (!params_->EdRecVar) {
		file_d_->OldLockMode(md);
	}
}

/// <summary>
/// Slouzi k aktualizaci (synchronizaci) referencnich zaznamu v navazanych (clenskych) souborech pri zmene klicovych hodnot v aktualnim zaznamu.
/// Tedy slouzi k udrzeni referncni integrity.
/// </summary>
/// <param name="POld"></param>
/// <param name="PNew"></param>
void DataEditor::UpdMemberRef(Record* POld, Record* PNew)
{
	XString x, xnew, xold;
	XScan* Scan = nullptr;
	FileD* cf = file_d_;
	Record* src_rec1 = nullptr;
	Record* src_rec2 = nullptr;
	XKey* k = nullptr;

	for (LinkD* link_descr : LinkDRoot) {
		if ((link_descr->MemberRef != 0) && (link_descr->ToFD == cf) && ((PNew != nullptr) || (link_descr->MemberRef != 2))) {
			xold.PackKF(cf, link_descr->ToKey->KFlds, POld);
			if (PNew != nullptr) {
				xnew.PackKF(cf, link_descr->ToKey->KFlds, PNew);
				if (xnew.S == xold.S) continue;
			}
			// TODO: FandSQL condition removed
			k = GetFromKey(link_descr);
			src_rec1 = new Record(link_descr->FromFD);
			if (PNew != nullptr) {
				src_rec2 = new Record(link_descr->FromFD);
			}
			std::vector<KeyInD*> empty;
			Scan = new XScan(link_descr->FromFD, k, empty, true);
			Scan->ResetOwner(&xold, nullptr);
			// TODO: FandSQL condition removed
			link_descr->FromFD->FF->ScanSubstWIndex(Scan, k->KFlds, OperationType::Work);
		label1:
			Scan->GetRec(src_rec1);
			if (!Scan->eof) {
				// TODO: FandSQL condition removed
				if (PNew == nullptr) {
					RunAddUpdate(link_descr->FromFD, '-', nullptr, false, nullptr, link_descr, src_rec1);
					UpdMemberRef(src_rec1, nullptr);
					// TODO: FandSQL condition removed
					link_descr->FromFD->FF->DeleteXRec(Scan->RecNr, src_rec1);
				}
				else {
					//memcpy(src_rec2->GetRecord(), src_rec1->GetRecord(), link_descr->FromFD->FF->RecLen);
					src_rec1->CopyTo(src_rec2);
					for (size_t i = 0; i < link_descr->Args.size(); i++) {
						KeyFldD* arg = link_descr->Args[i];
						KeyFldD* k1 = link_descr->ToKey->KFlds[i];
						//DuplFld(cf, link_descr->FromFD, PNew, src_rec2, nullptr, k1->FldD, arg->FldD);
						DuplicateField(PNew, k1->FldD, src_rec2, arg->FldD);
					}
					RunAddUpdate(link_descr->FromFD, 'd', src_rec1, false, nullptr, link_descr, src_rec2);
					UpdMemberRef(src_rec1, src_rec2);
					// TODO: FandSQL condition removed
					link_descr->FromFD->FF->OverWrXRec(Scan->RecNr, src_rec1, src_rec2, src_rec2);
				}
				goto label1;
			}
			Scan->Close();
			// TODO: is there TWork? LD->FromFD->ClearRecSpace(p);

			delete src_rec1; src_rec1 = nullptr;
			delete src_rec2; src_rec2 = nullptr;
		}
	} // for
}

void DataEditor::WrJournal(char Upd, Record* RP, double Time)
{
	// Upd:
	// + new record; - deleted record; O old record data; N new record data

	size_t srcOffset = 0;
	if (edit_->Journal != nullptr) {
		WORD l = file_d_->FF->RecLen;
		int n = AbsRecNr(CRec());
		if (file_d_->HasIndexFile()) {
			srcOffset += 2;
			l--;
		}
		file_d_ = edit_->Journal;

		const std::unique_ptr newData = std::make_unique<Record>(file_d_);

		std::vector<FieldDescr*>::iterator it = file_d_->FldD.begin();

		newData->SaveS((*it++), std::string(1, Upd));	// change type
		newData->SaveR((*it++), n);									// record number
		newData->SaveR((*it++), user->get_user_code());		// user code
		newData->SaveR((*it++), Time);								// timestamp

		Record* record = new Record(edit_->Journal);
		for (size_t i = 0; i < RP->_values.size(); i++) {
			FieldDescr* f = file_d_->FldD[i];
			switch (f->frml_type) {
			case 'S': {
				std::string s = RP->LoadS(f);
				newData->SaveS(f, s);
				break;
			}
			case 'R': {
				double r = RP->LoadR(f);
				newData->SaveR(f, r);
				break;
			}
			case 'B': {
				bool b = RP->LoadB(f);
				newData->SaveB(f, b);
				break;
			}
			}
		}

		LockMode md = file_d_->NewLockMode(CrMode);
		file_d_->IncNRecs(1);
		file_d_->WriteRec(file_d_->FF->NRecs, record);
		file_d_->OldLockMode(md);

		file_d_ = edit_->FD;
		//record_ = edit_->NewRec->GetRecord();
		delete record; record = nullptr;
		delete current_rec_; 
		current_rec_ = new Record(file_d_);
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
	for (LinkD*& ld : LinkDRoot) {
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
		file_d_->SetPathAndVolume();
		SetMsgPar(CPath, LockModeTxt[md]);
		w1 = PushWrLLMsg(825, true);
		if (w == 0) {
			w = w1;
		}
		else {
			// TWork.Delete(w1);
		}
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
		//if (HasTF) {
		//	if (params_->NoDelTFlds) {
		//		for (FieldDescr* field : file_d_->FldD) {
		//			if ((field->isStored()) && (field->field_type == FieldType::TEXT))
		//				*(int*)((original_rec_->GetRecord()) + f->Displ) = *(int*)((current_rec_->GetRecord() + f->Displ));
		//			//f = f->pChain;
		//		}
		//	}
		//	else {
		//		file_d_->DelAllDifTFlds(current_rec_, original_rec_);
		//	}
		//}

		original_rec_->CopyTo(current_rec_); //memcpy(current_rec_->GetRecord(), original_rec_->GetRecord(), file_d_->FF->RecLen);
		params_->WasUpdated = false; params_->NoDelTFlds = false;
		UnLockRec(edit_);
		DisplayRecord(IRec);
		HighLightOn();
	}
}

bool DataEditor::CleanUp()
{
	if (HasIndex && current_rec_->IsDeleted()) return false;
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
			if ((ld->MemberRef == 2) && (ld->ToFD == file_d_) && Owned(file_d_, nullptr, nullptr, ld, current_rec_) > 0) {
				WrLLF10Msg(662);
				return false;
			}
		}
		if (!RunAddUpdate(file_d_, '-', nullptr, false, nullptr, nullptr, current_rec_)) return false;
		UpdMemberRef(current_rec_, nullptr);
	}
	if (!runner_->ChptDel(file_d_, this)) {
		return false;
	}
	WrJournal('-', current_rec_, Today() + CurrTime());
	return true;
}

bool DataEditor::DelIndRec(int I, int N)
{
	bool result = false;
	if (CleanUp()) {
		file_d_->FF->DeleteXRec(N, current_rec_);
		//SetUpdHandle(file_d_->FF->Handle); // navic
		file_d_->FF->SetUpdateFlag(); // -''- navic
		//SetUpdHandle(file_d_->FF->XF->Handle); // navic
		file_d_->FF->XF->SetUpdateFlag(); // -''- navic
		if ((edit_->SelKey != nullptr) && edit_->SelKey->Delete(file_d_, N, current_rec_)) {
			edit_->SelKey->NR--;
		}
		if (params_->Subset) {
			WK->DeleteAtNr(file_d_, I);
		}
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
	RdRec(CRec(), current_rec_);
	oIRec = IRec;
	oBaseRec = BaseRec;    /* exit proc uses CRec for locking etc.*/
	if (HasIndex) {
		//log->log(loglevel::DEBUG, "... from file with index ...");
		file_d_->FF->TestXFExist();
		if (Group) {
			IRec = 1; BaseRec = 1;
			while (BaseRec <= CNRecs()) {
				N = AbsRecNr(BaseRec);
				current_rec_->ClearDeleted(); //file_d_->ClearDeletedFlag(current_rec_->GetRecord()); /*prevent err msg 148*/
				if (!ELockRec(edit_, N, false, params_->Subset)) {
					goto label1;
				}
				RdRec(BaseRec, current_rec_);
				if (RunBool(file_d_, edit_->Bool, current_rec_)) {
					b = DelIndRec(BaseRec, N);
				}
				else {
					b = true;
					BaseRec++;
				}
				UnLockRec(edit_);
				if (!b) {
					goto label1;
				}
			}
		label1:
			{}
		}
		else {
			if (!ELockRec(edit_, N, false, params_->Subset)) {
				goto label1;
			}
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
			file_d_->ReadRec(I, current_rec_);
			if (fail) goto label2;
			if (params_->Subset) {
				if ((BaseRec > WK->NRecs()) || (WK->NrToRecNr(file_d_, BaseRec) != J + 1)) goto label2;
			}
			else BaseRec = I;
			if (RunBool(file_d_, edit_->Bool, current_rec_)) {
				if (!CleanUp()) {
					fail = true;
					goto label2;
				}
				if (params_->Subset) {
					WK->DeleteAtNr(file_d_, BaseRec);
					WK->AddToRecNr(file_d_, J + 1, -1);
				}
				// TODO: delete T fields?
				//file_d_->DelAllDifTFlds(current_rec_, nullptr);
			}
			else {
				if (params_->Subset) BaseRec++;
			label2:
				J++;
				file_d_->WriteRec(J, current_rec_);
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
		file_d_->DeleteRec(N, current_rec_);
	}

	CFld = edit_->FirstFld.begin();
	IRec = (uint8_t)oIRec;
	BaseRec = oBaseRec;
	current_rec_->ClearDeleted(); // file_d_->ClearDeletedFlag(current_rec_->GetRecord());
	AdjustCRec();

	if (IsNewRec) { DuplOwnerKey(); }
	else { RdRec(CRec(), current_rec_); }

	DisplWwRecsOrPage(CPage, &RT);
	UnLockWithDep(OldMd);
	result = true;
	return result;
}

LogicControl* DataEditor::CompChk(EditableField* D, char Typ)
{
	bool w = params_->WarnSwitch && (Typ == 'W' || Typ == '?');
	bool f = (Typ == 'F' || Typ == '?');
	LogicControl* result = nullptr;

	for (LogicControl* C : D->Checks) {
		if ((w && C->Warning || f && !C->Warning)
			&& !RunBool(file_d_, C->Bool, current_rec_)) {
			result = C;
			break;
		}
	}

	return result;
}

void DataEditor::FindExistTest(FrmlElem* Z, LinkD** LD)
{
	*LD = nullptr;
	if (Z == nullptr) return;

	switch (Z->Op) {
	case _field: {
		auto iZ = (FrmlElemRecVarField*)Z;
		if ((iZ->Field->Flg & f_Stored) == 0) FindExistTest(iZ->Field->Frml, LD);
		break;
	}
	case _access: {
		auto iZ = (FrmlElemAccess*)Z;
		if (iZ->Frml == nullptr) *LD = iZ->Link; /*file.exist*/
		break;
	}
	default: {
		auto iZ = (FrmlElemFunction*)Z;
		if (Z->Op >= 0x60 && Z->Op <= 0xAF) /*1-ary*/ {
			FindExistTest(iZ->P1, LD);
			break;
		}
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

bool DataEditor::TestAccRight(const std::string& acc_rights)
{
	if (user->get_user_code() == 0) { return true; }
	return user->trust(acc_rights);
}

bool DataEditor::ForNavigate(FileD* FD)
{
	bool result = true;
	if (user->get_user_code() == 0) return result;

	//StringListEl* S = FD->ViewNames;
	//while (S != nullptr) {
	//	if (TestAccRight(S)) return result;
	//	S = S->pChain;
	//}
	for (std::string& view_name : FD->ViewNames) {
		size_t colon = view_name.find_first_of(':');
		std::string acc = view_name.substr(colon + 1);
		if (TestAccRight(acc)) return result;
	}

	result = false;
	return result;
}

std::string DataEditor::GetFileViewName(FileD* FD, const std::string& view_name)
{
	std::string result;
	size_t colon = view_name.find_first_of(':');
	std::string name = view_name.substr(0, colon);
	std::string acc = view_name.substr(colon + 1);

	if (TestAccRight(acc)) {
		result = "\x1" + name; // ^A + name
	}

	return result;
}

void DataEditor::SetPointTo(LinkD* LD, std::string* s1, std::string* s2)
{
	//KeyFldD* KF = Link->Args;
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
		for (std::string s : FD->ViewNames) {
			size_t index = s.find_first_of(':');
			s = s.substr(0, index);
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
		(*EO)->Flds = gc->AllFldsList(FD, false);
		return result;
	}

	result = false;
	return result;
}

void DataEditor::UpwEdit(LinkD* LkD)
{
	wwmix ww;

	uint8_t* p = nullptr;
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

				if (ToFD->ViewNames.empty()) {
					s1 = ToFD->Name;
					ww.PutSelect(s1);
					data_editor2->CFld = this->CFld;
					data_editor2->SetPointTo(ld, &s1, &s2);
				}
				else {
					for (size_t i = 0; i < ToFD->ViewNames.size(); i++) {
						s1 = data_editor2->GetFileViewName(ToFD, ToFD->ViewNames[i]);
						if (s1.empty()) {
							continue;
						}
						else {
							s1 += s;
							ww.PutSelect(s1);
							data_editor2->CFld = this->CFld;
							data_editor2->SetPointTo(ld, &s1, &s2);
						}
					}
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

		//std::vector<std::string> SL = Link->ToFD->ViewNames;
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
			EO->Flds = gc->AllFldsList(LD->ToFD, false);
		}
		else {
			RdUserView(LD->ToFD, sl1, EO);
		}
		EO->SetOnlyView = true;
	}

	// prepare DB key from current item
	x.PackKF(file_d_, LD->Args, current_rec_);
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
		reader->NewEditD(LD->ToFD, EO, data_editor2->current_rec_);
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

void DataEditor::DisplChkErr(LogicControl* logic_control)
{
	LinkD* LD = nullptr;

	FindExistTest(logic_control->Bool, &LD);
	if (!logic_control->Warning && (LD != nullptr) && ForNavigate(LD->ToFD) && (*CFld)->Ed(IsNewRec)) {
		FileD* cf = file_d_;
		//uint8_t* cr = record_;
		Record* prev_rec = current_rec_;
		current_rec_ = new Record(file_d_);

		int n = 0;

		Record* rec = LinkUpw(LD, n, false, current_rec_);
		bool b = (rec != nullptr);
		delete rec; rec = nullptr;

		file_d_ = cf;
		delete current_rec_; current_rec_ = prev_rec;

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

	if (!logic_control->HelpName.empty()) {
		if (F10SpecKey == __SHIFT_F7) F10SpecKey = 0xfffe;
		else F10SpecKey = __F1;
	}

	SetMsgPar(RunString(file_d_, logic_control->TxtZ, current_rec_));
	WrLLF10Msg(110);

	if (Event.Pressed.KeyCombination() == __F1) {
		Help(file_d_->ChptPos.rdb, logic_control->HelpName, false);
	}
	else if (Event.Pressed.KeyCombination() == __SHIFT_F7) {
		UpwEdit(LD);
	}
}

bool DataEditor::OldRecDiffers()
{
	// TODO: kde se tato metoda vola? Je nutne zajistit smazani T!
	XString x;
	FieldDescr* f = nullptr;
	bool result = false;
	if (file_d_ == CRdb->project_file || (
		// TODO: FandSQL condition removed
		(!file_d_->NotCached()))) return result;
	Record* rec = new Record(file_d_);

	// TODO: FandSQL condition removed

	file_d_->ReadRec(edit_->LockedRec, rec);
	if (Record::Compare(rec, original_rec_) != _equ) {
		// TODO: !!! file_d_->DelAllDifTFlds(current_rec_, original_rec_);
		rec->CopyTo(current_rec_);
		params_->WasUpdated = false;
		result = true;
	}
	// TODO: is there TWork? file_d_->ClearRecSpace(rec->GetRecord());
	delete rec; rec = nullptr;

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
		//record_ = edit_->OldRec->GetRecord();
		if (KSel->RecNrToPath(file_d_, x, NNew, original_rec_)) {
			KSel->DeleteOnPath(file_d_);
			//record_ = edit_->NewRec->GetRecord();
			KSel->Insert(file_d_, NNew, false, current_rec_);
		}
		//record_ = edit_->NewRec->GetRecord();
	}

	if (VK->RecNrToPath(file_d_, x, edit_->LockedRec, current_rec_) && !params_->WasWK) {
		if (IsNewRec) {
			VK->InsertOnPath(file_d_, x, NNew);
			if (params_->Subset) {
				WK->InsertAtNr(file_d_, CRec(), NNew, current_rec_);
			}
		}
		N = CRec();
	}
	else {
		if (!IsNewRec) {
			//record_ = edit_->OldRec->GetRecord();
			VK->Delete(file_d_, edit_->LockedRec, original_rec_);
			if (params_->Subset) {
				WK->DeleteAtNr(file_d_, CRec());
			}
			//record_ = edit_->NewRec->GetRecord();
			x.PackKF(file_d_, VK->KFlds, current_rec_);
			VK->Search(file_d_, x, true, N);
		}
		N = VK->PathToNr(file_d_);
		VK->InsertOnPath(file_d_, x, NNew);
		if (VK->InWork) {
			VK->NR++;
		}
		if (params_->Subset) {
			N = WK->InsertGetNr(file_d_, NNew, current_rec_);
		}
	}

	WORD result = N;
	for (size_t i = 0; i < file_d_->Keys.size(); i++) {
		auto K = file_d_->Keys[i];
		if (K != VK) {
			if (!IsNewRec) {
				//record_ = edit_->OldRec->GetRecord();
				K->Delete(file_d_, edit_->LockedRec, original_rec_);
			}
			//record_ = edit_->NewRec->GetRecord();
			K->Insert(file_d_, NNew, true, current_rec_);
		}
	}
	//record_ = edit_->NewRec->GetRecord();
	return result;
}

bool DataEditor::WriteCRec(bool MayDispl, bool& Displ)
{
	int32_t N = 0, CNew = 0;
	Implicit* ID = nullptr;
	double time = 0.0;
	LockMode OldMd = LockMode::NullMode;
	Displ = false;
	bool result = false;

	// check if record was changed
	if (!params_->WasUpdated || (!IsNewRec && EquOldNewRec())) {
		IsNewRec = false;
		params_->WasUpdated = false;
		result = true;
		UnLockRec(edit_);
		return result;
	}

	// assign implicit fields for new record
	if (IsNewRec) {
		for (Implicit* id : edit_->Impl) {
			AssgnFrml(current_rec_, id->FldD, id->Frml, false);
		}
	}

	// fields validation
	if (params_->MustCheck) {   /* repeat field checking */
		std::vector<EditableField*>::iterator D = edit_->FirstFld.begin();
		while (D != edit_->FirstFld.end()) {
			LogicControl* C = CompChk(*D, 'F');
			if (C != nullptr) {
				if (MayDispl) GotoRecFld(CRec(), D);
				else CFld = D;
				DisplChkErr(C);
				return result;
			}
			++D;
		}
	}

	// lock record and all other dependent files
	if (IsNewRec) {
		if (!LockWithDep(CrMode, NullMode, OldMd)) return result;
	}
	else if (!params_->EdRecVar) {
		if (!LockWithDep(WrMode, WrMode, OldMd)) return result;
		if (OldRecDiffers()) {
			UnLockRec(edit_);
			UnLockWithDep(OldMd);
			WrLLF10Msg(149);
			DisplayRecord(CRec());
			HighLightOn();
			return result;
		}
	}

	if (params_->Subset
		&& !(params_->NoCondCheck || RunBool(file_d_, edit_->Cond, current_rec_)
			&& CheckKeyIn(edit_))) {
		UnLockWithDep(OldMd);
		WrLLF10Msg(823);
		return result;
	}

	if (edit_->DownSet) {
		DuplOwnerKey();
		Displ = true;
	}

	if (!ExitCheck(MayDispl)) {
		UnLockWithDep(OldMd);
		return result;
	}

	if (params_->EdRecVar) {
		// it's a record variable edit - just unlock and exit
	}
	else {
		// TODO: FandSQL condition removed
		if (HasIndex) {   /* test duplicate keys */
			for (XKey* K : file_d_->Keys) {
				if (!K->Duplic && TestDuplKey(file_d_, K)) {
					UnLockWithDep(OldMd);
					DuplKeyMsg(K);
					return result;
				}
			}
		}
		current_rec_->ClearDeleted();

		if (HasIndex) {
			file_d_->FF->TestXFExist();

			if (IsNewRec) {
				if (params_->AddSwitch
					&& !RunAddUpdate(file_d_, '+', nullptr, false, nullptr, nullptr, current_rec_)) {
					UnLockWithDep(OldMd);
					return result;
				}
				CNew = UpdateIndexes();
				file_d_->CreateRec(file_d_->FF->NRecs + 1, current_rec_);
			}
			else {
				if (params_->AddSwitch) {
					if (!RunAddUpdate(file_d_, 'd', original_rec_, false, nullptr, nullptr, current_rec_)) {
						UnLockWithDep(OldMd);
						return result;
					}
					UpdMemberRef(original_rec_, current_rec_);
				}
				CNew = UpdateIndexes();
				file_d_->WriteRec(edit_->LockedRec, current_rec_);
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
				if (N == CNRecs()) {
					N = file_d_->GetNRecs() + 1;
				}
				else if (params_->Subset) {
					N = WK->NrToRecNr(file_d_, N);
				}
				else {
					// do nothing
				}
			}

			if (params_->AddSwitch && !RunAddUpdate(file_d_, '+', nullptr, false, nullptr, nullptr, current_rec_)) {
				UnLockWithDep(OldMd);
				return result;
			}

			if (runner_->ChptWriteCRec(this, edit_) != 0) {
				UnLockWithDep(OldMd);
				return result;
			}

			file_d_->CreateRec(N, current_rec_);

			if (params_->Subset) {
				WK->AddToRecNr(file_d_, N, 1);
				WK->InsertAtNr(file_d_, CRec(), N, current_rec_);
			}
		}
		else {
			// non-index file, not a new record
			if (params_->AddSwitch) {
				if (!RunAddUpdate(file_d_, 'd', original_rec_, false, nullptr, nullptr, current_rec_)) {
					UnLockWithDep(OldMd);
					return result;
				}
				UpdMemberRef(original_rec_, current_rec_);
			}
			WORD chptWrite = runner_->ChptWriteCRec(this, edit_);

			switch (chptWrite) {
			case 1: {
				UnLockWithDep(OldMd);
				return result;
				break;
			}
			case 2: {
				// are old and new text positions same?
				if ((original_rec_->LoadS(ChptTxt) == original_rec_->LoadS(ChptTxt)) && PromptYN(157)) {
					// TWork.Delete(ClpBdPos);
					std::string data = current_rec_->LoadS(ChptTxt);
					// ClpBdPos = TWork.Store(data);
				}
				UndoRecord();
				UnLockWithDep(OldMd);
				return result;
				break;
			}
			}

			file_d_->WriteRec(edit_->LockedRec, current_rec_);
		}
		time = Today() + CurrTime();

		if (IsNewRec) {
			WrJournal('+', current_rec_, time);
		}
		else {
			WrJournal('O', original_rec_, time);
			WrJournal('N', current_rec_, time);
		}
	}

	if (!IsNewRec && !params_->NoDelTFlds) {
		// TODO: how to delete all 'T' here?
		//file_d_->DelAllDifTFlds(original_rec_, current_rec_);
	}

	edit_->EdUpdated = true;
	params_->NoDelTFlds = false;
	IsNewRec = false;
	params_->WasUpdated = false;
	result = true;
	UnLockRec(edit_);

	UnLockWithDep(OldMd);
	return result;
}

void DataEditor::DuplFromPrevRec()
{
	if ((*CFld)->Ed(IsNewRec)) {
		FieldDescr* field = (*CFld)->FldD;

		LockMode md = RdMode;
		if (field->field_type == FieldType::TEXT) {
			md = WrMode;
		}
		md = file_d_->NewLockMode(md);
		SetWasUpdated();

		Record* prev_record = new Record(file_d_);
		RdRec(CRec() - 1, current_rec_);
		DuplicateField(prev_record, field, current_rec_, field);
		delete prev_record; prev_record = nullptr;

		file_d_->OldLockMode(md);
	}
}

void DataEditor::InsertRecProc(Record* RP)
{
	GotoRecFld(CRec(), edit_->FirstFld.begin());
	IsNewRec = true;
	LockRec(false);
	if (RP != nullptr) {
		RP->CopyTo(current_rec_); //memcpy(current_rec_->GetRecord(), RP->GetRecord(), file_d_->GetRecLen());
	}
	else {
		current_rec_->Reset(); //file_d_->ZeroAllFlds(current_rec_, false);
	}
	DuplOwnerKey();
	SetWasUpdated();
	HighLightOff();
	MoveDispl(edit_->NRecs - 1, edit_->NRecs, edit_->NRecs - IRec);
	FirstEmptyFld = CFld;
	DisplayRecord(IRec);
	HighLightOn();
	NewDisplLL = true;
	NewRecExit();
}

void DataEditor::AppendRecord(Record* RP)
{
	HighLightOff();
	IsNewRec = true;
	WORD Max = edit_->NRecs;
	CFld = edit_->FirstFld.begin();
	FirstEmptyFld = CFld;
	if (IRec < Max) {
		IRec++;
		MoveDispl(Max - 1, Max, Max - IRec);
		DisplayRecord(IRec);
		HighLightOn();
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
		RP->CopyTo(current_rec_); // memcpy(current_rec_->GetRecord(), RP, file_d_->GetRecLen());
	}
	else {
		current_rec_->Reset(); // file_d_->ZeroAllFlds(current_rec_, false);
	}
	DuplOwnerKey();
	DisplRecNr(CRec());
	SetWasUpdated();
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
		result = file_d_->SearchKey(*PX, k, N, current_rec_);
	}
	RdRec(CRec(), current_rec_);
	GotoRecFld(N, CFld);
	file_d_->OldLockMode(md);
	return result;
}

std::vector<EditableField*>::iterator DataEditor::FindEFld(FieldDescr* F)
{
	std::vector<EditableField*>::iterator D = edit_->FirstFld.begin();
	while (D != edit_->FirstFld.end()) {
		if ((*D)->FldD == F) {
			break;
		}
		++D; // = D->pChain;
	}

	return D;
}

void DataEditor::CreateOrErr(bool create, Record* RP, int N)
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

	Record* RP = new Record(file_d_);
	//record_ = RP;

	current_rec_->Reset(); // file_d_->ZeroAllFlds(current_rec_, false);
	x.Clear();
	bool li = params_->F3LeadIn && !IsNewRec;
	int w = PushW(1, TxtRows, TxtCols, TxtRows, true, false);
	if (KF == K->KFlds.end()) {
		result = true;
		//record_ = edit_->NewRecPtr;
		PopW(w);
		delete RP; RP = nullptr;
		return result;
	}
	if (HasIndex && edit_->DownSet && (VK == edit_->DownKey)) {
		FileD* FD2 = edit_->DownLD->ToFD;
		// Record* RP2 = edit_->DownRecord;
		std::vector<KeyFldD*>::iterator KF2 = edit_->DownLD->ToKey->KFlds.begin();

		while (KF2 != edit_->DownLD->ToKey->KFlds.end()) {
			F = (*KF)->FldD;
			FieldDescr* F2 = (*KF2)->FldD;
			switch (F->frml_type) {
			case 'S': {
				s = current_rec_->LoadS(F2); // FD2->loadS(F2, current_rec_);
				x.StoreStr(s, *KF);
				RP->SaveS(F, s);
				break;
			}
			case 'R': {
				r = current_rec_->LoadR(F2); // FD2->loadR(F2, current_rec_);
				x.StoreReal(r, *KF);
				RP->SaveR(F, r);
				break;
			}
			case 'B': {
				b = current_rec_->LoadB(F2); // FD2->loadB(F2, current_rec_);
				x.StoreBool(b, *KF);
				RP->SaveB(F, b);
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
		delete RP; RP = nullptr;
		return result;
	}

	while (KF != K->KFlds.end()) {
		F = (*KF)->FldD;
		if (li) {
			std::vector<EditableField*>::iterator D = FindEFld(F);
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
				delete RP; RP = nullptr;
				return result;
			}
			switch (F->frml_type) {
			case 'S': {
				x.StoreStr(s, *KF);
				RP->SaveS(F, s);
				break;
			}
			case 'R': {
				x.StoreReal(r, *KF);
				RP->SaveR(F, r);
				break;
			}
			case 'B': {
				b = s[0] = AbbrYes;
				x.StoreBool(b, *KF);
				RP->SaveB(F, b);
				break;
			}
			}
			if (li) {
				found = GotoXRec(&x, n);
				if ((pos == 0) && (F->frml_type == 'S')) {
					x = x_old;
					x.StoreStr(current_rec_->LoadS(F), *KF);
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
		// memcpy(current_rec_->GetRecord(), RP->GetRecord(), file_d_->FF->RecLen);
		RP->CopyTo(current_rec_);
	}
	else if (!GotoXRec(&x, n)) {
		CreateOrErr(create, RP, n);
	}
	result = true;

	PopW(w);
	delete RP; RP = nullptr;
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
	std::vector<EditableField*>::iterator D = CFld;
	int N = CRec();
	LockMode md = file_d_->NewLockMode(RdMode);

	while (true) {
		if (!current_rec_->IsDeleted())
			while (D != edit_->FirstFld.end()) {
				LogicControl* C = CompChk(*D, '?');
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
			RdRec(N, current_rec_);
			D = edit_->FirstFld.begin();
			continue;
		}
		break;
	}

	RdRec(CRec(), current_rec_);
	DisplRecNr(CRec());
	file_d_->OldLockMode(md);
	WrLLF10Msg(120);
}

void DataEditor::Sorting()
{
	std::vector<KeyFldD*> SKRoot;
	uint8_t* p = nullptr;
	LockMode md;
	SaveFiles();
	MarkStore(p);

	if (!gc->PromptSortKeys(edit_->FD, edit_->Flds, SKRoot) || (SKRoot.empty())) {
		ReleaseStore(&p);
		//record_ = edit_->NewRec->GetRecord();
		DisplAllWwRecs();
		return;
	}

	if (!file_d_->TryLockMode(ExclMode, md, 1)) {
		ReleaseStore(&p);
		//record_ = edit_->NewRec->GetRecord();
		DisplAllWwRecs();
		return;
	}

	try {
		file_d_->SortByKey(SKRoot, RunMsgOn, RunMsgN, RunMsgOff);
		edit_->EdUpdated = true;
	}
	catch (std::exception&) {
		file_d_ = edit_->FD;
		file_d_->OldLockMode(md);
	}

	ReleaseStore(&p);
	//record_ = edit_->NewRec->GetRecord();
	DisplAllWwRecs();
}

void DataEditor::AutoReport()
{
	uint8_t* p = nullptr; RprtOpt* RO = nullptr;
	FileUseMode UM = Closed;
	MarkStore(p); RO = gc->GetRprtOpt();
	RO->FDL[0]->FD = file_d_;
	RO->Flds = edit_->Flds;
	if (params_->Select) {
		RO->FDL[0]->Cond = edit_->Bool;
		RO->CondTxt = edit_->BoolTxt;
	}
	if (params_->Subset) {
		RO->FDL[0]->ViewKey = WK;
	}
	else if (HasIndex) {
		RO->FDL[0]->ViewKey = VK;
	}
	PrintView = false;
	const std::unique_ptr auto_report = std::make_unique<ReportGenerator>();
	if (auto_report->SelForAutoRprt(RO)) {
		SpecFDNameAllowed = (file_d_ == CRdb->project_file);
		auto_report->RunAutoReport(RO);
		SpecFDNameAllowed = false;
	}
	ReleaseStore(&p);
	std::unique_ptr<TextEditor> text_editor = std::make_unique<TextEditor>(EditorMode::Unknown, TextType::Unknown);
	text_editor->ViewPrinterTxt();
	//record_ = edit_->NewRec->GetRecord();
}

void DataEditor::AutoGraph()
{
#ifdef FandGraph
	FrmlElem* Bool = nullptr;
	if (params_->Select) Bool = edit_->Bool;
	XKey* K = nullptr;
	if (params_->Subset) K = WK;
	else if (HasIndex) K = VK;
	//RunAutoGraph(edit_->Flds, K, Bool);
#endif
	//file_d_ = edit_->FD;
	//record_ = edit_->NewRec->GetRecord();
}

bool DataEditor::IsDependItem()
{
	if (!IsNewRec && (edit_->NEdSet == 0)) return false;
	//Dependency* Dp = CFld->Dependencies;
	//while (Dp != nullptr) {
	//	if (RunBool(Dp->Bool)) {
	//		return true;
	//	}
	//	Dp = Dp->pChain;
	//}

	for (const Dependency* dep : (*CFld)->Dependencies) {
		if (RunBool(file_d_, dep->Bool, current_rec_)) {
			return true;
		}
	}

	return false;
}

void DataEditor::SetDependItem()
{
	for (const Dependency* dep : (*CFld)->Dependencies) {
		if (RunBool(file_d_, dep->Bool, current_rec_)) {
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

bool DataEditor::IsSkipFld(EditableField* D)
{
	return !D->Tab &&
		(edit_->NTabsSet > 0 || (D->FldD->Flg & f_Stored) == 0 || params_->OnlySearch && FldInModeF3Key(D->FldD));
}

bool DataEditor::ExNotSkipFld()
{
	bool result = false;
	if (edit_->NFlds == 1) return result;

	std::vector<EditableField*>::iterator D = edit_->FirstFld.begin();
	while (D != edit_->FirstFld.end()) {
		if ((D != CFld) && !IsSkipFld(*D)) {
			result = true;
			break;
		}
		++D; // D = D->pChain;
	}

	return result;
}

bool DataEditor::ProcessEnter(uint16_t mode)
{
	int i = 0;
	bool b = false;
	LogicControl* C = nullptr;
	EdExitD* X = nullptr;
	WORD Brk = 0, NR = 0;
	bool displ = false;
	bool skip = false;
	bool quit = false;
	bool was_new_rec = false;
	LockMode md;
	char Typ = '\0';

	int OldCRec = CRec();
	std::vector<EditableField*>::iterator OldCFld = CFld;

	bool result = true;
	if (mode == 0 /*only bypass unrelevant fields*/) {
		goto label2;
	}
label1:
	if (IsFirstEmptyFld()) {
		//FirstEmptyFld =	FirstEmptyFld->pChain;
		++FirstEmptyFld;
	}
	quit = false;

	if (!CheckForExit(quit)) return result;

	TextAttr = edit_->dHiLi;
	DisplFld(*CFld, IRec, TextAttr, current_rec_);

	if (params_->ChkSwitch) {
		if (mode == 1 || mode == 3) {
			Typ = '?';
		}
		else {
			Typ = 'F';
		}
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

	if (quit && !IsNewRec && (mode == 1 || mode == 3)) {
		EdBreak = 12;
		result = false;
		return result;
	}

	//++CFld;

	if (CFld != edit_->FirstFld.end() - 1) {
		// this is not the last field
		//--CFld;
		GotoNextRecFld(CRec(), CFld);
		if (mode == 1 || mode == 3) {
			mode = 0;
		}
	}
	else {
		// this is the last field
		//--CFld;
		was_new_rec = IsNewRec;
		mode = 0;
		NR++;
		if (!WriteCRec(true, displ)) return result;

		if (displ) {
			DisplAllWwRecs();
		}
		else {
			SetRecAttr(IRec);
		}

		if (params_->Only1Record) {
			if (params_->NoESCPrompt) {
				EdBreak = 0;
				return false;
			}
			else {
				params_->Append = false;
				GotoRecFld(CRec(), OldCFld);
				Beep(); Beep();
				return result;
			}
		}

		if (params_->OnlySearch) {
			params_->Append = false;
			GotoRecFld(CRec(), OldCFld);
			Beep(); Beep();
			return result;
		}

		if (params_->Append) {
			AppendRecord(nullptr);
		}
		else {
			if (was_new_rec) {
				NewDisplLL = true;
			}

			if (CRec() < CNRecs())
				if (params_->Select) {
					for (i = CRec() + 1; i <= CNRecs(); i++) {
						if (KeyPressed() && (ReadKey() != 'M') && PromptYN(23)) goto label4;
						RdRec(i, current_rec_);
						DisplRecNr(i);
						if (!current_rec_->IsDeleted() && RunBool(file_d_, edit_->Bool, current_rec_)) {
							RdRec(CRec(), current_rec_);
							GotoRecFld(i, edit_->FirstFld.begin());
							goto label2;
						}
					}
				label4:
					RdRec(CRec(), current_rec_);
					DisplRecNr(CRec());
					GotoRecFld(OldCRec, OldCFld);
					Beep(); Beep();
					return result;
				}
				else {
					GotoRecFld(CRec() + 1, edit_->FirstFld.begin());
				}
			else {
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
		// it's the 1st touch of an empty field -> assign default (implicit) value (#I)
		// or assign value from previous record in duplicate mode
		if (((*CFld)->Impl != nullptr) && LockRec(true)) {
			AssignFld((*CFld)->FldD, (*CFld)->Impl);
			displ = true;
		}
		if ((*CFld)->Dupl && (CRec() > 1) && LockRec(true)) {
			DuplFromPrevRec();
			displ = true;
			skip = true;
		}
	}
	if (IsDependItem() && LockRec(true)) {
		SetDependItem();
		displ = true;
		skip = true;
	}
	if (IsSkipFld(*CFld)) skip = true;
	if ((*CFld)->Tab) skip = false;
	if (displ) {
		TextAttr = edit_->dHiLi;
		DisplFld(*CFld, IRec, TextAttr, current_rec_);
	}
	if (mode == 2) {
		/* bypass all remaining fields of the record */
		goto label1;
	}
	if (skip && ExNotSkipFld() && (NR <= 1)) {
		goto label1;
	}
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
	if (Displ) HighLightOff();
label0:
	i += Delta;
	if ((i > 0) && (i <= CNRecs())) {
		RdRec(i, current_rec_);
		if (Displ) DisplRecNr(i); // zobrazi cislo zaznamu v hlavicce
		if (!params_->Select || !current_rec_->IsDeleted() && RunBool(file_d_, edit_->Bool, current_rec_)) goto label2;
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
	RdRec(CRec(), current_rec_);
	if (Displ) {
		DisplRecNr(CRec());
		HighLightOn();
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
			for (i = Max - D + 1; i <= Max; i++) DisplayRecord(i);
		}
		else if (D < 0) {
			D = -D;
			MoveDispl(Max - D, Max, Max - D);
			for (i = 1; i <= D; i++) DisplayRecord(i);
		}
	}
label3:
	if (Displ)HighLightOn();
label4:
	file_d_->OldLockMode(md);
	return result;
}

bool DataEditor::GetChpt(pstring Heslo, int& NN)
{
	pstring s(12);

	for (int j = 1; j <= file_d_->FF->NRecs; j++) {
		file_d_->ReadRec(j, current_rec_);
		if (file_d_ == CRdb->project_file) {
			s = OldTrailChar(' ', current_rec_->LoadS(ChptName));
			short i = s.first('.');
			if (i > 0) s.Delete(i, 255);
			if (EquUpCase(Heslo, s)) {
				NN = j;
				return true;
			}
		}
		else {
			s = OldTrailChar(' ', current_rec_->LoadS(file_d_->FldD.front()));
			ConvToNoDiakr(&s[1], s.length(), fonts.VFont);
			if (EqualsMask(&Heslo[1], Heslo.length(), s)) {
				NN = j;
				return true;
			}
		}
	}
	RdRec(CRec(), current_rec_);

	return false;
}

void DataEditor::SetCRec(int I)
{
	if (I > BaseRec + edit_->NRecs - 1) BaseRec = I - edit_->NRecs + 1;
	else if (I < BaseRec) BaseRec = I;
	IRec = I - BaseRec + 1;
	RdRec(CRec(), current_rec_);
}

void DataEditor::UpdateTextField(std::string& text)
{
	LockMode md;
	//if (!params_->EdRecVar) { // nezapisujeme do souboru
	//	md = file_d_->NewLockMode(WrMode);
	//}
	SetWasUpdated();
	current_rec_->SaveS((*CFld)->FldD, text);
	//file_d_->FF->DelDifTFld((*CFld)->FldD, current_rec_->GetRecord(), original_rec_->GetRecord());
	//file_d_->saveS((*CFld)->FldD, S, current_rec_->GetRecord());
	/*if (!params_->EdRecVar) { // nezapisujeme do souboru
		file_d_->OldLockMode(md);
	}*/
}

void DataEditor::UpdateTxtPos(WORD TxtPos)
{
	LockMode md;
	if (file_d_ == CRdb->project_file) {
		md = file_d_->NewLockMode(WrMode);
		SetWasUpdated();
		current_rec_->SaveR(ChptTxtPos, (short)TxtPos);
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

	const uint8_t maxStk = 10;

	bool Srch = false, Upd = false, WasUpd = false, Displ = false, quit;
	std::string HdTxt;
	MsgStr TxtMsgS;
	MsgStr* PTxtMsgS;
	int TxtXY = 0;
	WORD R1 = 0;
	size_t OldTxtPos = 0;
	size_t TxtPos = 0; // vychozi pozice zobrazeni textu (index 1. znaku v editoru)
	WORD CtrlMsgNr = 0;
	WORD c = 0, last_len = 0;
	std::string edit_text;
	EditorMode kind = EditorMode::Unknown;
	LockMode md;
	uint8_t* p = nullptr;
	int i = 0, w = 0;
	std::vector<EdExitD*> X;
	WORD iStk = 0;
	struct { int N = 0; int I = 0; } Stk[maxStk];
	std::string heslo;

	MarkStore(p);
	Brk = 0;
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
	if (file_d_ == CRdb->project_file) {
		HdTxt = current_rec_->LoadS(ChptTyp) + ':' + current_rec_->LoadS(ChptName) + HdTxt;
		TxtPos = trunc(current_rec_->LoadR(ChptTxtPos));
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
	kind = EditorMode::View;
	OldTxtPos = TxtPos;
	if (Ed) LockRec(false);
	if ((F->Flg & f_Stored) != 0) {
		edit_text = current_rec_->LoadS(F);
		if (Ed) kind = EditorMode::Text;
	}
	else {
		edit_text = RunString(file_d_, F->Frml, current_rec_);
	}
label2:
	if (params_->TTExit) {
		X = edit_->ExD;
	}
	else {
		X.clear();
	}

	Upd = false;
	std::unique_ptr<TextEditor> editor = std::make_unique<TextEditor>(kind, TextType::Memo);
	result =
		editor->EditText(kind, TextType::Memo, HdTxt, ErrMsg, edit_text, MaxLStrLen, TxtPos, TxtXY, Breaks, X,
			Srch, Upd, 141, CtrlMsgNr, PTxtMsgS);
	ErrMsg = "";
	heslo = gc->LexWord;
	last_len = edit_text.length();
	if (EdBreak == 0xffff) {
		c = Event.Pressed.KeyCombination();
	}
	else {
		c = 0;
	}

	if (c == __ALT_EQUAL) {
		c = __ESC;
	}
	else {
		WasUpd = WasUpd || Upd;
	}

	switch (c) {
	case __ALT_F3: {
		runner_->EditHelpOrCat(c, 0, "");
		goto label2;
		break;
	}
	case 'U': {
		TxtXY = 0;
		goto label1;
		break;
	}
	}
	screen.Window(1, 1, TxtCols, TxtRows);

	if (WasUpd) {
		UpdateTextField(edit_text);
	}

	if ((OldTxtPos != TxtPos) && !Srch) {
		UpdateTxtPos(TxtPos);
	}

	if (Ed && !params_->WasUpdated) {
		UnLockRec(edit_);
	}

	if (Srch) {
		if (WriteCRec(false, Displ)) {
			goto label31;
		}
	}

	switch (c) {
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
		if (file_d_ == CRdb->project_file || (file_d_ == CRdb->help_file)) {
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
		heslo = current_rec_->LoadS(ChptTyp);
	label3:
		Help(CRdb, heslo, false);
		goto label4;
		break;
	}
	} // switch end

	if ((c > 0xFF) && WriteCRec(false, Displ)) {
		params_->Append = false;
		if (c == __CTRL_HOME) {
			GoPrevNextRec(-1, false);
			TxtXY = 0;
			goto label4;
		}
		if (c == __CTRL_END) {
		label31:
			if (!GoPrevNextRec(+1, false) && Srch) {
				UpdateTxtPos(last_len);
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
		Event.Pressed.UpdateKey(c);
		goto label6;

	}
label5:
	ReleaseStore(&p);
	DisplEditWw();
label6:
	if (w != 0) PopW(w);
	return result;
}

bool DataEditor::EditItemProc(bool del, bool ed, WORD& brk)
{
	bool result = true;

	LogicControl* C = nullptr;
	double r = 0;
	bool b = false;

	EditableField* current_field = *CFld;
	FieldDescr* field = current_field->FldD;

	if (field->field_type == FieldType::TEXT) {
		if (!EditFreeTxt(field, "", ed, brk)) {
			return false;
		}
	}
	else {
		TextAttr = edit_->dHiLi;
		std::string text = decodeField(field, field->L, current_rec_);
		screen.GotoXY(current_field->Col, FldRow(current_field, IRec));
		unsigned int wd = 0;

		if (file_d_->NotCached()) {
			wd = edit_->WatchDelay;
		}

		FieldEdit(field, current_field->Impl, current_field->L, 1, text, r, del, ed, false, wd);

		if (Event.Pressed.KeyCombination() == __ESC || !ed) {
			DisplFld(current_field, IRec, TextAttr, current_rec_);
			if (ed && !params_->WasUpdated) UnLockRec(edit_);
			return result;
		}

		SetWasUpdated();

		switch (field->frml_type) {
		case 'B': {
			//file_d_->saveB(F, toupper(text[0]) == AbbrYes, current_rec_->GetRecord());
			current_rec_->SaveB(field, toupper(text[0]) == AbbrYes);
			break;
		}
		case 'S': {
			//file_d_->saveS(F, text, current_rec_->GetRecord());
			current_rec_->SaveS(field, text);
			break;
		}
		case 'R': {
			//file_d_->saveR(F, r, current_rec_->GetRecord());
			current_rec_->SaveR(field, r);
			break;
		}
		}
	}

	if (brk == 0) {
		result = ProcessEnter(1);
	}

	return result;
}

void DataEditor::SetSwitchProc()
{
	bool B; WORD N, iMsg;
	iMsg = 104;
	if (params_->EdRecVar) goto label1;
	iMsg = 101;
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
	if (file_d_ == CRdb->project_file) {
		runner_->ReleaseFilesAndLinksAfterChapter(edit_);
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
	LockMode md;
	int n1, n2;
	Record* p1 = new Record(file_d_);
	Record* p2 = new Record(file_d_);
	XString x1, x2;
#ifdef FandSQL
	if (file_d_->IsSQLFile) return;
#endif
	if (params_->NoCreate && params_->NoDelete || params_->WasWK) return;
	if (!file_d_->TryLockMode(WrMode, md, 1)) return;

	n1 = AbsRecNr(CRec());
	file_d_->ReadRec(n1, p1);
	if (HasIndex) {
		x1.PackKF(file_d_, VK->KFlds, p1);
	}
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
	if (file_d_ == CRdb->project_file) SetCompileAll();
label1:
	file_d_->OldLockMode(md);

	delete p1; p1 = nullptr;
	delete p2; p2 = nullptr;
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

	uint8_t* p = nullptr;
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

	uint8_t* p = nullptr;
	std::string s1, s2;
	WORD Brk;
	//std::vector<std::string> SL;
	EditOpt* EO = nullptr;
	Project* R = nullptr; int w = 0;

	MarkStore(p);
	w = PushW(1, 1, TxtCols, TxtRows, true, true);
	file_d_->IRec = AbsRecNr(CRec());

	//EditD* EE = WriteParamsToE();
	std::unique_ptr<DataEditor> data_editor2 = std::make_unique<DataEditor>();

	R = CRdb;
	while (R != nullptr) {
		for (size_t i = 0; i < R->data_files.size(); i++) {
			FileD* f = R->data_files[i];
			if (data_editor2->ForNavigate(f)) {
				if (f->ViewNames.empty()) {
					std::string s = f->Name;
					if (R != CRdb) {
						s = R->project_file->Name + "." + s;
					}
					ww.PutSelect(s);
				}
				else {
					for (size_t j = 0; j < f->ViewNames.size(); j++) {
						std::string s = data_editor2->GetFileViewName(f, f->ViewNames[j]);
						if (s.empty()) {
							continue;
						}
						else {
							if (R != CRdb) {
								s = R->project_file->Name + "." + s;
							}
							ww.PutSelect(s);
						}
					}
				}
			}
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
			} while (R->project_file->Name != ss2);
		}

		for (FileD* v_file : R->data_files) {
			if (data_editor2->EquFileViewName(v_file, s1, &EO)) {
				data_editor2->file_d_ = v_file;
			}
		}

		if (data_editor2->SelFldsForEO(EO, nullptr)) {
			std::unique_ptr<EditReader> reader = std::make_unique<EditReader>();
			reader->NewEditD(data_editor2->file_d_, EO, data_editor2->current_rec_);
			data_editor2->edit_ = reader->GetEditD();
			if (data_editor2->OpenEditWw()) {
				data_editor2->RunEdit(nullptr, Brk);
			}
			SaveFiles();
		}
	}

	PopW(w);
	ReleaseStore(&p);
	DisplEditWw();
}

void DataEditor::DownEdit()
{
	wwmix ww;

	EditOpt* EO = nullptr;
	uint8_t* p = nullptr;
	MarkStore(p);

	int w = PushW(1, 1, TxtCols, TxtRows, true, true);
	file_d_->IRec = AbsRecNr(CRec());

	std::unique_ptr<DataEditor> data_editor2 = std::make_unique<DataEditor>();

	for (LinkD* ld : LinkDRoot) {
		FileD* FD = ld->FromFD;
		if ((ld->ToFD == file_d_) && data_editor2->ForNavigate(FD) && (ld->IndexRoot != 0)) {
			/*own key with equal beginning*/
			XKey* K = GetFromKey(ld);

			if (FD->ViewNames.empty()) {
				std::string s = FD->Name;
				std::string kali = K->Alias;
				if (!K->Alias.empty()) {
					s += "/" + kali;
				}
				ww.PutSelect(s);
			}
			else {
				for (size_t i = 0; i < FD->ViewNames.size(); i++) {
					std::string s = data_editor2->GetFileViewName(FD, FD->ViewNames[i]);
					if (s.empty()) {
						continue;
					}
					else {
						std::string kali = K->Alias;
						if (!K->Alias.empty()) {
							s += "/" + kali;
						}
						ww.PutSelect(s);
					}
				}
			}
		}
	}
	ss.Abcd = true;
	ww.SelectStr(0, 0, 35, "");

	if (Event.Pressed.KeyCombination() == __ESC) {
		// do nothing;
	}
	else {
		std::string s2;
		std::string s1;
		LinkD* LD = *LinkDRoot.begin();
		data_editor2->GetSel2S(s1, s2, '/', 2);

		for (LinkD* ld : LinkDRoot) {
			if ((ld->ToFD != file_d_)
				|| (ld->IndexRoot == 0)
				|| (s2 != GetFromKey(LD)->Alias)
				|| !data_editor2->EquFileViewName(ld->FromFD, s1, &EO)) {
				continue;
			}
			else {
				LD = ld;
			}
		}

		data_editor2->file_d_ = LD->FromFD;
		if (EO == nullptr) {
			EO = new EditOpt();
		}
		if (data_editor2->SelFldsForEO(EO, LD)) {
			EO->DownLD = LD;
			EO->DownRecord = new Record(file_d_);
			EditReader* reader = new EditReader();
			reader->NewEditD(data_editor2->file_d_, EO, data_editor2->current_rec_);
			data_editor2->edit_ = reader->GetEditD();
			if (data_editor2->OpenEditWw()) {
				WORD Brk;
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
	delete EO; EO = nullptr;
}

void DataEditor::ShiftF7Proc()
{
	/* find last (first decl.) foreign key link with CFld as an argument */
	FieldDescr* F = (*CFld)->FldD;
	LinkD* LD1 = nullptr;
	for (LinkD* ld : LinkDRoot) {
		for (KeyFldD* arg : ld->Args) {
			if ((arg->FldD == F) && ForNavigate(ld->ToFD)) LD1 = ld;
		}
	}
	if (LD1 != nullptr) {
		UpwEdit(LD1);
	}
}

bool DataEditor::ShiftF7Duplicate()
{
	bool result = false;

	//EditD* ee = edit_->ShiftF7_caller;
	//file_d_ = ee->FD;
	//record_ = ee->NewRec->GetRecord();

	//if (!ELockRec(ee, file_d_->IRec, ee->IsNewRec, ee->params_->Subset)) {
	//	return result;
	//}

	//if (!params_->WasUpdated) {
	//	Move(record_, ee->OldRec->GetRecord(), file_d_->FF->RecLen);
	//	params_->WasUpdated = true;
	//}

	//for (KeyFldD* kf2 : edit_->ShiftF7_link->ToKey->KFlds) {
	//	for (KeyFldD* arg : edit_->ShiftF7_link->Args) {
	//		DuplFld(edit_->FD, file_d_, edit_->NewRec->GetRecord(), record_, ee->OldRec->GetRecord(), kf2->FldD, arg->FldD);
	//		//kf2 = kf2->pChain;
	//	}
	//}

	//file_d_->SetRecordUpdateFlag(record_);
	//file_d_ = edit_->FD;
	//record_ = edit_->NewRec->GetRecord();
	//result = true;

	return result;
}

bool DataEditor::DuplToPrevEdit()
{
	LockMode md;
	bool result = false;
	//EditD* ee = nullptr; // TODO: edit_->pChain;
	//if (ee == nullptr) return result;
	//FieldDescr* f1 = (*CFld)->FldD;

	///* !!! with ee^ do!!! */
	//FieldDescr* f2 = (*CFld)->FldD;
	//if ((f2->Flg && f_Stored == 0) || (f1->field_type != f2->field_type) || (f1->L != f2->L)
	//	|| (f1->M != f2->M) || !(*CFld)->Ed(IsNewRec)) {
	//	WrLLF10Msg(140);
	//	return result;
	//}
	//file_d_ = ee->FD;
	//record_ = ee->NewRec->GetRecord();
	//if (!ELockRec(ee, file_d_->IRec, ee->IsNewRec, ee->params_->Subset)) {
	//	return result;
	//}
	//if (!params_->WasUpdated) {
	//	Move(record_, ee->OldRec->GetRecord(), file_d_->FF->RecLen);
	//	params_->WasUpdated = true;
	//}
	//DuplFld(edit_->FD, file_d_, edit_->NewRec->GetRecord(), record_, ee->OldRec->GetRecord(), f1, f2);
	//file_d_->SetRecordUpdateFlag(record_);

	//file_d_ = edit_->FD; 
	//record_ = edit_->NewRec->GetRecord();
	//result = true;

	//keyboard.AddToFrontKeyBuf(0x0D); // ^M .. \r .. #13
	////pstring oldKbdBuffer = KbdBuffer;
	////KbdBuffer = 0x0D; // ^M
	////KbdBuffer += oldKbdBuffer;
	return result;
}

void DataEditor::Calculate2()
{
	wwmix ww;

	FrmlElem* Z; std::string txt; WORD I; pstring Msg;
	uint8_t* p = nullptr; char FTyp; double R; FieldDescr* F; bool Del;
	//MarkStore(p);
	//NewExit(Ovr(), er);
	//goto label2;
	try {
		gc->ResetCompilePars();
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
		gc->SetInpStr(txt);
		gc->RdLex();
		Z = gc->RdFrml(FTyp, nullptr);
		if (gc->Lexem != 0x1A) gc->Error(21);
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
						SetWasUpdated();
						current_rec_->SaveR(F, R * Power10[F->M]);
					}
					else
						label5:
					AssignFld(F, Z);
					DisplFld(*CFld, IRec, TextAttr, current_rec_);
					HighLightOn();
					goto label3;
				}
			}
			else WrLLF10Msg(140);
		}
		switch (FTyp) {
		case 'R': {
			R = RunReal(file_d_, Z, current_rec_);
			str(R, 30, 10, txt);
			txt = LeadChar(' ', TrailChar(txt, '0'));
			if (txt[txt.length() - 1] == '.') {
				txt = txt.substr(0, txt.length() - 1);
			}
			break;
		}
		case 'S': {
			/* wie RdMode fuer T ??*/
			txt = RunString(file_d_, Z, current_rec_);
			break;
		}
		case 'B': {
			if (RunBool(file_d_, Z, current_rec_)) txt = AbbrYes;
			else txt = AbbrNo;
			break;
		}
		}
		goto label4;
	}
	catch (std::exception& e) {
		//label2:
		Msg = MsgLine;
		I = gc->input_pos;
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
	// a new rec should not have saved T fields in a file:
	// file_d_->DelAllDifTFlds(current_rec_, nullptr);
	if (CNRecs() == 1) return;
	IsNewRec = false;
	params_->Append = false;
	params_->WasUpdated = false;
	CFld = edit_->FirstFld.begin();
	if (CRec() > CNRecs()) { // pozor! uspodarani IF a ELSE neni jasne !!!
		if (IRec > 1) IRec--;
		else BaseRec--;
	}
	RdRec(CRec(), current_rec_);
	NewDisplLL = true;
	DisplWwRecsOrPage(CPage, &RT);
}

std::vector<EditableField*>::iterator DataEditor::FrstFldOnPage(WORD Page)
{
	std::vector<EditableField*>::iterator D = edit_->FirstFld.begin();
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
		XKey* k = VK;
		if (params_->Subset) {
			k = WK;
		}
		if (params_->WasUpdated) {
			x.PackKF(file_d_, k->KFlds, original_rec_);
		}
		else {
			x.PackKF(file_d_, k->KFlds, current_rec_);
		}
	}
	EdRecKey = x.S;
}

bool DataEditor::StartProc(Instr_proc* ExitProc, bool Displ)
{
	//std::unique_ptr<uint8_t[]> p;
	/*float t;*/

	bool result = false;
	file_d_->FF->WasWrRec = false;

	//if (HasTF) {
	//	p = file_d_->GetRecSpaceUnique();
	//	memcpy(p.get(), current_rec_->GetRecord(), file_d_->FF->RecLen);
	//}

	SetEdRecNoEtc(0);
	bool lkd = edit_->IsLocked;

	if (!lkd && !LockRec(false)) return result;

	bool b = params_->WasUpdated;
	EdUpdated = b;
	bool b2 = current_rec_->IsUpdated(); //file_d_->HasRecordUpdateFlag(current_rec_->GetRecord());
	SetWasUpdated();
	current_rec_->ClearUpdated(); //file_d_->ClearRecordUpdateFlag(current_rec_->GetRecord());

	// upravime argumenty exit procedury
	ExitProc->TArg[ExitProc->N - 1].FD = file_d_;
	ExitProc->TArg[ExitProc->N - 1].record = current_rec_;

	// some methods use RunString "_edfile" to identify caller
	EditDRoot = edit_;

	LockMode md = file_d_->FF->LMode;

	//EditD* EE = WriteParamsToE();                            /*t = currtime;*/

	std::unique_ptr<RunProcedure> runner = std::make_unique<RunProcedure>();
	runner->CallProcedure(ExitProc);

	//ReadParamsFromE(EE);

	file_d_->NewLockMode(md);
	bool upd = file_d_->FF->WasWrRec;      /*writeln(strdate(currtime-t,"ss mm.ttt"));wait;*/

	if (current_rec_->IsUpdated()) {
		b = true;
		upd = true;
	}

	params_->WasUpdated = b;

	if (b2) {
		current_rec_->SetUpdated();
	}
	if (!params_->WasUpdated && !lkd) {
		UnLockRec(edit_);
	}
	if (Displ && upd) {
		DisplAllWwRecs();
	}
	if (Displ) {
		NewDisplLL = true;
	}

	result = true;

	//if (HasTF) {
	//	for (FieldDescr* f : file_d_->FldD) {
	//		if ((f->field_type == FieldType::TEXT) && f->isStored() && (p.get() == original_rec_->GetRecord()))
	//			params_->NoDelTFlds = true;
	//	}
	//}

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
		k->OneRecIdx(file_d_, VK->KFlds, AbsRecNr(CRec()), current_rec_);
	}
	else {
		std::vector<KeyFldD*> unused;
		k->OneRecIdx(file_d_, unused, AbsRecNr(CRec()), current_rec_);
	}

	RO->FDL[0]->FD = file_d_;
	RO->FDL[0]->ViewKey = k;

	std::unique_ptr<RunProcedure> runner = std::make_unique<RunProcedure>();
	runner->ReportProc(RO, false);
	
	file_d_ = edit_->FD;

	//record_ = edit_->NewRec->GetRecord();
	delete current_rec_; current_rec_ = new Record(file_d_);
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
	WORD result = 0;
	WORD key_code = Event.Pressed.KeyCombination();
	for (EdExitD* X : edit_->ExD) {
		if (TestExitKey(key_code, X)) {
			ClrEvent();
			LastTxtPos = -1;
			if (X->Typ == 'Q') {
				result = 1;
			}
			else {
				bool prev_EdOk = EdOk;
				EdOk = false;
				StartExit(X, true);
				result = EdOk ? 3 : 2;
				EdOk = prev_EdOk;
			}
		}
	}
	if (((result == 0) || (result == 3)) && (key_code == __SHIFT_F7) && (*CFld)->Ed(IsNewRec)) {
		ShiftF7Proc();
		result = 2;
	}
	return result;
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
		if (!edit_->Last.empty() /*&& edit_->Last != " "*/) {
			WrLLMsgTxt(edit_->Last);
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
			WrLLMsgTxt(edit_->CtrlLast);
		}
		else if (file_d_ == CRdb->project_file) WrLLMsg(125);
		else if (params_->EdRecVar) WrLLMsg(154);
		else WrLLMsg(127);
	}
	else if ((Flags & 0x03) != 0) {         /* Shift */
		if (!edit_->ShiftLast.empty()) {
			MsgLine = edit_->ShiftLast;
			WrLLMsgTxt(edit_->ShiftLast);
		}
		else DisplLL();
	}
	else if ((Flags & 0x08) != 0) {         /* Alt */
		if (!edit_->AltLast.empty()) {
			MsgLine = edit_->AltLast;
			WrLLMsgTxt(edit_->AltLast);
		}
		else DisplLL();
	}
}

// po nacteni editoru se smycka drzi tady a ceka na stisknuti klavesy
void DataEditor::CtrlReadKbd()
{
	uint8_t flgs = 0;
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

	if (file_d_->NotCached()) {
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
		std::vector<EditableField*>::iterator D = edit_->FirstFld.begin();
		while (D != edit_->FirstFld.end()) {
			if (IsNewRec && (i == IRec) && (D == FirstEmptyFld)) goto label1;
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
	if (k->RecNrToPath(file_d_, x, n, current_rec_)) {
		k->NR--;
		k->DeleteOnPath(file_d_);
	}
	else {
		k->NR++;
		k->Insert(file_d_, n, false, current_rec_);
	}
	SetRecAttr(IRec);
	HighLightOn();
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

void DataEditor::GoStartFld(EditableField* SFld)
{
	std::vector<EditableField*>::iterator nextCFld = CFld;
	++nextCFld;

	while ((*CFld != SFld) && (nextCFld != edit_->FirstFld.end())) {
		if (IsFirstEmptyFld()) {
			if (((*CFld)->Impl != nullptr) && LockRec(true)) {
				AssignFld((*CFld)->FldD, (*CFld)->Impl);
			}
			++FirstEmptyFld; // = FirstEmptyFld->pChain;
			DisplFld(*CFld, IRec, TextAttr, current_rec_);
		}
		GotoRecFld(CRec(), nextCFld);
		nextCFld = CFld;
		++nextCFld;
	}
}

std::string DataEditor::decodeField(FieldDescr* F, WORD LWw, Record* record)
{
	double r = 0;
	std::string s;
	bool b = false;
	switch (F->frml_type) {
	case 'R': {
		r = record->LoadR(F);
		break;
	}
	case 'S': {
		if (F->field_type == FieldType::TEXT) {
			std::string txt;
			if (F->isStored() && record->LoadS(F).empty()) {
				txt = ".";
			}
			else {
				txt = "*";
			}
			return txt;
		}
		else {
			s = record->LoadS(F);
		}
		break;
	}
	default: {
		b = record->LoadB(F);
		break;
	}
	}
	return decodeFieldRSB(F, LWw, r, s, b);
}

std::string DataEditor::decodeFieldRSB(FieldDescr* F, WORD LWw, double R, std::string& T, bool B)
{
	WORD L = 0, M = 0;
	char C = 0;
	L = F->L; M = F->M;
	switch (F->field_type) {
	case FieldType::DATE: {
		T = StrDate(R, F->Mask);
		break;
	}
	case FieldType::NUMERIC: {
		C = '0';
		justifyString(T, L, M, C);
		break;
	}
	case FieldType::ALFANUM: {
		C = ' ';
		justifyString(T, L, M, C);
		break;
	}
	case FieldType::BOOL: {
		if (B) T = AbbrYes;
		else T = AbbrNo;
		break;
	}
	case FieldType::REAL: {
		str(R, L, T);
		break;
	}
	default: /*"F"*/ {
		if ((F->Flg & f_Comma) != 0) R = R / Power10[M];
		str(RoundReal(R, M), L, M, T);
		break;
	}
	}
	if (T.length() > L) {
		T = T.substr(0, L);
		T[L - 1] = '>';
	}
	if (T.length() > LWw) {
		if (M == LeftJust) {
			T = T.substr(0, LWw);
		}
		else {
			// T = copy(T, T.length() - LWw + 1, LWw);
			T = T.substr(T.length() - LWw + 1, LWw);
		}
	}
	return T;
}

void DataEditor::justifyString(std::string& T, WORD L, WORD M, char C)
{
	if (M == LeftJust)
		while (T.length() < L) T += C;
	else {
		if (T.length() < L) {
			char buf[256]{ 0 };
			sprintf_s(buf, 256, "%*s", L, T.c_str());
			std::string s(buf, L);
			T = s;
		}
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
	uint8_t EdBr = 0;
	WORD KbdChar;

	Brk = 0;
	DisplLL();
	if (params_->OnlySearch) goto label2;
	if (!IsNewRec && (PX != nullptr)) {
		GotoXRec(PX, n);
	}
	if (params_->Select && !RunBool(file_d_, edit_->Bool, current_rec_)) {
		GoPrevNextRec(+1, true);
	}
	//if (/*edit_->StartFld != nullptr*/ true) { GoStartFld(&edit_->StartFld); goto label1; }
	if (edit_->StartFld != nullptr) {
		GoStartFld(edit_->StartFld);
		goto label1;
	}
label0:
	if (!ProcessEnter(0)) {
		goto label7;
	}
label1:
	LongBeep = 0;
	OldTimeW = getMillisecondsNow();
label81:
	OldTimeR = getMillisecondsNow();
	CtrlReadKbd();
	if (file_d_->NotCached()) {
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

		switch (ExitKeyProc()) {
		case 1: {
			// quit
			ClrEvent();
			goto label7;
			break;
		}
		case 2: {
			// exit
			ClrEvent();
			goto label1;
			break;
		}
		default:;
		}

		if (Event.Pressed.isChar()) {
			// jedna se o tisknutelny znak
			if ((*CFld)->Ed(IsNewRec) && (((*CFld)->FldD->field_type != FieldType::TEXT) || current_rec_->LoadS((*CFld)->FldD).empty())
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
				std::string msg = ReadMessage(7);
				FandHelp(HelpFD, msg, false);
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
				if ((IsNewRec
					|| WriteCRec(true, Displ)) && (EdBreak == 11)
					|| params_->NoESCPrompt
					|| (!spec.ESCverify && !params_->MustESCPrompt)
					|| PromptYN(137)) {
					EdBreak = EdBr;
					SetEdRecNoEtc(n);
				label71:
					if (IsNewRec && !params_->EdRecVar) DelNewRec();
					HighLightOff();
					EdUpdated = edit_->EdUpdated;
					if (!params_->EdRecVar) {
						// TODO: is there TWork? file_d_->ClearRecSpace(current_rec_->GetRecord());
					}
					if (params_->Subset && !params_->WasWK) {
						WK->Close(file_d_);
					}
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
				if (!ProcessEnter(2)) goto label7;
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
				if (IsNewRec && (FirstEmptyFld != edit_->FirstFld.end())) {
					GotoRecFld(CRec(), FirstEmptyFld);
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
						if (!ProcessEnter(3)) {
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
				// duplikace zaznamu 
				if ((CRec() > 1) && (IsFirstEmptyFld() || PromptYN(121)) && LockRec(true)) {
					DuplFromPrevRec();
					if (!ProcessEnter(1)) goto label7;
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
								if (b && !ProcessEnter(0)) {
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
							if (file_d_ == CRdb->project_file) {
								if (KbdChar == __ALT_F3) {
									ForAllFDs(ForAllFilesOperation::close_passive_fd);
									runner_->EditHelpOrCat(KbdChar, 0, "");
								}
								else {
									Brk = 2;
									SetEdRecNoEtc(0);
									goto label71;
								}
							}
							else if (IsTestRun && (file_d_ != catalog->GetCatalogFile()) && (KbdChar == __ALT_F2)) {
								runner_->EditHelpOrCat(KbdChar, 1, file_d_->Name + "." + (*CFld)->FldD->Name);
							}
							break;
						case __F6: {
							if (!params_->EdRecVar) {
								F6Proc();
							}
							break;
						}
						case __F4: {
							if (DuplToPrevEdit()) {
								EdBreak = 14;
								SetEdRecNoEtc(0);
								goto label71;
							}
							break;
						}
						case __CTRL_F7: {
							DownEdit();
							break;
						}
						case __F8: {
							if (edit_->SelKey != nullptr) {
								ToggleSelectRec();
								GoPrevNextRec(+1, true);
							}
							break;
						}
						case __F3: {
							// najdi vetu podle klic. udaje
							if (!params_->EdRecVar)
								if (file_d_ == CRdb->help_file) {
									if (runner_->PromptHelpName(file_d_, i)) {
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
							if (file_d_ == CRdb->project_file) {
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
	uint8_t* p = nullptr;
	int w1 = 0, w2 = 0, w3 = 0;
	WORD Brk = 0, r1 = 0, r2 = 0;
	bool pix = false;
	EditReader* reader = new EditReader();

	MarkStore(p);
	if (EO->SyntxChk) {
		IsCompileErr = false;

		try {
			reader->NewEditD(FD, EO, current_rec_);
		}
		catch (std::exception& e) {
			// TODO: log error
		}

		if (IsCompileErr) {
			EdRecKey = MsgLine;
			LastExitCode = gc->input_pos + 1;
			IsCompileErr = false;
		}
		else {
			LastExitCode = 0;
		}
		//PopEdit();
		return;
	}
	reader->NewEditD(FD, EO, current_rec_);

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
