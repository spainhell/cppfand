#include "RunProlog.h"
#include "../Core/GlobalVariables.h"
#include "../Common/Compare.h"
#include "../Core/Models/Instr.h"

RunProlog::RunProlog(RdbPos* rdb_pos, std::string chapter_name)
{
}

void RunProlog::Run()
{
	FileD* param_f = FindFile("PARAM3");
	FieldDescr* param_fld = FindField(param_f, "TTT");

	// find File by name in TTT
	std::string file_name = ReadParam(param_f, param_fld);
	FileD* dbf_file = FindFile(file_name);

	std::vector<std::string> dbf_decl = GetDbfDeclaration(dbf_file);

	// save a result back to TTT
	SaveParam(param_f, param_fld, "BYL JSEM ZDE! FANTOMAS");
}

FieldDescr* RunProlog::FindField(FileD* file_d, std::string field_name)
{
	if (file_d != nullptr) {
		for (FieldDescr* i : file_d->FldD) {
			if (EquUpCase(i->Name, field_name)) return i;
		}
	}
	return nullptr;
}

FileD* RunProlog::FindFile(std::string file_name)
{
	RdbD* R = CRdb;

	while (R != nullptr) {
		for (FileD* f : R->v_files) {
			if (EquUpCase(f->Name, file_name)) {
				return f;
			}
		}
		R = R->ChainBack;
	}

	return nullptr;
}

std::string RunProlog::ReadParam(FileD* file_d, FieldDescr* field_d)
{
	int RecNo = 0;

	LockMode lm = file_d->NewLockMode(RdMode);
	BYTE* newRecord = nullptr;
	LinkLastRec(file_d, RecNo, true, &newRecord);

	std::string result = file_d->loadS(field_d, newRecord);

	file_d->OldLockMode(lm);  /*possibly reading .T*/
	file_d->ClearRecSpace(newRecord);
	delete[] newRecord; newRecord = nullptr;

	return result;
}

void RunProlog::SaveParam(FileD* file_d, FieldDescr* field_d, std::string value)
{
	int n = 0;
	LockMode md = file_d->NewLockMode(WrMode);
	BYTE* rec = nullptr;
	LinkLastRec(file_d, n, true, &rec);

	file_d->FF->DelTFld(field_d, rec);
	file_d->saveS(field_d, value, rec);

	file_d->WriteRec(n, rec);
	file_d->OldLockMode(md);
	delete[] rec; rec = nullptr;
}

std::vector<std::string> RunProlog::GetDbfDeclaration(FileD* file_d)
{
	std::vector<std::string> result;

	for (FieldDescr* field : file_d->FldD) {
		if (field->isStored()) {
			std::string s;
			switch (field->field_type) {
			case FieldType::ALFANUM: {
				s = field->Name + ":A," + std::to_string(field->L) + ";";
				break;
			}
			case FieldType::BOOL: {
				break;
			}
			case FieldType::FIXED:
				break;
			case FieldType::NUMERIC:
				break;
			case FieldType::DATE:
				break;
			case FieldType::TEXT:
				break;
			case FieldType::REAL:
				break;
			case FieldType::UNKNOWN:
				break;
			}
			result.push_back(s);
		}
		else {
			// process only stored fields
			continue;
		}
	}

	return result;
}
