#pragma once
#include <string>

#include "TcFile.h"
class TyFile : public TcFile
{
public:
    TyFile();
    BYTE Drive;
    char DrvNm;
	std::string Vol;
    std::string Path;
    bool IsBackup, Floppy, Continued;
    void MountVol(bool IsFirst);
};

