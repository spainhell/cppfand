#pragma once
#include "AddD.h"
#include "../Common/pstring.h"
#include "rdrun.h"
#include "EditOpt.h"
#include "FileD.h"

extern bool HasTT;
FieldDescr* RdFieldDescr(std::string name, bool Stored); // r25
ChkD* RdChkD(WORD Low);
void RdChkDChain(ChkD** CRoot);
void RdChkDsFromPos(FileD* FD, ChkD* C); // r98
void RdBegViewDcl(EditOpt* EO); // r110
void RdByteList(pstring* s);
void RdByteListInStore();
bool RdUserView(FileD* file_d, std::string ViewName, EditOpt* EO);

extern bool issql;

void TestUserView();
void TestDupl(FileD* FD);
void RdFieldDList(FileD* file_d, bool Stored);
FileD* RdFileD(FileD* file_d, std::string FileName, FileType FDTyp, std::string Ext);
void RdKeyD();
void CheckDuplAlias(pstring Name);
void LookForK(pstring* Name, FileD* F);
XKey* RdFileOrAlias1(FileD* F);
void RdFileOrAlias(FileD** FD, XKey** KD);
void TestDepend();
void RdImpl(ImplD** IDRoot);
void RdKumul();
void RdRoleField(AddD* AD);
void RdImper(AddD* AD);
void RdAssign(AddD* AD);
void SetHCatTyp(FileD* file_d, FileType FDTyp);
void GetTFileD(FileD* file_d, FileType file_type);
void GetXFileD(FileD* file_d);
CompInpD* OrigInp();
