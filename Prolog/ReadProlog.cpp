#include "ReadProlog.h"
#include "../Core/Compiler.h"
#include "../Common/FileD.h"

ReadProlog::ReadProlog()
{
}

ReadProlog::~ReadProlog()
{
}

std::string ReadProlog::Read(RdbPos* rdb_pos)
{
	RdbD* ChptLRdb = rdb_pos->rdb;
	uint8_t* CRecPtr = ChptLRdb->v_files[0]->GetRecSpace();
	//ChptLRdb->v_files[0]->ReadRec(rdb_pos->i_rec, CRecPtr);
	FieldDescr* ChptTxt = ChptLRdb->v_files[0]->FldD[5];

	std::unique_ptr<Compiler> compiler = std::make_unique<Compiler>();
	compiler->SetInpTTPos(ChptLRdb->v_files[0], ChptLRdb->v_files[0]->loadT(ChptTxt, CRecPtr), ChptLRdb->Encrypted);

	delete[] CRecPtr;
	CRecPtr = nullptr;

	return compiler->input_string;
}
