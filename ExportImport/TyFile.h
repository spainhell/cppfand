#pragma once
#include <string>
#include "TcFile.h"

class TyFile : public TcFile
{
public:
    TyFile();
    BYTE Drive = '\0';
    char DrvNm = '\0';
	std::string Vol;
    std::string Path;
    bool IsBackup = false;
    bool Floppy = false;
	bool Continued = false;
    void MountVol(bool IsFirst);
};
