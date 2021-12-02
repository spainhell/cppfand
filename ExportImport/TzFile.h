#pragma once
#include "TyFile.h"
#include "../cppfand/pstring.h"
#include "../cppfand/olongstr.h"

class TzFile : public TyFile
{
public:
    TzFile(bool BkUp, bool NoCompr, bool SubDirO, bool OverwrO, WORD Ir, pstring aDir);
    longint WBase, WPos;
    FILE* Handle;
    longint SpaceOnDisk, Size, OrigSize;
    std::string OldDir, Dir;
    bool SubDirOpt, OverwrOpt;
    void Close();
    longint GetWPtr();
    void StoreWPtr(longint Pos, longint N);
    longint StoreWStr(pstring s);
    longint ReadWPtr(longint Pos);
    pstring ReadWStr(longint& Pos);
    longint StoreDirD(pstring RDir);
    void SetDir(pstring RDir);
    void Get1Dir(StringList Msk, longint D, longint& DLast);
    void GetDirs(LongStr* Mask);
    void Reset();
    void Rewrite();
    void ReadBuf2() = 0;
    void WriteBuf2() = 0;
    void RdH(WORD H, bool Skip);
    void WrH(WORD H, longint Sz);
    void ProcFileList();
    void Backup(LongStr* aMsk);
    void Restore();
};

