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


extern FileDPtr TFD02;
extern TFilePtr TF02;
extern longint TF02Pos; // r33

double Owned(FrmlPtr Bool, FrmlPtr Sum, LinkDPtr LD);
integer CompBool(bool B1, bool B2);
integer CompReal(double R1, double R2, integer M); // r42
LongStr* CopyToLongStr(pstring& SS);
pstring LeadChar(char C, pstring S); // r69
pstring TrailChar(char C, pstring s); // r73
LongStr* CopyLine(LongStr* S, WORD N, WORD M);
bool RunBool(FrmlPtr X);
bool InReal(double R, BYTE* L, integer M);
bool LexInStr(LongStr* S, BYTE* L);
bool InStr(LongStr* S, BYTE* L);
bool RunModulo(FrmlElem1* X);
bool RunEquMask(FrmlElem0* X);
double RunReal(FrmlElem* X);
longint RunInt(FrmlPtr X);
void TestTFrml(FieldDescr* F, FrmlElem* Z);
bool CanCopyT(FieldDPtr F, FrmlPtr Z);
bool TryCopyT(FieldDPtr F, TFilePtr TF, longint& pos, FrmlPtr Z);
void AssgnFrml(FieldDPtr F, FrmlPtr X, bool Delete, bool Add);
void LVAssignFrml(LocVar* LV, void* OldBP, bool Add, FrmlPtr X);
void DecodeFieldRSB(FieldDPtr F, WORD LWw, double R, pstring T, bool B, pstring& Txt);
void DecodeField(FieldDPtr F, WORD LWw, pstring& Txt);
void RunWFrml(WRectFrml& X, BYTE WFlags, WRect& W);
WORD RunWordImpl(FrmlElem* Z, WORD Impl);
bool FieldInList(FieldDPtr F, FieldListEl* FL);
KeyDPtr GetFromKey(LinkDPtr LD);
FrmlPtr RunEvalFrml(FrmlPtr Z);
LongStr* RunLongStr(FrmlPtr X);  // r417 zaèíná od 555
pstring RunShortStr(FrmlPtr X); // r629 ASM
void ConcatLongStr(LongStr* S1, LongStr* S2); // r418 ASM
void CopyLongStr(LongStr* S, WORD From, WORD Number); // r425 ASM
void AddToLongStr(LongStr* S, void* P, WORD L); // r433
void StrMask(double R, pstring& Mask); // r438
LongStr* RunS(FrmlElem* Z); // r469
LongStr* RunSelectStr(FrmlElem0* Z); // r522
void LowCase(LongStr* S); //543 ASM
double RoundReal(double RR, integer M);
LongStr* LongLeadChar(char C, char CNew, LongStr* S);
LongStr* LongTrailChar(char C, char CNew, LongStr* S);
LongStr* RepeatStr(LongStr* S, integer N);
void AccRecNoProc(FrmlElem14* X, WORD Msg);
void* RunUserFunc(FrmlElem19* X);
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

