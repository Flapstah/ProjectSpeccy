#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glfw.h>

#include "zxspectrum.h"
#include "display.h"
#include "z80.h"

//=============================================================================

CZXSpectrum::CZXSpectrum(void)
	: m_pDisplay(NULL)
	, m_pZ80(NULL)
	, m_detectROMCorruption(false)
{
	memset(m_keyState, 0, sizeof(m_keyState) * sizeof(bool));
}

//=============================================================================

CZXSpectrum::~CZXSpectrum(void)
{
	fprintf(stdout, "[ZX Spectrum] Shutting down\n");

	if (m_pDisplay != NULL)
	{
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

	m_pDisplay = new CDisplay(640, 480, "ZX Spectrum");
	if (m_pDisplay != NULL)
	{
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

	UpdateKeyboard();

	if ((m_pZ80 != NULL) && (m_pDisplay != NULL))
	{
		if (IsKeyPressed(GLFW_KEY_F1))
		{
			m_pZ80->SetEnableDebug(!m_pZ80->GetEnableDebug());
		}

		if (IsKeyPressed(GLFW_KEY_F2))
		{
			m_pZ80->SetEnableUnattendedDebug(!m_pZ80->GetEnableUnattendedDebug());
		}

		if (IsKeyPressed(GLFW_KEY_F3))
		{
			m_pZ80->SetEnableOutputStatus(!m_pZ80->GetEnableOutputStatus());
		}

		if (IsKeyPressed(GLFW_KEY_F4))
		{
			m_pZ80->SetEnableBreakpoints(!m_pZ80->GetEnableBreakpoints());
		}

		if (IsKeyPressed(GLFW_KEY_F5))
		{
			m_pZ80->SetEnableProgramFlowBreakpoints(!m_pZ80->GetEnableProgramFlowBreakpoints());
		}

		if (IsKeyPressed(GLFW_KEY_F6))
		{
			m_detectROMCorruption = !m_detectROMCorruption;
			fprintf(stderr, "[ZX Spectrum] Turning %s ROM corruption detection mode\n", (m_detectROMCorruption) ? "on" : "off");
		}

		bool updateZ80 = !m_pZ80->GetEnableDebug() || m_pZ80->GetEnableUnattendedDebug() || IsKeyPressed(GLFW_KEY_SPACE);

		if (updateZ80)
		{
			m_pZ80->Update(10.0f);
			UpdateScreen(&m_memory[SC_SCREEN_START_ADDRESS]);
		}

		if (m_pZ80->GetEnableDebug() || m_detectROMCorruption)
		{
			if (!memcmp(m_shadowMemory, m_memory, SC_SCREEN_START_ADDRESS))
			{
				fprintf(stderr, "[ZX Spectrum]: ROM corruption detected!\n");
				m_pZ80->SetEnableDebug(true);
			}
		}

		ret = m_pDisplay->Update(this) && !IsKeyPressed(GLFW_KEY_ESC);
	}

	return ret;
}

//=============================================================================

void CZXSpectrum::UpdateKeyboard(void)
{
	memcpy(m_keyPrevState, m_keyState, sizeof(m_keyPrevState));

	m_keyState[GLFW_KEY_F1] = (glfwGetKey(GLFW_KEY_F1) == GLFW_PRESS);				// toggle debug mode
	m_keyState[GLFW_KEY_F2] = (glfwGetKey(GLFW_KEY_F2) == GLFW_PRESS);				// toggle unattended debug mode
	m_keyState[GLFW_KEY_F3] = (glfwGetKey(GLFW_KEY_F3) == GLFW_PRESS);				// toggle status output
	m_keyState[GLFW_KEY_F4] = (glfwGetKey(GLFW_KEY_F4) == GLFW_PRESS);				// toggle enable breakpoints
	m_keyState[GLFW_KEY_F5] = (glfwGetKey(GLFW_KEY_F5) == GLFW_PRESS);				// toggle enable program flow breakpoints
	m_keyState[GLFW_KEY_F6] = (glfwGetKey(GLFW_KEY_F6) == GLFW_PRESS);				// toggle ROM corruption detection
	m_keyState[GLFW_KEY_SPACE] = (glfwGetKey(GLFW_KEY_SPACE) == GLFW_PRESS);	// single step
	m_keyState[GLFW_KEY_ESC] = (glfwGetKey(GLFW_KEY_ESC) == GLFW_PRESS);			// quit
}

//=============================================================================

bool CZXSpectrum::IsKeyPressed(uint32 keyID) const
{
	return (m_keyState[keyID] && !m_keyPrevState[keyID]);
}

//=============================================================================

bool CZXSpectrum::IsKeyHeld(uint32 keyID) const
{
	return (m_keyState[keyID] && m_keyPrevState[keyID]);
}

//=============================================================================

const void* CZXSpectrum::GetScreenMemory(void) const
{
	return m_videoMemory;
}

//=============================================================================

uint32 CZXSpectrum::GetScreenWidth(void) const
{
	return SC_PIXEL_SCREEN_WIDTH;
}

//=============================================================================

uint32 CZXSpectrum::GetScreenHeight(void) const
{
	return SC_PIXEL_SCREEN_HEIGHT;
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

		memcmp(m_shadowMemory, m_memory, SC_48K_SPECTRUM);
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
	for (uint8 y = 0; y < SC_PIXEL_SCREEN_HEIGHT; ++y)
	{
		for (uint16 x = 0; x < SC_PIXEL_SCREEN_WIDTH; ++x)
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

			uint32 colour = (pScreenMemory[PixelByteIndex(x, y)] & (1 << (7 - (x & 0x07)))) ? inkRGB : paperRGB;
			m_videoMemory[x + (y * SC_PIXEL_SCREEN_WIDTH)] = colour;
		}
	}
}

//=============================================================================

