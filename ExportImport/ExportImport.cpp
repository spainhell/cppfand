#include "ExportImport.h"
#include <memory>
#include "TbFile.h"
#include "ThFile.h"
#include "TzFile.h"
#include "../cppfand/FileD.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obaseww.h"
#include "../Editor/rdedit.h"
#include "../Editor/runedi.h"
#include "../cppfand/wwmix.h"
#include "../FileSystem/directory.h"
#include "../cppfand/compile.h"
#include "../MergeReport/rdmerg.h"
#include "../MergeReport/runmerg.h"
#include "../textfunc/textfunc.h"


bool OldToNewCat(longint& FilSz)
{
	struct stX { longint NRecs; WORD  RecLen; } x;
	longint off, offNew;
	BYTE a[91]; // budeme cislovat od 1, jako v Pascalu (a:array[1..90] of byte;)

	auto result = false;
	bool cached = CFile->NotCached();
	if (CFile->Typ != 'C') return result;
	RdWrCache(true, CFile->Handle, cached, 0, 6, &x);
	if (x.RecLen != 106) return result;
	x.RecLen = 107;
	RdWrCache(false, CFile->Handle, cached, 0, 6, &x);
	for (longint i = x.NRecs; i >= 1; i--) {
		off = 6 + (i - 1) * 106;
		offNew = off + (i - 1);
		RdWrCache(true, CFile->Handle, cached, off + 16, 90, a);
		RdWrCache(false, CFile->Handle, cached, offNew + 17, 90, a);
		a[17] = 0;
		RdWrCache(true, CFile->Handle, cached, off, 16, a);
		RdWrCache(false, CFile->Handle, cached, offNew, 17, a);
	}
	CFile->NRecs = x.NRecs;
	FilSz = x.NRecs * 107 + 6;
	result = true;
	return result;
}

void ConvWinCp(unsigned char* pBuf, unsigned char* pKod, WORD L)
{
	for (size_t i = 0; i < L; i++) {
		if (pBuf[i] < 0x80) continue;
		pBuf[i] = pKod[pBuf[i] - 0x80];
	}
}

void VarFixImp(ThFile* F1, CpOption Opt)
{
	pstring s;
	longint pos;
	double r; WORD err;

	F1->IsEOL = false;
	for (FieldDescr* F : CFile->FldD) {
		if ((F->Flg & f_Stored) != 0) {
			if (F1->IsEOL) {
				switch (F->FrmlTyp) {
				case 'R': R_(F, 0); break;
				case 'B': B_(F, false); break;
				case 'S': S_(F, ""); break;
				default: ;
				}
			}
			else {
				switch (F->Typ) {
				case 'F': {
					if (Opt == CpOption::cpFix) {
						s = F1->RdFix(F->L);
					}
					else {
						s = F1->RdDelim(',');
					}
					val(LeadChar(' ', s), r, err);
					if ((F->Flg & f_Comma) != 0) {
						r = r * Power10[F->M]; R_(F, r);
					}
					break;
				}
				case 'A': {
					if (Opt == CpOption::cpFix) {
						S_(F, F1->RdFix(F->L));
					}
					else {
						char c = (char)ForwChar;
						if (c == '\'' || c == '"') {
							F1->RdChar();
							s = F1->RdDelim(c);
							F1->RdDelim(',');
						}
						else {
							s = F1->RdDelim(',');
						}
						S_(F, s);
					}
					break;
				}
				case 'N': {
					if (Opt == CpOption::cpFix) {
						S_(F, F1->RdFix(F->L));
					}
					else {
						S_(F, F1->RdDelim(','));
					}
					break;
				}
				case 'D':
				case 'R': {
					if (Opt == CpOption::cpFix) {
						s = F1->RdFix(F->L);
					}
					else {
						s = F1->RdDelim(',');
						if (s[1] == '\'' || s[1] == '"') {
							s = copy(s, 2, s.length() - 2);
						}
					}
					if (s == "") {
						R_(F, 0.0);
					}
					else if (F->Typ == 'R') {
						val(s, r, err);
						R_(F, r);
					}
					else {
						R_(F, ValDate(s, F->Mask));
					}
					break;
				}
				case 'B': {
					s = F1->RdFix(1);
					B_(F, s[1] = 'A');
					if (Opt == CpOption::cpVar) {
						F1->RdDelim(',');
					}
					break;
				}
				case 'T': {
					if (Opt == CpOption::cpVar) {
						std::string x = F1->RdLongStr();
						s = F1->RdDelim(',');
						S_(F, x);
					}
					else {
						T_(F, 0);
					}
					break;
				}
				default: ;
				}
			}
		}
	}
	if (!F1->IsEOL) F1->RdDelim('\r');
}

