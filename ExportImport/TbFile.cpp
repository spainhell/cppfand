#include "TbFile.h"
#include "../Core/compile.h"
#include "../Core/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/oaccess.h"
#include "../Core/RunMessage.h"
#include "../Common/compare.h"

FileD* x_FD = nullptr;
WORD x_I = 0;

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
	SaveAndCloseAllFiles();
	std::string ArNr = CatFD->GetArchive(Ir);
	Vol = CatFD->GetVolume(Ir);
	Path = CatFD->GetPathName(Ir);
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
		if (!EquUpCase(CatFD->GetRdbName(i), "ARCHIVES")) {
			if (CatFD->GetArchive(i) == ArNr) {
				FSplit(CatFD->GetPathName(i), d, FName, e);
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
				ForAllFDs(ForAllFilesOperation::find_fd_for_i, &x_FD, x_I);
				CFile = x_FD;
				if (CFile != nullptr) {
					HANDLE h = CFile->FF->Handle;
					if (IsBackup) BackupFD();
					else RestoreFD();
					/*if (h == 0xff)*/ CFile->CloseFile();
				}
				else {
					CPath = FExpand(CatFD->GetPathName(i));
					CVol = CatFD->GetVolume(i);
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
	HANDLE h = OpenH(CPath, _isOldFile, RdOnly);
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
