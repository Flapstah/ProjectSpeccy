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
	speccy.LoadTAP("roms/zexbit.tap");
	speccy.LoadTAP("roms/zexfix.tap");
	speccy.LoadTZX("roms/The Lords Of Midnight - Side 1.tzx");
	speccy.LoadTZX("roms/Wheelie.tzx");
	speccy.LoadRAW("roms/manic.raw");
	speccy.LoadRAW("roms/jsw.raw");
	speccy.LoadTZX("roms/Fairlight - 48k - Release 1.tzx");
	speccy.LoadTZX("roms/Magnetron.tzx");

	// not working
*/
	speccy.LoadTZX("roms/Fairlight - 48k - Release 1.tzx");
	while (speccy.Update());

	return EXIT_SUCCESS;
}
