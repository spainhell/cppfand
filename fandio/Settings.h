#pragma once
#include <functional>
#include <string>

class FileD;

namespace fandio
{
	// Settings of the host application that fandio needs.
	// The host sets them with SetSettings() at startup.
	struct Settings
	{
		// directory for local work files (temporary copies of files on network volumes)
		std::string workDir;

		// program version (4 characters) stored in the header of text files (.T00)
		std::string version = "4.20";

		// Whether the file is the chapter file (.RDB) of the current project. When its
		// text file was written by another program version, all chapters are recompiled.
		// Default: no file is.
		std::function<bool(FileD* file)> isCurrentProjectFile;
	};

	void SetSettings(Settings settings);
	const Settings& GetSettings();
}
