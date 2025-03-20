#pragma once
#include <map>
#include <vector>

#include "TyFile.h"
#include "../Common/pstring.h"

typedef void* HANDLE;

class TzFile : public TyFile
{
public:
    TzFile(bool BkUp, bool compress, bool SubDirO, bool OverwrO, int Ir, std::string& aDir);
    int WBase, WPos;
    HANDLE Handle;
    int SpaceOnDisk, Size, OrigSize;
    std::string OldDir, Dir;
    bool SubDirOpt, OverwrOpt;
    
    void Close();
    int GetWPtr();
    void StoreWPtr(int Pos, int N);
    int StoreWStr(const std::string& s);
    int ReadWPtr(int Pos);
    std::string ReadWStr(int& Pos);
    int StoreDirD(std::string RDir);
    void SetDir(std::string RDir);
    void Get1Dir(int D, int& DLast);
	void GetDirs();
    void Reset();
    void Rewrite();
    //void ReadBuf2() override;
    void WriteBuf2() override;
    void RdH(HANDLE H, bool Skip);
    void WrH(HANDLE src_file, uint32_t file_size);
    void ProcFileList();
    void Backup(std::string& mask);
    void Restore();
    void ParseMask(const std::string& mask);

private:
    std::vector<std::string> v_masks_;
    std::map<std::string, std::vector<std::string>> paths_;
};
