#pragma once
#include <cstdint>

class FileD;

class DataFileCallbacks
{
public:
	DataFileCallbacks();
	void (*progressOnCb)(int8_t, int32_t);
	void (*progressUpdateCb)(int32_t);
	void (*progressOffCb)();
	void (*shortMsgCb)(int32_t);
	void (*fileMsgCb)(FileD*, unsigned long, int8_t);
	void (*errorCb)(int32_t);
	bool (*promptCb)(int32_t);
};

