// fandcat - minimal consumer of the fandio library.
//
// It opens a PC-FAND data file (.000), reads its prefix and dumps the first
// records as hex. The program intentionally uses only the fandio API, so it
// serves as a check that fandio can be used outside of CppFand: the project
// links fandio.lib and fandbase.lib with /WHOLEARCHIVE, so any dependency of
// fandio on the rest of the application breaks the build.

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

#include "../fandio/Fand0File.h"
#include "../fandio/FileIO.h"
#include "../fandio/Messages.h"

static void dump_record(int32_t rec_nr, const uint8_t* data, uint16_t len)
{
	printf("#%d", rec_nr);
	for (uint16_t i = 0; i < len; i++) {
		if (i % 16 == 0) printf("\n  %04X ", i);
		printf(" %02X", data[i]);
	}
	printf("\n");
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		fprintf(stderr, "usage: fandcat <file.000> [max_records]\n");
		return 2;
	}

	const std::string path = argv[1];
	const int32_t max_records = argc > 2 ? atoi(argv[2]) : 10;

	HANDLE h = OpenH(path, _isOldFile, RdOnly);
	if (h == nullptr) {
		fprintf(stderr, "cannot open '%s' (error %lu)\n", path.c_str(), HandleError);
		return 1;
	}

	try {
		Fand0File file(nullptr, ProgressCallbacks{});
		file.Handle = h; // closed by the destructor
		file.file_type = FandFileType::FAND16;
		file.FirstRecPos = 6;

		// RecLen is still 0, so RdPrefix() returns the record length stored in the file
		const uint16_t rec_len = file.RdPrefix();
		if (rec_len == 0 || rec_len == 0xffff) {
			fprintf(stderr, "invalid prefix in '%s'\n", path.c_str());
			return 1;
		}
		file.RecLen = rec_len;

		printf("file: %s\nrecords: %d\nrecord length: %u\n", path.c_str(), file.NRecs, rec_len);

		std::unique_ptr<uint8_t[]> buffer = file.GetRecSpaceUnique();
		const int32_t n = file.NRecs < max_records ? file.NRecs : max_records;
		for (int32_t i = 1; i <= n; i++) {
			file.ReadRec(i, buffer.get());
			dump_record(i, buffer.get(), rec_len);
		}
	}
	catch (const fandio::Error& e) {
		fprintf(stderr, "%s\n", e.what());
		return 1;
	}

	return 0;
}
