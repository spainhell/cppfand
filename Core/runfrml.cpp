#include "runfrml.h"

#include <cmath>
#include <ctime>
#include <memory>

#include "../Common/CommonVariables.h"
#include "../Common/compare.h"
#include "../Common/pstring.h"
#include "../Common/textfunc.h"
#include "../DataEditor/DataEditor.h"
#include "../fandio/directory.h"
#include "../fandio/FandTFile.h"
#include "../fandio/XWKey.h"
#include "../Common/random.h"
#include "../Drivers/constants.h"
#include "../MergeReport/InpD.h"

#include "../fandio/FieldDescr.h"
#include "../Common/FileD.h"
#include "GlobalVariables.h"
#include "../fandio/KeyFldD.h"
#include "legacy.h"
#include "../Common/LinkD.h"
#include "oaccess.h"
#include "obaseww.h"
#include "rdproc.h"
#include "rdrun.h"
#include "runproc.h"
#include "wwmix.h"
#include "../Common/DateTime.h"
#include "../Common/Record.h"

double Owned(FileD* file_d, FrmlElem* Bool, FrmlElem* Sum, LinkD* LD, Record* record)
{
	double r;
	XString x;
	x.PackKF(file_d, LD->ToKey->KFlds, record);

	FileD* fromFD = LD->FromFD;
	LockMode md = fromFD->NewLockMode(RdMode);
	fromFD->FF->TestXFExist();
	XKey* K = GetFromKey(LD);

	if ((Bool == nullptr) && (Sum == nullptr) && !fromFD->IsSQLFile) {
		int n;
		int nBeg;
		K->FindNr(fromFD, x.S, nBeg);
		x.S[0];
		x.S[x.S.length()] = 0xFF;
		K->FindNr(fromFD, x.S, n);
		r = n - nBeg;
	}
	else {
		r = 0;
		Record* newRecord = new Record(fromFD);
		std::vector<KeyInD*> empty;
		XScan* Scan = new XScan(fromFD, K, empty, true);
		Scan->ResetOwner(&x, nullptr);
		while (true) {
			Scan->GetRec(newRecord);
			if (!Scan->eof) {
				if (RunBool(fromFD, Bool, newRecord)) {
					if (Sum == nullptr) {
						r = r + 1;
					}
					else {
						r = r + RunReal(fromFD, Sum, newRecord);
					}
				}
				continue;
			}
			break;
		}
		Scan->Close();
		delete newRecord; newRecord = nullptr;
	}
	fromFD->OldLockMode(md);

	return r;
}

short CompBool(bool B1, bool B2)
{
	if (B1 > B2) return _gt;
	if (B1 < B2) return _lt;
	return _equ;
}

short CompReal(double R1, double R2, short M)
{
	// porovname nejdriv celou cast
	auto tR1 = trunc(R1);
	auto tR2 = trunc(R2);
	if (tR1 > tR2) return _gt;
	if (tR1 < tR2) return _lt;
	// cisla jsou stejna nebo se lisi jen desetinnou casti
	// zjistime znamenko (zaporne jsou ve vysledku opacne)
	bool neg = R1 < 0;
	// vezmeme jen desetinnou cast
	R1 = R1 - tR1;
	R2 = R2 - tR2;
	if (M > 0) {
		R1 = trunc(R1 * Power10[M]);
		R2 = trunc(R2 * Power10[M]);
	}
	//if (M >= 0) {
	//	if (R1 >= 0) R1 = int(R1 + 0.5);
	//	else R1 = int(R1 - 0.5);
	//	if (R2 >= 0) R2 = int(R2 + 0.5);
	//	else R2 = int(R2 - 0.5);
	//}
	if (neg) { R1 = -R1; R2 = -R2; }
	if (R1 > R2) return _gt;
	if (R1 < R2) return _lt;
	return _equ;
}

pstring LeadChar(char C, pstring S)
{
	// TODO: do it better
	while (S.length() > 0 && (S[1] == (uint8_t)C)) {
		S = S.substr(1);
	}
	return S;
}

double RunRealStr(FileD* file_d, FrmlElem* X, Record* record)
{
	pstring Mask;
	double result;
	switch (X->Op) {
	case _valdate: {
		FrmlElemDateMask* iX = (FrmlElemDateMask*)X;
		result = ValDate(RunString(file_d, iX->P1, record), iX->Mask);
		break;
	}
	case _val: {
		FrmlElemDateMask* iX = (FrmlElemDateMask*)X;
		std::string valS = RunString(file_d, iX->P1, record);
		valS = TrailChar(valS, ' ');
		valS = LeadChar(valS, ' ');
		short i;
		result = valDouble(valS, i);
		break;
	}
	case _length: {
		FrmlElemDateMask* iX = (FrmlElemDateMask*)X;
		std::string s = RunString(file_d, iX->P1, record);
		result = s.length();
		break;
	}
	case _linecnt: {
		// get line counts of input text
		FrmlElemDateMask* iX = (FrmlElemDateMask*)X;
		std::string s = RunString(file_d, iX->P1, record);
		result = CountLines(s, '\r'); // 0x0D, #13
		break;
	}
	case _ord: {
		FrmlElemDateMask* iX = (FrmlElemDateMask*)X;
		std::string s = RunString(file_d, iX->P1, record);
		if (s.empty()) result = 0;
		else result = (uint8_t)s[0];
		break;
	}
	case _prompt: {
		FrmlElemPrompt* iX = (FrmlElemPrompt*)X;
		std::string s = RunString(file_d, iX->P1, record);
		std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>();
		result = data_editor->PromptR(s, iX->P2, iX->FldD);
		break;
	}
	case _pos: {
		FrmlElemPosReplace* iX = (FrmlElemPosReplace*)X;
		std::string strS = RunString(file_d, iX->P2, record);
		std::string strMask = RunString(file_d, iX->P1, record);
		size_t n = 1; // kolikaty vyskyt najit

		if (iX->Options.find('~') != std::string::npos) {
			// convert to lexical strings
			for (char& i : strS) {
				i = CharOrdTab[(uint8_t)i];
			}
			for (char& i : strMask) {
				i = CharOrdTab[(uint8_t)i];
			}
		}

		if (iX->P3 != nullptr) {
			// which occurrence - count (kolikaty vyskyt)
			n = RunInt(file_d, iX->P3, record);
			if (n < 1) return 0; // 0 - nenalezeno
		}
		size_t offset = 0;
		for (size_t i = 0; i < n; i++) {
			const size_t found = strS.find(strMask, offset);
			if (found == std::string::npos) {
				// n-ty vyskyt nenalezen
				return 0; // 0 - nenalezeno
			}
			offset = found;
			if (i < n - 1) offset += strMask.length(); // next search behind last found 
		}
		return offset + 1; // vraci se PASCAL pozice v retezci (1 .. N);
		break;
	}
	case _diskfree: {
		auto iX = (FrmlElemFunction*)X;
		std::string s = RunString(file_d, iX->P1, record);
		DWORD error;
		result = GetDiskFree(s[0], error);
		break;
	}
#ifdef FandSQL
	case _sqlfun: if (Strm1 = nullptr) RunRealStr = 0 else {
		S = RunString(X->frml_elem); RunRealStr = Strm1->SendTxt(S, false);
		ReleaseStore(S);
	}
#endif

	default: {
		result = 0;
		break;
	}
	}
	return result;
}

double RMod(FileD* file_d, FrmlElemFunction* X, Record* record)
{
	double R1, R2;
	R1 = RunReal(file_d, X->P1, record);
	R2 = RunReal(file_d, X->P2, record);
	return int(R1 - int(R1 / R2) * R2);
}

double LastUpdate(const std::string& path)
{
	double result = 0.0;
	const time_t dt = lastWriteTime(path);
	if (dt != 0) {
		struct tm lt;
		errno_t err = localtime_s(&lt, &dt);
		result = RDate(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec, 0);
	}
	return result;
}

WORD TypeDay(double R)
{
	WORD d;
	if ((R >= WDaysFirst) && (R <= WDaysLast)) {
		d = trunc(R - WDaysFirst);
		for (WORD i = 0; i < NWDaysTab; i++)
			if (WDaysTab[i].Nr == d) {
				return WDaysTab[i].Typ;
			}
	}
	d = (int)trunc(R) % 7;
	switch (d) {
	case 0: d = 2/*Su*/; break;
	case 6: d = 1/*Sa*/; break;
	default: d = 0; break;
	}
	return d;
}

