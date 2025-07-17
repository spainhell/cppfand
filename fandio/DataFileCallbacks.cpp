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
