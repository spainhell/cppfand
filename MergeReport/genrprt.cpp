#include "genrprt.h"

#include "rdrprt.h"
#include "runrprt.h"
#include "../cppfand/compile.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/runfrml.h"
#include "../cppfand/wwmenu.h"
#include "../cppfand/wwmix.h"
#include "../textfunc/textfunc.h"


std::vector<PFldD> PFldDs;
bool KpLetter = false;
integer MaxCol = 0, MaxColOld = 0, MaxColUsed = 0, NLines = 0, NLevels = 0;
AutoRprtMode ARMode = _ALstg;

void Design(RprtOpt* RO)
{
	integer L, L2, LTxt, LItem, Col;
	size_t indexD1 = 0;
	FieldDescr* F;
	bool WasTT, LastTT, frstOnLine;

	MaxCol = RO->Width; MaxColOld = MaxCol;
	bool First = true;
	
	switch (RO->Style) {
	case 'C': {
		KpLetter = true;
		break;
	}
	case '?': {
		KpLetter = true;
		MaxCol = trunc(MaxCol / 0.6);
		break;
	}
	}
label1:
	NLines = 1; Col = 1; WasTT = false; LastTT = false;
	//D = PFldDs;
	frstOnLine = true;
	for (size_t i = 0; i < PFldDs.size(); i++) /*while (D != nullptr)*/ {
		PFldD* D = &PFldDs[i];
		F = D->FldD; 
		LTxt = F->Name.length(); 
		LItem = F->L; 
		L = MaxI(LTxt, LItem);
		if (D->IsSum) L2 = 2; 
		else L2 = 0;
		D->NxtLine = false;
		if (LastTT || (F->Typ == 'T') || !frstOnLine && (Col + L2 + L > MaxCol + 1)) {
			D->NxtLine = true; 
			NLines++; 
			Col = 1; 
			indexD1 = i; // TODO: replace with iterator
		}
		frstOnLine = false;
		Col = Col + L2;
		D->ColItem = Col + (L - LItem + 1) / 2;
		if ((F->Typ == 'A' || F->Typ == 'N') && (F->M == LeftJust)) D->ColTxt = Col;
		else D->ColTxt = Col + L - LTxt;
		if (F->Typ == 'T') {
			D->ColItem = 1; 
			D->ColTxt = 1; 
			WasTT = true; 
			LastTT = true;
		}
		else LastTT = false;
		Col += (L + 1);
		//D = D->Chain;
	}
	if (NLines > 1) {
		if (First && (RO->Style == '?')) {
			KpLetter = false;
			MaxCol = RO->Width;
			First = false;
			goto label1;
		}
		MaxColUsed = MaxCol;
		L = MaxCol + 1 - Col;
		if (L > 0 && !WasTT) {
			for (size_t i = indexD1; i < PFldDs.size(); i++) /*while (D != nullptr)*/ {
				PFldD* D = &PFldDs[i];
				D->ColTxt = D->ColTxt + L;
				D->ColItem = D->ColItem + L;
				//D = D->Chain;
			}
		}
	}
	else {
		MaxColUsed = Col;
		if ((MaxColUsed <= RO->Width) && (RO->Style == '?')) {
			MaxCol = RO->Width; 
			KpLetter = false;
		}
	}
}

void WrChar(std::string& report, char C)
{
	report += C;
}

void WrBlks(std::string& report, int N)
{
	if (N <= 0) return;
	else {
		for (size_t i = 0; i < N; i++) {
			report += ' ';
		}
	}
}

void WrStr(std::string& report, std::string& S)
{
	report.append(S);
}

void WrStr(std::string& report, const char* s)
{
	report.append(s);
}

