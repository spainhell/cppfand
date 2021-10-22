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

extern FileD* TFD02;
extern TFile* TF02;
extern longint TF02Pos; // r33

double Owned(FrmlPtr Bool, FrmlPtr Sum, LinkDPtr LD);
integer CompBool(bool B1, bool B2);
integer CompReal(double R1, double R2, integer M); // r42
LongStr* CopyToLongStr(pstring& SS);
LongStr* CopyToLongStr(std::string& SS);
pstring LeadChar(char C, pstring S); // r69
bool RunBool(FrmlPtr X);
bool InReal(FrmlElemIn* frml);
bool LexInStr(LongStr* S, FrmlElemIn* X);
bool InStr(LongStr* S, FrmlElemIn* X);
bool RunModulo(FrmlElem1* X);
bool RunEquMask(FrmlElem0* X);
double RunReal(FrmlElem* X);
longint RunInt(FrmlPtr X);
void TestTFrml(FieldDescr* F, FrmlElem* Z);
bool CanCopyT(FieldDescr* F, FrmlElem* Z);
bool TryCopyT(FieldDescr* F, TFile* TF, longint& pos, FrmlPtr Z);
void AssgnFrml(FieldDescr* F, FrmlElem* X, bool Delete, bool Add);
void LVAssignFrml(LocVar* LV, void* OldBP, bool Add, FrmlElem* X);
std::string DecodeFieldRSB(FieldDescr* F, WORD LWw, double R, std::string& T, bool B);
std::string DecodeField(FieldDescr* F, WORD LWw);
void RunWFrml(WRectFrml& X, BYTE WFlags, WRect& W);
WORD RunWordImpl(FrmlElem* Z, WORD Impl);
bool FieldInList(FieldDescr* F, FieldListEl* FL);
bool FieldInList(FieldDescr* F, std::vector<FieldDescr*>& FL);
bool FieldInList(FieldDescr* F, std::vector<FieldDescr*>* FL);
XKey* GetFromKey(LinkDPtr LD);
FrmlPtr RunEvalFrml(FrmlPtr Z);
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
double LastUpdate(FILE* Handle);
WORD TypeDay(double R);
double AddWDays(double R, integer N, WORD d);

double DifWDays(double R1, double R2, WORD d);
longint GetFileSize();
longint RecNoFun(FrmlElem13* Z);
longint AbsLogRecNoFun(FrmlElem13* Z);
double LinkProc(FrmlElem15* X);
WORD IntTSR(FrmlElem* X);
WORD PortIn(bool IsWord, WORD Port); // ASM

