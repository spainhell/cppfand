#pragma once
#include <functional>

// Callback function type for progress messages with a status code and a value
using RunMsgOnCallback = std::function<void(int8_t, int32_t)>;

// Callback function type for progress messages without parameters
using RunMsgOffCallback = std::function<void()>;

// Callback function type for progress messages with a single integer parameter
using RunMsgNCallback = std::function<void(int32_t)>;

struct ProgressCallbacks
{
	RunMsgOnCallback runMsgOn;   // Callback for starting progress messages
	RunMsgOffCallback runMsgOff; // Callback for stopping progress messages
	RunMsgNCallback runMsgN;     // Callback for progress updates with a value
};
