//#pragma once
//#include "../cppfand/rdrun.h"
//
//extern char WhatToRd; /*i=Oi output FDs;O=O outp.FDs*/
//extern bool ReadingOutpBool;
//extern WORD Ii, Oi, SumIi;
//extern OutpRD* RD;
//
//FileD* InpFD_M(WORD I); // InpFD exituje i v rdrprt.cpp -> p�ejmenov�no na *_M
//FrmlElem* FindIiandFldFrml_M(FileD** FD, char& FTyp); // exituje i v rdrprt.cpp -> p�ejmenov�no na *_M
//FrmlElem* RdFldNameFrmlM(char& FTyp);
//void RdDirFilVar_M(char& FTyp, FrmlElem** res, bool wasIiPrefix); // exituje i v rdrprt.cpp -> p�ejmenov�no na *_M
//void RdOutpFldName(char& FTyp, FrmlElem** res);
//void ChainSumElM();
//void ReadMerge();
//void MakeOldMFlds();
//void RdAutoSortSK_M(InpD* ID); // exituje i v rdrprt.cpp -> p�ejmenov�no na *_M
//void ImplAssign(OutpRD* outputRD, FieldDescr* outputField);
//FrmlElem* AdjustComma_M(FrmlElem* Z1, FieldDescr* F, instr_type Op);// exituje -> p�ejmenov�no na *_M
//FieldDescr* FindIiandFldD(std::string fieldName);
//bool FindAssignToF(std::vector<AssignD*> A, FieldDescr* F);
//void MakeImplAssign();
//void TestIsOutpFile(FileD* FD);
//std::vector<AssignD*> RdAssign_M();
//std::vector<AssignD*> RdAssSequ();
//void RdOutpRD(OutpRD** RDRoot);
//
