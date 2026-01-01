#pragma once
#include "access.h"
#include "models/FrmlElem.h"
#include "../Common/pstring.h"

#ifdef FandGraph
#include "graph.h"
#endif
#ifdef FandSQL
#include "channel.h"
#endif

double Owned(FrmlElem* Bool, FrmlElem* Sum, LinkD* LD, Record* record);
pstring LeadChar(char C, pstring S);
bool RunBool(FileD* file_d, FrmlElem* X, Record* record);
bool InReal(FileD* file_d, FrmlElemIn* frml, Record* record);
bool LexInStr(std::string& S, FrmlElemIn* X);
bool InStr(std::string& S, FrmlElemIn* X);
bool RunModulo(FileD* file_d, FrmlElemFunction* X, Record* record);
bool RunEquMask(FileD* file_d, FrmlElemFunction* X, Record* record);
double RunReal(FileD* file_d, FrmlElem* X, Record* record);
int RunInt(FileD* file_d, FrmlElem* X, Record* record);
void AssgnFrml(Record* record, FieldDescr* field_d, FrmlElem* X, bool add);
void LVAssignFrml(FileD* file_d, LocVar* LV, bool Add, FrmlElem* X, Record* record);

void RunWFrml(FileD* file_d, WRectFrml& X, uint8_t WFlags, WRect& W, Record* record);
WORD RunWordImpl(FileD* file_d, FrmlElem* Z, WORD Impl, Record* record);
bool FieldInList(FieldDescr* F, std::vector<FieldDescr*>& FL);
XKey* GetFromKey(LinkD* LD);
FrmlElem* RunEvalFrml(FileD* file_d, FrmlElem* Z, Record* record);
std::string RunString(FileD* file_d, FrmlElem* X, Record* record);
void StrMask(double R, pstring& Mask);
std::string RunSelectStr(FileD* file_d, FrmlElemFunction* Z, Record* record);
void LowCase(std::string& text);
double RoundReal(double RR, short M);
void AccRecNoProc(FrmlElem14* X, WORD Msg, Record* record);
void GetRecNoXString(FileD* file_d, FrmlElemRecNo* Z, XString& X, Record* record);
double RunRealStr(FileD* file_d, FrmlElem* X, Record* record);

WORD TypeDay(double R);
double AddWDays(double R, short N, WORD d);
double DifWDays(double R1, double R2, WORD d);
double LastUpdate(const std::string& path);

int GetFileSize();

std::string GetTxt(FileD* file_d, FrmlElem16* Z, Record* record);
