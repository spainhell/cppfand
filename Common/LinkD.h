#pragma once
#include <string>
#include <vector>

#include "FileD.h"

class LinkD
{
public:
	LinkD();
	LinkD(const LinkD& orig);
	uint16_t IndexRoot = 0;	// 0 - non index file || to only primary key
	uint8_t MemberRef = 0;	// { 0-no, 1-!, 2-!!(no delete)}
	std::vector<KeyFldD*> Args;
	FileD* FromFD = nullptr;
	FileD* ToFD = nullptr;
	XKey* ToKey = nullptr;
	std::string RoleName;
};
