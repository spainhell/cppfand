#include "oaccess.h"

#include "base.h"
#include "Compiler.h"
#include "../Common/FileD.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "obaseww.h"
#include "../fandio/FandTFile.h"
#include "../fandio/FandXFile.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"
#include "../Drivers/constants.h"

void OpenXWorkH()
{
	CVol = "";
	FileOpenMode m = _isOldNewFile;
	if (XWork.MaxPage == 0) m = _isOverwriteFile;
	CPath = FandWorkXName;
	XWork.Handle = OpenH(CPath, m, Exclusive);
	XWork.TestErr();
	if (FileSizeH(XWork.Handle) == 0) { XWork.FreeRoot = 0; XWork.MaxPage = 0; }
}

void OpenTWorkH()
{
	CVol = "";
	if (TWork.MaxPage == 0) {
		CPath = FandWorkTName;
		TWork.IsWork = true;
		TWork.Create(CPath);
	}
	else {
		CPath = FandWorkTName;
		CVol = "";
		TWork.Handle = OpenH(CPath, _isOldNewFile, Exclusive);
		TWork.TestErr();
	}
}

void SaveFiles()
{
	Logging* log = Logging::getInstance();

	if (!CacheExist()) return;

	// save catalog
	FileD* catalog_file = catalog->GetCatalogFile();
	log->log(loglevel::DEBUG, "SaveFiles() Catalog: 0x%p, %s ", catalog_file->FF->Handle, catalog_file->Name.c_str());
	catalog_file->FF->WrPrefixes();

	log->log(loglevel::DEBUG, "SaveFiles() calling ForAllFDs(::save)");
	ForAllFDs(ForAllFilesOperation::save);

	bool b = SaveCache(0, catalog_file->FF->Handle);
	// TODO: HANDLE: FlushHandles();

	if (!b) GoExit(MsgLine);
}

void CloseFANDFiles()
{
	RdbD* RD = CRdb;
	while (RD != nullptr) {
		FileD* f = RD->v_files[0];
		for (FileD* f : RD->v_files) {
			f->FF->ExLMode = f->FF->LMode;
			f->CloseFile();
		}
		RD = RD->ChainBack;
	}
	if (CRdb != nullptr) {
		catalog->GetCatalogFile()->CloseFile();
	}
	HelpFD->CloseFile();
	CloseH(&TWork.Handle);
	CloseH(&XWork.Handle);
}

void OpenFANDFiles()
{
	RdbD* RD = nullptr;
	LockMode md = NullMode;

	OpenXWorkH();
	OpenTWorkH();
	HelpFD->OpenF(CPath, RdOnly);
	if (CRdb == nullptr) return;

	catalog->GetCatalogFile()->OpenF(CPath, Exclusive);
	RD = CRdb;

	while (RD != nullptr) {
		if (IsTestRun) {
			RD->v_files[0]->OpenF(CPath, Exclusive);
		}
		else {
			RD->v_files[0]->OpenF(CPath, RdOnly);
		}

		auto it0 = RD->v_files.begin();
		++it0;

		while (it0 != RD->v_files.end()) {
			if ((*it0)->FF->ExLMode != NullMode) {
				(*it0)->OpenF(CPath, Shared);
				md = (*it0)->NewLockMode((*it0)->FF->ExLMode);
			}
			++it0;
		}

		RD = RD->ChainBack;
	}

}

//void CloseFilesAfter(FileD* first_for_close, std::vector<FileD*>& v_files)
//{
//	// find first_for_close in v_files
//	std::ranges::borrowed_iterator_t<std::vector<FileD*>&> it0
//		= std::ranges::find(v_files, first_for_close);
//
//	while (it0 != v_files.end()) {
//		(*it0)->CloseFile();
//		++it0;
//	}
//}

