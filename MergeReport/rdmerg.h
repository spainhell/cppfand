#pragma once
#include "../cppfand/rdrun.h"

extern char WhatToRd; /*i=Oi output FDs;O=O outp.FDs*/
extern bool ReadingOutpBool;
extern WORD Ii, Oi, SumIi;
extern OutpRD* RD;

FileD* InpFD_M(WORD I); // InpFD exituje i v rdrprt.cpp -> pøejmenováno na *_M
FrmlElem* FindIiandFldFrml_M(FileD** FD, char& FTyp); // exituje i v rdrprt.cpp -> pøejmenováno na *_M
FrmlPtr RdFldNameFrmlM(char& FTyp);
void RdDirFilVar_M(char& FTyp, FrmlElem** res, bool wasIiPrefix); // exituje i v rdrprt.cpp -> pøejmenováno na *_M
void RdOutpFldName(char& FTyp, FrmlElem** res);
void ChainSumElM();
void ReadMerge();
void MakeOldMFlds();
void RdAutoSortSK_M(InpD* ID); // exituje i v rdrprt.cpp -> pøejmenováno na *_M
void ImplAssign(OutpRD* RD, FieldDescr* FNew);
FrmlElem* AdjustComma_M(FrmlElem* Z1, FieldDescr* F, instr_type Op);// exituje -> pøejmenováno na *_M
void FindIiandFldD(FieldDescr* F);
bool FindAssignToF(AssignD* A, FieldDescr* F);
void MakeImplAssign();
void TestIsOutpFile(FileD* FD);
AssignD* RdAssign_M();
AssignD* RdAssSequ();
void RdOutpRD(OutpRD** RDRoot);

