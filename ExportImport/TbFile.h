#pragma once

#include <string>
#include "TyFile.h"

class FileD;

// BACKUP/RESTORE: every file of the archive (catalog records with the archive number)
// is saved in its own archive file <archive dir>\<file name>.0nn (text file .Tnn)
class TbFile : public TyFile
{
public:
	TbFile(bool compress);

	std::string Dir;
	std::string FName;
	std::string Ext;

	void Backup(bool isBackup, WORD Ir);

private:
	void ResetF();
	void RewriteF();
	void NextExt();
	void BackupH();
	void RestoreH();
	void BackupHFD(HANDLE h);
	void RestoreHFD(HANDLE h);
	void BackupFD(FileD* file_d);
	void RestoreFD(FileD* file_d);
};
