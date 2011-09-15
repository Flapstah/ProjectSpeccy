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
	speccy.LoadROM("roms/48.rom");
/*
  // working
	speccy.LoadSNA("roms/android2.sna");
	speccy.LoadSNA("roms/hobbit.sna");
	speccy.LoadSNA("roms/zexall.sna");
	speccy.LoadRAW("roms/z80tests.raw");

	// not working
	speccy.LoadSNA("roms/LORDSMID.SNA");
	speccy.LoadRAW("roms/manic.raw");
	speccy.LoadRAW("roms/jsw.raw");
	speccy.LoadRAW("roms/007Spy.raw");
	speccy.LoadRAW("roms/Magnetron.raw");
*/
	speccy.LoadRAW("roms/z80tests.raw");
	while (speccy.Update());

	return EXIT_SUCCESS;
}