void WrLevel(std::string& report, int Level)
{
	bool first; FieldDescr* f;
	std::string s; 
	bool b = (Level == 0) && (ARMode == _AErrRecs);
	if (b) WrStr(report, "(warning) { noErrRecs+=1},");
	first = true; 
	for (size_t i = 0; i < PFldDs.size(); i++) /*while (d != nullptr)*/ {
		PFldD* d = &PFldDs[i];
		if ((Level == 0) || d->IsSum || d->IsCtrl && (d->Level >= Level)) {
			if (!first) WrChar(report, ',');
			f = d->FldD;
			s = f->Name;
			if ((Level != 0) && d->IsSum) { 
				s = "sum(" + s + ')'; 
			}
			if (f->Typ == 'D') {
				WrStr(report, "strdate(");
				WrStr(report, s);
				WrStr(report, ",'");
				//x = FieldDMask(f);
				std::string x = f->Mask;
				ReplaceChar(x, '\'', '\"');
				WrStr(report, x);
				WrStr(report, "')");
			}
			else WrStr(report, s);
			first = false;
		}
		//d = d->Chain;
	}
	if (b) {
		if (!first) WrChar(report, ',');
		WrStr(report, "errortext+cond(^error:' ??')");
	}
	WrStr(report, ";\r\n");
	int col = 1;
	if (CFile->Typ == '0'/*RDB*/) WrChar(report, 0x11);

	for (size_t i = 0; i < PFldDs.size(); i++) /*while (d != nullptr)*/ {
		PFldD* d = &PFldDs[i];
		if ((CFile->Typ == '0') && (i + 1 == PFldDs.size())) {
			WrChar(report, 0x11);
		}
		if (d->NxtLine) { 
			WrStr(report, "\r\n");
			col = 1; 
		}
		f = d->FldD; 
		int l = f->L; 
		int n = d->ColItem - col; 
		col = d->ColItem + l;
		if ((Level == 0) || d->IsSum || d->IsCtrl && (d->Level >= Level)) {
			if ((Level != 0) && d->IsSum) { n -= 2; l += 2; }
			WrBlks(report, n);
			if (f->Typ == 'F' || f->Typ == 'R') {
				int m = f->M;
				if (m != 0) {
					for (size_t j = 0; j < l - m - 1; j++) WrChar(report, '_');
					l = m;
					if ((f->Flg & f_Comma) != 0) WrChar(report, ',');
					else WrChar(report, '.');
				}
			}
			for (size_t j = 0; j < l; j++) WrChar(report, '_');
		}
		else WrBlks(report, n + l);
		//d = d->Chain;
	}
	if (Level > 0) {
		WrBlks(report, MaxColUsed - col + 1);
		for (size_t j = 0; j < Level; j++) WrChar(report, '*');
	}
	if (b) {
		WrStr(report, "\r\n\x17");
		WrBlks(report, 5);
		WrStr(report, "_\x17");
	}
	if ((ARMode != _AErrRecs) && (NLines > 1)) WrStr(report, "\r\n");
}

