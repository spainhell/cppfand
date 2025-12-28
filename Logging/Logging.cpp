#include "Logging.h"

#include <cstdarg>
#include <ctime>
#include <memory>


Logging* Logging::_instance = nullptr;
FILE* Logging::_file = nullptr;
loglevel Logging::_level = loglevel::DEBUG;

Logging* Logging::getInstance()
{
	if (_instance == nullptr) _instance = new Logging();
	return _instance;
}

void Logging::log(loglevel level, char const* const _Format, ...)
{
	// 2020-10-07 15:00:39.775 [8064] :INFO:
	char buffer[1024];
	std::time_t t = std::time(0);   // get time now
	struct tm lt;
	errno_t err = localtime_s(&lt, &t);
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &lt);
	std::string str(buffer);

	switch (level) {
	case loglevel::DEBUG: str += " :DEBUG: "; break;
	case loglevel::INFO: str += " :INFO: "; break;
	case loglevel::WARN: str += " :WARNING: "; break;
	case loglevel::ERR: str += " :ERROR: "; break;
	case loglevel::EXCEPTION: str += " :EXCEPTION: "; break;
	}

	va_list args;
	va_start(args, _Format);
	vsnprintf(buffer, sizeof(buffer), _Format, args);
	va_end(args);

	str += buffer;
	str += '\n';
		
	fprintf_s(_file, str.c_str());
}

void Logging::finish()
{
	fflush(_file);
	fclose(_file);
}

Logging::Logging()
{
	std::string path = GetEnv("FANDWORK");
	if (path.empty() || path.ends_with("\\"))
	{
		path += "fand.log";
	}
	else
	{
		path += "\\";
		path += "fand.log";
	}

	static loglevel _level = loglevel::DEBUG;
	auto err = fopen_s(&_file, path.c_str(), "a");
	fprintf_s(_file, "\n");
}

std::string Logging::GetEnv(const char* name)
{
	std::string result;
	size_t requiredSize = 0;
	getenv_s(&requiredSize, NULL, 0, name);
	if (requiredSize == 0) {
		result = "";
	}
	else {
		std::unique_ptr<char[]> buffer = std::make_unique<char[]>(requiredSize * sizeof(char));
		getenv_s(&requiredSize, buffer.get(), requiredSize, name);
		result = std::string(buffer.get());
	}
	return result;
}