#pragma once
#include "AddD.h"
#include "../Common/pstring.h"
#include "rdrun.h"
#include "EditOpt.h"
#include "FileD.h"

extern bool HasTT;
FieldDescr* RdFieldDescr(std::string name, bool Stored); // r25
ChkD* RdChkD(WORD Low);
void RdChkDChain(std::vector<ChkD*>& C);
void RdChkDsFromPos(FileD* FD, std::vector<ChkD*>& C); // r98
void RdBegViewDcl(EditOpt* EO); // r110
void RdByteList(pstring* s);
void RdByteListInStore();
bool RdUserView(FileD* file_d, std::string ViewName, EditOpt* EO);

extern bool isSql;

void TestDupl(FileD* FD);
void RdFieldDList(FileD* file_d, bool Stored);
[[nodiscard]] FileD* RdFileD(std::string FileName, FileType FDTyp, std::string Ext);
void RdKeyD(FileD* file_d);
void CheckDuplAlias(FileD* file_d, pstring name);
void LookForK(FileD* file_d, pstring* Name, FileD* F);
XKey* RdFileOrAlias1(FileD* F);
void RdFileOrAlias(FileD* file_d, FileD** FD, XKey** KD);
void TestDepend();
void RdImpl(FileD* file_d, std::vector<ImplD*>& IDRoot);
void RdKumul();
void RdRoleField(AddD* AD);
void RdImper(AddD* AD);
void RdAssign(AddD* AD);
void SetHCatTyp(FileD* file_d, FileType FDTyp);
void GetTFileD(FileD* file_d, FileType file_type);
void GetXFileD(FileD* file_d);
CompInpD* OrigInp();
