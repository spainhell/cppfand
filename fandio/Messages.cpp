#include "Messages.h"

#include <chrono>
#include <thread>
#include <utility>

#include "../Logging/Logging.h"

namespace fandio
{
	namespace
	{
		MessageHandlers handlers_;

		std::string describe(const Message& message)
		{
			std::string s = "fandio message " + std::to_string(message.code);
			for (size_t i = 0; i < message.params.size(); i++) {
				s += (i == 0 ? ": " : ", ") + message.params[i];
			}
			return s;
		}

		const char* part_name(FilePart part)
		{
			switch (part) {
			case FilePart::Text: return "text";
			case FilePart::Index: return "index";
			default: return "data";
			}
		}
	}

	Error::Error(Message message)
		: std::runtime_error(describe(message)), message_(std::move(message))
	{
	}

	void SetMessageHandlers(MessageHandlers handlers)
	{
		handlers_ = std::move(handlers);
	}

	void RaiseError(int code, std::vector<std::string> params)
	{
		Message message{ code, std::move(params) };
		if (handlers_.error) {
			handlers_.error(message);
		}
		// the handler must not return; if it does, stop the operation anyway
		throw Error(std::move(message));
	}

	void Abort(int code, std::vector<std::string> params)
	{
		Message message{ code, std::move(params) };
		if (handlers_.abort) {
			handlers_.abort(message);
		}
		throw Error(std::move(message));
	}

	void ShowMessage(int code, std::vector<std::string> params)
	{
		Message message{ code, std::move(params) };
		if (handlers_.message) {
			handlers_.message(message);
		}
		else {
			SPDLOG_WARN("{}", describe(message));
		}
	}

	void ShowFileMessage(FileD* file, FilePart part, int code)
	{
		if (handlers_.fileMessage) {
			handlers_.fileMessage(file, part, code);
		}
		else {
			SPDLOG_WARN("fandio message {} ({} file)", code, part_name(part));
		}
	}

	bool Confirm(int code, std::vector<std::string> params)
	{
		Message message{ code, std::move(params) };
		if (handlers_.confirm) {
			return handlers_.confirm(message);
		}
		SPDLOG_WARN("{} - answered 'no'", describe(message));
		return false;
	}

	bool WaitForLock(LockWait& wait)
	{
		wait.attempt++;
		bool result;
		if (handlers_.lockWait) {
			result = handlers_.lockWait(wait);
		}
		else {
			if (wait.attempt == 1) {
				SPDLOG_WARN("fandio: waiting for lock {} on '{}'", wait.mode, wait.path);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			result = wait.attempt < 20;
		}
		return result || !wait.cancellable;
	}

	void EndLockWait(LockWait& wait)
	{
		if (wait.attempt > 0 && handlers_.lockWaitEnd) {
			handlers_.lockWaitEnd(wait);
		}
	}
}
