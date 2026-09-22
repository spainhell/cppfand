#include <iostream>
#include <ostream>
#include <windows.h>

#include "../Common/textfunc.h"
#include "../Common/Record.h"
#include "../Core/CfgFile.h"
#include "../Core/GlobalVariables.h"
#include "../Core/runproj.h"
#include "../Core/runfand.h"

#ifdef _EXPORTS
#define FAND_API __declspec(dllexport)
#else
#define FAND_API __declspec(dllimport)
#endif

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
		// cekani na debugger jen na vyzadani (jinak by hostitel v Debug konfiguraci zamrzl)
		if (GetEnvironmentVariableA("FAND_WAIT_DEBUGGER", nullptr, 0) > 0) {
			printf("WAITING FOR DEBUGGER !!!");
			while (!::IsDebuggerPresent())
				::Sleep(1000);
			printf(" ... DONE\n");
		}
#endif
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

Project* rdb = nullptr;
FileD* rdbFile = nullptr;
Record* data = nullptr;
std::string code;


std::string ConvertCP852toUnicode(std::string cp852)
{
	int len = MultiByteToWideChar(852, 0, cp852.c_str(), (int)cp852.length(), NULL, 0);
	uint8_t* uni = new byte[len * 2]{ 0 };
	int result = MultiByteToWideChar(852, 0, cp852.c_str(), (int)cp852.length(), (LPWSTR)uni, len);
	std::string uni_str = std::string((char*)uni, len * 2);
	delete[] uni; uni = nullptr;
	return uni_str;
}

std::string ConvertUnicodetoCP852(char* utf8)
{
	int containsUnsupported;
	int len = WideCharToMultiByte(852, 0, (LPCWCH)utf8, -1, NULL, 0, NULL, &containsUnsupported);
	uint8_t* cp852 = new byte[len]{ 0 };
	int result = WideCharToMultiByte(852, 0, (LPCWCH)utf8, -1, (LPSTR)cp852, len, NULL, &containsUnsupported);
	std::string cp852str = std::string((char*)cp852, len - 1);
	delete[] cp852; cp852 = nullptr;
	return cp852str;
}

extern "C" int FAND_API OpenRDB(char* rdbName)
{
	if (rdbFile != nullptr) {
		return -3; // already opened
	}

	std::string p = rdbName;
	std::string n;

	CfgFile cfgFile;
	cfgFile.Open("FAND.CFG");
	cfgFile.ReadSpec(spec);
	cfgFile.ReadCodeTables(); // for compare strings

	resFile.Open("FAND.RES");
	resFile.ReadInfo(); // read messages

	IsTestRun = true; // debug mode - open files in rdb/W mode
	CompileHelpCatDcl();
	SetTopDir(p, n);
	std::unique_ptr<ProjectRunner> runner = std::make_unique<ProjectRunner>();
	runner->CreateOpenChpt(n, true);

	rdb = CRdb;
	rdbFile = rdb->project_file;
	data = new Record(rdbFile);

	return rdbFile->FF->NRecs;
}

extern "C" int FAND_API GetRecordsCount()
{
	if (rdbFile == nullptr) {
		return -1;
	}
	return rdbFile->FF->NRecs;
}

extern "C" int FAND_API LoadRecord(int32_t recNr)
{
	if (recNr == 0 || recNr > rdbFile->FF->NRecs) {
		return -1;
	}
	rdbFile->FF->ReadRec(recNr, data);
	return 0;
}

extern "C" void FAND_API GetChapterType(char* chapterType)
{
	std::string chapter_type = data->LoadS(rdbFile->FldD[3]);
	memcpy(chapterType, chapter_type.c_str(), chapter_type.length());
}

extern "C" void FAND_API GetChapterName(char* chapterName)
{
	std::string chapter_name = data->LoadS(rdbFile->FldD[4]);
	chapter_name = ConvertCP852toUnicode(chapter_name);
	memcpy(chapterName, chapter_name.c_str(), chapter_name.length());
}

