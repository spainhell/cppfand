#include "TbFile.h"
#include "../cppfand/compile.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obaseww.h"
#include "../Common/compare.h"

FileD* x_FD = nullptr;
WORD x_I = 0;

void FindFDforI() {
	if ((x_FD == nullptr) && (CFile->CatIRec == x_I)) {
		x_FD = CFile;
	}
}

TbFile::TbFile(bool noCompress) : TyFile()
{
	Size = 0;
	OrigSize = 0;
	SpaceOnDisk = 0;
}

TbFile::~TbFile()
{
}

void TbFile::TestErr()
{
}

void TbFile::Reset()
{
}

void TbFile::Rewrite()
{
}

void TbFile::ReadBuf2()
{
	TyFile::ReadBuf2();
}

void TbFile::WriteBuf2()
{
	TyFile::WriteBuf2();
}

void TbFile::Backup(bool isBackup, WORD Ir)
{
	std::string d;
	std::string n;
	std::string e;

	IsBackup = isBackup;
	SaveFiles();
	std::string ArNr = CatFD->ReadField(Ir, CatFD->CatArchiv);
	Vol = CatFD->ReadField(Ir, CatFD->cat_volume_);
	Path = CatFD->ReadField(Ir, CatFD->cat_path_name_);
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
	for (WORD i = 1; i <= CatFD->GetCatalogFile()->FF->NRecs; i++) {
		if (!EquUpCase(CatFD->ReadField(i, CatFD->cat_rdb_name_), "ARCHIVES")) {
			if (CatFD->ReadField(i, CatFD->CatArchiv) == ArNr) {
				FSplit(CatFD->ReadField(i, CatFD->cat_path_name_), d, FName, e);
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
					FILE* h = CFile->FF->Handle;
					if (IsBackup) BackupFD();
					else RestoreFD();
					/*if (h == 0xff)*/ CloseFile();
				}
				else {
					CPath = FExpand(CatFD->ReadField(i, CatFD->cat_path_name_));
					CVol = CatFD->ReadField(i, CatFD->cat_volume_);
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
	FILE* h = OpenH(CPath, _isoldfile, RdOnly);
	if (HandleError == 2) {
		Rewrite();
		InitBufOutp();
	}
	else {
		TestCPathError();
		Rewrite();
		InitBufOutp();
		int sz = FileSizeH(h);
		RunMsgOn('C', sz);
		int i = 0;
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

void TbFile::RestoreH()
{
}

void TbFile::BackupHFD(WORD h)
{
}

void TbFile::RestoreHFD(WORD h)
{
}

void TbFile::BackupFD()
{
}

void TbFile::RestoreFD()
{
}