void VarFixExp(ThFile* F2, CpOption Opt)
{
	WORD i, n;
	std::string s, s1;
	LongStr* x;
	bool first; double r;

	first = true;
	for (auto& F : CFile->FldD) {
		if ((F->Flg & f_Stored) != 0) {
			if (first) first = false;
			else if (Opt == CpOption::cpVar) F2->WrChar(',');

			switch (F->Typ) {
			case 'F': {
				r = _R(F);
				if ((F->Flg & f_Comma) != 0) r = r / Power10[F->M];
				str(r, F->L, F->M, s);
				if (s.length() > F->L) {
					s[0] = '>';
					s = s.substr(0, F->L);
				}
				if (Opt == CpOption::cpVar) {
					s = LeadChar(' ', s);
					if (F->M > 0) {
						s = TrailChar(s, '0');
						if ((s.length() > 0) and (s[s.length() - 1] == '.')) s = s.substr(0, s.length() - 1);
					}
					if (s == "0") s = "";

				}
				break;
			}
			case 'A': {
				s = _StdS(F);
				if (Opt == CpOption::cpVar) {
					if (F->M == 1) s = TrailChar(s, ' ');
					else s = LeadChar(' ', s);
					s1 = "";
					for (i = 0; i < s.length(); i++) {
						s1 += s[i];
						if (s[i] == '\'') s1 += '\'';
					}
					s = '\'' + s1 + '\'';
				}
				break;
			}
			case 'N': {
				s = _StdS(F);
				if (Opt == CpOption::cpVar) {
					if (F->M == 1) s = TrailChar(s, '0');
					else s = LeadChar('0', s);
				}
				break;
			}
			case 'D':
			case 'R': {
				r = _R(F);
				if ((r == 0.0) && (Opt == CpOption::cpVar)) s = "";
				else if (F->Typ == 'R') str(r, F->L, s);
				else {
					s = StrDate(r, F->Mask);
					if (Opt == CpOption::cpVar) s = '\'' + s + '\'';
				}
				break;
			}
			case 'B': {
				if (_B(F)) s = 'A';
				else s = 'N';
				break;
			}
			case 'T': {
				if (Opt == CpOption::cpVar) {
					x = _LongS(F);
					F2->WrLongStr(x, true);
					delete x;
				}
				break;
			}
			}

			if (F->Typ != 'T') F2->WrString(s);
		}
	}
}

void ImportTxt(CopyD* CD)
{
	ThFile* F1 = nullptr;
	LockMode md;
	auto FE = std::make_unique<FrmlElem4>(_const, 0);

#ifdef FandSQL
	SQLStreamPtr q;
#endif
	try {
#ifdef FandSQL
		q = nullptr;
#endif
		F1 = new ThFile(CD->Path1, CD->CatIRec1, InOutMode::_inp, 0, nullptr);
		if (CD->HdFD != nullptr) {
			FE->Op = _const;
			FE->S = F1->RdDelim(0x1A); //^Z
			AsgnParFldFrml(CD->HdFD, CD->HdF, FE.get(), false);
		}
		CFile = CD->FD2;
		CRecPtr = GetRecSpace();
#ifdef FandSQL
		if (CFile->IsSQLFile) {
			New(q, Init);
			q->OutpRewrite(Append);
		}
		else
#endif
			md = RewriteF(CD->Append);
		while (!(F1->eof) && (F1->ForwChar() != 0x1A)) {
			ZeroAllFlds();
			ClearDeletedFlag();
			VarFixImp(F1, CD->Opt1);
			F1->ForwChar(); //{set IsEOF at End}
#ifdef FandSQL
			if (CFile->IsSQLFile) q->PutRec();
			else
#endif
			{
				PutRec(CFile, CRecPtr);
				if (CD->Append && (CFile->Typ == 'X')) TryInsertAllIndexes(CFile->IRec);
			}
		}
		LastExitCode = 0;
	}
	catch (std::exception& e) {

	}
#ifdef FandSQL
	if (q != nullptr) {
		q->OutpClose();
		ClearRecSpace(CRecPtr);
	};
#endif
	if ((F1 != nullptr) && (F1->Handle != nullptr)) {
		delete F1;
		OldLMode(md);
	}
}

