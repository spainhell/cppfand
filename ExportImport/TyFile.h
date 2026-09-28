#pragma once
#include <string>
#include "TcFile.h"

typedef void* HANDLE;

// archive file of BACKUP/RESTORE and BACKUPM/RESTOREM (optionally LZSS compressed)
class TyFile : public TcFile
{
public:
    TyFile(bool compress);
    ~TyFile() override;

    char drive_letter = '\0';
    std::string Vol;
    std::string Path;       // path of the archive file
    HANDLE Handle = nullptr;  // handle of the archive file
    int Size = 0, OrigSize = 0;
    bool IsBackup = false;
    bool Floppy = false;
    bool Continued = false;

    void MountVol(bool IsFirst);
    // opens the archive file Path for reading
    void Reset();
    // creates the archive file Path
    void Rewrite();
    void CloseArchive();
    void TestErr();
    void ReadBuf2() override;
    void WriteBuf2() override;
};