double AddWDays(double R, short N, WORD d)
{
	if (N > 0) {
		while ((N > 0) && (R <= 748383.0/*2050*/)) {
			R = R + 1;
			if (TypeDay(R) == d) N--;
		}
	}
	else {
		while ((N < 0) && (R >= 1)) {
			R = R - 1;
			if (TypeDay(R) == d) N++;
		}
	}
	return R;
}

double DifWDays(double R1, double R2, WORD d)
{
	short N = 0;
	double x1 = R1; double x2 = R2;
	bool neg = false;
	if (x1 > x2) {
		x1 = R2; x2 = R1;
		neg = true;
	}
	x1 = x1 + 1;
	if ((x1 >= 697248.0 /*1910*/) && (x2 <= 748383.0 /*2050*/))
		while (x1 <= x2) {
			if (TypeDay(x1) == d) N++;
			x1++;
		}
	if (neg) N = -N;
	return (int)N;
}

int GetFileSize()
{
	TestMountVol(CPath[1]);
	FileUseMode um = RdOnly;
	if (IsNetCVol()) um = Shared;
	HANDLE h = OpenH(CPath, _isOldFile, um);
	if (HandleError != 0) {
		return -1;
	}
	long result = FileSizeH(h);
	CloseH(&h);
	return result;
}

int RecNoFun(FileD* file_d, FrmlElemRecNo* Z, Record* record)
{
	int n = 0;
	XString x;
	GetRecNoXString(file_d, Z, x, record);

	XKey* k = Z->Key;
	FileD* funcFD = Z->FFD;

	LockMode md = funcFD->NewLockMode(RdMode);
	Record* newRecord = new Record(funcFD);
	if (funcFD->GetNRecs() > 0) {
		bool b;
		if (funcFD->IsIndexFile()) {
			funcFD->FF->TestXFExist();
			b = k->SearchInterval(funcFD, x, false, n);
		}
		else b = funcFD->SearchKey(x, k, n, newRecord);
		if (!b) n = -n;
	}
	else {
		n = -1;
	}
	funcFD->OldLockMode(md);
	delete newRecord; newRecord = nullptr;

	return n;
}

int AbsLogRecNoFun(FileD* file_d, FrmlElemRecNo* Z, Record* record)
{
	int result = 0;

	XKey* k = Z->Key;
	int N = RunInt(file_d, Z->Arg[0], record);
	if (N <= 0) {
		return result;
	}

	FileD* funcFD = Z->FFD;
	LockMode md = funcFD->NewLockMode(RdMode);
	if (N > funcFD->GetNRecs()) {
		funcFD->OldLockMode(md);
		return result;
	}
	if (funcFD->IsIndexFile()) {
		funcFD->FF->TestXFExist();
		if (Z->Op == _recnolog) {
			Record* newRecord = new Record(funcFD);
			funcFD->ReadRec(N, newRecord);
			if (newRecord->IsDeleted()) {
				funcFD->OldLockMode(md);
				return result;
			}
			result = k->RecNrToNr(funcFD, N, newRecord);
			delete newRecord; newRecord = nullptr;
		}
		else /*_recnoabs*/ {
			if (N > k->NRecs()) {
				funcFD->OldLockMode(md);
				return result;
			}
			result = k->NrToRecNr(funcFD, N);
		}
	}
	else {
		result = N;
	}

	funcFD->OldLockMode(md);
	return result;
}

double LinkProc(FrmlElemLink* X, Record* record)
{
	int N;

	LinkD* LD = X->LinkLD;
	FileD* fromFD = LD->FromFD;
	if (X->LinkFromRec) {
		//if (!LinkUpw(LD, N, false, X->LinkLV->record->GetRecord(), &rec)) {
		//	N = -N;
		//}
		Record* rec = LinkUpw(LD, N, false, X->LinkLV->record);
		if (rec == nullptr) {
			N = -N;
		}
		delete rec; rec = nullptr;
	}
	else {
		N = RunInt(fromFD, X->LinkRecFrml, record);
		LockMode md = fromFD->NewLockMode(RdMode);
		if ((N <= 0) || (N > fromFD->GetNRecs())) {
			SetMsgPar(fromFD->Name, LD->RoleName);
			fromFD->RunErrorM(md);
			RunError(609);
		}
		Record* newRecord = new Record(fromFD);
		fromFD->ReadRec(N, newRecord);
		fromFD->OldLockMode(md);
		Record* rec = LinkUpw(LD, N, false, X->LinkLV->record);
		if (rec == nullptr) {
			N = -N;
		}
		delete rec; rec = nullptr;
		delete newRecord; newRecord = nullptr;
	}
	return N;
}

WORD IntTSR(FileD* file_d, FrmlElem* X, Record* record)
{
	void* p;
	pstring s;
	bool b = false;
	double r = 0.0;

	auto iX0 = (FrmlElemFunction*)X;
	uint8_t IntNr = RunInt(file_d, iX0->P1, record);
	WORD FunNr = RunInt(file_d, iX0->P2, record);
	FrmlElem* z = iX0->P3;

	switch (iX0->N31) {
	case 'r': { p = z; break; }
	case 'S': { s = RunString(file_d, z, record); p = &s; break; }
	case 'B': { b = RunBool(file_d, z, record); p = &b; break; }
	case 'R': { r = RunReal(file_d, z, record); p = &r; break; }
	}

	if (IntNr == 0x16 && FunNr == 0x200 && iX0->N31 == 'R' && r == 0.0) {
		// IntTSR(22, 512, 0) - get key states
		WORD result = 0;
		const bool numLock = keyboard.GetState(VK_NUMLOCK) & 0x0001;
		const bool capsLock = keyboard.GetState(VK_CAPITAL) & 0x0001;
		const bool scrollL = keyboard.GetState(VK_SCROLL) & 0x0001;
		const bool alt = keyboard.GetState(VK_MENU) & 0x8000;
		const bool ctrl = keyboard.GetState(VK_CONTROL) & 0x8000;
		const bool l_shift = keyboard.GetState(VK_LSHIFT) & 0x8000;
		const bool r_shift = keyboard.GetState(VK_RSHIFT) & 0x8000;
		if (capsLock) result += 0b01000000;
		if (numLock)  result += 0b00100000;
		if (scrollL)  result += 0b00010000;
		if (alt)      result += 0b00001000;
		if (ctrl)     result += 0b00000100;
		if (l_shift)  result += 0b00000010;
		if (r_shift)  result += 0b00000001;
		return result;
	}

	if (z->Op == _getlocvar) {
		p = ((FrmlElemLocVar*)z)->locvar;
		switch (iX0->N31) {
		case 'R': {
			p = &r;
			break;
		}
		case 'S': {
			//std::string ss = s;
			//TWork.Delete((int)p);
			//int tmp = TWork.Store(ss);
			//p = &tmp;
			break;
		}
		case 'B': {
			p = &b;
			break;
		}
		}
	}
	return 0; // puvodne se vracel obsah AX registru
}

WORD PortIn(bool IsWord, WORD Port)
{
	/*asm mov dx, Port;xor ax, ax; cmp IsWord, 0; je @1; in ax, dx; jmp @2;
	@1:   in al, dx;
	@2:;*/
	return 0;
}

//std::string CopyLine(std::string& S, WORD N, WORD M)
//{
//	size_t begin = 0;
//	size_t end = 0;
//	size_t LFcount = 0;
//	for (size_t i = 0; i < S.length(); i++) {
//		if (S[i] == '\r') LFcount++;
//		if (LFcount + 1 == N) {
//			if (N != 1) i++; // 1. radek pred sebou nema \r
//			begin = i; // nastavujeme jen pri prvnim nalezu
//			break;
//		}
//	}
//	LFcount = 0;
//	for (size_t i = begin + 1; i < S.length(); i++) {
//		if (S[i] == '\r') LFcount++;
//		if (LFcount == M) {
//			end = i;
//			break;
//		}
//	}
//	std::string line = S.substr(begin, end - begin);
//	return line;
//}

//LongStr* CopyLine(LongStr* S, WORD N, WORD M) {
//	WORD i = 1;
//	if (N > 1) {
//		i = FindCtrlM(S, 1, N - 1);
//		i = SkipCtrlMJ(S, i);
//	}
//	WORD j = FindCtrlM(S, i, M);
//	WORD l = j - i;
//	if ((i > 1) && (l > 0)) Move(&S->A[i], &S->A[1], l);
//	S->LL = l;
//	//ReleaseAfterLongStr(S);
//	return S;
//}

