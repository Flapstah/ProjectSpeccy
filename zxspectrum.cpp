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
	, m_scanline(0)
	, m_frameNumber(0)
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
	bool ret = true;

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

		// Timings:
		// Each line is 224 tstates (24 tstates of left border, 128 tstates of
		// screen, 24 tstates of right border and 48 tstates of flyback
		// Each frame is 64 + 192 + 56 lines
	
		double currentTime = glfwGetTime();
		double elapsedTime = currentTime - m_time;

		if (elapsedTime >= (1.0 / 50.0))
		{
			if (updateZ80)
			{
				m_time = currentTime;

				int32 tstates = m_pZ80->ServiceInterrupts();
				for (m_scanline = 0; m_scanline < (SC_TOP_BORDER + SC_PIXEL_SCREEN_HEIGHT + SC_BOTTOM_BORDER); ++m_scanline)
				{
					while (tstates < 224)
					{
						tstates += m_pZ80->SingleStep();
					}
					tstates -= 224;

					UpdateScanline();
				}

				++m_frameNumber;
			}

			ret &= m_pDisplay->Update(this) && !CKeyboard::IsKeyPressed(GLFW_KEY_ESC);
		}

		//glfwSleep(0.005);
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
			if (flash & ((m_frameNumber >> 5) & 0x0001))
			{
				// Flash attribute swaps ink and paper every 32 frames on a real Speccy
				pixel = !pixel;
			}

			uint32 colour = pixel ? inkRGB : paperRGB;
			m_videoMemory[(x + SC_VISIBLE_BORDER_SIZE) + ((y + SC_VISIBLE_BORDER_SIZE) * SC_VIDEO_MEMORY_WIDTH)] = colour;
		}
	}
}

//=============================================================================

void CZXSpectrum::UpdateScanline(void)
{
	if ((m_scanline < (SC_TOP_BORDER - SC_VISIBLE_BORDER_SIZE)) || (m_scanline >= (SC_TOP_BORDER + SC_PIXEL_SCREEN_HEIGHT + SC_VISIBLE_BORDER_SIZE)))
	{
		//printf("CZXSpectrum::UpdateScanline() outside visible bounds %d\n", m_scanline);
		return;
	}

	uint32 scanline = m_scanline - (SC_TOP_BORDER - SC_VISIBLE_BORDER_SIZE);
	uint8* pScreenMemory = &m_memory[SC_SCREEN_START_ADDRESS];

	uint8 borderColour = 7;
	uint32 borderRGB = 0xFF000000;
	if (borderColour & 0x01) borderRGB |= 0x00CD0000;
	if (borderColour & 0x02) borderRGB |= 0x000000CD;
	if (borderColour & 0x04) borderRGB |= 0x0000CD00;

	if ((scanline < SC_VISIBLE_BORDER_SIZE) || (scanline >= (SC_VISIBLE_BORDER_SIZE + SC_PIXEL_SCREEN_HEIGHT)))
	{
		//printf("CZXSpectrum::UpdateScanline() top or bottom border %d\n", m_scanline);
		for (uint32 x = 0; x < SC_VIDEO_MEMORY_WIDTH; ++x)
		{
			m_videoMemory[(scanline * SC_VIDEO_MEMORY_WIDTH) + x] = borderRGB; 
		}
	}
	else
	{
		//printf("CZXSpectrum::UpdateScanline() screen scanline %d, scanline %d\n", m_scanline, scanline);

		uint32 pixelByte = PixelByteIndex(0, scanline - SC_VISIBLE_BORDER_SIZE);
		uint32 attributeByte = AttributeByteIndex(0, scanline - SC_VISIBLE_BORDER_SIZE);
		//printf("CZXSpectrum::UpdateScanline() pixel offset %d attribute offset %d\n", pixelByte, attributeByte);

		for (uint32 x = 0; x < SC_VIDEO_MEMORY_WIDTH; ++x)
		{
			if ((x < SC_VISIBLE_BORDER_SIZE) || (x >= (SC_VISIBLE_BORDER_SIZE + SC_PIXEL_SCREEN_WIDTH)))
			{
				//printf("CZXSpectrum::UpdateScanline() x %d\n", x);
				m_videoMemory[(scanline * SC_VIDEO_MEMORY_WIDTH) + x] = borderRGB; 
			}
			else
			{
				uint32 offset = (x - SC_VISIBLE_BORDER_SIZE) >> 3;
				//printf("CZXSpectrum::UpdateScanline() byte offset %d\n", offset);

				uint8 ink = (pScreenMemory[attributeByte + offset] & 0x07) >> 0;
				uint8 paper = (pScreenMemory[attributeByte + offset] & 0x38) >> 3;
				uint32 bright = (pScreenMemory[attributeByte + offset] & 0x40) ? 0x00FFFFFF : 0x00CDCDCD;
				bool flash = pScreenMemory[attributeByte + offset] & 0x80;

				uint32 paperRGB = 0xFF000000;
				if (paper & 0x01) paperRGB |= 0x00FF0000;
				if (paper & 0x02) paperRGB |= 0x000000FF;
				if (paper & 0x04) paperRGB |= 0x0000FF00;
				paperRGB &= bright;
				uint32 inkRGB = 0xFF000000;
				if (ink & 0x01) inkRGB |= 0x00FF0000;
				if (ink & 0x02) inkRGB |= 0x000000FF;
				if (ink & 0x04) inkRGB |= 0x0000FF00;
				inkRGB &= bright;

				bool pixel = (pScreenMemory[pixelByte + offset] & (1 << (7 - (x & 0x07))));
				if (flash & ((m_frameNumber >> 5) & 0x0001))
				{
					// Flash attribute swaps ink and paper every 32 frames on a real Speccy
					pixel = !pixel;
				}

				uint32 colour = pixel ? inkRGB : paperRGB;
				m_videoMemory[x + (scanline * SC_VIDEO_MEMORY_WIDTH)] = colour;
			}
		}
	}
}

//=============================================================================