extern "C" int FAND_API GetChapterCodeLength()
{
	code = data->LoadS(rdbFile->FldD[5]);
	code = ConvertCP852toUnicode(code);
	return static_cast<int>(code.length());
}

extern "C" void FAND_API GetChapterCode(char* chapterCode)
{
	memcpy(chapterCode, code.c_str(), code.length());
}

extern "C" int FAND_API CloseRecord(int32_t recNr)
{
	return 0;
}

extern "C" int FAND_API ClearRdb()
{
	rdbFile->FF->NRecs = 0;
	return 0;
}

extern "C" int FAND_API SaveChapter(char* chapterType, char* chapterName, char* chapterCode)
{
	//rdbFile->ClearRecSpace(data);

	std::string chapter_type = ConvertUnicodetoCP852(chapterType);
	data->SaveS(rdbFile->FldD[3], chapter_type);

	std::string chapter_name = ConvertUnicodetoCP852(chapterName);
	data->SaveS(rdbFile->FldD[4], chapter_name);

	std::string chapter_code = ConvertUnicodetoCP852(chapterCode);
	data->SaveS(rdbFile->FldD[5], chapter_code);

	rdbFile->CreateRec(rdbFile->FF->NRecs + 1, data);

	return rdbFile->FF->NRecs;
}

extern "C" int FAND_API UpdateChapter(int32_t recNr, char* chapterType, char* chapterName, char* chapterCode)
{
	//rdbFile->ClearRecSpace(data);

	std::string chapter_type = ConvertUnicodetoCP852(chapterType);
	data->SaveS(rdbFile->FldD[3], chapter_type);

	std::string chapter_name = ConvertUnicodetoCP852(chapterName);
	data->SaveS(rdbFile->FldD[4], chapter_name);

	std::string chapter_code = ConvertUnicodetoCP852(chapterCode);
	data->SaveS(rdbFile->FldD[5], chapter_code);

	rdbFile->UpdateRec(recNr, data);

	return rdbFile->FF->NRecs;
}

extern "C" int FAND_API CloseRdb()
{
	int result = rdbFile->FF->NRecs;
	rdbFile->FF->SaveFile();
	rdbFile->FF->CloseFile();
	rdb = nullptr;
	return result;
}
// ===========================================================================
// Hostitelsky rezim: interpret FANDu bezi na vlastnim vlakne, hostitel (napr. WPF)
// si vyzvedava obrazovku a posila klavesy. Viz Drivers/host.h.
// ===========================================================================
#include <atomic>
#include <thread>
#include "../Core/OldDrivers.h"
#include "../Core/legacy.h"
#include "../Drivers/host.h"
#include "../Logging/Logging.h"

namespace
{
	std::atomic<bool> g_running{ false };
	std::atomic<int> g_exitCode{ 0 };
	std::thread g_thread;
	std::string g_lastError;
}

struct FandScreenInfo
{
	int32_t Cols;
	int32_t Rows;
	int32_t CursorX;      // 0-based
	int32_t CursorY;      // 0-based
	int32_t CursorVisible;
	int32_t CursorSize;   // 1 = normalni, 50 = velky (rezim prepisu)
	int32_t Running;
	int32_t FieldX;       // zvyraznene pole v prohlizecim rezimu, 0-based; -1 = zadne
	int32_t FieldY;
	int32_t FieldLen;
};

