#pragma once
#include <string>

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
}
