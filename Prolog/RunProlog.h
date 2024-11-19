#pragma once
#include <string>
#include "../Core/Rdb.h"
#include "../Core/FieldDescr.h"


class RunProlog
{
public:
	RunProlog(RdbPos* rdb_pos, std::string chapter_name);

	void Run();

private:
	FieldDescr* FindField(FileD* file_d, std::string field_name);
	FileD* FindFile(std::string file_name);
	std::string ReadParam(FileD* file_d, FieldDescr* field_d);
	void SaveParam(FileD* file_d, FieldDescr* field_d, std::string value);
	std::vector<std::string> GetDbfDeclaration(FileD* file_d);
};

