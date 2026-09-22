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

		// totez pro editaci celeho textu; vlastni zamek, aby se obe cesty nemichaly
		std::atomic<bool> g_textEditEnabled{ true };
		std::mutex g_textMutex;
		std::condition_variable g_textCv;
		bool g_textPending = false;
		bool g_textTaken = false;
		bool g_textReady = false;
		TextEditRequest g_textRequest;
		TextEditResult g_textResult;
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
		// a totez, kdyz ceka na editaci celeho textu
		std::lock_guard<std::mutex> textLock(g_textMutex);
		if (g_textPending || g_textTaken) {
			g_textResult = TextEditResult();
			g_textResult.Key = 27; // Esc
			g_textReady = true;
			g_textPending = false;
			g_textTaken = false;
			g_textCv.notify_all();
		}
	}

	bool StopRequested() { return g_stop; }

	namespace
	{
		std::atomic<int> g_screenCols{ 0 };
		std::atomic<int> g_screenRows{ 0 };
	}

	void SetScreenSize(int cols, int rows)
	{
		// sirka je omezena buffery radku (MaxTxtCols = 132), souradnice okna jsou uint8_t
		g_screenCols = cols <= 0 ? 0 : (cols < 40 ? 40 : (cols > 132 ? 132 : cols));
		g_screenRows = rows <= 0 ? 0 : (rows < 25 ? 25 : (rows > 100 ? 100 : rows));
	}

	void ApplyScreenSize(uint16_t& cols, uint16_t& rows)
	{
		if (g_screenCols > 0) cols = (uint16_t)g_screenCols;
		if (g_screenRows > 0) rows = (uint16_t)g_screenRows;
	}

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

	bool TextEditEnabled() { return g_enabled && g_textEditEnabled; }
	void SetTextEditEnabled(bool enabled) { g_textEditEnabled = enabled; }

	bool RunTextEdit(const TextEditRequest& request, TextEditResult& result)
	{
		if (!TextEditEnabled() || g_stop) return false;

		std::unique_lock<std::mutex> lock(g_textMutex);
		g_textRequest = request;
		g_textResult = TextEditResult();
		g_textPending = true;
		g_textTaken = false;
		g_textReady = false;
		g_textCv.wait(lock, [] { return g_textReady; });
		result = g_textResult;
		g_textReady = false;
		return true;
	}

	bool PollTextEdit(TextEditRequest& request)
	{
		std::lock_guard<std::mutex> lock(g_textMutex);
		if (!g_textPending) return false;
		request = g_textRequest;
		g_textPending = false;
		g_textTaken = true;
		return true;
	}

	void CompleteTextEdit(const TextEditResult& result)
	{
		std::lock_guard<std::mutex> lock(g_textMutex);
		if (!g_textTaken) return;
		g_textResult = result;
		g_textTaken = false;
		g_textReady = true;
		g_textCv.notify_all();
	}
}
