#pragma once
#include <cstdio>

enum class loglevel
{
	DEBUG, INFO, WARN, ERR, EXCEPTION
};

class Logging
{
public:
	static Logging* getInstance();
	void log(loglevel level, char const* const _Format, ...);
	static void finish();
	Logging();
	
private:
	static Logging* _instance;
	static FILE* _file;
	static loglevel _level;
};

