#include "FilePath.h"
#include "../fandbase/switches.h"

namespace fandio
{
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
}
