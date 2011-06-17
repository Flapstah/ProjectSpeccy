#include <stdio.h>
#include <stdlib.h>

#include "common/platform_types.h"
#include "common/macros.h"
#include "display.h"
#include "zxspectrum.h"

int main(int argc, char* argv[])
{
	IGNORE_PARAMETER(argc);
	IGNORE_PARAMETER(argv);

	CZXSpectrum speccy;
	CDisplay display(640, 480, "Test");
/*
	speccy.OpenSCR("roms/Fairlight.scr");
	speccy.OpenSCR("roms/KnightLore.scr");
	speccy.OpenSCR("roms/LordsOfMidnightThe.scr");
	speccy.OpenSCR("roms/SabreWulf.scr");
*/
	speccy.OpenSCR("roms/LordsOfMidnightThe.scr");

	while (display.Update(&speccy));

	return EXIT_SUCCESS;
}
