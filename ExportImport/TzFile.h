#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "TyFile.h"

// BACKUPM/RESTOREM: files of a directory (optionally with subdirectories) in one archive file.
// Archive: <4 B size><table of directories and file names>, then for every file <4 B size><content>
// (the whole stream optionally LZSS compressed).
// Table (offsets relative to its start): directory = <4 B next dir><4 B 1st file name><4 B count>
// <pascal string: relative dir with '\'>, file names = pascal strings.
class TzFile : public TyFile
{
public:
    TzFile(bool BkUp, bool compress, bool SubDirO, bool OverwrO, int Ir, const std::string& aDir);

    std::string Dir;
    bool SubDirOpt, OverwrOpt;

    void Close();
    void Backup(const std::string& mask);
    void Restore();

private:
    std::vector<uint8_t> table_; // PC-FAND keeps it in the work file
    std::vector<std::string> masks_;

    int32_t GetWPtr();
    void StoreWPtr(int32_t Pos, int32_t N);
    int32_t StoreWStr(const std::string& s);
    int32_t ReadWPtr(int32_t Pos) const;
    std::string ReadWStr(int32_t& Pos) const;
    int32_t StoreDirD(const std::string& RDir);
    std::string SetDir(const std::string& RDir);
    bool MatchesMask(const std::string& name) const;
    void Get1Dir(int32_t D, int32_t& DLast);
    void GetDirs(const std::string& mask);
    void RdH(const std::function<void(const uint8_t*, size_t)>& write);
    void WrH(uint32_t Sz, const std::function<void(uint8_t*, size_t)>& read);
    void ProcFileList();
};
