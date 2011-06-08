#include <stdio.h>
#include <stdlib.h>

#include <GL/glfw.h>

#include "render.h"

CRender::CRender(uint32 width, uint32 height, const char* title)
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
}

CRender::~CRender(void)
{
	switch (m_state)
	{
		case eS_Window: // intentional fall-through
		case eS_Initialised:
			glfwTerminate();
			break;
	}
}

bool CRender::Update(void)
{
	int width, height;

	// Get window size (and protect against height being 0)
	glfwGetWindowSize(&width, &height);
	height = (height > 0) ? height : 1;

	glViewport(0, 0, width, height);

	// Clear back buffer
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glfwSwapBuffers();

	bool cont = ((glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS) && (glfwGetWindowParam(GLFW_OPENED)));

	return cont;
}



