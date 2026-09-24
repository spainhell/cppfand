#pragma once
#include <cstdint>
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

		// number of this station in the network (LANNODE), used for lock ranges
		uint16_t lanNode = 0;

		// Whether the file is the chapter file (.RDB) of the current project. When its
		// text file was written by another program version, all chapters are recompiled.
		// Default: no file is.
		std::function<bool(FileD* file)> isCurrentProjectFile;

		// Whether the file is the chapter file of the current project or of one of
		// its parent projects; only then its text file may be read without a password.
		// Default: no file is.
		std::function<bool(FileD* file)> isActiveProjectFile;

		// Whether the file is the catalog; when opening it is treated as a project file.
		// Default: no file is.
		std::function<bool(FileD* file)> isCatalogFile;

		// Whether read-only project files are made writable while open (test and
		// install run). Default: no.
		std::function<bool()> writableProjectFiles;

		// Called when the prefix of a file does not match its declaration; the host
		// may convert an older format of the file (CppFand: catalog). Returns true
		// when converted. Default: no conversion.
		std::function<bool(FileD* file, int& file_size)> upgradeFile;
	};

	void SetSettings(Settings settings);
	const Settings& GetSettings();
}
