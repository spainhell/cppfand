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
LongStr* CopyToLongStr(std::string& SS);
pstring LeadChar(char C, pstring S);
bool RunBool(FileD* file_d, FrmlElem* X, void* record);
bool InReal(FileD* file_d, FrmlElemIn* frml, void* record);
bool LexInStr(std::string& S, FrmlElemIn* X);
bool InStr(LongStr* S, FrmlElemIn* X);
bool InStr(std::string& S, FrmlElemIn* X);
bool RunModulo(FileD* file_d, FrmlElem1* X, void* record);
bool RunEquMask(FileD* file_d, FrmlElem0* X, void* record);
double RunReal(FileD* file_d, FrmlElem* X, void* record);
int RunInt(FileD* file_d, FrmlElem* X, void* record);
bool CanCopyT(FileD* file_d, FieldDescr* F, FrmlElem* Z, FandTFile** TF02, FileD** TFD02, int& TF02Pos, void* record);
bool TryCopyT(FieldDescr* F, FandTFile* TF, int& pos, FrmlElem1* Z);
void AssgnFrml(FileD* file_d, void* record, FieldDescr* F, FrmlElem* X, bool Delete, bool Add);
void LVAssignFrml(FileD* file_d, LocVar* LV, bool Add, FrmlElem* X, void* record);
std::string DecodeFieldRSB(FieldDescr* F, WORD LWw, double R, std::string& T, bool B);
std::string DecodeField(FileD* file_d, FieldDescr* F, WORD LWw, void* record);
void RunWFrml(FileD* file_d, WRectFrml& X, BYTE WFlags, WRect& W, void* record);
WORD RunWordImpl(FileD* file_d, FrmlElem* Z, WORD Impl, void* record);
bool FieldInList(FieldDescr* F, FieldListEl* FL);
bool FieldInList(FieldDescr* F, std::vector<FieldDescr*>& FL);
XKey* GetFromKey(LinkD* LD);
FrmlElem* RunEvalFrml(FileD* file_d, FrmlElem* Z, void* record);
LongStr* RunLongStr(FileD* file_d, FrmlElem* X, void* record);
std::string RunStdStr(FileD* file_d, FrmlElem* X, void* record);
std::string RunShortStr(FileD* file_d, FrmlElem* X, void* record);
void CopyLongStr(LongStr* S, WORD From, WORD Number);
void AddToLongStr(LongStr* S, void* P, WORD L);
void StrMask(double R, pstring& Mask);
LongStr* RunS(FileD* file_d, FrmlElem* Z, void* record);
LongStr* RunSelectStr(FileD* file_d, FrmlElem0* Z, void* record);
void LowCase(LongStr* S);
void LowCase(std::string& text);
double RoundReal(double RR, short M);
LongStr* LongLeadChar(char C, char CNew, LongStr* S);
LongStr* LongTrailChar(char C, char CNew, LongStr* S);
LongStr* RepeatStr(LongStr* S, short N);
void AccRecNoProc(FrmlElem14* X, WORD Msg, BYTE** record);
void GetRecNoXString(FileD* file_d, FrmlElem13* Z, XString& X, void* record);
double RunRealStr(FileD* file_d, FrmlElem* X, void* record);

WORD TypeDay(double R);
double AddWDays(double R, short N, WORD d);
double DifWDays(double R1, double R2, WORD d);
double LastUpdate(const std::string& path);

int GetFileSize();
