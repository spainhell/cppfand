#pragma once
#include <map>
#include <vector>

#include "TyFile.h"
#include "../Common/pstring.h"

typedef void* HANDLE;

class TzFile : public TyFile
{
public:
    TzFile(bool BkUp, bool NoCompr, bool SubDirO, bool OverwrO, int Ir, pstring aDir);
    int WBase, WPos;
    HANDLE Handle;
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
    //void Get1Dir(std::vector<std::string>& Msk, int D, int& DLast);
    void GetFilesInDir(const std::string& main_dir, const std::string& sub_dir);
    void WriteSubdirFiles(const std::vector<std::string>& files, uint32_t& index);
    void WriteSubdirRecord(const std::string& sub_dir, uint32_t& index, uint32_t next_rec_address, uint32_t file_desr_address, uint32_t files_count);
	void GetDirs();
    void Reset();
    void Rewrite();
    //void ReadBuf2() override;
    void WriteBuf2() override;
    void RdH(HANDLE H, bool Skip);
    void WrH(HANDLE src_file, uint32_t file_size);
    void ProcFileList();
    void ParseMask(const std::string& mask);
    void Backup(std::string& mask);
    void Restore();

private:
    std::vector<std::string> v_masks_;
    std::map<std::string, std::vector<std::string>> paths_;
};

