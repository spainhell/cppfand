#pragma once

enum FileOperation
{
	READ,
	WRITE
};

enum FileOpenMode
{
	_isNewFile = 0,
	_isOldFile = 1,
	_isOverwriteFile = 2,
	_isOldNewFile = 3
}; // poradi se nesmi zmenit!!!

enum FileUseMode
{
	Closed = 0,
	RdOnly = 1,
	RdShared = 2,
	Shared = 3,
	Exclusive = 4
}; // poradi se nesmi zmenit!!!

enum LockMode
{
	NullMode = 0,
	NoExclMode = 1,
	NoDelMode = 2,
	NoCrMode = 3,
	RdMode = 4,
	WrMode = 5,
	CrMode = 6,
	DelMode = 7,
	ExclMode = 8
};