#pragma once

#include <cstdio>
#include <string>
#include "TyFile.h"
#include "../cppfand/constants.h"

class TbFile : TyFile
{
public:
	TbFile(bool noCompress);
	~TbFile();

	FILE* Handle = nullptr;
	std::string Dir;
	std::string FName;
	std::string Ext;

    void TestErr();
    void Reset();
    void Rewrite();
    void ReadBuf2();
    void WriteBuf2();
    void BackupH();
    void RestoreH();
    void BackupHFD(WORD h);
    void RestoreHFD(WORD h);
    void BackupFD();
    void RestoreFD();
	void Backup(bool isBackup, WORD Ir);
};