/// Spusti interpret na pozadi. fandDir = slozka s FAND.CFG a FAND.RES,
/// workDir = pracovni adresar (odtud se hleda uloha), rdbName = nazev ulohy (identifikator).
/// Vraci 0, nebo -1 pokud uz bezi.
extern "C" int FAND_API FandStart(const char* fandDir, const char* workDir, const char* rdbName)
{
	if (g_running) return -1;
	FandHost::Enable();
	FandHost::SetFieldEditEnabled(true);

	std::string dir = fandDir != nullptr ? fandDir : "";
	std::string wrk = workDir != nullptr ? workDir : "";
	std::string rdb = rdbName != nullptr ? rdbName : "";

	if (g_thread.joinable()) g_thread.join();
	g_running = true;
	g_exitCode = 0;
	g_lastError.clear();

	g_thread = std::thread([dir, wrk, rdb]() {
		if (!wrk.empty()) SetCurrentDirectoryA(wrk.c_str());
		Log::Init();
		SPDLOG_INFO("*** *** *** *** *** *** HOSTED FAND STARTED *** *** *** *** *** ***");
		paramstr.clear();
		std::string exe = dir;
		if (!exe.empty() && exe.back() != '\\') exe += '\\';
		exe += "cppfand.exe"; // FandDir se odvozuje z cesty k programu
		paramstr.push_back(exe);
		if (!rdb.empty()) paramstr.push_back(rdb);
		try {
			InitRunFand();
		}
		catch (FandHost::HaltException& h) {
			g_exitCode = h.Code;
		}
		catch (std::exception& ex) {
			g_lastError = ex.what();
			g_exitCode = -2;
			SPDLOG_CRITICAL("{}", ex.what());
		}
		catch (...) {
			g_lastError = "unknown exception";
			g_exitCode = -3;
		}
		try { DeleteFandFiles(); } catch (...) {}
		SPDLOG_INFO("*** *** *** *** *** ***  HOSTED FAND ENDED   *** *** *** *** *** ***");
		Log::Shutdown(); // dopsat buffery; dalsi FandStart si log otevre znovu
		g_running = false;
	});
	return 0;
}

extern "C" int FAND_API FandIsRunning()
{
	return g_running ? 1 : 0;
}

extern "C" int FAND_API FandExitCode()
{
	return g_exitCode;
}

/// Posledni chybova hlaska (UTF-8 neni, je to ASCII/CP852). Vraci delku.
extern "C" int FAND_API FandLastError(char* buffer, int capacity)
{
	if (buffer == nullptr || capacity <= 0) return (int)g_lastError.length();
	strncpy_s(buffer, capacity, g_lastError.c_str(), _TRUNCATE);
	return (int)g_lastError.length();
}

/// Pozada o zastaveni interpretu (dokonci se pri dalsim cteni klavesnice).
extern "C" void FAND_API FandStop()
{
	FandHost::RequestStop();
}

/// Pocka na ukonceni vlakna interpretu (max. timeoutMs). Vraci 1 = skoncil.
extern "C" int FAND_API FandWait(int timeoutMs)
{
	int waited = 0;
	while (g_running && waited < timeoutMs) { Sleep(20); waited += 20; }
	if (!g_running && g_thread.joinable()) g_thread.join();
	return g_running ? 0 : 1;
}

/// Zkopiruje obrazovku: cells[i] = znak CP852 | (atribut << 8), po radcich.
/// Vraci cislo verze (roste s kazdou zmenou), 0 pri chybe.
extern "C" uint64_t FAND_API FandGetScreen(uint16_t* cells, int capacity, FandScreenInfo* info)
{
	if (cells == nullptr || info == nullptr) return 0;
	int crsX = 0, crsY = 0, crsSize = 1;
	bool visible = false;
	uint64_t version = screen.Snapshot(cells, (size_t)capacity, crsX, crsY, visible, crsSize);
	info->Cols = screen.Cols();
	info->Rows = screen.Rows();
	info->CursorX = crsX;
	info->CursorY = crsY;
	info->CursorVisible = visible ? 1 : 0;
	info->CursorSize = crsSize;
	info->Running = g_running ? 1 : 0;
	int fx = -1, fy = -1, fl = 0;
	FandHost::GetCurrentField(fx, fy, fl);
	info->FieldX = fx;
	info->FieldY = fy;
	info->FieldLen = fl;
	return version;
}

extern "C" uint64_t FAND_API FandScreenVersion()
{
	return screen.Version();
}

