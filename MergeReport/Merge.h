#pragma once
#include "MergeReportBase.h"
#include "../Common/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/runfrml.h"


class Compiler;

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
	FrmlElem* RdDirFilVar_M(char& FTyp, bool wasIiPrefix);
	FrmlElem* RdOutpFldName(char& FTyp);
	void MakeOldMFlds();
	void RdAutoSortSK_M(InpD* ID, Compiler* compiler);
	void ImplAssign(OutpRD* outputRD, FieldDescr* outputField);
	FrmlElem* AdjustComma_M(FrmlElem* Z1, FieldDescr* F, instr_type Op);
	FieldDescr* FindIiandFldD(std::string fieldName);
	bool FindAssignToF(std::vector<AssignD*> A, FieldDescr* F);
	void MakeImplAssign();
	void TestIsOutpFile(FileD* FD);
	std::vector<AssignD*> RdAssign_M();
	std::vector<AssignD*> RdAssSequ();
	void RdOutpRD(std::vector<OutpRD*>& RDRoot);

	WORD CompMFlds(FileD* file_d, Record* record, std::vector<KeyFldD*>& M);
	void SetOldMFlds(FileD* file_d, Record* record, std::vector<KeyFldD*>& M);
	void ReadInpFileM(InpD* ID);
	void RunAssign(FileD* file_d, Record* record, std::vector<AssignD*> Assigns);
	void WriteOutp(std::vector<OutpRD*>& v_outputs);
	void OpenInpM();
	void OpenOutp();
	void CloseInpOutp();
	void MoveForwToRecM(InpD* ID);
	void SetMFlds(FileD* file_d, Record* record, std::vector<KeyFldD*>& M);
	void MergeProc(FileD* file_d, Record* record);
	void JoinProc(FileD* file_d, Record* record, WORD Ii, bool& EmptyGroup);

	std::vector<OutpFD*> OutpFDRoot;
	std::vector<OutpRD*> OutpRDs;

};

