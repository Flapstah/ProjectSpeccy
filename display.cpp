#include <stdio.h>
#include <stdlib.h>

#include <GL/glfw.h>
#include "includes/glext.h"

#include "display.h"

CDisplay::CDisplay(uint32 width, uint32 height, const char* title)
: m_state(eS_Uninitialised)
{
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialise GLFW\n");
		exit(EXIT_FAILURE);
	}

	m_state = eS_Initialised;

	if (!glfwOpenWindow(width, height, 0, 0, 0, 0, 0, 0, GLFW_WINDOW))
	{
		fprintf(stderr, "Failed to open GLFW window\n");
		exit(EXIT_FAILURE);
	}

	m_state = eS_Window;

	glfwSetWindowTitle(title);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glShadeModel(GL_FLAT);

	// Do texture stuff (http://www.gamedev.net/page/resources/_/reference/programming/opengl/269/opengl-texture-mapping-an-introduction-r947)
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // or GL_NEAREST?
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // or GL_NEAREST?
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	for (uint32 index = 0; index < sizeof(m_videoMemory); ++index)
	{
		m_videoMemory[index] = 128;
	}
}

CDisplay::~CDisplay(void)
{
	switch (m_state)
	{
		case eS_Window: // intentional fall-through
		case eS_Initialised:
			glfwTerminate();
			break;
	}
}

bool CDisplay::Update(void)
{
	int width, height;

	// Get window size (and protect against height being 0)
	glfwGetWindowSize(&width, &height);
	height = (height > 0) ? height : 1;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, (GLdouble)width, 0.0, (GLdouble)height);

	// Clear back buffer
	glClear(GL_COLOR_BUFFER_BIT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, m_videoMemory);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2i((width - WIDTH) / 2, (height - HEIGHT) / 2);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2i((width + WIDTH) / 2, (height - HEIGHT) / 2);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2i((width + WIDTH) / 2, (height + HEIGHT) / 2);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2i((width - WIDTH) / 2, (height + HEIGHT) / 2);
	glEnd();

	glfwSwapBuffers();

	bool cont = ((glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS) && (glfwGetWindowParam(GLFW_OPENED)));

	return cont;
}