/// Vlozi udalost klavesnice ve tvaru KEY_EVENT_RECORD konzole.
/// unicodeChar = znak (Unicode), prevede se do CP852; 0 = bez znaku.
extern "C" void FAND_API FandPushKey(uint16_t virtualKey, uint16_t scanCode, uint16_t unicodeChar, uint32_t controlKeyState, int keyDown)
{
	INPUT_RECORD rec{};
	rec.EventType = KEY_EVENT;
	rec.Event.KeyEvent.bKeyDown = keyDown != 0;
	rec.Event.KeyEvent.wRepeatCount = 1;
	rec.Event.KeyEvent.wVirtualKeyCode = virtualKey;
	rec.Event.KeyEvent.wVirtualScanCode = scanCode;
	rec.Event.KeyEvent.dwControlKeyState = controlKeyState;
	if (unicodeChar != 0) {
		wchar_t wc = (wchar_t)unicodeChar;
		char mb[4] = { 0 };
		BOOL unsupported = FALSE;
		int n = WideCharToMultiByte(852, 0, &wc, 1, mb, sizeof(mb), NULL, &unsupported);
		rec.Event.KeyEvent.uChar.AsciiChar = (n > 0 && !unsupported) ? mb[0] : '?';
	}
	else {
		rec.Event.KeyEvent.uChar.AsciiChar = 0;
	}
	keyboard.PushEvent(rec);
}

/// Vlozi udalost mysi (souradnice v bunkach, 0-based).
extern "C" void FAND_API FandPushMouse(int x, int y, uint32_t buttonState, uint32_t eventFlags, uint32_t controlKeyState)
{
	INPUT_RECORD rec{};
	rec.EventType = MOUSE_EVENT;
	rec.Event.MouseEvent.dwMousePosition = { (short)x, (short)y };
	rec.Event.MouseEvent.dwButtonState = buttonState;
	rec.Event.MouseEvent.dwEventFlags = eventFlags;
	rec.Event.MouseEvent.dwControlKeyState = controlKeyState;
	keyboard.PushEvent(rec);
}

// --- editace pole v hostiteli ----------------------------------------------

extern "C" void FAND_API FandSetFieldEditHost(int enabled)
{
	FandHost::SetFieldEditEnabled(enabled != 0);
}

/// Vyzvedne cekajici pozadavek na editaci pole. Vraci 1, pokud byl.
extern "C" int FAND_API FandPollFieldEdit(FandHost::FieldEditRequest* request)
{
	if (request == nullptr) return 0;
	return FandHost::PollFieldEdit(*request) ? 1 : 0;
}

// --- editace celeho textu -------------------------------------------------
// Text byva velky, takze se nepreleva pres strukturu: Poll oznami jeho delku
// a hostitel si ho vyzvedne zvlast pres FandGetTextEditText.

/// Skalarni cast pozadavku na editaci textu; rozlozeni musi sedet s Native.cs.
struct FandTextEditInfo
{
	int Mode;
	int TextType;
	int Pos;
	int Scroll;
	int Scrolling;
	int ReadOnly;
	int TextLength;      // v bajtech CP852, bez ukoncujici nuly
	int BreakKeyCount;
	unsigned char ColKey[8];
	unsigned char TxtColor;
	unsigned char BlockColor;
	char Name[128];
};

namespace
{
	// pozadavek drzime mezi Poll a Complete, aby si hostitel mohl text vyzvednout
	FandHost::TextEditRequest g_pendingTextEdit;
}

extern "C" void FAND_API FandSetTextEditHost(int enabled)
{
	FandHost::SetTextEditEnabled(enabled != 0);
}

