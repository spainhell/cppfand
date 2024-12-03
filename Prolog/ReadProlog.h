#pragma once
#include <string>
#include "../Core/Rdb.h"

class ReadProlog
{
public:
	ReadProlog();
	~ReadProlog();

	void Read(RdbPos* rdb_pos);
};

