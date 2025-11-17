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

double Owned(FileD* file_d, FrmlElem* Bool, FrmlElem* Sum, LinkD* LD, void* record);
LongStr* CopyToLongStr(pstring& SS);
pstring LeadChar(char C, pstring S);
bool RunBool(FileD* file_d, FrmlElem* X, void* record);
bool InReal(FileD* file_d, FrmlElemIn* frml, void* record);
bool LexInStr(std::string& S, FrmlElemIn* X);
bool InStr(std::string& S, FrmlElemIn* X);
bool RunModulo(FileD* file_d, FrmlElemFunction* X, void* record);
bool RunEquMask(FileD* file_d, FrmlElemFunction* X, void* record);
double RunReal(FileD* file_d, FrmlElem* X, void* record);
int RunInt(FileD* file_d, FrmlElem* X, void* record);
bool CanCopyT(FileD* file_d, FieldDescr* F, FrmlElem* Z, FandTFile** TF02, FileD** TFD02, int& TF02Pos, void* record);
bool TryCopyT(FieldDescr* F, FandTFile* TF, int& pos, FrmlElem* Z);
void AssgnFrml(FileD* file_d, void* record, FieldDescr* field_d, FrmlElem* X, bool deleteT, bool add);
void LVAssignFrml(FileD* file_d, LocVar* LV, bool Add, FrmlElem* X, void* record);
std::string DecodeFieldRSB(FieldDescr* F, WORD LWw, double R, std::string& T, bool B);
std::string DecodeField(FileD* file_d, FieldDescr* F, WORD LWw, void* record);
void RunWFrml(FileD* file_d, WRectFrml& X, uint8_t WFlags, WRect& W, void* record);
WORD RunWordImpl(FileD* file_d, FrmlElem* Z, WORD Impl, void* record);
bool FieldInList(FieldDescr* F, std::vector<FieldDescr*>& FL);
XKey* GetFromKey(LinkD* LD);
FrmlElem* RunEvalFrml(FileD* file_d, FrmlElem* Z, void* record);
std::string RunString(FileD* file_d, FrmlElem* X, void* record);
//std::string RunShortStr(FileD* file_d, FrmlElem* X, void* record);
void AddToLongStr(LongStr* S, void* P, WORD L);
void StrMask(double R, pstring& Mask);
std::string RunSelectStr(FileD* file_d, FrmlElemFunction* Z, void* record);
void LowCase(std::string& text);
double RoundReal(double RR, short M);
void AccRecNoProc(FrmlElem14* X, WORD Msg, uint8_t** record);
void GetRecNoXString(FileD* file_d, FrmlElemRecNo* Z, XString& X, void* record);
double RunRealStr(FileD* file_d, FrmlElem* X, void* record);

WORD TypeDay(double R);
double AddWDays(double R, short N, WORD d);
double DifWDays(double R1, double R2, WORD d);
double LastUpdate(const std::string& path);

int GetFileSize();

std::string GetTxt(FileD* file_d, FrmlElem16* Z, void* record);