LocVar* RunUserFunc(FileD* file_d, FrmlElemUserFunc* X, Record* record)
{
	LocVar* return_lv = nullptr; // tady bude ulozena posledni promenna, ktera je pak navratovou hodnotou

	//for (LocVar* lv : X->FC->LVB.vLocVar) {
	for (size_t i = 0; i < X->FC->LVB.variables.size(); i++) {
		LocVar* lv = X->FC->LVB.variables[i];
		if (lv->is_param || lv->is_return_param) {
			// parameter or return parameter variable -> assign value
			LVAssignFrml(file_d, lv, false, X->FrmlL[i], record);
		}
		else if (lv->is_return_value) {
			// it's return value -> initialize it and mark it
			lv->B = false;
			lv->R = 0.0;
			lv->S = "";
			return_lv = lv;
		}
		else {
			// it's local variable -> initialize it
			lv->B = false;
			lv->R = 0.0;
			lv->S = "";
		}
		//if (fl != nullptr) {
		//	fl = fl->pChain;
		//}
	}

	LocVarBlock oldLVBD = LVBD;
	LVBD = X->FC->LVB;

	std::unique_ptr<RunProcedure> runner = std::make_unique<RunProcedure>();
	runner->Run(X->FC->v_instr);

	LVBD = oldLVBD;

	return return_lv;
}

bool RunBool(FileD* file_d, FrmlElem* X, Record* record)
{
	bool result = false;
	if (X == nullptr) { return true; }

	switch (X->Op) {
	case _and: {
		auto iX0 = (FrmlElemFunction*)X;
		if (RunBool(file_d, iX0->P1, record)) {
			result = RunBool(file_d, iX0->P2, record);
		}
		else result = false;
		break;
	}
	case _or: {
		auto iX0 = (FrmlElemFunction*)X;
		if (RunBool(file_d, iX0->P1, record)) result = true;
		else result = RunBool(file_d, iX0->P2, record);
		break;
	}
	case _lneg: {
		auto iX0 = (FrmlElemFunction*)X;
		result = !RunBool(file_d, iX0->P1, record);
		break;
	}
	case _limpl: {
		auto iX0 = (FrmlElemFunction*)X;
		if (RunBool(file_d, iX0->P1, record)) result = RunBool(file_d, iX0->P2, record);
		else result = true;
		break;
	}
	case _lequ: {
		auto iX0 = (FrmlElemFunction*)X;
		if (RunBool(file_d, iX0->P1, record) == RunBool(file_d, iX0->P2, record)) result = true;
		else result = false;
		break;
	}
	case _instr: {
		auto iX0 = (FrmlElemIn*)X;
		std::string s = RunString(file_d, iX0->frml_elem, record);
		switch (iX0->param) {
		case 0:
			result = InStr(s, iX0); break;
		case 1:
			// '~' lexikalni porovnani
			result = LexInStr(s, iX0); break;
		default:
			throw std::exception("_instr iX0->param for value %i not implemented.", iX0->param);
		}
		break;
	}
	case _inreal: {
		result = InReal(file_d, (FrmlElemIn*)X, record);
		break;
	}
	case _compreal: {
		FrmlElemFunction* iX0 = (FrmlElemFunction*)X;
		double rrP1 = 0;
		// pokud jde o smycku FOR, je hodnota ulozena v LV1
		if (iX0->P1 == nullptr) rrP1 = iX0->LV1->R;
		// v ostatnich pripadech je v frml_elem
		else rrP1 = RunReal(file_d, iX0->P1, record);
		double rrP2 = RunReal(file_d, iX0->P2, record);
		short cmpR = CompReal(rrP1, rrP2, iX0->N22);
		result = (cmpR & iX0->N21) != 0;
		break;
	}
	case _compstr: {
		FrmlElemFunction* iX0 = (FrmlElemFunction*)X;
		std::string s1 = RunString(file_d, iX0->P1, record);
		std::string s2 = RunString(file_d, iX0->P2, record);
		WORD cmpRes = 0;
		if (iX0->N22 == 1) {
			cmpRes = CompLexStrings(TrailChar(s1, ' '), TrailChar(s2, ' '));
		}
		else {
			//cmpRes = CompLexStrings(s1, s2);
			cmpRes = CompStr(s1, s2);
		}
		result = (cmpRes & iX0->N21) != 0;
		break;
	}
	case _const: {
		result = ((FrmlElemBool*)X)->B; break;
	}
	case _mouseevent: {
	label2:
		auto iX1 = (FrmlElem1*)X;
		Event.What = evNothing;
		GetMouseEvent();
		if (Event.What == 0) {
			result = false;
		}
		else {
			if ((Event.What && iX1->W01) == 0) {
				goto label2;
			}
			result = true;
		}
		break;
	}
	case _ismouse: {
		auto iX1 = (FrmlElem1*)X;
		result = false;
		if (((Event.What && iX1->W01) != 0) && ((Event.Buttons && iX1->W02) == iX1->W02))
			result = true;
		break;
	}
	case _mousein: {
		auto iX0 = (FrmlElemFunction*)X;

		int RecNo;
		LongStr* S = nullptr;
		WORD* w1 = (WORD*)&RecNo;
		WORD* w2 = (WORD*)S;

		*w1 = RunInt(file_d, iX0->P1, record);
		*w2 = RunInt(file_d, iX0->P2, record);
		result = MouseInRectProc(*w1, *w2, RunInt(file_d, iX0->P3, record) - *w1 + 1, RunInt(file_d, iX0->P4, record) - *w2 + 1);
		break;
	}
	case _getlocvar: {
		result = ((FrmlElem20*)X)->LV->B;
		break;
	}
	case _modulo: {
		result = RunModulo(file_d, (FrmlElemFunction*)X, record);
		break;
	}
	case _field: {
		FrmlElemRecVarField* x7 = (FrmlElemRecVarField*)X;
		result = record->LoadB(x7->Field); //file_d->loadB(x7->Field, record);
		break;
	}
	case _access: {
		// nacita hodnotu ze souboru
		auto iX = (FrmlElemAccess*)X;
		int RecNo;

		if (iX->Link != nullptr) {
			Record* new_rec = LinkUpw(iX->Link, RecNo, false, record);
			bool b7 = (new_rec != nullptr);
			if ((iX->Frml == nullptr)) {
				result = b7;
			}
			else {
				result = RunBool(file_d, iX->Frml, new_rec);
			}
			delete new_rec; new_rec = nullptr;
		}
		else {
			Record* r = iX->File->LinkLastRec(RecNo);
			if (iX->Frml == nullptr) {
				result = (r != nullptr);
			}
			else {
				if (r == nullptr) {
					r = new Record(iX->File);
				}
				result = RunBool(iX->File, iX->Frml, r);
			}
			delete r; r = nullptr;
		}

		break;
	}
	case _recvarfld: {
		auto iX = (FrmlElemRecVarField*)X;
		result = RunBool(iX->File, iX->Frml, iX->record);
		break;
	}
	case _eval: {
		FrmlElem* frml = GetEvalFrml(file_d, (FrmlElemEval*)X, record);
		result = RunBool(file_d, frml, record);
		break;
	}
	case _newfile: {
		FrmlElemNewFile* iX = (FrmlElemNewFile*)X;
		result = RunBool(iX->NewFile, iX->Frml, iX->NewRP);
		break;
	}
	case _prompt: {
		FrmlElemPrompt* iX = (FrmlElemPrompt*)X;
		auto s = RunString(file_d, iX->P1, record);
		std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>();
		result = data_editor->PromptB(s, iX->P2, iX->FldD);
		break;
	}
	case _promptyn: {
		FrmlElemFunction* iX0 = (FrmlElemFunction*)X;
		SetMsgPar(RunString(file_d, iX0->P1, record));
		result = PromptYN(110);
		break;
	}
	case _accrecno: {
		FrmlElem14* iX = (FrmlElem14*)X;
		Record* rec = new Record(iX->RecFD);
		AccRecNoProc(iX, 640, rec);
		result = rec->LoadB(iX->RecFldD); //iX->RecFD->loadB(iX->RecFldD, rec);
		delete rec; rec = nullptr;
		break;
	}
	case _edupdated: result = EdUpdated; break;
	case _keypressed: result = KeyPressed(); break;/*Kbdpressed?*/
	case _escprompt: result = EscPrompt; break;
	case _isdeleted: {
		FrmlElem14* iX = (FrmlElem14*)X;
		Record* rec = new Record(iX->RecFD);
		AccRecNoProc(iX, 642, rec);
		result = rec->IsDeleted(); // iX->RecFD->DeletedFlag(rec);
		delete rec; rec = nullptr;
		break;
	}
	case _lvdeleted: {
		FrmlElem20* iX = (FrmlElem20*)X;
		//result = iX->LV->FD->DeletedFlag(iX->LV->record);
		result = iX->LV->record->IsDeleted();
		break;
	}
	case _trust: {
		FrmlElem1* iX1 = (FrmlElem1*)X;
		// TODO: result = (UserCode == 0) || OverlapByteStr(&iX1->N01, &AccRight);
		return user->get_user_code() == 0;
		break;
	}
	case _isnewrec: {
		std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>();
		result = data_editor->TestIsNewRec(); break;
	}
	case _testmode: {
		result = IsTestRun; break;
	}
	case _equmask: {
		result = RunEquMask(file_d, (FrmlElemFunction*)X, record);
		break;
	}
	case _userfunc: {
		result = RunUserFunc(file_d, (FrmlElemUserFunc*)X, record)->B;
		break;
	}
	case _setmybp: {
		FrmlElemFunction* iX0 = (FrmlElemFunction*)X;
		result = RunBool(file_d, iX0->P1, record);
		break;
	}
	}
	return result;
}