void ExportTxt(CopyD* CD)
{
	ThFile* F2 = nullptr;
	LockMode md = NullMode;
	XScan* Scan = nullptr;

	try {
		InOutMode m;
		if (CD->Append) {
			m = InOutMode::_append;
		}
		else {
			m = InOutMode::_outp;
		}

		F2 = new ThFile(CD->Path2, CD->CatIRec2, m, 0, nullptr);
		if (CD->HdFD != nullptr) {
			longint n = 0;
			LinkLastRec(CD->HdFD, n, true);
			pstring s = _ShortS(CD->HdF);
			longint i = s.first('\r');
			if (i > 0) s[0] = i - 1;
			F2->WrString(s);
			F2->WrString("\r\n");
			ClearRecSpace(CRecPtr);
			ReleaseStore(CRecPtr);
		}
		CFile = CD->FD1;
		CRecPtr = GetRecSpace();
		md = NewLMode(RdMode);
		Scan = new XScan(CFile, CD->ViewKey, nullptr, true);
		Scan->Reset(nullptr, false);
		RunMsgOn('C', Scan->NRecs);
		while (true) {
			Scan->GetRec();
			if (!Scan->eof) {
				VarFixExp(F2, CD->Opt2);
				F2->WrString("\r\n");
				RunMsgN(Scan->IRec);
				continue;
			}
			break;
		}
		LastExitCode = 0;
		RunMsgOff();
	}
	catch (std::exception& e) {
		// TODO: log error
	}

	if (Scan != nullptr) {
		Scan->Close();
		ClearRecSpace(CRecPtr);
		OldLMode(md);
	}
	if (F2 != nullptr && F2->Handle != nullptr) {
		if (LastExitCode != 0) {
			F2->ClearBuf();
		}
		delete F2;
	}
}

void Cpy(FILE* h, longint sz, ThFile* F2)
{
	SeekH(h, 0);
	longint i = 0;
	RunMsgOn('C', sz);
	while (i < sz) {
		WORD n;
		if (sz - i > F2->BufSize) n = F2->BufSize;
		else n = sz - i;
		i += n;
		ReadH(h, n, F2->Buf);
		TestCFileError();
		F2->lBuf = n;
		F2->WriteBuf(false);
		RunMsgN(i);
	}
	delete F2;
	RunMsgOff();
}

void ExportFD(CopyD* CD)
{
	ThFile* F2 = nullptr;
	LockMode md = NullMode;

	try {
		CFile = CD->FD1;
		SaveFiles();
		md = NewLMode(RdMode);
		F2 = new ThFile(CD->Path2, CD->CatIRec2, InOutMode::_outp, 0, nullptr);
		longint n = XNRecs(CD->FD1->Keys);

		if (n == 0) {
			delete F2;
			F2 = nullptr;
		}
		else {
			Cpy(CFile->Handle, CFile->UsedFileSize(), F2);
		}

		if (CFile->TF != nullptr) {
			F2->RewriteT(); /* !!! with CFile->TF^ do!!! */
			if (n == 0) {
				delete F2;
				F2 = nullptr;
			}
			else Cpy(CFile->TF->Handle, CFile->TF->UsedFileSize(), F2);
		}

		if (CD->WithX1) {
			F2->RewriteX(); /* !!! with CFile->GetXFile^ do!!! */
			if (n == 0) {
				delete F2;
				F2 = nullptr;
			}
			else Cpy(CFile->XF->Handle, CFile->XF->UsedFileSize(), F2);
		}

		LastExitCode = 0;
	}

	catch (std::exception& e) {
		// TODO: log error
	}

	if ((F2 != nullptr) && (F2->Handle != nullptr)) {
		if (LastExitCode != 0) F2->ClearBuf();
		delete F2; F2 = nullptr;
		OldLMode(md);
	}
}

void TxtCtrlJ(CopyD* CD)
{
	ThFile* F1 = nullptr;
	ThFile* F2 = nullptr;
	InOutMode m = InOutMode::_outp;

	if (CD->Append) m = InOutMode::_append;
	try {
		F1 = new ThFile(CD->Path1, CD->CatIRec1, InOutMode::_inp, 0, nullptr);
		F2 = new ThFile(CD->Path2, CD->CatIRec2, m, 0, F2);

		char c = F1->RdChar();
		while (!F1->eof) {
			switch (c) {
			case '\r': {
				F2->WrChar('\r');
				F2->WrChar('\n');
				break;
			}
			case '\n': {
				break;
			}
			default: {
				F2->WrChar(c);
				break;
			}
			}
			c = F1->RdChar();
		}
		LastExitCode = 0;
	}
	catch (std::exception& e) {
		// TODO: log error
	}

	if ((F1 != nullptr) && (F1->Handle != nullptr)) delete F1;
	if ((F2 != nullptr) && (F2->Handle != nullptr)) {
		if (LastExitCode != 0) F2->ClearBuf();
		delete F2;
	}
}

