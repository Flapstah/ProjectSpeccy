#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glfw.h>

#include "zxspectrum.h"
#include "display.h"
#include "keyboard.h"
#include "z80.h"

static CKeyboard g_keyboard;
#define DISPLAY_SCALE (2)

//=============================================================================

CZXSpectrum::CZXSpectrum(void)
	: m_pDisplay(NULL)
	, m_pZ80(NULL)
{
	for (uint32 index = 0; index < (sizeof(m_videoMemory) / 4); ++index)
	{
		m_videoMemory[index] = 0x00CDCDCD;
	}
}

//=============================================================================

CZXSpectrum::~CZXSpectrum(void)
{
	fprintf(stdout, "[ZX Spectrum] Shutting down\n");

	if (m_pDisplay != NULL)
	{
		CKeyboard::Uninitialise();
		delete m_pDisplay;
	}

	if (m_pZ80 != NULL)
	{
		delete m_pZ80;
	}
}

//=============================================================================

bool CZXSpectrum::Initialise(void)
{
	bool initialised = false;

	m_pDisplay = new CDisplay(SC_VIDEO_MEMORY_WIDTH * DISPLAY_SCALE, SC_VIDEO_MEMORY_HEIGHT * DISPLAY_SCALE, "ZX Spectrum");
	if (m_pDisplay != NULL)
	{
		m_pDisplay->SetDisplayScale(DISPLAY_SCALE);
		m_time = glfwGetTime();
		CKeyboard::Initialise();
		if (m_pZ80 = new CZ80(m_memory, 4.0f))
		{
			fprintf(stdout, "[ZX Spectrum] Initialised\n");
			initialised = true;
		}
	}
	
	return initialised;
}

//=============================================================================

bool CZXSpectrum::Update(void)
{
	bool ret = false;

	if ((m_pZ80 != NULL) && (m_pDisplay != NULL))
	{
		if (CKeyboard::IsKeyPressed(GLFW_KEY_F1))
		{
			DisplayHelp();
		}

		if (CKeyboard::IsKeyPressed(GLFW_KEY_F2))
		{
			m_pZ80->SetEnableDebug(!m_pZ80->GetEnableDebug());
		}

		if (CKeyboard::IsKeyPressed(GLFW_KEY_F3))
		{
			m_pZ80->SetEnableOutputStatus(!m_pZ80->GetEnableOutputStatus());
		}

		if (CKeyboard::IsKeyPressed(GLFW_KEY_F5))
		{
			m_pZ80->SetEnableUnattendedDebug(!m_pZ80->GetEnableUnattendedDebug());
		}

		if (CKeyboard::IsKeyPressed(GLFW_KEY_F7))
		{
			m_pZ80->SetEnableBreakpoints(!m_pZ80->GetEnableBreakpoints());
		}

		if (CKeyboard::IsKeyPressed(GLFW_KEY_F8))
		{
			m_pZ80->SetEnableProgramFlowBreakpoints(!m_pZ80->GetEnableProgramFlowBreakpoints());
		}

		bool updateZ80 = !m_pZ80->GetEnableDebug() || m_pZ80->GetEnableUnattendedDebug() || CKeyboard::IsKeyPressed(GLFW_KEY_F9) || CKeyboard::IsKeyPressed(GLFW_KEY_F10);

		if (updateZ80)
		{
			double currentTime = glfwGetTime();
			double elapsedTime = currentTime - m_time;
			m_time = currentTime;

			elapsedTime = (elapsedTime > (1.0/60.0)) ? (1.0/60.0) : elapsedTime;
			m_pZ80->Update(static_cast<float>(elapsedTime * 1000.0));
			UpdateScreen(&m_memory[SC_SCREEN_START_ADDRESS]);
		}

		ret = m_pDisplay->Update(this) && !CKeyboard::IsKeyPressed(GLFW_KEY_ESC);
		glfwSleep(0.005);
	}

	return ret;
}

//=============================================================================

