#pragma once
#include <set>

#include "Additive.h"
#include "../Common/pstring.h"
#include "rdrun.h"
#include "EditOpt.h"
#include "../Common/FileD.h"

extern bool HasTT;
FieldDescr* RdFieldDescr(std::string name, bool Stored); // r25
LogicControl* ReadLogicControl(WORD Low);
void RdChkDChain(std::vector<LogicControl*>& C);
void RdChkDsFromPos(FileD* FD, std::vector<LogicControl*>& C); // r98
void RdBegViewDcl(EditOpt* EO); // r110
std::string RdByteList();
[[nodiscard]] std::set<uint16_t> RdAccRights();
bool RdUserView(FileD* file_d, std::string ViewName, EditOpt* EO);

extern bool isSql;

void TestDupl(FileD* FD);
void RdFieldDList(FileD* file_d, bool Stored);
[[nodiscard]] FileD* RdFileD(std::string FileName, DataFileType data_file_type, FandFileType fand_file_type, std::string Ext);
void RdKeyD(FileD* file_d);
void CheckDuplAlias(FileD* file_d, pstring name);
void LookForK(FileD* file_d, pstring* Name, FileD* F);
XKey* RdFileOrAlias1(FileD* F);
void RdFileOrAlias(FileD* file_d, FileD** FD, XKey** KD);
void TestDepend();
void ReadImplicit(FileD* file_d, std::vector<Implicit*>& IDRoot);
void RdKumul();
void RdRoleField(Additive* AD);
void RdImper(Additive* AD);
void RdAssign(Additive* AD);
CompInpD* OrigInp();