void MakeCopy(CopyD* CD)
{
	try {
		ThFile F1 = ThFile(CD->Path1, CD->CatIRec1, InOutMode::_inp, 0, nullptr);
		ThFile F2 = ThFile(CD->Path2, CD->CatIRec2, CD->Append ? InOutMode::_append : InOutMode::_outp, 0, &F1);
		if (HandleError != 0) {
			//delete F1;
			LastExitCode = 1;
			return;
		}

		while (!F1.eof) {
			memcpy(F2.Buf, F1.Buf, F1.lBuf);
			F2.lBuf = F1.lBuf;
			switch (CD->Mode) {
			case 1: {
				ConvKamenLatin(F2.Buf, F2.lBuf, true);
				break;
			}
			case 2: {
				ConvKamenLatin(F2.Buf, F2.lBuf, false);
				break;
			}
			case 3: {
				ConvToNoDiakr(F2.Buf, F2.lBuf, TVideoFont::foKamen);
				break;
			}
			case 4: {
				ConvToNoDiakr(F2.Buf, F2.lBuf, TVideoFont::foLatin2);
				break;
			}
			case 5: {
				std::string pKod = ResFile.Get(LatToWinCp);
				ConvWinCp(F2.Buf, (unsigned char*)pKod.c_str(), F2.lBuf);
				break;
			}
			case 6: {
				std::string pKod = ResFile.Get(KamToWinCp);
				ConvWinCp(F2.Buf, (unsigned char*)pKod.c_str(), F2.lBuf);
				break;
			}
			case 7: {
				std::string pKod = ResFile.Get(WinCpToLat);
				ConvWinCp(F2.Buf, (unsigned char*)pKod.c_str(), F2.lBuf);
				break;
			}
			default: break;
			}
			F2.WriteBuf(false);
			F1.ReadBuf();
		}
		LastExitCode = 0;
	}

	catch (std::exception& e) {
		LastExitCode = 1;
	}
}

void FileCopy(CopyD* CD)
{
	void* p = nullptr, * p2;
	LastExitCode = 2;
	if (CD->Opt1 == CpOption::cpFix || CD->Opt1 == CpOption::cpVar) {
		ImportTxt(CD);
	}
	else if (CD->Opt2 == CpOption::cpFix || CD->Opt2 == CpOption::cpVar) {
		ExportTxt(CD);
	}
	else if (CD->FD1 != nullptr) {
		if (CD->FD2 != nullptr) {
			MakeMerge(CD);
		}
		else {
			ExportFD(CD);
		}
	}
	else if (CD->Opt1 == CpOption::cpTxt) {
		TxtCtrlJ(CD);
	}
	else {
		MakeCopy(CD);
	}
	SaveFiles();
	RunMsgOff();
	if (LastExitCode != 0 && !CD->NoCancel) GoExit();
}

void MakeMerge(CopyD* CD)
{
	try {
		std::string s = "#I1_" + CD->FD1->Name;
		if (CD->ViewKey != nullptr) {
			std::string ali = CD->ViewKey->Alias;
			if (ali.empty()) ali = '@';
			s = s + '/' + ali;
		}
		s = s + " #O1_" + CD->FD2->Name;
		if (CD->Append) s = s + '+';

		SetInpStr(s);
		ReadMerge();
		RunMerge();
		LastExitCode = 0;
	}

	catch (std::exception& e) {
	}
}

void Backup(bool IsBackup, bool NoCompress, WORD Ir, bool NoCancel)
{
	TbFile* F = new TbFile(NoCompress);

	try {
		LastExitCode = 1;
		F->Backup(IsBackup, Ir);
		LastExitCode = 0;
	}
	catch (std::exception& e) {
		// TODO: log error
	}

	ReleaseStore(F);
	if (LastExitCode != 0) {
		RunMsgOff();
		if (!NoCancel) GoExit();
	}
}

void BackupM(Instr_backup* PD)
{
	LongStr* s = nullptr;
	void* p = nullptr;

	MarkStore(p);
	if (PD->IsBackup) s = RunLongStr(PD->bmMasks);
	TzFile* F = new TzFile(PD->IsBackup, PD->NoCompress, PD->bmSubDir, PD->bmOverwr,
		PD->BrCatIRec, RunShortStr(PD->bmDir));
	try {
		LastExitCode = 1;
		if (PD->IsBackup) F->Backup(s);
		else F->Restore();
		LastExitCode = 0;
	}
	catch (std::exception& e) {
		// TODO: log error
	}

	F->Close();
	if (LastExitCode != 0) {
		RunMsgOff();
		if (!PD->BrNoCancel) GoExit();
	}
	ReleaseStore(p);
}