void CZXSpectrum::DisplayHelp(void) const
{
	fprintf(stderr, "[ZX Spectrum]: Help keys:\n");
	fprintf(stderr, "[ZX Spectrum]:      [F1]  Show help\n");
	fprintf(stderr, "[ZX Spectrum]:      [F2]  Toggle debug mode\n");
	fprintf(stderr, "[ZX Spectrum]:      [F3]  Toggle status output\n");
	fprintf(stderr, "[ZX Spectrum]:      [F5]  Toggle unattended debug mode\n");
	fprintf(stderr, "[ZX Spectrum]:      [F7]  Toggle enable break points\n");
	fprintf(stderr, "[ZX Spectrum]:      [F8]  Toggle enable program flow break points\n");
	fprintf(stderr, "[ZX Spectrum]:      [F10] Single step\n");
	fprintf(stderr, "[ZX Spectrum]:      [ESC] Quit\n");
}

//=============================================================================

const void* CZXSpectrum::GetScreenMemory(void) const
{
	return m_videoMemory;
}

//=============================================================================

uint32 CZXSpectrum::GetScreenWidth(void) const
{
	return SC_VIDEO_MEMORY_WIDTH;
}

//=============================================================================

uint32 CZXSpectrum::GetScreenHeight(void) const
{
	return SC_VIDEO_MEMORY_HEIGHT;
}

//=============================================================================

bool CZXSpectrum::OpenSCR(const char* fileName)
{
	FILE* pFile = fopen(fileName, "rb");
	bool success = false;

	if (pFile != NULL)
	{
		// load into memory
		fread(&m_memory[SC_SCREEN_START_ADDRESS], sizeof(SC_SCREEN_SIZE_BYTES), 1, pFile);
		fclose(pFile);

		UpdateScreen(&m_memory[SC_SCREEN_START_ADDRESS]);

		fprintf(stdout, "[ZX Spectrum]: loaded screen [%s] successfully\n", fileName);
		success = true;
	}
	else
	{
		fprintf(stderr, "[ZX Spectrum]: failed to load screen [%s]\n", fileName);
	}

	return success;
}

//=============================================================================

bool CZXSpectrum::LoadROM(const char* fileName)
{
	memset(m_memory, 0, sizeof(m_memory));

	FILE* pFile = fopen(fileName, "rb");
	bool success = false;

	if (pFile != NULL)
	{
		fread(m_memory, sizeof(m_memory), 1, pFile);
		fclose(pFile);

		fprintf(stdout, "[ZX Spectrum]: loaded rom [%s] successfully\n", fileName);

		success = true;
	}
	else
	{
		fprintf(stderr, "[ZX Spectrum]: failed to load rom [%s]\n", fileName);
	}

	return success;
}

//=============================================================================

bool CZXSpectrum::LoadSNA(const char* fileName)
{
	FILE* pFile = fopen(fileName, "rb");
	bool success = false;

	char scratch[49179];
	if (pFile != NULL)
	{
		fread(scratch, sizeof(scratch), 1, pFile);
		fclose(pFile);

		fprintf(stdout, "[ZX Spectrum]: loaded [%s] successfully\n", fileName);

		memcpy(&m_memory[SC_SCREEN_START_ADDRESS], &scratch[27], SC_48K_SPECTRUM - SC_SCREEN_START_ADDRESS);
		m_pZ80->LoadSNA(reinterpret_cast<uint8*>(scratch));

		success = true;
	}
	else
	{
		fprintf(stderr, "[ZX Spectrum]: failed to load rom [%s]\n", fileName);
	}

	return success;
}

//=============================================================================

void CZXSpectrum::UpdateScreen(const uint8* pScreenMemory)
{
	// swizzle into texture
	for (uint32 y = 0; y < SC_PIXEL_SCREEN_HEIGHT; ++y)
	{
		for (uint32 x = 0; x < SC_PIXEL_SCREEN_WIDTH; ++x)
		{
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

			bool pixel = (pScreenMemory[PixelByteIndex(x, y)] & (1 << (7 - (x & 0x07))));
			// Flash attribute swaps ink and paper every 32 frames on a real Speccy
			// So as a rough test I need to flash every 32/50 of a second...
			if (flash && (static_cast<uint32>(m_time * 50.0f / 32.0f) & 0x0001))
			{
				pixel = !pixel;
			}

			uint32 colour = pixel ? inkRGB : paperRGB;
			m_videoMemory[(x + SC_BORDER_SIZE) + ((y + SC_BORDER_SIZE) * SC_VIDEO_MEMORY_WIDTH)] = colour;
		}
	}
}

//=============================================================================

