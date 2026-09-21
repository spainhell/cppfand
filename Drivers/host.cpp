#include "host.h"
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>

namespace FandHost
{
	namespace
	{
		std::atomic<bool> g_enabled{ false };
		std::atomic<bool> g_stop{ false };
		std::atomic<bool> g_fieldEditEnabled{ true };

		// stav predavani editace pole mezi vlaknem interpretu a hostitele
		std::mutex g_editMutex;
		std::condition_variable g_editCv;
		bool g_requestPending = false;   // pozadavek zapsan, hostitel jej jeste nevyzvedl
		bool g_requestTaken = false;     // hostitel pozadavek vyzvedl, edituje
		bool g_resultReady = false;      // hostitel dodal vysledek
		FieldEditRequest g_request;
		FieldEditResult g_result;
	}

	bool IsEnabled() { return g_enabled; }
	void Enable() { g_enabled = true; }

	void RequestStop()
	{
		g_stop = true;
		// pokud interpret ceka na editaci pole, probudime ho, at si vsimne stopu
		std::lock_guard<std::mutex> lock(g_editMutex);
		if (g_requestPending || g_requestTaken) {
			g_result = FieldEditResult();
			g_result.Key = 27; // Esc
			g_resultReady = true;
			g_requestPending = false;
			g_requestTaken = false;
			g_editCv.notify_all();
		}
	}

	bool StopRequested() { return g_stop; }

	void Fatal(const std::string& message, int exitCode)
	{
		if (g_enabled) {
			throw HaltException(exitCode);
		}
		printf("%s\n", message.c_str());
		system("pause");
		exit(exitCode);
	}

	namespace
	{
		std::mutex g_fieldMutex;
		int g_fieldX = -1, g_fieldY = -1, g_fieldLen = 0;
		std::string g_fieldText;
	}

	void SetCurrentField(int x0, int y0, int len, const std::string& text)
	{
		std::lock_guard<std::mutex> lock(g_fieldMutex);
		g_fieldX = x0; g_fieldY = y0; g_fieldLen = len;
		g_fieldText = text;
	}

	std::string GetCurrentFieldText()
	{
		std::lock_guard<std::mutex> lock(g_fieldMutex);
		return g_fieldText;
	}

	void ClearCurrentField()
	{
		std::lock_guard<std::mutex> lock(g_fieldMutex);
		g_fieldX = -1; g_fieldY = -1; g_fieldLen = 0;
		g_fieldText.clear();
	}

	bool GetCurrentField(int& x0, int& y0, int& len)
	{
		std::lock_guard<std::mutex> lock(g_fieldMutex);
		x0 = g_fieldX; y0 = g_fieldY; len = g_fieldLen;
		return g_fieldX >= 0 && g_fieldLen > 0;
	}

	bool FieldEditEnabled() { return g_enabled && g_fieldEditEnabled; }
	void SetFieldEditEnabled(bool enabled) { g_fieldEditEnabled = enabled; }

	bool RunFieldEdit(const FieldEditRequest& request, FieldEditResult& result)
	{
		if (!FieldEditEnabled() || g_stop) return false;

		std::unique_lock<std::mutex> lock(g_editMutex);
		g_request = request;
		g_result = FieldEditResult();
		g_requestPending = true;
		g_requestTaken = false;
		g_resultReady = false;
		g_editCv.wait(lock, [] { return g_resultReady; });
		result = g_result;
		g_resultReady = false;
		return true;
	}

	bool PollFieldEdit(FieldEditRequest& request)
	{
		std::lock_guard<std::mutex> lock(g_editMutex);
		if (!g_requestPending) return false;
		request = g_request;
		g_requestPending = false;
		g_requestTaken = true;
		return true;
	}

	void CompleteFieldEdit(const FieldEditResult& result)
	{
		std::lock_guard<std::mutex> lock(g_editMutex);
		if (!g_requestTaken) return;
		g_result = result;
		g_result.Text[sizeof(g_result.Text) - 1] = 0;
		g_requestTaken = false;
		g_resultReady = true;
		g_editCv.notify_all();
	}
}