void CheckFile(FileD* FD)
{
	struct stPrfx { longint NRecs; WORD RecLen; } Prfx;
	FILE* h = nullptr;
	std::string d, n, e;
	longint fs = 0;

	TestMountVol(CPath[0]);

	const int fileStatus = fileExists(CPath);
	if (fileStatus != 0) {
		// path not exists
		if (fileStatus == 1) LastExitCode = 1;
		else LastExitCode = 2;
		return;
	}

	h = OpenH(_isoldfile, RdShared);
	LastExitCode = 0;
	if (HandleError != 0) {
		if (HandleError == 2) LastExitCode = 1;
		else LastExitCode = 2;
		return;
	}
	ReadH(h, 4, &Prfx.NRecs);
	ReadH(h, 2, &Prfx.RecLen);
	fs = FileSizeH(h);
	CloseH(&h);
	if ((FD->RecLen != Prfx.RecLen) || (Prfx.NRecs < 0) && (FD->Typ != 'X') ||
		((fs - FD->FrstDispl) / Prfx.RecLen < Prfx.NRecs) ||
		(Prfx.NRecs > 0) && (FD->Typ == 'X')) {
		LastExitCode = 3;
		return;
	}
	if (FD->TF == nullptr) return;
	FSplit(CPath, d, n, e);
	if (EquUpCase(e, ".RDB")) e = ".TTT";
	else e[1] = 'T';
	CPath = d + n + e;
	h = OpenH(_isoldfile, RdShared);
	if (HandleError == 0) CloseH(&h);
	else LastExitCode = 4;
}

void CodingCRdb(bool Rotate)
{
	auto crdb = std::make_unique<CodingRdb>();
	crdb->CodeRdb(Rotate);
}

void AddLicNr(FieldDescr* F)
{
	if (_T(F) != 0) T_(F, _T(F) + (WORD(UserLicNrShow) & 0x7FFF));
}

void CopyH(FILE* H, pstring Nm)
{
	WORD n; FILE* h2;
	BYTE* buf = new BYTE[MaxLStrLen]; // GetStore(MaxLStrLen);
	CPath = Nm;
	CVol = "";
	h2 = OpenH(_isoverwritefile, Exclusive);
	longint sz = FileSizeH(H);
	SeekH(H, 0);
	while (sz > 0) {
		if (sz > MaxLStrLen) n = MaxLStrLen;
		else n = sz;
		sz -= n;
		ReadH(H, n, buf);
		WriteH(h2, n, buf);
	}
	CloseH(&h2);
	delete[] buf;
	buf = nullptr;
}

bool PromptCodeRdb()
{
	WORD i;
	FileD* cf;
	void* cr;
	auto wx = std::make_unique<wwmix>();
	wx->SetPassWord(Chpt, 1, "");
	wx->SetPassWord(Chpt, 2, "");
	bool result = true;

	F10SpecKey = __ALT_F10;

	bool b = PromptYN(133);
	WORD KbdChar = Event.Pressed.KeyCombination();
	if (KbdChar == __ALT_F10) {
		F10SpecKey = _ESC_;
		b = PromptYN(147);
		if (KbdChar == __ESC) goto label1;
		if (b) {
			CFile = Chpt;
			WrPrefixes();
			// TODO: SaveCache(0);
			std::string s = CRdb->RdbDir;
			AddBackSlash(s);
			s = s + Chpt->Name;
			CopyH(Chpt->Handle, s + ".RD0x");
			CopyH(ChptTF->Handle, s + ".TT0x");
		}
		CodingCRdb(true);
		ChptTF->LicenseNr = WORD(UserLicNrShow) & 0x7FFF;
		cf = CFile;
		cr = CRecPtr;
		CFile = Chpt;
		CRecPtr = GetRecSpace;
		for (i = 1; i <= Chpt->NRecs; i++) {
			ReadRec(CFile, i, CRecPtr);
			AddLicNr(ChptOldTxt);
			AddLicNr(ChptTxt);
			WriteRec(CFile, i, CRecPtr);
		}
		ReleaseStore(CRecPtr);
		CFile = cf;
		CRecPtr = cr;
		return result;
	}
	if (b) {
		wx->SetPassWord(Chpt, 1, wx->PassWord(true));
		if (wx->HasPassWord(Chpt, 1, "")) {
			goto label1;
		}
		CodingCRdb(false);
	}
	else {
	label1:
		auto coding = std::make_unique<CodingRdb>();
		coding->CompressCRdb();
		result = false;
	}
	return result;
}
