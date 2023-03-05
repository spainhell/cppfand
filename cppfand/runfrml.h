#pragma once
#include "access.h"
#include "models/FrmlElem.h"
#include "pstring.h"

#ifdef FandGraph
#include "graph.h"
#endif
#ifdef FandSQL
#include "channel.h"
#endif

double Owned(FrmlElem* Bool, FrmlElem* Sum, LinkD* LD);
integer CompBool(bool B1, bool B2);
integer CompReal(double R1, double R2, integer M);
LongStr* CopyToLongStr(pstring& SS);
LongStr* CopyToLongStr(std::string& SS);
pstring LeadChar(char C, pstring S);
bool RunBool(FrmlElem* X);
bool InReal(FrmlElemIn* frml);
bool LexInStr(std::string& S, FrmlElemIn* X);
bool InStr(LongStr* S, FrmlElemIn* X);
bool InStr(std::string& S, FrmlElemIn* X);
bool RunModulo(FrmlElem1* X);
bool RunEquMask(FrmlElem0* X);
double RunReal(FrmlElem* X);
longint RunInt(FrmlElem* X);
bool CanCopyT(FieldDescr* F, FrmlElem* Z, FandTFile** TF02, FileD** TFD02, longint& TF02Pos);
bool TryCopyT(FieldDescr* F, FandTFile* TF, longint& pos, FrmlElem1* Z);
void AssgnFrml(FieldDescr* F, FrmlElem* X, bool Delete, bool Add);
void LVAssignFrml(LocVar* LV, bool Add, FrmlElem* X);
std::string DecodeFieldRSB(FieldDescr* F, WORD LWw, double R, std::string& T, bool B);
std::string DecodeField(FieldDescr* F, WORD LWw);
void RunWFrml(WRectFrml& X, BYTE WFlags, WRect& W);
WORD RunWordImpl(FrmlElem* Z, WORD Impl);
bool FieldInList(FieldDescr* F, FieldListEl* FL);
bool FieldInList(FieldDescr* F, std::vector<FieldDescr*>& FL);
bool FieldInList(FieldDescr* F, std::vector<FieldDescr*>* FL);
XKey* GetFromKey(LinkD* LD);
FrmlElem* RunEvalFrml(FrmlElem* Z);
LongStr* RunLongStr(FrmlElem* X);  // r417 zacina od 555
std::string RunStdStr(FrmlElem* X);  // nove, vraci std::string
std::string RunShortStr(FrmlElem* X); // r629 ASM
void CopyLongStr(LongStr* S, WORD From, WORD Number); // r425 
void AddToLongStr(LongStr* S, void* P, WORD L); // r433
void StrMask(double R, pstring& Mask); // r438
LongStr* RunS(FrmlElem* Z); // r469
LongStr* RunSelectStr(FrmlElem0* Z); // r522
void LowCase(LongStr* S); //543 ASM
void LowCase(std::string& text);
double RoundReal(double RR, integer M);
LongStr* LongLeadChar(char C, char CNew, LongStr* S);
LongStr* LongTrailChar(char C, char CNew, LongStr* S);
LongStr* RepeatStr(LongStr* S, integer N);
void AccRecNoProc(FrmlElem14* X, WORD Msg);
void GetRecNoXString(FrmlElem13* Z, XString& X);
double RunRealStr(FrmlElem* X);
double RMod(FrmlElem0* X);
double LastUpdate(const std::string& path);
WORD TypeDay(double R);
double AddWDays(double R, integer N, WORD d);

double DifWDays(double R1, double R2, WORD d);
longint GetFileSize();
longint RecNoFun(FrmlElem13* Z);
longint AbsLogRecNoFun(FrmlElem13* Z);
double LinkProc(FrmlElem15* X);
WORD IntTSR(FrmlElem* X);
WORD PortIn(bool IsWord, WORD Port);

