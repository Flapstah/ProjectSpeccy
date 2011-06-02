#include <stdio.h>
#include <stdlib.h>

#include "engine/common/macros.h"
#include "render.h"

int main(int argc, char* argv[])
{
	IGNORE_PARAMETER(argc);
	IGNORE_PARAMETER(argv);

	CRender render(640, 480, "Test");
	while (render.Update());

	return EXIT_SUCCESS;
}