bool InReal(FileD* file_d, FrmlElemIn* frml, Record* record)
{
	double R = RunReal(file_d, frml->frml_elem, record);
	for (auto r : frml->reals) {
		if (r == R) return true;
	}
	for (auto& range : frml->reals_range) {
		auto range1 = range.first;
		auto range2 = range.second;
		if (range1 <= R && R <= range2) return true;
	}
	return false;
}


bool LexInStr(std::string& S, FrmlElemIn* X)
{
	S = TrailChar(S, ' ');
	for (auto& pat : X->strings) {
		const int res = CompLexStrings(pat, S);
		if (res == _equ) return true;
	}

	for (const auto& ran : X->strings_range) {
		auto s1 = ran.first;
		auto s2 = ran.second;
		const int res1 = CompLexStrings(s1, S);
		const int res2 = CompLexStrings(s2, S);
		if (res1 == _equ || res2 == _equ) return true;
		if (res1 == _lt && res2 == _gt) return true;
	}
	return false;
}

bool InStr(std::string& S, FrmlElemIn* X)
{
	for (auto& pat : X->strings) {
		if (S == pat) return true;
	}

	for (const auto& ran : X->strings_range) {
		bool success = false;
		std::string s1 = ran.first;
		std::string s2 = ran.second;
		const int res1 = CompStr(S, s1);
		const int res2 = CompStr(S, s2);
		if (res1 == _equ || res2 == _equ) return true;
		if (res1 == _gt && res2 == _lt) return true;
	}

	return false;
}

bool RunModulo(FileD* file_d, FrmlElemFunction* X, Record* record)
{
	std::string input = RunString(file_d, X->P1, record);
	if (input.empty() || input.length() != X->vValues.size()) {
		return false;
	}

	int sum = 0;
	const int n = X->vValues[0];
	const uint8_t lastChar = input[input.length() - 1];

	for (size_t i = 0; i < input.length() - 1; i++) {
		char c = input[i];
		if (isdigit(c)) {
			sum += (c - 0x30) * X->vValues[i + 1];
		}
		else {
			return false;
		}
	}

	return (n - sum % n) % 10 == lastChar - 0x30;
}

bool RunEquMask(FileD* file_d, FrmlElemFunction* X, Record* record)
{
	const std::string value = RunString(file_d, X->P1, record);
	std::string mask = RunString(file_d, X->P2, record);
	const bool result = EqualsMask(value, mask);
	return result;
}

