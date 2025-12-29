#include "ReadProlog.h"
#include "../Core/Compiler.h"
#include "../Common/FileD.h"
#include "../Common/Record.h"

ReadProlog::ReadProlog()
{
}

ReadProlog::~ReadProlog()
{
}

std::string ReadProlog::Read(RdbPos* rdb_pos)
{
	//Project* ChptLRdb = rdb_pos->rdb;
	//Record* record = new Record(ChptLRdb->project_file);
	//ChptLRdb->project_file->ReadRec(rdb_pos->i_rec, record);
	//FieldDescr* ChptTxt = ChptLRdb->project_file->FldD[5];

	std::unique_ptr<Compiler> compiler = std::make_unique<Compiler>();
	compiler->SetInpTT(rdb_pos, true);

	//delete record;
	//record = nullptr;

	return compiler->input_string;
}
