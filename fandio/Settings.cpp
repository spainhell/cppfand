#include "Settings.h"

#include <utility>

namespace fandio
{
	namespace
	{
		Settings settings_;
	}

	void SetSettings(Settings settings)
	{
		settings_ = std::move(settings);
	}

	const Settings& GetSettings()
	{
		return settings_;
	}
}