double RunReal(FileD* file_d, FrmlElem* X, Record* record)
{
	if (X == nullptr) return 0;

	double R = 0.0;
	LockMode md;
	int RecNo = 0;

	double result = 0;
label1:
	FrmlElemFunction* iX0 = (FrmlElemFunction*)X;
	switch (X->Op) {
	case _field: {
		FrmlElemRecVarField* iX = (FrmlElemRecVarField*)X;
		result = record->LoadR(iX->Field); //file_d->loadR(iX->Field, record);
		break;
	}
	case _getlocvar: {
		auto iX = (FrmlElemLocVar*)X;
		result = iX->locvar->R;
		break;
	}
	case _const: {
		result = ((FrmlElemNumber*)X)->R;
		break;
	}
	case _plus: {
		result = RunReal(file_d, iX0->P1, record) + RunReal(file_d, iX0->P2, record);
		break;
	}
	case _minus: {
		double d1 = RunReal(file_d, iX0->P1, record);
		double d2 = RunReal(file_d, iX0->P2, record);
		result = d1 - d2;
		break;
	}
	case _times: {
		result = RunReal(file_d, iX0->P1, record) * RunReal(file_d, iX0->P2, record);
		break;
	}
	case _access: {
		auto iX = (FrmlElemAccess*)X;

		if (iX->Link != nullptr) {
			Record* newRecord = LinkUpw(iX->Link, RecNo, false, record);

			result = RunReal(iX->Link->ToFD, iX->Frml, newRecord);
			delete newRecord; newRecord = nullptr;
		}
		else {
			Record* r = iX->File->LinkLastRec(RecNo);
			if (r == nullptr) {
				r = new Record(iX->File);
			}
			result = RunReal(iX->File, iX->Frml, r);
			delete r; r = nullptr;
		}

		break;
	}
	case _recvarfld: {
		FrmlElemRecVarField* iX = (FrmlElemRecVarField*)X;
		result = RunReal(iX->File, iX->Frml, iX->record);
		break;
	}
	case _eval: {
		// MarkStore(p);
		result = RunReal(file_d, GetEvalFrml(file_d, (FrmlElemEval*)X, record), record);
		// ReleaseStore(p);
		break;
	}
	case _divide: {
		double a = RunReal(file_d, iX0->P1, record);
		double b = RunReal(file_d, iX0->P2, record);
		result = (b == 0.0) ? 0 : a / b;
		break;
	}
	case _cond: {
	label2:
		auto iX = (FrmlElemFunction*)X;
		if (iX->P1 != nullptr)
			if (!RunBool(file_d, iX->P1, record))
			{
				if (iX->P3 == nullptr) { result = 0; return result; }
				X = iX->P3;
				goto label2;
			}
		X = iX->P2;
		goto label1;
	}
	case _newfile: {
		FrmlElemNewFile* iX = (FrmlElemNewFile*)X;
		result = RunReal(iX->NewFile, iX->Frml, iX->NewRP);
		break;
	}
	case _getWORDvar: {
		uint8_t i = ((FrmlElem1*)X)->N01;
		switch (i) {
		case 0: return RprtLine;
		case 1: return RprtPage;
		case 2: return PgeLimit;
		case 3: return EdBreak;
		case 4: return EdIRec;
		case 5: return MenuX;
		case 6: return MenuY;
		case 7: return user->get_user_code();
		default: throw std::exception("RunFrml.cpp, RunReal(), case _getWORDvar: index out");
		}
		break;
	}
	case _div: result = (int)(RunReal(file_d, iX0->P1, record) / RunReal(file_d, iX0->P2, record)); break;
	case _mod: result = RMod(file_d, (FrmlElemFunction*)X, record); break;
	case _unminus: result = -RunReal(file_d, iX0->P1, record); break;
	case _today: result = Today(); break;
	case _pi: result = atan(1.0) * 4; break;
	case _random: result = Random(); break;
	case _round: result = RoundReal(RunReal(file_d, iX0->P1, record), RunInt(file_d, iX0->P2, record)); break;
	case _abs: result = abs(RunReal(file_d, iX0->P1, record)); break;
	case _int: result = (int)(RunReal(file_d, iX0->P1, record)); break;
	case _frac: {
		double dx;
		result = modf(RunReal(file_d, iX0->P1, record), &dx);
		break;
	}
	case _sqr: result = pow(RunReal(file_d, iX0->P1, record), 2); break;
	case _sqrt: result = sqrt(RunReal(file_d, iX0->P1, record)); break;
	case _sin: result = sin(RunReal(file_d, iX0->P1, record)); break;
	case _cos: result = cos(RunReal(file_d, iX0->P1, record)); break;
	case _arctan: result = atan(RunReal(file_d, iX0->P1, record)); break;
	case _ln: result = log(RunReal(file_d, iX0->P1, record)); break;
	case _exp: {
		R = RunReal(file_d, iX0->P1, record);
		if ((R <= -50) || (R > 88)) result = 0;
		else result = exp(R);
		break;
	}
	case _nrecs:
	case _nrecsabs: {
		FileD* fX = ((FrmlElem9*)X)->FD;
		md = fX->NewLockMode(RdMode);
		if (X->Op == _nrecs) {
			RecNo = fX->FF->XNRecs(fX->Keys);
		}
		else {
			RecNo = fX->GetNRecs();
		}
		fX->OldLockMode(md);
		result = RecNo;
		break;
	}
	case _generation: {
		FrmlElem9* iX = (FrmlElem9*)X;
		result = catalog->Generation(iX->FD, CPath, CVol);
		FSplit(CPath, CDir, CName, CExt);
		break;
	}
	case _lastupdate: {
		FrmlElem9* iX = (FrmlElem9*)X;
		md = iX->FD->NewLockMode(RdMode);
		result = LastUpdate(iX->FD->FullPath);
		iX->FD->OldLockMode(md);
		break;
	}
	case _catfield: {
		FrmlElemCatalogField* iX = (FrmlElemCatalogField*)X;
		CVol = catalog->GetVolume(iX->CatIRec);
		CPath = FExpand(catalog->GetPathName(iX->CatIRec));
		FSplit(CPath, CDir, CName, CExt);
		TestMountVol(CPath[0]);
		result = LastUpdate(CPath);
		break;
	}
	case _currtime: {
		result = CurrTime();
		break;
	}
	case _typeday: {
		auto rr = RunReal(file_d, iX0->P1, record);
		result = TypeDay(rr);
		break;
	}
	case _addwdays: {
		result = AddWDays(RunReal(file_d, iX0->P1, record), RunInt(file_d, iX0->P2, record), iX0->N21);
		break;
	}
	case _difwdays: {
		result = DifWDays(RunReal(file_d, iX0->P1, record), RunReal(file_d, iX0->P2, record), iX0->N21);
		break;
	}
	case _addmonth: {
		result = AddMonth(RunReal(file_d, iX0->P1, record), RunReal(file_d, iX0->P2, record));
		break;
	}
	case _difmonth: {
		result = DifMonth(RunReal(file_d, iX0->P1, record), RunReal(file_d, iX0->P2, record));
		break;
	}
	case _recno: {
		result = RecNoFun(file_d, (FrmlElemRecNo*)X, record);
		break;
	}
	case _recnoabs:
	case _recnolog: {
		result = AbsLogRecNoFun(file_d, (FrmlElemRecNo*)X, record);
		break;
	}
	case _accrecno: {
		FrmlElem14* iX = (FrmlElem14*)X;
		Record* rec = new Record(iX->RecFD);
		AccRecNoProc(iX, 640, rec);
		result = rec->LoadR(iX->RecFldD);
		delete rec; rec = nullptr;
		break;
	}
	case _link: {
		result = LinkProc((FrmlElemLink*)X, record);
		break;
	}
	case _memavail: {
		result = MemoryAvailable();
		break;
	}
	case _maxcol: {
		result = TxtCols;
		break;
	}
	case _maxrow: {
		result = TxtRows;
		break;
	}
#ifdef FandGraph
	case _getmaxx: {
		if (IsGraphMode) {
			ViewPortType* vp = (ViewPortType*)&R;
			GetViewSettings(*vp);
			result = vp->x2 - vp->x1;
		}
		else result = 0;
		break;
	}
	case _getmaxy: {
		if (IsGraphMode) {
			ViewPortType* vp = (ViewPortType*)&R;
			GetViewSettings(*vp);
			result = vp->y2 - vp->y1;
		}
		else result = 0;
		break;
	}
#endif
	case _exitcode: result = LastExitCode; break;
	case _edrecno: result = EdRecNo; break;
	case _txtpos: result = LastTxtPos; break;
	case _txtxy: result = TxtXY; break;
	case _cprinter: result = prCurr; break;
	case _mousex: {
		if (IsGraphMode) {
			result = Event.WhereG.X;
		}
		else {
			result = Event.Where.X + 1.0; break;
		}
		break;
	}
	case _mousey: {
		if (IsGraphMode) {
			result = Event.WhereG.Y;
		}
		else {
			result = Event.Where.Y + 1.0; break;
		}
		break;
	}
	case _filesize: {
		auto iX = (FrmlElem16*)X;
		SetTxtPathVol(iX->TxtPath, iX->TxtCatIRec);
		result = GetFileSize();
		break;
	}
	case _inttsr: {
		result = IntTSR(file_d, X, record);
		break;
	}
	case _userfunc: {
		result = RunUserFunc(file_d, (FrmlElemUserFunc*)X, record)->R;
		break;
	}
	case _indexnrecs: {
		auto iX = (FrmlElem22*)X;
		result = iX->WKey->NRecs();
		break;
	}
	case _owned: {
		auto iX = (FrmlElemOwned*)X;
		result = Owned(file_d, iX->ownBool, iX->ownSum, iX->ownLD, record);
		break;
	}
	case _color: {
		// Colors ma 54 prvku (uint8_t)
		size_t colorFromFrml = RunInt(file_d, iX0->P1, record);
		uint8_t* AColors = (uint8_t*)&screen.colors;
		result = AColors[min(colorFromFrml, 53)];
		break;
	}
	case _portin: {
		result = PortIn(RunBool(file_d, iX0->P1, record), WORD(RunInt(file_d, iX0->P2, record)));
		break;
	}
	case _setmybp: {
		result = RunReal(file_d, iX0->P1, record);
		break;
	}
	case _count: {
		result = ((FrmlElemInp*)X)->inp->Count;
		break;
	}
	case _mergegroup: {
		result = ((FrmlElemMerge*)X)->merge->Group;
		break;
	}
	default: {
		result = RunRealStr(file_d, X, record);
		break;
	}
	}
	return result;
}

int RunInt(FileD* file_d, FrmlElem* X, Record* record)
{
	double rr = RunReal(file_d, X, record);
	return trunc(rr);
}

//void TestTFrml(FileD* file_d, FieldDescr* F, FrmlElem* Z, FandTFile** TF02, FileD** TFD02, int& TF02Pos, Record* record)
//{
//	switch (Z->Op) {
//	case _newfile: {
//		FrmlElemNewFile* iZ = (FrmlElemNewFile*)Z;
//		TestTFrml(iZ->NewFile, F, iZ->Frml, TF02, TFD02, TF02Pos, iZ->NewRP);
//		break;
//	}
//	case _field: {
//		FrmlElemRecVarField* iZ = (FrmlElemRecVarField*)Z;
//		FieldDescr* f1 = iZ->Field;
//		if ((f1->field_type != FieldType::TEXT) || ((f1->Flg & f_Stored) == 0)) return;
//		if (F == nullptr) {
//			if ((f1->Flg & f_Encryp) != 0) return;
//		}
//		else if ((F->Flg & f_Encryp) != (f1->Flg & f_Encryp)) return;
//		*TFD02 = file_d;
//		*TF02 = file_d->FF->TF;
//		//if (file_d->HasTWorkFlag(record)) {
//		//	*TF02 = &TWork; // TODO: is there anything in TWork?: 
//		//}
//		// TODO: replace this: 
//		// TF02Pos = file_d->loadT(f1, record);
//		break;
//	}
//	case _getlocvar: {
//		// local var is now always in memory, not in the work file
//		// TODO: what if this is record of?
//		break;
//		//if ((F != nullptr) && ((F->Flg & f_Encryp) != 0)) return;
//		//*TFD02 = CFile;
//		//*TF02 = &TWork;
//		//TF02Pos = (int)((FrmlElemLocVar*)Z)->locvar->rdb;
//		//break;
//	}
//	case _access: {
//		int n;
//		FrmlElemAccess* iZ = (FrmlElemAccess*)Z;
//		LockMode md = iZ->File->NewLockMode(RdMode);
//		if (iZ->Link != nullptr) {
//			Record* newRecord = LinkUpw(iZ->Link, n, true, record);
//			TestTFrml(iZ->Link->ToFD, F, iZ->Frml, TF02, TFD02, TF02Pos, newRecord);
//			delete[] newRecord; newRecord = nullptr;
//		}
//		else {
//			Record* r = iZ->File->LinkLastRec(n);
//			if (r == nullptr) {
//				r = new Record(iZ->File);
//			}
//			TestTFrml(iZ->File, F, iZ->Frml, TF02, TFD02, TF02Pos, r);
//			delete r; r = nullptr;
//		}
//		iZ->File->OldLockMode(md);
//		break;
//	}
//	case _recvarfld: {
//		FrmlElemRecVarField* iZ = (FrmlElemRecVarField*)Z;
//		TestTFrml(iZ->File, F, iZ->Frml, TF02, TFD02, TF02Pos, iZ->record);
//		break;
//	}
//	}
//}

