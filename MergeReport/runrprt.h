#pragma once
#include "../cppfand/access-structs.h"
#include "../cppfand/rdrun.h"

struct InpD;
struct BlkD;
class KeyFldD;

struct TTD : public Chained<TTD>
{
    StringList SL = nullptr;
    WORD Col = 0, Width = 0, Ln = 0;
};

struct YRec
{
    const char* P = nullptr;
    WORD I = 0, Ln = 0, TLn = 0, Sz = 0;
    BlkD* Blk = nullptr;
    bool ChkPg = false;
    TTD* TD = nullptr;
};

extern WORD PrintDH;
extern YRec Y;
extern bool FrstBlk, NoFF, WasFF2, SetPage, WasOutput;
extern short LineLenLst, PageNo, PgeSize;
extern void* Store2Ptr;

extern int RecCount;
extern WORD NEof;
extern InpD* MinID;
extern bool FirstLines, WasDot;
extern int NLinesOutp;

void RunReport(RprtOpt* RO);
