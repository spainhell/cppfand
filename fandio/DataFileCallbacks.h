#pragma once
#include <cstdint>

class FileD;

class DataFileCallbacks
{
public:
	DataFileCallbacks();
	DataFileCallbacks(
		void (*progressOnCb)(int8_t, int32_t),
		void (*progressUpdateCb)(int32_t),
		void (*progressOffCb)(),
		void (*shortMsgCb)(int32_t),
		void (*fileMsgCb)(FileD*, int32_t, int8_t),
		void (*errorCb)(int32_t),
		bool (*promptCb)(int32_t)
	);
	void progressOn(int8_t c, int32_t n) const;
	void progressUpdate(int32_t n) const;
	void progressOff() const;
	void shortMsg(int32_t msg_nr) const;
	void fileMsg(FileD* file_d, int32_t n, int8_t typ) const;
	void runError(int32_t n);
	bool promptYN(int32_t msg_nr);

private:
	void (*progressOnCb)(int8_t, int32_t);
	void (*progressUpdateCb)(int32_t);
	void (*progressOffCb)();
	void (*shortMsgCb)(int32_t);
	void (*fileMsgCb)(FileD*, int32_t, int8_t);
	void (*errorCb)(int32_t);
	bool (*promptCb)(int32_t);
};

