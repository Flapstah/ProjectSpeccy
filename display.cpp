#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glfw.h>
//#include "includes/glext.h"

#include "display.h"

//=============================================================================

CDisplay::CDisplay(uint32 width, uint32 height, const char* title)
: m_state(eS_Uninitialised)
, m_displayScale(1.0f)
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

	glfwSwapInterval(0);
	glfwSetWindowTitle(title);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glShadeModel(GL_FLAT);

	// Do texture stuff (http://www.gamedev.net/page/resources/_/reference/programming/opengl/269/opengl-texture-mapping-an-introduction-r947)
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, eTID_Main);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

//=============================================================================

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

//=============================================================================

bool CDisplay::Update(IScreenMemory* pScreenMemory)
{
	int width, height;

	// Get window size (and protect against height being 0)
	glfwGetWindowSize(&width, &height);
	height = (height > 0) ? height : 1;

	// Set up view
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, (GLdouble)width, 0.0, (GLdouble)height);

	// Clear back buffer
	glClear(GL_COLOR_BUFFER_BIT);

	// Select texture
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, eTID_Main);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pScreenMemory->GetScreenWidth(), pScreenMemory->GetScreenHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, pScreenMemory->GetScreenMemory());

	int32 scaledWidth = static_cast<int32>(m_displayScale * pScreenMemory->GetScreenWidth());
	int32 scaledHeight = static_cast<int32>(m_displayScale * pScreenMemory->GetScreenHeight());
	int32 x = (width - scaledWidth) / 2;
	int32 y = (height - scaledHeight) / 2;

	// Render textured quad
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f);
		glVertex2i(x, y);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2i(x, y + scaledHeight);
		glTexCoord2f(1.0f, 0.0f);
		glVertex2i(x + scaledWidth, y + scaledHeight);
		glTexCoord2f(1.0f, 1.0f);
		glVertex2i(x + scaledWidth, y);
	glEnd();

	glfwSwapBuffers();
	glDisable(GL_TEXTURE_2D);

	bool cont = (glfwGetWindowParam(GLFW_OPENED) == GL_TRUE) ? true : false;

	return cont;
}

//=============================================================================

void CDisplay::SetDisplayScale(float scale)
{
	if (scale < 0.1f)
	{
		scale = 0.1f;
	}

	if (scale > 8.0f)
	{
		scale = 8.0f;
	}

	m_displayScale = scale;
}

//=============================================================================

float CDisplay::GetDisplayScale(void)
{
	return m_displayScale;
}

//=============================================================================

