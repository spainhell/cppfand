#pragma once
#include "rdfrml1.h"
#include "constants.h"

void TestBool(char FTyp);
void TestString(char FTyp);
void TestReal(char FTyp);
FrmlPtr RdPrim(char& FTyp);
bool FindFuncD(FrmlPtr* ZZ);
const BYTE MaxLen = 9;
bool IsFun(void* XFun, BYTE N, void* XCode, char& FunCode); // ASM

pstring QQdiv = "div";
pstring QQmod = "mod";
pstring QQround = "round";

FrmlPtr RdMult(char& FTyp);
FrmlPtr RdAdd(char& FTyp);
FrmlPtr RdComp(char& FTyp);
WORD RdPrecision();
WORD RdTilde();
void RdInConst(FrmlPtr Z, double& R, pstring* S, char& FTyp);
void StoreConst(double& R, pstring* S, char& FTyp);
FrmlPtr BOperation(char Typ, char Fun, FrmlPtr Frml);
FrmlPtr RdBAnd(char& FTyp);
FrmlPtr RdBOr(char& FTyp);
FrmlPtr RdFormula(char& FTyp);
FrmlPtr RdKeyInBool(KeyInD* KIRoot, bool NewMyBP, bool FromRdProc, bool& SQLFilter);
FrmlPtr MyBPContext(FrmlPtr Z, bool NewMyBP);
FrmlList RdFL(bool NewMyBP, FrmlList FL1);
FrmlPtr RdFrml(char& FTyp);
FrmlPtr RdBool();
FrmlPtr RdRealFrml();
FrmlPtr RdStrFrml();