std::string GenAutoRprt(RprtOpt* RO, bool WithNRecs)
{
	KeyFldD* kf;
	//char* p;
	bool first, point;
	std::string s;

	CFile = RO->FDL.FD;
	ARMode = RO->Mode;
	NLevels = RO->Ctrl.size(); // ListLength(RO->Ctrl);
	PFldDs.clear();
	FieldListEl* fl = RO->Flds;
	while (fl != nullptr) {
		PFldD d = PFldD(); //(PFldD*)GetZStore(sizeof(PFldD));
		FieldDescr* f = fl->FldD;
		d.FldD = f;
		d.IsSum = FieldInList(f, RO->Sum);
		//FieldListEl* fl1 = RO->Ctrl;
		int i = NLevels;
		for (size_t k = 0; k < RO->Ctrl.size(); k++) /*while (fl1 != nullptr)*/ {
			FieldListEl* fl1 = RO->Ctrl[k];
			if (fl1->FldD == f) {
				d.IsCtrl = true;
				d.Level = i;
			}
			i--;
			//fl1 = (FieldListEl*)fl1->Chain;
		}
		if ((ARMode == _ATotal) && !d.IsSum && !d.IsCtrl) {
			//ReleaseStore(d);
		}
		else {
			//ChainLast(PFldDs, d);
			PFldDs.push_back(std::move(d));
		}
		fl = (FieldListEl*)fl->Chain;
	}

	Design(RO);

	// Txt = new LongStr(2); // (LongStr*)GetZStore(2);
	std::string report = "";
	report.reserve(2048);

	if ((ARMode == _AErrRecs)) WrStr(report, "var noErrRecs:real;\r\n");
	WrStr(report, "#I1_");
	WrStr(report, CFile->Name);
	if (RO->SK != nullptr) WrChar(report, '!');
	WrBlks(report, 2);
	first = true; 
	// fl = RO->Ctrl; 
	kf = RO->SK;
	for (size_t k = 0; k < RO->Ctrl.size(); k++)/*while (fl != nullptr)*/ {
		fl = RO->Ctrl[k];
		if (!first) WrChar(report, ',');
		FieldDescr* f = fl->FldD;
		if ((kf != nullptr) && (f == kf->FldD)) {
			if (kf->Descend) WrChar(report, '>');
			if (kf->CompLex) WrChar(report, '~');
			kf = (KeyFldD*)kf->Chain;
		}
		else if (f->Typ == 'A') WrChar(report, '~');
		WrStr(report, f->Name);
		//fl = (FieldListEl*)fl->Chain; 
		first = false;
	}
	if (kf != nullptr) {
		if (!first) WrChar(report, ';'); first = true;
		while (kf != nullptr) {
			if (!first) WrChar(report, ',');
			if (kf->Descend) WrChar(report, '>');
			if (kf->CompLex) WrChar(report, '~');
			WrStr(report, kf->FldD->Name);
			kf = (KeyFldD*)kf->Chain; 
			first = false;
		}
	}

	if ((ARMode == _ATotal) && (NLevels == 0)) WrStr(report, "\r\n#RH");
	else WrStr(report, "\r\n#PH ");

	if (RO->HeadTxt.empty()) {
		WrStr(report, "today,page;\r\n");
		WrBlks(report, 19);
		WrChar(report, 0x11);
		s = CFile->Name;
		WrBlks(report, 8 - s.length());
		ReplaceChar(s, '_', '-');
		WrStr(report, s);
		WrChar(report, 0x11);
		WrBlks(report, 14);
		WrStr(report, "__.__.____");
		RdMsg(17);
		WrBlks(report, 12 - MsgLine.length());
		WrStr(report, MsgLine);
		WrStr(report, "___");
	}
	else {
		size_t l = RO->HeadTxt.length();
		const char* p = RO->HeadTxt.c_str();
		int i = 0;
		first = true;
		while (i < l) {
			if (p[i] == '_') {
				point = false;
				while ((i <= l) && (p[i] == '_' || p[i] == '.')) {
					if (p[i] == '.') point = true;
					i++;
				}
				if (!first) WrChar(report, ','); first = false;
				if (point) WrStr(report, "today");
				else WrStr(report, "page");
			}
			i++;
		}
		WrStr(report, ";\r\n");
		for (size_t j = 0; j < l - 1; j++) WrChar(report, p[j]);
	}

	if (ARMode == _AErrRecs) {
		RdMsg(18);
		WrStr(report, "\r\n\x17");
		WrBlks(report, (38 - MsgLine.length()) / 2);
		WrStr(report, MsgLine); 
		WrChar(report, 0x17);
	}
	if (!RO->CondTxt.empty()) {
		WrStr(report, "\r\n\x17");
		s = RO->CondTxt;
		ReplaceChar(s, '{', '%');
		ReplaceChar(s, '}', '%');
		ReplaceChar(s, '_', '-');
		ReplaceChar(s, '@', '*');
		ReplaceChar(s, '#', '=');
		ReplaceChar(s, '\\', '|');
		if (s.length() > MaxCol) s = s.substr(0, MaxCol);
		WrBlks(report, ((int)MaxColOld - (int)s.length()) / 2);
		WrStr(report, s);
		WrChar(report, 0x17);
	}
	WrStr(report, "\r\n");
	if (KpLetter) WrChar(report, 0x05);
	//d = PFldDs;
	int col = 1;
	for (size_t i = 0; i < PFldDs.size(); i++) /*while (d != nullptr)*/ {
		PFldD* d = &PFldDs[i];
		if (d->NxtLine) {
			WrStr(report, "\r\n");
			col = 1;
		}
		WrBlks(report, d->ColTxt - col);
		s = d->FldD->Name;
		ReplaceChar(s, '_', '-');
		WrStr(report, s);
		col = d->ColTxt + d->FldD->Name.length();
		//d = d->Chain;
	}

	if (KpLetter) WrStr(report, "\r\n#PF;\r\n\x05");

	WrStr(report, "\r\n#DH .notsolo;\r\n");
	if (ARMode != _ATotal) {
		WrStr(report, "\r\n#DE ");
		WrLevel(report, 0);
	}
	for (size_t i = 1; i <= NLevels; i++) {
		WrStr(report, "\r\n#CF_");
		//d = PFldDs;
		for (size_t j = 0; j < PFldDs.size(); j++) /*while (d != nullptr)*/ {
			PFldD* d = &PFldDs[j];
			if (d->IsCtrl && (d->Level == i)) WrStr(report, d->FldD->Name);
			//d = d->Chain;
		}
		WrChar(report, ' ');
		WrLevel(report, i);
	}
	if ((!RO->Ctrl.empty()) || (RO->Sum != nullptr)) {
		WrStr(report, "\r\n#RF (sum(1)>0) "); 
		WrLevel(report, NLevels + 1);
	}
	if (WithNRecs) {
		WrStr(report, "\r\n#RF ");
		if (ARMode == _AErrRecs) WrStr(report, "noErrRecs,");
		WrStr(report, "sum(1);\r\n\r\n");
		if (ARMode == _AErrRecs) {
			RdMsg(18);
			WrStr(report, MsgLine);
			WrStr(report, ":_____\r\n");
		}
		RdMsg(20);
		WrStr(report, MsgLine);
		WrStr(report, "_______");
	}
	return report;
	/* for i = 1 to Txt->LL do write(Txt->A[i]); writeln; wait; */
}

