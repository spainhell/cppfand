#include "WPage.h"

#include <algorithm>
#include <vector>
#include "WRec.h"

void WPage::Sort(unsigned short N, unsigned short RecLen)
{
	if (N <= 1) return;
	// vytvorime vektor zaznamu a vsechny do nej nacteme z 'A'
	std::vector<WRec> recs;
	size_t offset1 = 0;
	for (size_t i = 0; i < N; i++) {
		WRec r;
		r.Deserialize(&A[offset1]);
		recs.push_back(r);
		offset1 += RecLen;
	}

	// vektor setridime
	std::sort(recs.begin(), recs.end());

	// setridene zaznamy vlozime zpet do 'A'
	size_t offset2 = 0;
	unsigned char buffer[256]{ 0 };
	for (size_t i = 0; i < N; i++) {
		size_t len = recs[i].Serialize(buffer);
		memcpy(&A[offset2], buffer, len);
		offset2 += RecLen;
	}

	// zkotrolujeme delky offsetu pri nacitani a pri ukladani
	// mely by byt stejne
	if (offset1 != offset2) throw std::exception("WPage::Sort() error: Offset1 != Offset2");
}
