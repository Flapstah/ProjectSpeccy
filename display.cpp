#include <stdio.h>
#include <stdlib.h>

#include <GL/glfw.h>

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
	
	glfwSwapBuffers();

	bool cont = ((glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS) && (glfwGetWindowParam(GLFW_OPENED)));

	return cont;
}



