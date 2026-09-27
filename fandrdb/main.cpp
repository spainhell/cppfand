// fandrdb - packs and unpacks PC-FAND project files (.RDB + .TTT).
//
// A project file is an ordinary FAND data file with a fixed record
// (help.xml, "projektovy soubor"):
//   TxtPos:F,4.0  Overit:B  StText:T  Typ:A,1  Nazev:A,12  Text:T
// One record = one chapter. Chapters are unpacked to text files
// NNNN_<name>_<type>.txt (NNNN = record number), the same naming as the
// chapter tools in C:\PCFAND\tools use.
//
//   fandrdb unpack [--raw] <file.RDB> <dir>
//   fandrdb pack [--raw] <dir> <file.RDB>
//
// Chapter texts are in code page 852; the text files are UTF-8 unless --raw.

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "../fandio/FileD.h"
#include "../fandio/FileIO.h"
#include "../fandio/Messages.h"
#include "../fandio/Record.h"

namespace fs = std::filesystem;

namespace
{
	FieldDescr* make_field(const char* name, FieldType type, char frml_type, uint8_t L, uint8_t M, uint8_t n_bytes)
	{
		FieldDescr* f = new FieldDescr();
		f->Name = name;
		f->field_type = type;
		f->frml_type = frml_type;
		f->L = L;
		f->M = M;
		f->NBytes = n_bytes;
		f->Flg = f_Stored;
		return f;
	}

	struct ProjectFile
	{
		std::unique_ptr<FileD> file;
		FieldDescr* txt_pos = nullptr;
		FieldDescr* verif = nullptr;
		FieldDescr* old_txt = nullptr;
		FieldDescr* typ = nullptr;
		FieldDescr* name = nullptr;
		FieldDescr* txt = nullptr;
	};

	// FileD of a project file with its predeclared record structure
	ProjectFile make_project_file(const std::string& path)
	{
		ProjectFile p;
		p.file = std::make_unique<FileD>(DataFileType::FandFile, ProgressCallbacks{});
		FileD* f = p.file.get();
		f->Name = fs::path(path).stem().string();
		f->FullPath = path;
		f->FF->file_type = FandFileType::RDB;

		// sizes as the declaration parser (RdFieldDescr) computes them
		p.txt_pos = make_field("TxtPos", FieldType::FIXED, 'R', 5, 0, 2);   // F,4.0
		p.verif = make_field("Overit", FieldType::BOOL, 'B', 1, 0, 1);      // B
		p.old_txt = make_field("StText", FieldType::TEXT, 'S', 1, 0, 4);    // T
		p.typ = make_field("Typ", FieldType::ALFANUM, 'S', 1, LeftJust, 1); // A,1
		p.name = make_field("Nazev", FieldType::ALFANUM, 'S', 12, LeftJust, 12); // A,12
		p.txt = make_field("Text", FieldType::TEXT, 'S', 1, 0, 4);          // T
		f->FldD = { p.txt_pos, p.verif, p.old_txt, p.typ, p.name, p.txt };

		f->GetTFileD(true);
		f->CompileRecLen();
		return p;
	}

	std::string trim_right(std::string s)
	{
		while (!s.empty() && (s.back() == ' ' || s.back() == '\0')) s.pop_back();
		return s;
	}

