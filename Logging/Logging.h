#pragma once

// spdlog se includuje primo, takze volajici muze pouzivat SPDLOG_DEBUG(...) &spol.
// Uroven se resi na dvou mistech:
//   - SPDLOG_ACTIVE_LEVEL (preklad) -- nastaveno v spdlog.props, viz koren repozitare
//   - logger->set_level()  (beh)    -- vychozi debug, prepsatelne promennou SPDLOG_LEVEL
#include <spdlog/spdlog.h>

namespace Log
{
	// Zalozi rotujici log 'fand.log' v adresari FANDWORK (jinak v aktualnim adresari)
	// a nastavi ho jako vychozi spdlog logger. Opakovane volani nic nedela.
	// Kdyz soubor nejde otevrit, logovani se potichu vypne a aplikace bezi dal.
	void Init();

	// Dopise buffery a uvolni loggery. Po zavolani jsou dalsi zapisy no-op.
	void Shutdown();
}
