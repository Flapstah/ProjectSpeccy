#include <stdio.h>
#include <stdlib.h>

#include "zxspectrum.h"

//=============================================================================

CZXSpectrum::CZXSpectrum(void)
{
}

//=============================================================================

CZXSpectrum::~CZXSpectrum(void)
{
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
	uint8 screenData[SC_PIXEL_SCREEN_BYTES + SC_ATTRIBUTES_SCREEN_BYTES];
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

//=============================================================================

bool CZXSpectrum::LoadROM(const char* fileName)
{
	FILE* pFile = fopen(fileName, "rb");
	bool success = false;

	if (pFile != NULL)
	{
		fread(m_memory, sizeof(m_memory), 1, pFile);
		fclose(pFile);

		success = true;
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