void RunAutoReport(RprtOpt* RO)
{
	void* p = nullptr; void* p1 = nullptr;
	p1 = RO->FDL.FD->RecPtr;
	std::string txt = GenAutoRprt(RO, true);
	// SetInpLongStr(txt, false);
	SetInpStdStr(txt, false);
	ReadReport(RO);
	RunReport(RO);
	RO->FDL.FD->RecPtr = p1;
}

bool SelForAutoRprt(RprtOpt* RO)
{
	wwmix ww;

	FieldListEl* FL; WORD N;
	auto result = false;
	if ((RO->SK == nullptr) && !PromptSortKeys(RO->Flds, RO->SK)) return result;
	N = Menu(4, 1);
	if (N == 0) return result;
	RO->Mode = AutoRprtMode(N - 1);
	CFile = RO->FDL.FD;
	if (RO->Mode == _ARprt || RO->Mode == _ATotal) {
		FL = RO->Flds;
		while (FL != nullptr) {
			if (FL->FldD->Typ != 'T') ww.PutSelect(FL->FldD->Name);
			FL = (FieldListEl*)FL->Chain;
		}
		if (!ww.SelFieldList(37, false, &RO->Ctrl[0])) return result; // TODO: RO->Ctrl[0] is probably bad idea
		FL = RO->Flds;
		while (FL != nullptr) {
			if (FL->FldD->FrmlTyp == 'R') ww.PutSelect(FL->FldD->Name);
			FL = (FieldListEl*)FL->Chain;
		}
		if (!ww.SelFieldList(38, true, &RO->Sum)) return result;
	}
	if (spec.AutoRprtPrint) {
		RO->Path = "LPT1";
	}
	result = true;
	return result;
}

std::string SelGenRprt(pstring RprtName)
{
	wwmix ww;
	RdbD* r; FileD* fd; FieldDescr* f; RprtOpt* ro;
	std::string s; size_t i;
	FieldListEl* fl;
	std::string result;
	r = CRdb;
	while (r != nullptr) {
		fd = (FileD*)r->FD->Chain;
		while (fd != nullptr) {
			s = fd->Name;
			if (r != CRdb) s = r->FD->Name + '.' + s;
			ww.PutSelect(s);
			fd = (FileD*)fd->Chain;
		}
		r = r->ChainBack;
	}
	ss.Abcd = true;
	pstring tmpP = "\"";
	ww.SelectStr(0, 0, 19, tmpP + RprtName + '\"');
	if (Event.Pressed.KeyCombination() == __ESC) return result;
	s = ww.GetSelect();
	i = s.find('.'); r = CRdb;
	if (i != std::string::npos) {
		do { r = r->ChainBack; } while (r->FD->Name != s.substr(1, i - 1));
		s = s.substr(i + 1, 255);
	}
	fd = r->FD;
	do { fd = (FileD*)fd->Chain; } while (fd->Name != s);
	ro = GetRprtOpt(); ro->FDL.FD = fd;
	f = fd->FldD.front();
	while (f != nullptr) {
		s = f->Name;
		if ((f->Flg & f_Stored) == 0)
		{
			pstring oldS = s;
			s = SelMark; s += oldS;
		}
		ww.PutSelect(s);
		f = (FieldDescr*)f->Chain;
	}
	CFile = fd;
	ww.SelFieldList(36, true, &ro->Flds);
	if (ro->Flds == nullptr) return result;
	ro->Mode = _ARprt;
	fl = ro->Flds;
	while (fl != nullptr) {
		ww.PutSelect(fl->FldD->Name);
		fl = (FieldListEl*)fl->Chain;
	}
	if (!ww.SelFieldList(37, false, &ro->Ctrl[0])) return result; // TODO: ro->Ctrl[0] is probably bad idea
	fl = ro->Flds;
	while (fl != nullptr) {
		if (fl->FldD->FrmlTyp == 'R') ww.PutSelect(fl->FldD->Name);
		fl = (FieldListEl*)fl->Chain;
	}
	if (!ww.SelFieldList(38, false, &ro->Sum)) return result;
	result = GenAutoRprt(ro, false);
	return result;
}
