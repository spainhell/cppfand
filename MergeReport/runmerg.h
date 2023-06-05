#pragma once
#include "shared.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/runfrml.h"


class Merge : public MergeReportBase
{
public:
	Merge();
	~Merge();
	void Read();
	void Run();
	FrmlElem* RdFldNameFrml(char& FTyp) override;
	void ChainSumEl() override;

private:
	bool ReadingOutpBool = false;
	OutpRD* RD = nullptr;

	FileD* InpFD_M(WORD I);
	FrmlElem* FindIiandFldFrml_M(FileD** FD, char& FTyp);
	void RdDirFilVar_M(char& FTyp, FrmlElem** res, bool wasIiPrefix);
	void RdOutpFldName(char& FTyp, FrmlElem** res);
	void MakeOldMFlds();
	void RdAutoSortSK_M(InpD* ID);
	void ImplAssign(OutpRD* outputRD, FieldDescr* outputField);
	FrmlElem* AdjustComma_M(FrmlElem* Z1, FieldDescr* F, instr_type Op);
	FieldDescr* FindIiandFldD(std::string fieldName);
	bool FindAssignToF(std::vector<AssignD*> A, FieldDescr* F);
	void MakeImplAssign();
	void TestIsOutpFile(FileD* FD);
	std::vector<AssignD*> RdAssign_M();
	std::vector<AssignD*> RdAssSequ();
	void RdOutpRD(OutpRD** RDRoot);


	WORD CompMFlds(KeyFldD* M);
	void SetOldMFlds(KeyFldD* M);
	void ReadInpFileM(InpD* ID);
	void RunAssign(std::vector<AssignD*> Assigns);
	void WriteOutp(OutpRD* RD);
	void OpenInpM();
	void OpenOutp();
	void CloseInpOutp();
	void MoveForwToRecM(InpD* ID);
	void SetMFlds(KeyFldD* M);
	void MergeProcM();
	void JoinProc(WORD Ii, bool& EmptyGroup);

};