bool ActiveRdbOnDrive(WORD D)
{
	auto result = true;
	RdbD* R = CRdb;
	while (R != nullptr) {
		if (R->v_files[0]->FF->Drive == D) return result;
		R = R->ChainBack;
	}
	result = false;
	return result;
}

void CloseFilesOnDrive(WORD drive)
{
	RdbD* R = CRdb;

	while (R != nullptr) {
		for (FileD* f : R->v_files) {
			if (f->FF->Drive == drive) {
				f->CloseFile();
			}
		}
		R = R->ChainBack;
	}
}

WORD TestMountVol(char DriveC)
{
	WORD D = 0, i = 0;
	std::string Vol;
	pstring Drive(1);
	Drive = "A";

	if (IsNetCVol()) return 0;

	D = toupper(DriveC) - '@';

	if (D >= FloppyDrives) {
		if (!CDir.empty() && toupper(CDir[0]) == spec.CPMdrive) {
			D = FloppyDrives;
		}
		else {
			return 0;
		}
	}

	const std::string MountedVolD = MountedVol[D - 1];

	if (CVol.empty() || EquUpCase(MountedVolD, CVol)) {
		// do nothing
	}
	else {

		Drive[1] = DriveC;
		if (ActiveRdbOnDrive(D)) {
			SetMsgPar(Drive, CVol, MountedVol[D - 1]);
			RunError(812);
		}
		Vol = CVol;
		CloseFilesOnDrive(D);
		CVol = Vol;

		while (true) {
			F10SpecKey = __ESC;
			SetMsgPar(Drive, CVol);
			WrLLF10Msg(808);
			if (Event.Pressed.KeyCombination() == __ESC) {
				if (PromptYN(21)) {
					GoExit(MsgLine);
				}
			}
			else {
				continue;
			}

			switch (DosError()) {
			case 18: {
				/*label missing*/
				WrLLF10Msg(809);
				continue;
				break;
			}
			case 0: {
				break;
			}
			default: {
				WrLLF10Msg(810);
				continue;
				break;
			}
			}

			break;
		}
		MountedVol[D - 1] = CVol;
	}

	return D;
}

void ReleaseDrive(WORD D)
{
	pstring Drive(1);
	if (MountedVol[D - 1].empty()) return;

	if (D == FloppyDrives) Drive = spec.CPMdrive;
	else Drive = (char)(D + '@');

	if (ActiveRdbOnDrive(D)) {
		SetMsgPar(Drive);
		RunError(813);
	}

	CloseFilesOnDrive(D);
	SetMsgPar(MountedVol[D - 1], Drive);
	WrLLF10Msg(818);
	MountedVol[D - 1] = "";
}

bool SetContextDir(FileD* file_d, std::string& dir, bool& isRdb)
{
	RdbD* R = CRdb;
	isRdb = false;

	while (R != nullptr) {
		if (!R->v_files.empty()) {
			FileD* F = R->v_files[0];
			if ((file_d == F) && (file_d->CatIRec != 0)) {
				dir = R->RdbDir;
				isRdb = true;
				return true;
			}
		}

		for (FileD* f : R->v_files) {
			if (file_d == f) {
				if ((file_d == R->help_file) || (file_d->FileType == DataFileType::FandFile && file_d->FF->file_type == FandFileType::RDB)) {
					//.RDB
					dir = R->RdbDir;
				}
				else {
					dir = R->DataDir;
				}
				return true;
			}
		}

		R = R->ChainBack;
	}

	return false;
}

void SetTxtPathVol(std::string& Path, int CatIRec)
{
	if (CatIRec != 0) {
		CVol = catalog->GetVolume(CatIRec);
		CPath = FExpand(catalog->GetPathName(CatIRec));
		FSplit(CPath, CDir, CName, CExt);
	}
	else {
		CPath = FExpand(Path);
		CVol = "";
	}
}

void TestDelErr(std::string& P)
{
	if (HandleError != 0) {
		SetMsgPar(P);
		RunError(827);
	}
}