//bool CanCopyT(FileD* file_d, FieldDescr* F, FrmlElem* Z, FandTFile** TF02, FileD** TFD02, int& TF02Pos, Record* record)
//{
//	bool result = false;
//	*TF02 = nullptr;
//
//	TestTFrml(file_d, F, Z, TF02, TFD02, TF02Pos, record);
//
//	result = (*TF02) != nullptr;
//	return result;
//}

//bool TryCopyT(FileD* dst_file, FieldDescr* F, FandTFile* dst_T_file, int& pos, FrmlElem* Z, Record* record)
//{
//	FileD* src_file;
//	FandTFile* src_T_file;
//	int src_T_pos;
//	bool result;
//
//	if (Z->Op == _gettxt) {
//		std::string text = GetTxt(dst_file, (FrmlElem16*)Z, record);
//		record->SaveS(F, text);
//		// TODO: replace this:
//		//pos = dst_file->FF->loadT(F, record->GetRecord());
//		result = true;
//	}
//	else if (CanCopyT(dst_file, F, Z, &src_T_file, &src_file, src_T_pos, record)) {
//		LockMode md1 = NullMode, md2 = NullMode;
//		if (!src_T_file->IsWork) md2 = src_file->NewLockMode(RdMode);
//		if (!dst_T_file->IsWork) md1 = dst_file->NewLockMode(WrMode);
//
//		std::string s = src_T_file->Read(src_T_pos);
//		pos = dst_T_file->Store(s);
//
//		if (!src_T_file->IsWork) src_file->OldLockMode(md2);
//		if (!dst_T_file->IsWork) dst_file->OldLockMode(md1);
//		result = true;
//	}
//	else {
//		result = false;
//	}
//
//	return result;
//}

void AssgnFrml(Record* record, FieldDescr* field_d, FrmlElem* X, bool add)
{
	FileD* file_d = record->GetFileD();

	switch (field_d->frml_type) {
	case 'S': {
		//if (field_d->field_type == FieldType::TEXT) {
		//	FandTFile* tf = nullptr;

		//	/*bool work = file_d->HasTWorkFlag(record);
		//	if (work) {
		//		tf = &TWork;
		//	}
		//	else */if (file_d->FileType == DataFileType::FandFile) {
		//		tf = file_d->FF->TF;
		//	}

		//	int pos = 0;

		//	//if (file_d->FileType == DataFileType::FandFile /*&& TryCopyT(file_d, field_d, tf, pos, X, record)*/) {
		//	//	//if (deleteT && !work) {
		//	//	//	file_d->FF->DelTFld(field_d, record);
		//	//	//}
		//	//	// TODO: replace this:
		//	//	// file_d->saveT(field_d, pos, record);
		//	//}
		//	//else {
		//		std::string s = RunString(file_d, X, record);
		//		/*if (deleteT && !work) {
		//			switch (file_d->FileType) {
		//			case DataFileType::FandFile: {
		//				file_d->FF->DelTFld(field_d, record);
		//				break;
		//			}
		//			case DataFileType::DBF: {
		//				file_d->DbfF->DelTFld(field_d, record);
		//				break;
		//			}
		//			default: break;
		//			}
		//		}*/
		//		record->SaveS(field_d, s);
		//	//}
		//}
		//else {
			std::string s = RunString(file_d, X, record);
			record->SaveS(field_d, s);
		//}
		break;
	}
	case 'R': {
		if (add) {
			double r1 = record->LoadR(field_d);
			double r2 = RunReal(file_d, X, record);
			record->SaveR(field_d, r1 + r2);
		}
		else {
			double r = RunReal(file_d, X, record);
			record->SaveR(field_d, r);
		}
		break;
	}
	case 'B': {
		bool b = RunBool(file_d, X, record);
		record->SaveB(field_d, b);
		break;
	}
	default:;
	}
}

void LVAssignFrml(FileD* file_d, LocVar* LV, bool Add, FrmlElem* X, Record* record)
{
	switch (LV->f_typ) {
	case 'S': {
		LV->S = RunString(file_d, X, record);
		break;
	}
	case 'R': {
		if (Add) {
			LV->R += RunReal(file_d, X, record);
		}
		else {
			LV->R = RunReal(file_d, X, record);
		}
		break;
	}
	case 'B': {
		LV->B = RunBool(file_d, X, record);
		break;
	}
	}
}

void RunWFrml(FileD* file_d, WRectFrml& X, uint8_t WFlags, WRect& W, Record* record)
{
	W.C1 = RunInt(file_d, X.C1, record);
	W.R1 = RunInt(file_d, X.R1, record);
	W.C2 = RunInt(file_d, X.C2, record);
	W.R2 = RunInt(file_d, X.R2, record);
	CenterWw(W.C1, W.R1, W.C2, W.R2, WFlags);
}

WORD RunWordImpl(FileD* file_d, FrmlElem* Z, WORD Impl, Record* record)
{
	WORD n = RunInt(file_d, Z, record);
	if (n == 0) n = Impl;
	return n;
}

bool FieldInList(FieldDescr* F, std::vector<FieldDescr*>& FL)
{
	bool result = false;
	if (std::find(FL.begin(), FL.end(), F) != FL.end())
		result = true;
	return result;
}

XKey* GetFromKey(LinkD* LD)
{
	if (LD->FromFD->Keys.empty()) return nullptr;

	// find key in Link->FromFD->Keys with the same index root as Link->IndexRoot
	vector<XKey*>::iterator it = ranges::find_if(LD->FromFD->Keys, [LD](XKey* K) {
		return K->IndexRoot == LD->IndexRoot;
		});

	if (it != LD->FromFD->Keys.end()) {
		return *it;
	}
	else {
		return nullptr;
	}
}

FrmlElem* RunEvalFrml(FileD* file_d, FrmlElem* Z, Record* record)
{
	if ((Z != nullptr) && (Z->Op == _eval)) {
		Z = GetEvalFrml(file_d, (FrmlElemEval*)Z, record);
	}
	return Z;
}

std::string Replace(std::string text, std::string oldText, std::string& newText, std::string options)
{

	bool tilda = options.find('~') != std::string::npos;
	bool words = (options.find('w') != std::string::npos) || (options.find('W') != std::string::npos);
	bool upper = (options.find('u') != std::string::npos) || (options.find('U') != std::string::npos);

	std::string copyInputText = text;

	if (upper) {
		for (size_t i = 0; i < copyInputText.length(); i++) {
			copyInputText[i] = UpcCharTab[(uint8_t)copyInputText[i]];
		}
		for (size_t i = 0; i < oldText.length(); i++) {
			oldText[i] = UpcCharTab[(uint8_t)oldText[i]];
		}
	}

	if (tilda) {
		for (size_t i = 0; i < copyInputText.length(); i++) {
			copyInputText[i] = CharOrdTab[(uint8_t)copyInputText[i]];
		}
		for (size_t i = 0; i < oldText.length(); i++) {
			oldText[i] = CharOrdTab[(uint8_t)oldText[i]];
		}
	}

	if (words) {
		throw std::exception("Replace() not implemented.");
	}

	// we are looking for equ strings in transformed string (copyInputText),
	// but we change text in original string (text) too!!!
	size_t old_len = oldText.length();
	size_t pos = copyInputText.find(oldText);

	while (pos != std::string::npos) {
		text = text.replace(pos, old_len, newText);
		copyInputText = copyInputText.replace(pos, old_len, newText);
		pos = copyInputText.find(oldText, pos + newText.length());
	}

	return text;
}

