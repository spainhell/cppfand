#pragma once
#include "TyFile.h"
#include "../cppfand/pstring.h"
#include "../cppfand/olongstr.h"

class TzFile : public TyFile
{
public:
    TzFile(bool BkUp, bool NoCompr, bool SubDirO, bool OverwrO, WORD Ir, pstring aDir);
    int WBase, WPos;
    FILE* Handle;
    int SpaceOnDisk, Size, OrigSize;
    std::string OldDir, Dir;
    bool SubDirOpt, OverwrOpt;
    void Close();
    int GetWPtr();
    void StoreWPtr(int Pos, int N);
    int StoreWStr(pstring s);
    int ReadWPtr(int Pos);
    pstring ReadWStr(int& Pos);
    int StoreDirD(std::string RDir);
    void SetDir(std::string RDir);
    void Get1Dir(StringList Msk, int D, int& DLast);
    void GetDirs(LongStr* Mask);
    void Reset();
    void Rewrite();
    //void ReadBuf2() override;
    //void WriteBuf2() override;
    void RdH(FILE* H, bool Skip);
    void WrH(FILE* H, int Sz);
    void ProcFileList();
    void Backup(LongStr* aMsk);
    void Restore();
};