	// chapter texts are in code page 852, text files in the repository in UTF-8
	std::string convert(const std::string& s, UINT from, UINT to)
	{
		if (s.empty()) return s;
		const int wlen = MultiByteToWideChar(from, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
		std::wstring w(wlen, L'\0');
		MultiByteToWideChar(from, 0, s.data(), static_cast<int>(s.size()), w.data(), wlen);
		const int len = WideCharToMultiByte(to, 0, w.data(), wlen, nullptr, 0, nullptr, nullptr);
		std::string result(len, '\0');
		WideCharToMultiByte(to, 0, w.data(), wlen, result.data(), len, nullptr, nullptr);
		return result;
	}

	const UINT FandCodePage = 852;

	int unpack(const std::string& rdb_path, const std::string& dir, bool raw)
	{
		ProjectFile p = make_project_file(rdb_path);
		FileD* f = p.file.get();
		if (!f->OpenF(rdb_path, RdOnly, true)) {
			fprintf(stderr, "cannot open '%s' (error %lu)\n", rdb_path.c_str(), HandleError);
			return 1;
		}
		if (f->FF->TF->LicenseNr != 0) {
			// texts of such projects are coded (Coding::CodingString)
			fprintf(stderr, "'%s' has license number %d, coded texts are not supported\n", rdb_path.c_str(), f->FF->TF->LicenseNr);
			f->Close();
			return 1;
		}

		fs::create_directories(dir);
		const int n = f->GetNRecs();
		Record record(f);
		for (int i = 1; i <= n; i++) {
			f->ReadRec(i, &record);
			const std::string typ = trim_right(record.LoadS(p.typ));
			const std::string name = trim_right(record.LoadS(p.name));
			const std::string text = raw ? record.LoadS(p.txt) : convert(record.LoadS(p.txt), FandCodePage, CP_UTF8);

			char prefix[16];
			snprintf(prefix, sizeof(prefix), "%04d", i);
			const std::string file_name = std::string(prefix) + "_" + (name.empty() ? "unnamed" : name) + "_" + typ + ".txt";
			std::ofstream out(fs::path(dir) / file_name, std::ios::binary);
			out.write(text.data(), static_cast<std::streamsize>(text.size()));
			printf("%s  %zu B\n", file_name.c_str(), text.size());
		}

		f->Close();
		return 0;
	}

	struct ChapterFile
	{
		int order = 0;
		std::string name;
		std::string typ;
		fs::path path;
	};

	// chapter files NNNN_<name>_<type>.txt of the directory, in the order of NNNN
	std::vector<ChapterFile> list_chapters(const std::string& dir)
	{
		static const std::regex rx(R"(^(\d+)_(.*)_([A-Za-z]?)\.txt$)");
		std::vector<ChapterFile> result;
		for (const auto& entry : fs::directory_iterator(dir)) {
			if (!entry.is_regular_file()) continue;
			const std::string file_name = entry.path().filename().string();
			std::smatch m;
			if (!std::regex_match(file_name, m, rx)) continue;
			ChapterFile c;
			c.order = std::stoi(m[1].str());
			c.name = m[2].str() == "unnamed" ? "" : m[2].str();
			c.typ = m[3].str();
			c.path = entry.path();
			result.push_back(c);
		}
		std::sort(result.begin(), result.end(), [](const ChapterFile& a, const ChapterFile& b) { return a.order < b.order; });
		return result;
	}

	std::string read_file(const fs::path& path)
	{
		std::ifstream in(path, std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
	}

	int pack(const std::string& dir, const std::string& rdb_path, bool raw)
	{
		const std::vector<ChapterFile> chapters = list_chapters(dir);
		if (chapters.empty()) {
			fprintf(stderr, "no chapter files NNNN_<name>_<type>.txt in '%s'\n", dir.c_str());
			return 1;
		}
		for (const ChapterFile& c : chapters) {
			if (c.name.length() > 12) {
				fprintf(stderr, "%s: chapter name longer than 12 characters\n", c.path.filename().string().c_str());
				return 1;
			}
		}

		ProjectFile p = make_project_file(rdb_path);
		FileD* f = p.file.get();
		f->CreateF(true); // .RDB and .TTT, existing files are overwritten

		Record record(f);
		for (const ChapterFile& c : chapters) {
			std::string text = read_file(c.path);
			if (!raw) text = convert(text, CP_UTF8, FandCodePage);

			record.Reset();
			record.SaveR(p.txt_pos, 0);
			record.SaveB(p.verif, false);
			record.SaveS(p.old_txt, "");
			record.SaveS(p.typ, c.typ);
			record.SaveS(p.name, c.name);
			record.SaveS(p.txt, text);
			f->PutRec(&record);
			printf("%s\n", c.path.filename().string().c_str());
		}

		// the chapters have not been compiled yet
		f->FF->TF->CompileAll = true;
		f->FF->TF->SetUpdateFlag();

		f->CloseFile();
		printf("%zu chapters -> %s\n", chapters.size(), rdb_path.c_str());
		return 0;
	}
}

int main(int argc, char* argv[])
{
	std::vector<std::string> args;
	bool raw = false;
	for (int i = 1; i < argc; i++) {
		if (std::string(argv[i]) == "--raw") raw = true;
		else args.push_back(argv[i]);
	}
	if (args.size() != 3) {
		fprintf(stderr,
			"usage: fandrdb unpack [--raw] <file.RDB> <dir>\n"
			"       fandrdb pack [--raw] <dir> <file.RDB>\n"
			"  --raw  keep texts in code page 852 (default: convert to/from UTF-8)\n");
		return 2;
	}

	const std::string& command = args[0];
	try {
		if (command == "unpack") return unpack(args[1], args[2], raw);
		if (command == "pack") return pack(args[1], args[2], raw);
	}
	catch (const fandio::Error& e) {
		fprintf(stderr, "%s\n", e.what());
		return 1;
	}
	catch (const std::exception& e) {
		fprintf(stderr, "error: %s\n", e.what());
		return 1;
	}

	fprintf(stderr, "unknown command '%s'\n", command.c_str());
	return 2;
}