std::string RunString(FileD* file_d, FrmlElem* X, Record* record)
{
	int RecNo = 0;
	std::string result;

	if (X == nullptr) return "";
label1:
	switch (X->Op) {
	case _field: {
		FrmlElemRecVarField* iX7 = (FrmlElemRecVarField*)X;
		//result = file_d->loadS(iX7->Field, record);
		result = record->LoadS(iX7->Field);
		break;
	}
	case _getlocvar: {
		return ((FrmlElemLocVar*)X)->locvar->S;
	}
	case _access: {
		FrmlElemAccess* iX7 = (FrmlElemAccess*)X;
		LockMode lm = iX7->File->NewLockMode(RdMode);

		if (iX7->Link != nullptr) {
			Record* newRecord = LinkUpw(iX7->Link, RecNo, true, record);
			result = RunString(iX7->Link->ToFD, iX7->Frml, newRecord);
			delete newRecord; newRecord = nullptr;
		}
		else {
			Record* r = iX7->File->LinkLastRec(RecNo);
			if (r == nullptr) {
				r = new Record(iX7->File);
			}
			result = RunString(iX7->File, iX7->Frml, r);
			delete r; r = nullptr;
		}

		iX7->File->OldLockMode(lm);  /*possibly reading .T*/
		break;
	}
	case _recvarfld: {
		auto iX7 = (FrmlElemRecVarField*)X;
		result = RunString(iX7->File, iX7->Frml, iX7->record);
		break;
	}
	case _eval: {
		return RunString(file_d, GetEvalFrml(file_d, (FrmlElemEval*)X, record), record);
		break;
	}
	case _newfile: {
		auto iX8 = (FrmlElemNewFile*)X;
		result = RunString(iX8->NewFile, iX8->Frml, iX8->NewRP);
		break;
	}
	case _cond: {
	label2:
		if (((FrmlElemFunction*)X)->P1 != nullptr)
			if (!RunBool(file_d, ((FrmlElemFunction*)X)->P1, record)) {
				if (((FrmlElemFunction*)X)->P3 == nullptr) {
					return "";
				}
				X = ((FrmlElemFunction*)X)->P3;
				goto label2;
			}
		X = ((FrmlElemFunction*)X)->P2;
		goto label1;
		break;
	}
	case _copy: {
		FrmlElemFunction* const iX0 = static_cast<FrmlElemFunction*>(X);
		std::string str = RunString(file_d, iX0->P1, record);
		int L1 = RunInt(file_d, iX0->P2, record);
		if (L1 > 0) { L1--; } // RUNFRML.PAS 427 (dec bx)
		const int L2 = RunInt(file_d, iX0->P3, record);

		if ((L1 < 0) || (L2 < 0)) {
			str = "";
		}
		else {
			if (L1 >= (int)str.length()) str = ""; // index L1 je vetsi nez delka retezce
			else str = str.substr(L1, L2); // L2 udava delku
		}
		return str;
		break;
	}
	case _concat: {
		FrmlElemFunction* iX0 = (FrmlElemFunction*)X;
		std::string S1 = RunString(file_d, iX0->P1, record);
		std::string S2 = RunString(file_d, iX0->P2, record);
		result = S1 + S2;
		break;
	}
	case _const: {
		result = ((FrmlElemString*)X)->S;
		break;
	}
	case _leadchar: {
		auto iX0 = (FrmlElemFunction*)X;
		char c = iX0->N11;
		char cnew = iX0->N12;
		auto sp1 = RunString(file_d, iX0->P1, record);
		result = LeadChar(sp1, c, cnew);
		break;
	}
	case _trailchar: {
		auto iX0 = (FrmlElemFunction*)X;
		char c = iX0->N11;
		char cnew = iX0->N12;
		auto sp1 = RunString(file_d, iX0->P1, record);
		result = TrailChar(sp1, c, cnew);
		break;
	}
	case _upcase: {
		auto iX0 = (FrmlElemFunction*)X;
		result = RunString(file_d, iX0->P1, record);
		for (WORD i = 0; i < result.length(); i++) {
			result[i] = UpcCharTab[(uint8_t)result[i]];
		}
		break;
	}
	case _lowcase: {
		auto iX0 = (FrmlElemFunction*)X;
		result = RunString(file_d, iX0->P1, record);
		LowCase(result);
		break;
	}
	case _copyline: {
		auto iX0 = (FrmlElemFunction*)X;
		size_t i = 1;
		if (iX0->P3 != nullptr) i = RunInt(file_d, iX0->P3, record);
		std::string s = RunString(file_d, iX0->P1, record);
		size_t start = RunInt(file_d, iX0->P2, record);
		result = GetNthLine(s, start, i);
		break;
	}
	case _repeatstr: {
		FrmlElemFunction* iX0 = (FrmlElemFunction*)X;
		int32_t i = RunInt(file_d, iX0->P2, record);
		std::string input = RunString(file_d, iX0->P1, record);
		result = RepeatString(input, i);
		break;
	}
	case _accrecno: {
		FrmlElem14* iX = (FrmlElem14*)X;
		Record* rec = new Record(iX->RecFD);
		AccRecNoProc(iX, 640, rec);
		result = rec->LoadS(iX->RecFldD); //iX->RecFD->loadS(iX->RecFldD, rec);
		delete rec; rec = nullptr;
		break;
	}
	case _gettxt: {
		result = GetTxt(file_d, static_cast<FrmlElem16*>(X), record);
		break;
	}
	case _nodiakr: {
		FrmlElemFunction* iX0 = (FrmlElemFunction*)X;
		result = RunString(file_d, iX0->P1, record);
		ConvToNoDiakr(result, fonts.VFont);
		break;
	}
	case _userfunc: {
		LocVar* lv = RunUserFunc(file_d, (FrmlElemUserFunc*)X, record);
		result = lv->S;
		break;
	}
	case _setmybp: {
		auto iX0 = (FrmlElemFunction*)X;
		result = RunString(file_d, iX0->P1, record);
		break;
	}
	case _selectstr: {
		result = RunSelectStr(file_d, (FrmlElemFunction*)X, record);
		break;
	}
	case _clipbd: {
		result = ""; // TWork.Read(ClpBdPos);
		break;
	}
	case _char: {
		FrmlElemFunction* iZ0 = (FrmlElemFunction*)X;
		double r = RunReal(file_d, iZ0->P1, record);
		char s = (char)trunc(r);
		result = s;
		break;
	}
	case _strdate1: {
		FrmlElemDateMask* iZ = (FrmlElemDateMask*)X;
		result = StrDate(RunReal(file_d, iZ->P1, record), iZ->Mask);
		break;
	}
	case _str: {
		FrmlElemFunction* iZ0 = (FrmlElemFunction*)X;
		if (iZ0->P3 != nullptr) {
			double r = RunReal(file_d, iZ0->P1, record);
			WORD l = RunInt(file_d, iZ0->P2, record);
			uint8_t m = RunInt(file_d, iZ0->P3, record);

			if (m == 255) {
				result = to_string(static_cast<int>(r));
			}
			else {
				std::string s;
				str(r, l, m, s);
				result = s;
			}
		}
		else {
			pstring s = RunString(file_d, iZ0->P2, record);
			StrMask(RunReal(file_d, iZ0->P1, record), s);
			result = s;
		}
		break;
	}
	case _replace: {
		FrmlElemPosReplace* iZ = (FrmlElemPosReplace*)X;
		std::string text = RunString(file_d, iZ->P2, record);
		std::string oldText = RunString(file_d, iZ->P1, record); //j = 1;
		std::string newText = RunString(file_d, iZ->P3, record);
		result = Replace(text, oldText, newText, iZ->Options);
		break;
	}
	case _prompt: {
		FrmlElemPrompt* iZ = (FrmlElemPrompt*)X;
		std::string s0 = RunString(file_d, iZ->P1, record);
		std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>();
		result = data_editor->PromptS(s0, iZ->P2, iZ->FldD);
		break;
	}
	case _getpath: {
		FrmlElemFunction* iZ0 = (FrmlElemFunction*)X;
		wwmix ww;
		std::string s = ".*";
		if (iZ0->P1 != nullptr) {
			s = RunString(file_d, iZ0->P1, record);
		}
		result = ww.SelectDiskFile(s, 35, false);
		break;
	}
	case _catfield: {
		FrmlElemCatalogField* iZ = (FrmlElemCatalogField*)X;
		std::string stdS = catalog->GetField(iZ->CatIRec, iZ->CatFld);
		bool empty = stdS.empty(); // bude se jednat jen o cestu, bez nazvu souboru
		if (iZ->CatFld == catalog->CatalogPathNameField()) {
			stdS = FExpand(stdS);
			if (empty) AddBackSlash(stdS); // za cestu pridame '\'
		}
		result = stdS;
		break;
	}
	case _password: {
		wwmix ww;
		result = ww.PassWord(false);
		break;
	}
	case _readkey: {
		WORD keyb = ReadKbd();
		if (keyb <= 0x00FF) {
			// it's char
			result = (char)Lo(keyb);
		}
		else {
			result = '\0';
			result += (char)Lo(keyb);
		}
		break;
	}
	case _username: {
		result = user->get_user_name();
		break;
	}
	case _accright: {
		result = user->get_acc_rights();
		break;
	}
	case _version: {
		result = Version;
		break;
	}
	case _edfield: {
		result = EdField;
		break;
	}
	case _edfile: {
		if (EditDRoot != nullptr) {
			result = EditDRoot->FD->Name;
		}
		break;
	}
	case _edkey: {
		result = EdKey;
		break;
	}
	case _edreckey: {
		result = EdRecKey;
		break;
	}
	case _getenv: {
		FrmlElemFunction* iZ0 = (FrmlElemFunction*)X;
		std::string s = RunString(file_d, iZ0->P1, record);
		if (s == "") s = paramstr[0];
		else s = GetEnv(s.c_str());
		result = s;
		break;
	}
	case _keyof: {
		auto iZ = (FrmlElem20*)X;
		XString x;
		x.PackKF(iZ->LV->FD, iZ->PackKey->KFlds, iZ->LV->record);
		result = x.S;
		break;
	}
	case _keybuf: {
		//while (KeyPressed()) {
		//	AddToKbdBuf(ReadKey());
		//}
		result = keyboard.GetKeyBufAsString(); // KbdBuffer;
		break;
	}
	case _recno: {
		XString x;
		GetRecNoXString(file_d, (FrmlElemRecNo*)X, x, record);
		result = x.S;
		break;
	}
	case _edbool: {
		// TODO: who is calling this method? Is EditDRoot set?
		if ((EditDRoot != nullptr) && EditDRoot->params_->Select
			&& (!EditDRoot->BoolTxt.empty())) {
			result = EditDRoot->BoolTxt;
		}
		break;
	}
	default: {
		//auto s = RunS(file_d, X, record);
		//result = std::string(s->A, s->LL);
		//delete s;
		throw std::exception("RunString() not implemented.");
	}
	}
	return result;
}

