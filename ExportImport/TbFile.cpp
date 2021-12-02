#include "TbFile.h"
#include "../cppfand/compile.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obaseww.h"

FileD* x_FD = nullptr;
WORD x_I = 0;

void FindFDforI() {
	if ((x_FD == nullptr) && (CFile->CatIRec == x_I)) {
		x_FD = CFile;
	}
}

TbFile::TbFile(bool noCompress) : TyFile()
{
}

void TbFile::Backup(bool isBackup, WORD Ir)
{
	std::string d;
	std::string n;
	std::string e;

	IsBackup = IsBackup;
	SaveFiles();
	std::string ArNr = RdCatField(Ir, CatArchiv);
	Vol = RdCatField(Ir, CatVolume);
	Path = RdCatField(Ir, CatPathName);
	size_t j = Path.find(' ');
	std::string numbers;
	if (j != std::string::npos) {
		numbers = Path.substr(j, 255);
	}
	Path = FExpand(Path);
	FSplit(Path, Dir, n, e);
	Ext = ".000";
	DrvNm = Dir[0];
	MountVol(true);
	SetInpStr(numbers);
	RdLex();
label1:
	for (WORD i = 1; i <= CatFD->NRecs; i++) {
		if (!SEquUpcase(RdCatField(i, CatRdbName), "ARCHIVES")) {
			if (RdCatField(i, CatArchiv) == ArNr) {
				FSplit(RdCatField(i, CatPathName), d, FName, e);
				Ext[1] = '0';
				switch (Ext[3]) {
				case '9': {
					Ext[3] = 'A';
					break;
				}
				case 'Z': {
					Ext[3] = '0';
					if (Ext[2] == '9') Ext[2] = 'A';
					else Ext[2]++;
					break;
				}
				default: {
					Ext[3]++;
					break;
				}
				}
				x_FD = nullptr;
				x_I = i;
				ForAllFDs(FindFDforI);
				CFile = x_FD;
				if (CFile != nullptr) {
					FILE* h = CFile->Handle;
					if (IsBackup) BackupFD();
					else RestoreFD();
					/*if (h == 0xff)*/ CloseFile();
				}
				else {
					CPath = FExpand(RdCatField(i, CatPathName));
					CVol = RdCatField(i, CatVolume);
					TestMountVol(CPath[1]);
					if (IsBackup) BackupH();
					else RestoreH();
				}
			}
		}
		while (!(Lexem == 0x1A || Lexem == _number)) RdLex();
		if (Lexem == _number) {
			if (LexWord.length() == 1) {
				std::string lexword = LexWord;
				ArNr = "0" + lexword;
			}
			else {
				ArNr = LexWord;
			}
			RdLex();
			goto label1;
		}
	}
}

void TbFile::BackupH()
{
	FILE* h = OpenH(_isoldfile, RdOnly);
	if (HandleError == 2) {
		Rewrite();
		InitBufOutp();
	}
	else {
		TestCPathError();
		Rewrite();
		InitBufOutp();
		longint sz = FileSizeH(h);
		RunMsgOn('C', sz);
		longint i = 0;
		while (i < sz) {
			WORD n;
			if (sz - i > BufSize) n = BufSize;
			else n = sz - i;
			i += n;
			ReadH(h, n, Buf);
			lBuf = n;
			WriteBuf(false);
			RunMsgN(i);
		}
		CloseH(&h);
		RunMsgOff();
	}
	WriteBuf(true);
	CloseH(&Handle);
}
