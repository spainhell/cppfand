#pragma once
#include "shared.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/runfrml.h"


class Merge
{
public:
	Merge();
	~Merge();
	void Run();

private:
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

