#pragma once
#include "editor.h"

extern bool HasTT;
FieldDPtr RdFldDescr(pstring Name, bool Stored); // r25
ChkD* RdChkD(WORD Low);
void RdChkDChain(ChkD** CRoot);
void RdChkDsFromPos(FileD* FD, ChkD* C); // r98
void RdBegViewDcl(EditOpt* EO); // r110
void RdByteList(pstring* s);
void RdByteListInStore();
bool RdUserView(pstring ViewName, EditOpt* EO);

extern bool issql;

void TestUserView();
void TestDupl(FileD* FD);
void RdFieldDList(bool Stored);
void* RdFileD(std::string FileName, char FDTyp, std::string Ext); // r220
void RdKeyD();
void CheckDuplAlias(pstring Name);
void LookForK(pstring* Name, FileD* F);
KeyD* RdFileOrAlias1(FileD* F);
void RdFileOrAlias(FileD** FD, KeyD** KD);
void SetLDIndexRoot(LinkD* L, LinkD* L2);
void TestDepend();
void RdImpl(ImplD** IDRoot);
void RdKumul();
void RdRoleField(AddD* AD);
void RdImper(AddD* AD);
void RdAssign(AddD* AD);
void SetHCatTyp(char FDTyp);
void GetTFileD(char FDTyp);
void GetXFileD();
void CallRdFDSegment(FileD* FD);
CompInpD* OrigInp();
