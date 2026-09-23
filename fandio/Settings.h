#pragma once
#include <string>

namespace fandio
{
	// Settings of the host application that fandio needs.
	// The host sets them with SetSettings() at startup.
	struct Settings
	{
		// directory for local work files (temporary copies of files on network volumes)
		std::string workDir;
	};

	void SetSettings(Settings settings);
	const Settings& GetSettings();
}
