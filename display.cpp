#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glfw.h>
//#include "includes/glext.h"

#include "display.h"

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

	glfwSetWindowTitle(title);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glShadeModel(GL_FLAT);

	// Do texture stuff (http://www.gamedev.net/page/resources/_/reference/programming/opengl/269/opengl-texture-mapping-an-introduction-r947)
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//	glGenTextures(1, (GLuint*)&m_textureID);
//	fprintf(stdout, "m_textureID %i", m_textureID);
	glBindTexture(GL_TEXTURE_2D, eTID_Main/*m_textureID*/);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	memset(m_videoMemory, 0, sizeof(m_videoMemory));
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
	glBindTexture(GL_TEXTURE_2D, eTID_Main/*m_textureID*/);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_videoMemory);

	int32 scaledWidth = m_displayScale * WIDTH;
	int32 scaledHeight = m_displayScale * HEIGHT;
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

	bool cont = ((glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS) && (glfwGetWindowParam(GLFW_OPENED)));

	return cont;
}

bool CDisplay::OpenSCR(const char* fileName)
{
	uint8 screenData[((WIDTH / 8) * HEIGHT) + ATTR_SIZE];
	FILE* pFile = fopen(fileName, "rb");
	bool success = false;

	if (pFile != NULL)
	{
		// load into memory
		fread(screenData, sizeof(screenData), 1, pFile);
		fclose(pFile);

		UpdateScreen(screenData);

		success = true;
	}

	return success;
}

uint32 CDisplay::PixelByteIndex(uint8 x, uint8 y)
{
	uint32 byteOffset =
		// ((y & 0xC0) >> 6) = index to which 3rd of the screen we need (0 based)
		// multiplied by 2048 ( << 11) => shift right by 6 then left by 11 so just left by 5
		((y & 0xC0) << 5) +
		// ((y & 0x38) >> 3) = index to which character line of the screen we need (0 based)
		// multiplied by 32 ( << 5) => shift right by 3 then left by 8 so just left by 2
		((y & 0x38) << 2) +
		// (y & 0x07) << 8 = 256 * pixel row
		((y & 0x07) << 8) +
		// (x & 0xF8) >> 3 = byte in row
		((x & 0xF8) >> 3);
	return byteOffset;
}

uint32 CDisplay::AttributeByteIndex(uint8 x, uint8 y)
{
	uint32 byteOffset = ((WIDTH / 8) * HEIGHT) + (((y & 0xF8) >> 3) * (WIDTH / 8)) + ((x & 0xF8) >> 3);
	return byteOffset;
}

void CDisplay::UpdateScreen(const uint8* pScreenMemory)
{
	// swizzle into texture
	for (uint8 y = 0; y < HEIGHT; ++y)
	{
		for (uint16 x = 0; x < WIDTH; ++x)
		{
			bool pixel = (pScreenMemory[PixelByteIndex(x, y)] & (1 << (7 - (x & 0x07)))) ? true : false;
			uint32 attrOffset = AttributeByteIndex(x, y);

			uint8 ink = (pScreenMemory[attrOffset] & 0x07) >> 0;
			uint8 paper = (pScreenMemory[attrOffset] & 0x38) >> 3;
			uint32 bright = (pScreenMemory[attrOffset] & 0x40) ? 0x00FFFFFF : 0x00CDCDCD;
			bool flash = pScreenMemory[attrOffset] & 0x80;

			uint32 paperRGB = 0xFF000000;
			uint32 inkRGB = 0xFF000000;
			if (paper & 0x01) paperRGB |= 0x00FF0000;
			if (paper & 0x02) paperRGB |= 0x000000FF;
			if (paper & 0x04) paperRGB |= 0x0000FF00;
			paperRGB &= bright;
			if (ink & 0x01) inkRGB |= 0x00FF0000;
			if (ink & 0x02) inkRGB |= 0x000000FF;
			if (ink & 0x04) inkRGB |= 0x0000FF00;
			inkRGB &= bright;

			m_videoMemory[x + (y * WIDTH)] = (pixel) ? inkRGB : paperRGB;
		}
	}
}
