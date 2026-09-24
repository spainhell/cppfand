#pragma once
#include <cstdint>
#include <functional>
#include <string>

class FileD;
typedef void* HANDLE;

namespace fandio
{
	// Location of a data file split into parts, e.g. "C:\DATA\" + "FIRMY" + ".000"
	struct FilePath
	{
		std::string dir;	// with trailing delimiter
		std::string name;
		std::string ext;	// with leading dot
		std::string volume;	// catalog volume ("#", "##", "#R" for network volumes)

		std::string Full() const { return dir + name + ext; }
	};

	// Path of the index file belonging to the data file (.000 -> .X00)
	std::string IndexFilePath(const FilePath& path);

	// Whether the volume is a network one; work files for such files are kept locally
	bool IsNetVolume(const std::string& volume);

	// Splits a path into dir, name and ext (the volume stays empty)
	FilePath SplitPath(const std::string& path);

	// How fandio finds out where files are. A file has only a name; the host
	// application knows where it lies (catalog, project directories, ...).
	struct PathHandlers
	{
		// Location of the file; delimiter '\\' or '/' is used in dir.
		// CppFand also keeps the result in its global state (CPath, CDir, ...).
		// Default: FileD::FullPath split into parts.
		std::function<FilePath(FileD* file, char delimiter)> resolve;

		// Volume of the file currently being worked with; OpenH waits for files
		// locked by another user only on network volumes. Default: "" (local).
		std::function<std::string()> currentVolume;

		// Forget the current volume (a local work file is going to be opened).
		std::function<void()> resetCurrentVolume;

		// Path of an open file, for error messages. Default: path passed to OpenH.
		std::function<std::string(HANDLE handle)> pathOfHandle;

		// Record of the file in the catalog, 0 = not there (multilevel: search also
		// the catalogs of parent projects). Default: 0.
		std::function<int32_t(const std::string& name, bool multilevel)> catalogRecord;

		// Drive number to be set to the file (removable volumes). Default: 0.
		std::function<uint8_t(const std::string& path)> mountVolume;
	};

	void SetPathHandlers(PathHandlers handlers);

	// Calls of the handlers (or their defaults)
	FilePath ResolvePath(FileD* file, char delimiter = '\\');
	std::string CurrentVolume();
	void ResetCurrentVolume();
	std::string PathOfHandle(HANDLE handle);
	uint8_t MountVolume(const std::string& path);
	int32_t CatalogRecord(const std::string& name, bool multilevel);
}
