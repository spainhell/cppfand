#include "DataFileCallbacks.h"

DataFileCallbacks::DataFileCallbacks()
{
	progressOnCb = nullptr;
	progressUpdateCb = nullptr;
	progressOffCb = nullptr;
	shortMsgCb = nullptr;
	fileMsgCb = nullptr;
	errorCb = nullptr;
	promptCb = nullptr;
}

DataFileCallbacks::DataFileCallbacks(void(* progressOnCb)(int8_t, int32_t), void(* progressUpdateCb)(int32_t),
	void(* progressOffCb)(), void(* shortMsgCb)(int32_t), void(* fileMsgCb)(FileD*, int32_t, int8_t),
	void(* errorCb)(int32_t), bool(* promptCb)(int32_t))
{
	this->progressOnCb = progressOnCb;
	this->progressUpdateCb = progressUpdateCb;
	this->progressOffCb = progressOffCb;
	this->shortMsgCb = shortMsgCb;
	this->fileMsgCb = fileMsgCb;
	this->errorCb = errorCb;
	this->promptCb = promptCb;
}

void DataFileCallbacks::progressOn(int8_t c, int32_t n) const
{
	if (progressOnCb != nullptr) {
		progressOnCb(c, n);
	}
}

void DataFileCallbacks::progressUpdate(int32_t n) const
{
	if (progressUpdateCb != nullptr) {
		progressUpdateCb(n);
	}
}

void DataFileCallbacks::progressOff() const
{
	if (progressOffCb != nullptr) {
		progressOffCb();
	}
}

void DataFileCallbacks::shortMsg(int32_t msg_nr) const
{
	if (shortMsgCb != nullptr) {
		shortMsgCb(msg_nr);
	}
}

void DataFileCallbacks::fileMsg(FileD* file_d, int32_t n, int8_t typ) const
{
	if (fileMsgCb != nullptr) {
		fileMsgCb(file_d, n, typ);
	}
}

void DataFileCallbacks::runError(int32_t n)
{
	if (errorCb != nullptr) {
		errorCb(n);
	}
}

bool DataFileCallbacks::promptYN(int32_t msg_nr)
{
	if (promptCb != nullptr) {
		return promptCb(msg_nr);
	}
	return false; // default return value if no callback is set
}
