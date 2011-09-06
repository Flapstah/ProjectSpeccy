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
	speccy.LoadSNA("roms/android2.sna");
	speccy.LoadSNA("roms/LORDSMID.sna");
*/
	while (speccy.Update());

	return EXIT_SUCCESS;
}
