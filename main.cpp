#include <stdio.h>
#include <stdlib.h>

#include "common/platform_types.h"
#include "common/macros.h"
#include "display.h"

int main(int argc, char* argv[])
{
	IGNORE_PARAMETER(argc);
	IGNORE_PARAMETER(argv);

	CDisplay display(640, 480, "Test");
//	display.OpenSCR("roms/Fairlight.scr");
//	display.OpenSCR("roms/KnightLore.scr");
//	display.OpenSCR("roms/LordsOfMidnightThe.scr");
	display.OpenSCR("roms/SabreWulf.scr");
	while (display.Update());

	return EXIT_SUCCESS;
}
