#include "FilePath.h"

#include <utility>

#include "FileD.h"
#include "FileIO.h"
#include "../fandbase/pascal.h"
#include "../fandbase/switches.h"
#include "../fandbase/textfunc.h"

namespace fandio
{
	namespace
	{
		PathHandlers handlers_;
	}

	std::string IndexFilePath(const FilePath& path)
	{
		std::string ext = path.ext;
		ext[1] = 'X';
		return path.dir + path.name + ext;
	}

	bool IsNetVolume(const std::string& volume)
	{
#ifdef FandNetV
		return volume == "#" || volume == "##"
			|| (volume.length() == 2 && volume[0] == '#' && (volume[1] == 'R' || volume[1] == 'r'));
#else
		return false;
#endif
	}

	FilePath SplitPath(const std::string& path)
	{
		FilePath result;
		FSplit(path, result.dir, result.name, result.ext);
		return result;
	}

	void SetPathHandlers(PathHandlers handlers)
	{
		handlers_ = std::move(handlers);
	}

	FilePath ResolvePath(FileD* file, char delimiter)
	{
		if (handlers_.resolve) {
			return handlers_.resolve(file, delimiter);
		}
		FilePath result = SplitPath(file->FullPath);
		if (delimiter == '/') ReplaceChar(result.dir, '\\', '/');
		if (delimiter == '\\') ReplaceChar(result.dir, '/', '\\');
		return result;
	}

	std::string CurrentVolume()
	{
		return handlers_.currentVolume ? handlers_.currentVolume() : std::string();
	}

	void ResetCurrentVolume()
	{
		if (handlers_.resetCurrentVolume) {
			handlers_.resetCurrentVolume();
		}
	}

	std::string PathOfHandle(HANDLE handle)
	{
		return handlers_.pathOfHandle ? handlers_.pathOfHandle(handle) : OpenedPath(handle);
	}

	uint8_t MountVolume(const std::string& path)
	{
		return handlers_.mountVolume ? handlers_.mountVolume(path) : 0;
	}

	int32_t CatalogRecord(const std::string& name, bool multilevel)
	{
		return handlers_.catalogRecord ? handlers_.catalogRecord(name, multilevel) : 0;
	}
}
