#include "RunProlog.h"

#include "ReadProlog.h"
#include "../Core/GlobalVariables.h"
#include "../Common/Compare.h"
#include "../Core/Models/Instr.h"

RunProlog::RunProlog(Instr_lproc* prolog_instr)
{
	_rdb_pos = &prolog_instr->lpPos;
	_procedure_name = prolog_instr->lpName;
	_chapter_name = prolog_instr->name;

	auto reader = std::make_unique<ReadProlog>();
	reader->Read(_rdb_pos);
}

void RunProlog::Run()
{
	if (_chapter_name == "DejDeklDBF") {
		RunDbfExport();
	}
	else if (_chapter_name == "Indexy")
	{
		RunIndexy();
	}
	else {
		// other chapters
	}
}

void RunProlog::RunDbfExport()
{
	FileD* param_f = FindFile("PARAM3");
	FieldDescr* param_fld = FindField(param_f, "TTT");

	// find File by name in TTT
	std::string file_name = ReadFromParamFile(param_f, param_fld);
	FileD* dbf_file = FindFile(file_name);

	std::vector<std::string> dbf_decl = GetDbfDeclaration(dbf_file);

	std::string result = ConvertStringsVectorToString(dbf_decl);

	// save a result back to TTT
	SaveToParamFile(param_f, param_fld, result);
}

void RunProlog::RunIndexy()
{
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

std::string RunProlog::ReadFromParamFile(FileD* file_d, FieldDescr* field_d)
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

void RunProlog::SaveToParamFile(FileD* file_d, FieldDescr* field_d, std::string value)
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
		std::string s;

		if (!field->isStored()) {
			s += '\xF0'; // NBSP - non-breaking space
		}

		switch (field->field_type) {
		case FieldType::ALFANUM: {
			s += field->Name + ":A," + std::to_string(field->L) + ";";
			break; 
		}
		case FieldType::BOOL: {
			s += field->Name + ":B;";
			break;
		}
		case FieldType::FIXED: {
			int l;
			if (field->M == 0) {
				l = field->L - 1;
			}
			else {
				l = field->L - field->M - 2;
			}
			s += field->Name + ":F," + std::to_string(l + 1) + "." +
				std::to_string(field->M) + ";";
			break;
		}
		case FieldType::NUMERIC: {
			s += field->Name + ":A," + std::to_string(field->L) + ";";
			break;
		}
		case FieldType::DATE: {
			s += field->Name + ":D;";
			break;
		}
		case FieldType::TEXT: {
			s += field->Name + ":T;";
			break;
		}
		case FieldType::REAL: {
			s += field->Name + ":F,12.6;";
			break;
		}
		case FieldType::UNKNOWN:
			break;
		}

		result.push_back(s);
	}

	return result;
}

std::string RunProlog::ConvertStringsVectorToString(const std::vector<std::string>& vector)
{
	std::string result;
	for (const std::string& s : vector) {
		result += s;
		result += "\r";
	}
	return result;
}
