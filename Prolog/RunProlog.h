#pragma once
#include <string>
#include "../Core/Rdb.h"
#include "../Core/FieldDescr.h"


class Instr_lproc;

class RunProlog
{
public:
	RunProlog(Instr_lproc* prolog_instr);

	void Run();
	void RunDbfExport();
	void RunIndexy();

private:
	RdbPos* _rdb_pos;
	std::string _chapter_name;
	std::string _procedure_name;

	FieldDescr* FindField(FileD* file_d, std::string field_name);
	FileD* FindFile(std::string file_name);
	std::string ReadFromParamFile(FileD* file_d, FieldDescr* field_d);
	void SaveToParamFile(FileD* file_d, FieldDescr* field_d, std::string value);
	std::vector<std::string> GetDbfDeclaration(FileD* file_d);
	std::string ConvertStringsVectorToString(const std::vector<std::string>& vector);
};

