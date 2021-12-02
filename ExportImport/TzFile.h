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
    longint StoreDirD(std::string RDir);
    void SetDir(std::string RDir);
    void Get1Dir(StringList Msk, longint D, longint& DLast);
    void GetDirs(LongStr* Mask);
    void Reset();
    void Rewrite();
    //void ReadBuf2() override;
    //void WriteBuf2() override;
    void RdH(FILE* H, bool Skip);
    void WrH(FILE* H, longint Sz);
    void ProcFileList();
    void Backup(LongStr* aMsk);
    void Restore();
};

