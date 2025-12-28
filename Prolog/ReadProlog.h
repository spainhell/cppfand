#pragma once
#include <string>
#include "../Core/Project.h"

class ReadProlog
{
public:
	ReadProlog();
	~ReadProlog();

	std::string Read(RdbPos* rdb_pos);
};

