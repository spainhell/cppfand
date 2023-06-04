#include "ExportImport.h"
#include <memory>
#include "TbFile.h"
#include "ThFile.h"
#include "TzFile.h"
#include "../cppfand/Coding.h"
#include "../cppfand/FileD.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obaseww.h"
#include "../Editor/rdedit.h"
#include "../Editor/runedi.h"
#include "../cppfand/wwmix.h"
#include "../fandio/directory.h"
#include "../cppfand/compile.h"
#include "../MergeReport/rdmerg.h"
#include "../MergeReport/runmerg.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"
#include "../Common/codePages.h"
#include "../fandio/files.h"


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
	int pos;
	double r; WORD err;

	F1->IsEOL = false;
	for (FieldDescr* F : CFile->FldD) {
		if ((F->Flg & f_Stored) != 0) {
			if (F1->IsEOL) {
				switch (F->frml_type) {
				case 'R': CFile->saveR(F, 0, CRecPtr); break;
				case 'B': CFile->saveB(F, false, CRecPtr); break;
				case 'S': CFile->saveS(F, "", CRecPtr); break;
				default: ;
				}
			}
			else {
				switch (F->field_type) {
				case FieldType::FIXED: {
					if (Opt == CpOption::cpFix) {
						s = F1->RdFix(F->L);
					}
					else {
						s = F1->RdDelim(',');
					}
					val(LeadChar(' ', s), r, err);
					if ((F->Flg & f_Comma) != 0) {
						r = r * Power10[F->M];
						CFile->saveR(F, r, CRecPtr);
					}
					break;
				}
				case FieldType::ALFANUM: {
					if (Opt == CpOption::cpFix) {
						CFile->saveS(F, F1->RdFix(F->L), CRecPtr);
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
						CFile->saveS(F, s, CRecPtr);
					}
					break;
				}
				case FieldType::NUMERIC: {
					if (Opt == CpOption::cpFix) {
						CFile->saveS(F, F1->RdFix(F->L), CRecPtr);
					}
					else {
						CFile->saveS(F, F1->RdDelim(','), CRecPtr);
					}
					break;
				}
				case FieldType::DATE:
				case FieldType::REAL: {
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
						CFile->saveR(F, 0.0, CRecPtr);
					}
					else if (F->field_type == FieldType::REAL) {
						val(s, r, err);
						CFile->saveR(F, r, CRecPtr);
					}
					else {
						CFile->saveR(F, ValDate(s, F->Mask), CRecPtr);
					}
					break;
				}
				case FieldType::BOOL: {
					s = F1->RdFix(1);
					CFile->saveB(F, s[1] = 'A', CRecPtr);
					if (Opt == CpOption::cpVar) {
						F1->RdDelim(',');
					}
					break;
				}
				case FieldType::TEXT: {
					if (Opt == CpOption::cpVar) {
						std::string x = F1->RdLongStr();
						s = F1->RdDelim(',');
						CFile->saveS(F, x, CRecPtr);
					}
					else {
						CFile->saveT(F, 0, CRecPtr);
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

			switch (F->field_type) {
			case FieldType::FIXED: {
				r = CFile->loadR(F, CRecPtr);
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
			case FieldType::ALFANUM: {
				s = CFile->loadS(F, CRecPtr);
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
			case FieldType::NUMERIC: {
				s = CFile->loadS(F, CRecPtr);
				if (Opt == CpOption::cpVar) {
					if (F->M == 1) s = TrailChar(s, '0');
					else s = LeadChar('0', s);
				}
				break;
			}
			case FieldType::DATE:
			case FieldType::REAL: {
				r = CFile->loadR(F, CRecPtr);
				if ((r == 0.0) && (Opt == CpOption::cpVar)) s = "";
				else if (F->field_type == FieldType::REAL) str(r, F->L, s);
				else {
					s = StrDate(r, F->Mask);
					if (Opt == CpOption::cpVar) s = '\'' + s + '\'';
				}
				break;
			}
			case FieldType::BOOL: {
				if (CFile->loadB(F, CRecPtr)) s = 'A';
				else s = 'N';
				break;
			}
			case FieldType::TEXT: {
				if (Opt == CpOption::cpVar) {
					x = CFile->loadLongS(F, CRecPtr);
					F2->WrLongStr(x, true);
					delete x;
				}
				break;
			}
			}

			if (F->field_type != FieldType::TEXT) F2->WrString(s);
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
		CRecPtr = CD->FD2->GetRecSpace();
#ifdef FandSQL
		if (CFile->IsSQLFile) {
			New(q, Init);
			q->OutpRewrite(Append);
		}
		else
#endif
			md = CFile->FF->RewriteFile(CD->Append);

		while (!(F1->eof) && (F1->ForwChar() != 0x1A)) {
			CFile->ZeroAllFlds(CRecPtr);
			CFile->ClearDeletedFlag(CRecPtr);
			VarFixImp(F1, CD->Opt1);
			F1->ForwChar(); //{set IsEOF at End}
#ifdef FandSQL
			if (CFile->IsSQLFile) q->PutRec();
			else
#endif
			{
				CFile->PutRec(CRecPtr);
				if (CD->Append && (CFile->FF->file_type == FileType::INDEX)) {
					CFile->FF->TryInsertAllIndexes(CFile->IRec, CRecPtr);
				}
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
		CFile->OldLockMode(md);
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
			int n = 0;
			BYTE* rec = nullptr;
			LinkLastRec(CD->HdFD, n, true, &rec);
			pstring s = CFile->loadOldS(CD->HdF, rec);
			int i = s.first('\r');
			if (i > 0) s[0] = i - 1;
			F2->WrString(s);
			F2->WrString("\r\n");
			CFile->ClearRecSpace(rec);
			delete[] rec; rec = nullptr;
		}
		CFile = CD->FD1;
		CRecPtr = CD->FD1->GetRecSpace();
		md = CFile->NewLockMode(RdMode);
		Scan = new XScan(CFile, CD->ViewKey, nullptr, true);
		Scan->Reset(nullptr, false, CRecPtr);
		RunMsgOn('C', Scan->NRecs);
		while (true) {
			Scan->GetRec(CRecPtr);
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
		CFile->ClearRecSpace(CRecPtr);
		CFile->OldLockMode(md);
	}
	if (F2 != nullptr && F2->Handle != nullptr) {
		if (LastExitCode != 0) {
			F2->ClearBuf();
		}
		delete F2;
	}
}

void Cpy(FILE* h, int sz, ThFile* F2)
{
	SeekH(h, 0);
	int i = 0;
	RunMsgOn('C', sz);
	while (i < sz) {
		WORD n;
		if (sz - i > F2->BufSize) n = F2->BufSize;
		else n = sz - i;
		i += n;
		ReadH(h, n, F2->Buf);
		TestCFileError(CFile);
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
		SaveAndCloseAllFiles();
		md = CFile->NewLockMode(RdMode);
		F2 = new ThFile(CD->Path2, CD->CatIRec2, InOutMode::_outp, 0, nullptr);
		int n = CFile->FF->XNRecs(CD->FD1->Keys);

		if (n == 0) {
			delete F2;
			F2 = nullptr;
		}
		else {
			Cpy(CFile->FF->Handle, CFile->FF->UsedFileSize(), F2);
		}

		if (CFile->FF->TF != nullptr) {
			F2->RewriteT();
			if (n == 0) {
				delete F2;
				F2 = nullptr;
			}
			else {
				Cpy(CFile->FF->TF->Handle, CFile->FF->TF->UsedFileSize(), F2);
			}
		}

		if (CD->WithX1) {
			F2->RewriteX();
			if (n == 0) {
				delete F2;
				F2 = nullptr;
			}
			else {
				Cpy(CFile->FF->XF->Handle, CFile->FF->XF->UsedFileSize(), F2);
			}
		}

		LastExitCode = 0;
	}

	catch (std::exception& e) {
		// TODO: log error
	}

	if ((F2 != nullptr) && (F2->Handle != nullptr)) {
		if (LastExitCode != 0) F2->ClearBuf();
		delete F2; F2 = nullptr;
		CFile->OldLockMode(md);
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
				std::string pKod = resFile.Get(LatToWinCp);
				ConvWinCp(F2.Buf, (unsigned char*)pKod.c_str(), F2.lBuf);
				break;
			}
			case 6: {
				std::string pKod = resFile.Get(KamToWinCp);
				ConvWinCp(F2.Buf, (unsigned char*)pKod.c_str(), F2.lBuf);
				break;
			}
			case 7: {
				std::string pKod = resFile.Get(WinCpToLat);
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
	SaveAndCloseAllFiles();
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

	delete F; F = nullptr;
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
	if (PD->IsBackup) s = RunLongStr(CFile, PD->bmMasks, CRecPtr);
	TzFile* F = new TzFile(PD->IsBackup, PD->NoCompress, PD->bmSubDir, PD->bmOverwr,
		PD->BrCatIRec, RunShortStr(CFile, PD->bmDir, CRecPtr));
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
	ReleaseStore(&p);
}


void CheckFile(FileD* FD)
{
	struct stPrfx { int NRecs; WORD RecLen; } Prfx;
	FILE* h = nullptr;
	std::string d, n, e;
	int fs = 0;

	TestMountVol(CPath[0]);

	const int fileStatus = fileExists(CPath);
	if (fileStatus != 0) {
		// path not exists
		if (fileStatus == 1) LastExitCode = 1;
		else LastExitCode = 2;
		return;
	}

	h = OpenH(CPath, _isOldFile, RdShared);
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
	if ((FD->FF->RecLen != Prfx.RecLen) || (Prfx.NRecs < 0) && (FD->FF->file_type != FileType::INDEX) ||
		((fs - FD->FF->FirstRecPos) / Prfx.RecLen < Prfx.NRecs) ||
		(Prfx.NRecs > 0) && (FD->FF->file_type == FileType::INDEX)) {
		LastExitCode = 3;
		return;
	}
	if (FD->FF->TF == nullptr) return;
	FSplit(CPath, d, n, e);
	if (EquUpCase(e, ".RDB")) e = ".TTT";
	else e[1] = 'T'; // .T__
	CPath = d + n + e;
	h = OpenH(CPath, _isOldFile, RdShared);
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
	if (CFile->loadT(F, CRecPtr) != 0) {
		CFile->saveT(F, CFile->loadT(F, CRecPtr) + (WORD(UserLicNrShow) & 0x7FFF), CRecPtr);
	}
}

void CopyH(FILE* H, pstring Nm)
{
	WORD n; FILE* h2;
	BYTE* buf = new BYTE[MaxLStrLen]; // GetStore(MaxLStrLen);
	CPath = Nm;
	CVol = "";
	h2 = OpenH(CPath, _isOverwriteFile, Exclusive);
	int sz = FileSizeH(H);
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
	FileD* cf;
	void* cr;
	auto wx = std::make_unique<wwmix>();
	Coding::SetPassword(Chpt, 1, "");
	Coding::SetPassword(Chpt, 2, "");
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
			CFile->FF->WrPrefixes();
			// TODO: SaveCache(0);
			std::string s = CRdb->RdbDir;
			AddBackSlash(s);
			s = s + Chpt->Name;
			CopyH(Chpt->FF->Handle, s + ".RD0x");
			CopyH(ChptTF->Handle, s + ".TT0x");
		}
		CodingCRdb(true);
		ChptTF->LicenseNr = WORD(UserLicNrShow) & 0x7FFF;
		cf = CFile;
		cr = CRecPtr;
		CFile = Chpt;
		CRecPtr = CFile->GetRecSpace();
		for (int i = 1; i <= Chpt->FF->NRecs; i++) {
			CFile->ReadRec(i, CRecPtr);
			AddLicNr(ChptOldTxt);
			AddLicNr(ChptTxt);
			CFile->WriteRec(i, CRecPtr);
		}
		ReleaseStore(&CRecPtr);
		CFile = cf;
		CRecPtr = cr;
		return result;
	}
	if (b) {
		Coding::SetPassword(Chpt, 1, wx->PassWord(true));
		if (Coding::HasPassword(Chpt, 1, "")) {
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

void XEncode(LongStr* S1, LongStr* S2)
{
	/*unsigned char Flags, Mask, RMask, t;
	unsigned short FlagPos, SequPos, SequLen, Len, NewLen, Displ;
	unsigned char* s1 = static_cast<unsigned char*>(S1);
	unsigned char* s2 = static_cast<unsigned char*>(S2) + 2;
	unsigned short bx = s1[0] + (s1[1] << 8);

	t = Timer.byte;
	t &= 0x03;
	RMask = 0x9c;
	RMask = (RMask << t) | (RMask >> (8 - t));

	Len = bx;
	Displ = 0;

	while (true) {
		FlagPos = s2 - static_cast<unsigned char*>(S2) - 1;
		Mask = 1;
		Flags = 0;

		while (Len != 0) {
			unsigned char* si = s1;
			unsigned char* di = s2;
			SequLen = 0;

			while (true) {
				unsigned short cx = si - s1;
				cx = Len < cx ? Len : cx;

				if (cx >= 2) {
					const auto cmpres = std::memcmp(si, di, cx);
					if (cmpres == 0) {
						si += cx;
						di += cx;
						Len -= cx;
						continue;
					}
					++cx;
				}
				si = s1 + (di - s2) + SequLen;
				unsigned short dx = si - s1;
				dx = Len < dx ? Len : dx;

				if (dx > SequLen && dx <= 255) {
					SequPos = *di;
					SequLen = dx;
				}
				break;
			}

			if (SequLen < 2) {
				unsigned char val = *si++;
				RMask = (RMask << 1) | (RMask >> 7);
				val ^= RMask;
				*di++ = val;
				--Len;
			}
			else {
				*s2++ = SequLen;
				*(unsigned short*)s2 = static_cast<unsigned short>(SequPos - s1);
				s2 += 2;
				Flags |= Mask;
				Len -= SequLen;
				s1 += SequLen;
			}

			Mask <<= 1;
			if (Mask == 0) {
				*reinterpret_cast<unsigned char*>(S2) = Flags;
				s2 = static_cast<unsigned char*>(S2) + FlagPos + 1;
				Mask = 1;
				Flags = 0;
			}
		}

		if (Mask == 1) {
			--s2;
		}
		else {
			*reinterpret_cast<unsigned char*>(S2) = Flags;
		}

		s2 = static_cast<unsigned char*>(S2) + 2;
		NewLen = s2 - static_cast<unsigned char*>(S2) + Displ - 3;
		*reinterpret_cast<unsigned short*>(S2) = NewLen;
		++s2;
		unsigned char* si = s2 + NewLen - 1;
		*si-- = t;
		*reinterpret_cast<unsigned short*>(si - 1) = Displ ^ 0xcccc;
		break;
	}*/
}
