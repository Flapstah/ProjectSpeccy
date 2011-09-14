#include <stdio.h>
#include <stdlib.h>

#include "common/platform_types.h"
#include "common/macros.h"
#include "zxspectrum.h"

int main(int argc, char* argv[])
{
	IGNORE_PARAMETER(argc);
	IGNORE_PARAMETER(argv);

	CZXSpectrum speccy;
	speccy.Initialise();
/*
	speccy.OpenSCR("roms/Fairlight.scr");
	speccy.OpenSCR("roms/KnightLore.scr");
	speccy.OpenSCR("roms/LordsOfMidnightThe.scr");
	speccy.OpenSCR("roms/SabreWulf.scr");
*/
	speccy.LoadROM("roms/48.rom");
/*
  // working
	speccy.LoadSNA("roms/android2.sna");
	speccy.LoadSNA("roms/hobbit.sna");
	// not working
	speccy.LoadSNA("roms/LORDSMID.SNA");
	speccy.LoadRAW("roms/manic.raw");
	speccy.LoadRAW("roms/jsw.raw");
	speccy.LoadRAW("roms/007Spy.raw");
	speccy.LoadRAW("roms/Magnetron.raw");
*/
	speccy.LoadRAW("roms/manic.raw");
	while (speccy.Update());

	return EXIT_SUCCESS;
}
