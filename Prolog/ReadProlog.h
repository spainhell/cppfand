#pragma once
#include <string>
#include "../Core/Project.h"
#include "../Common/RdbPos.h"

class ReadProlog
{
public:
	ReadProlog();
	~ReadProlog();

	std::string Read(RdbPos* rdb_pos);
};