//std::string RunShortStr(FileD* file_d, FrmlElem* X, void* record)
//{
//	std::string s = RunString(file_d, X, record);
//	if (s.length() > 255) s = s.substr(0, 255);
//	return s;
//}

void AddToLongStr(LongStr* S, void* P, WORD L)
{
	L = MinW(L, MaxLStrLen - S->LL);
	auto oldA = S->A;
	S->A = new char[S->LL + L];
	memcpy(S->A, oldA, S->LL);
	memcpy(&S->A[S->LL], P, L);
	S->LL += L;
	delete[] oldA;
}

void StrMask(double R, pstring& Mask)
{
	pstring Num;
	WORD i = 0;
	WORD sw = 2;
	WORD l = Mask.length();
	WORD n = 0;
	WORD pos = l + 1;
	WORD pos1 = pos;

	for (i = l; i >= 1; i--) {
		switch (Mask[i]) {
		case ',': {
			if (sw == 2) sw = 1;
			break;
		}
		case '0':
		case '*': {
			pos = i;
			goto label1;
			break;
		}
		case '_': {
			if (sw == 1) pos1 = i;
		label1:
			if (sw == 1) sw = 0;
			else if (sw == 2) n++;
			break;
		}
		}
	}

	if (sw == 2) n = 0;

	R = R * Power10[n];
	R = RoundReal(R, 0);
	bool minus = false;

	if (R == 0.0) Num[0] = 0;
	else {
		if (R < 0) { minus = true; R = -R; }
		str(R, 1, 0, Num); // str(rdb:1:0,Num)
		pos = MinW(pos, pos1);
	}

	i = Num.length();

	if ((Num == "INF") || (Num == "NAN")) {
		Mask = Num;
		while ((Mask.length() < l))
		{
			pstring tmp = " ";
			Mask = tmp + Mask;
		}
		return;
	}

	while (l > 0) {
		switch (Mask[l]) {
		case '0':
		case '*': if (i > 0) goto label3; break;
		case '.':
		case ',': if ((i == 0) && (l < pos)) goto label2; break;
		case '-': if (minus) minus = false; else Mask[l] = ' '; break;
		case '_': {
			if (i == 0) {
				if (l >= pos) Mask[l] = '0';
				else {
				label2:
					if (minus) { minus = false; Mask[l] = '-'; }
					else { Mask[l] = ' '; }
				}
			}
			else {
			label3:
				Mask[l] = Num[i];
				i--;
			}
			break;
		}
		}
		l--;
	}

	if (i > 0) Mask = copy(Num, 1, i) + Mask;

	pstring tmp = "-";

	if (minus) Mask = tmp + Mask;
}

std::string RunSelectStr(FileD* file_d, FrmlElemFunction* Z, Record* record)
{
	wwmix ww;

	std::string std_s = RunString(file_d, Z->P3, record);
	size_t n = CountLines(std_s, Z->Delim);
	for (size_t i = 1; i <= n; i++) {
		std::string x = GetNthLine(std_s, i, 1, Z->Delim);
		if (!x.empty()) {
			ww.PutSelect(x);
		}
	}

	std::string mode = RunString(file_d, Z->P6, record);
	for (char i : mode) {
		switch (toupper(i)) {
		case 'A': ss.Abcd = true; break;
		case 'S': ss.Subset = true; break;
		case 'I': ss.ImplAll = true; break;
		default: break;
		}
	}

	SetMsgPar(RunString(file_d, Z->P4, record));
	ww.SelectStr(
		RunInt(file_d, Z->P1, record), RunInt(file_d, Z->P2, record),
		110, RunString(file_d, Z->P5, record)
	);

	std::string s2;
	LastExitCode = 0;

	size_t count = 0;
	if (Event.Pressed.KeyCombination() == __ESC) {
		LastExitCode = 1;
	}
	else {
		std::string x;
		do {
			x = ww.GetSelect();
			if (!x.empty()) {
				if (count > 0) {
					s2 += 0x0D;
				}
				count++;
				s2 += x;
			}
		} while (!(!ss.Subset || (x.empty())));
	}

	return s2;
}

void LowCase(std::string& text)
{
	for (char& c : text) {
		c = static_cast<char>(tolower(c));
	}
}

double RoundReal(double RR, short M)
{
	M = MaxI(0, MinI(M, 10));
	double R = RR * Power10[M];
	if (R < 0) R = R - 0.50001;
	else R = R + 0.50001;
	return int(R) / Power10[M];
}

void AccRecNoProc(FrmlElem14* X, WORD Msg, Record* record)
{
	FileD* fd = X->RecFD;
	LockMode md = fd->NewLockMode(RdMode);

	record->Reset();

	int N = RunInt(fd, X->P1, record);
	if ((N <= 0) || (N > fd->GetNRecs())) {
		SetMsgPar(fd->Name, X->RecFldD->Name);
		fd->RunErrorM(md);
		RunError(Msg);
	}
	fd->ReadRec(N, record);
	fd->OldLockMode(md);
}

void GetRecNoXString(FileD* file_d, FrmlElemRecNo* Z, XString& X, Record* record)
{
	X.Clear();

	for (size_t i = 0; i < Z->Key->KFlds.size(); i++) {
		KeyFldD* kf = Z->Key->KFlds[i];
		FrmlElem* zz = Z->Arg[i];
		switch (kf->FldD->frml_type) {
		case 'S': {
			X.StoreStr(RunString(file_d, zz, record), kf);
			break;
		}
		case 'R': {
			X.StoreReal(RunReal(file_d, zz, record), kf);
			break;
		}
		case 'B': {
			X.StoreBool(RunBool(file_d, zz, record), kf);
			break;
		}
		default: {
			// do nothing
			break;
		}
		}
	}
}

void GetTxtPrepare(FileD* file_d, FrmlElem16* Z, HANDLE* h, size_t& off, size_t& len, Record* record)
{
	int l = 0;
	off = 0;
	if (Z->P1 != nullptr) {
		int o = RunInt(file_d, Z->P1, record) - 1;
		if (o < 0) off = 0;
	}
	SetTxtPathVol(Z->TxtPath, Z->TxtCatIRec);
	TestMountVol(CPath[1]);
	*h = OpenH(CPath, _isOldFile, RdOnly);
	if (HandleError != 0) {
		if (HandleError == 2) {
			*h = nullptr;
			len = 0;
			return;
		}
		TestCPathError();
	}
	len = FileSizeH(*h);
	LastExitCode = 0;
	if (off >= len) off = len;
	len -= off;
	if (Z->P2 != nullptr) {
		l = RunInt(file_d, Z->P2, record);
		if (l < 0) l = 0;
		if (l < len) len = l;
	}
	SeekH(*h, off);
}

std::string GetTxt(FileD* file_d, FrmlElem16* Z, Record* record)
{
	HANDLE h = nullptr;
	size_t len = 0, off = 0;

	GetTxtPrepare(file_d, Z, &h, off, len, record);

	if (len > MaxLStrLen) {
		//len = MaxLStrLen;
		LastExitCode = 1; // text is long, but we can process it now -> only inform user
	}

	char* buffer = new char[len];
	LastTxtPos = (int32_t)(off + len);
	if (len > 0) ReadH(h, len, buffer);
	CloseH(&h);

	std::string s = std::string(buffer, len);

	delete[] buffer;
	buffer = nullptr;

	return s;
}