/// Vyzvedne cekajici pozadavek na editaci textu. Vraci 1, pokud byl.
extern "C" int FAND_API FandPollTextEdit(FandTextEditInfo* info)
{
	if (info == nullptr) return 0;
	if (!FandHost::PollTextEdit(g_pendingTextEdit)) return 0;

	info->Mode = g_pendingTextEdit.Mode;
	info->TextType = g_pendingTextEdit.TextType;
	info->Pos = g_pendingTextEdit.Pos;
	info->Scroll = g_pendingTextEdit.Scroll;
	info->Scrolling = g_pendingTextEdit.Scrolling;
	info->ReadOnly = g_pendingTextEdit.ReadOnly;
	info->TextLength = static_cast<int>(g_pendingTextEdit.Text.size());
	info->BreakKeyCount = static_cast<int>(g_pendingTextEdit.BreakKeys.size());
	memcpy(info->ColKey, g_pendingTextEdit.ColKey, sizeof(info->ColKey));
	info->TxtColor = g_pendingTextEdit.TxtColor;
	info->BlockColor = g_pendingTextEdit.BlockColor;
	memcpy(info->Name, g_pendingTextEdit.Name, sizeof(info->Name));
	return 1;
}

/// Zkopiruje text vyzvednuteho pozadavku (CP852, bez ukoncujici nuly).
/// Vraci pocet zapsanych bajtu.
extern "C" int FAND_API FandGetTextEditText(char* buffer, int capacity)
{
	if (buffer == nullptr || capacity <= 0) return 0;
	int len = static_cast<int>(g_pendingTextEdit.Text.size());
	if (len > capacity) len = capacity;
	memcpy(buffer, g_pendingTextEdit.Text.data(), len);
	return len;
}

/// Zkopiruje klavesy, ktere maji editaci ukoncit. Vraci jejich pocet.
extern "C" int FAND_API FandGetTextEditBreakKeys(uint16_t* buffer, int capacity)
{
	if (buffer == nullptr || capacity <= 0) return 0;
	int count = static_cast<int>(g_pendingTextEdit.BreakKeys.size());
	if (count > capacity) count = capacity;
	memcpy(buffer, g_pendingTextEdit.BreakKeys.data(), count * sizeof(uint16_t));
	return count;
}

/// Preda vysledek editace textu a probudi interpret.
/// word = slovo pod kurzorem, v napovede zvoleny odkaz (CP852, muze byt nullptr).
extern "C" void FAND_API FandCompleteTextEdit(const char* text, int textLength, int pos, int scroll, int updated,
	uint16_t key, const char* word, int wordLength)
{
	FandHost::TextEditResult res;
	if (text != nullptr && textLength > 0) res.Text.assign(text, textLength);
	if (word != nullptr && wordLength > 0) res.Word.assign(word, wordLength);
	res.Pos = pos;
	res.Scroll = scroll;
	res.Updated = updated;
	res.Key = key;
	FandHost::CompleteTextEdit(res);
}

/// Preda vysledek editace: text (CP852), pozice kurzoru (1-based), rezim vkladani,
/// ukoncovaci klavesa v kodovani KeyCombination (0x8000 = neznakova, 0x0400 Alt, 0x0200 Ctrl, 0x0100 Shift).
extern "C" void FAND_API FandCompleteFieldEdit(const char* text, int pos, int insertMode, uint16_t key)
{
	FandHost::FieldEditResult res;
	if (text != nullptr) strncpy_s(res.Text, text, sizeof(res.Text) - 1);
	res.Pos = pos;
	res.InsertMode = insertMode;
	res.Key = key;
	FandHost::CompleteFieldEdit(res);
}

/// Cela hodnota zvyrazneneho pole v prohlizecim rezimu (CP852, s nulou na konci).
/// Vraci 1, kdyz nejake pole je; x/y/len jsou souradnice na obrazovce (0-based).
extern "C" int FAND_API FandGetCurrentField(int* x, int* y, int* len, char* text, int capacity)
{
	int fx = -1, fy = -1, fl = 0;
	bool has = FandHost::GetCurrentField(fx, fy, fl);
	if (x) *x = fx;
	if (y) *y = fy;
	if (len) *len = fl;
	if (text && capacity > 0) {
		std::string t = FandHost::GetCurrentFieldText();
		strncpy_s(text, capacity, t.c_str(), _TRUNCATE);
	}
	return has ? 1 : 0;
}
