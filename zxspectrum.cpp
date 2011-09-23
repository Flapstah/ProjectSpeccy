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
#define MAX_CLOCKRATE_MULTIPLIER (64.0f)
#define MIN_CLOCKRATE_MULTIPLIER (0.5f)
//#define SHOW_FRAMERATE

// TODO:
// fix up UpdateScanline() to use new enumerated constants

//=============================================================================

CZXSpectrum::CZXSpectrum(void)
	: m_frameTime(1.0 / 50.0)
	, m_clockRate(1.0f)
	, m_pDisplay(NULL)
	, m_pZ80(NULL)
	, m_pFile(NULL)
	, m_scanline(0)
	, m_xpos(0)
	, m_frameNumber(0)
	, m_scanlineTstates(0)
	, m_tapeTstates(0)
	, m_writePortFE(0)
	, m_readPortFE(0)
	, m_tapePlaying(false)
	, m_tapeFormat(TC_FORMAT_UNKNOWN)
	, m_tapeState(TC_STATE_READING_FORMAT)
{
}

//=============================================================================

CZXSpectrum::~CZXSpectrum(void)
{
	fprintf(stdout, "[ZX Spectrum]: Shutting down\n");

	if (m_pFile != NULL)
	{
		fclose(m_pFile);
	}

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
		if (m_pZ80 = new CZ80(this))
		{
			fprintf(stdout, "[ZX Spectrum]: Initialised\n");
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

		if (CKeyboard::IsKeyPressed(GLFW_KEY_PAGEUP))
		{
			if (m_pFile != NULL)
			{
				m_tapePlaying = !m_tapePlaying;
				fprintf(stdout, "[ZX Spectrum]: tape is now %s\n", m_tapePlaying ? "playing" : "stopped");
			}
			else
			{
				fprintf(stdout, "[ZX Spectrum]: no tape loaded\n");
			}
		}

		if (CKeyboard::IsKeyPressed(GLFW_KEY_HOME))
		{
			if (m_pFile != NULL)
			{
				fseek(m_pFile, 0, SEEK_SET);
				fprintf(stdout, "[ZX Spectrum]: tape is now at start (and %s)\n", m_tapePlaying ? "playing" : "stopped");
			}
			else
			{
				fprintf(stdout, "[ZX Spectrum]: no tape loaded\n");
			}
		}

		if (CKeyboard::IsKeyPressed(GLFW_KEY_UP))
		{
			if (m_clockRate < MAX_CLOCKRATE_MULTIPLIER)
			{
				m_clockRate *= 2.0f;
				m_frameTime = (1.0 / 50.0) / m_clockRate;
				fprintf(stdout, "[ZX Spectrum]: increased emulation speed to %.02f\n", m_clockRate);
			}
		}

		if (CKeyboard::IsKeyPressed(GLFW_KEY_DOWN))
		{
			if (m_clockRate > MIN_CLOCKRATE_MULTIPLIER)
			{
				m_clockRate /= 2.0f;
				m_frameTime = (1.0 / 50.0) / m_clockRate;
				fprintf(stdout, "[ZX Spectrum]: decreased emulation speed to %.02f\n", m_clockRate);
			}
		}

		bool updateZ80 = !m_pZ80->GetEnableDebug() || m_pZ80->GetEnableUnattendedDebug() || CKeyboard::IsKeyPressed(GLFW_KEY_F9) || CKeyboard::IsKeyPressed(GLFW_KEY_F10);

		// Timings:
		// Each line is 224 tstates (24 tstates of left border, 128 tstates of
		// screen, 24 tstates of right border and 48 tstates of flyback
		// Each frame is 64 + 192 + 56 lines
	
		double currentTime = glfwGetTime();
		double elapsedTime = currentTime - m_time;

		if (elapsedTime >= m_frameTime)
		{
			if (updateZ80)
			{
				m_time = currentTime;

				uint32 tstates = m_pZ80->ServiceInterrupts();
				UpdateTape(tstates);
				for (m_scanline = 0; m_scanline < (SC_TOP_BORDER + SC_PIXEL_SCREEN_HEIGHT + SC_BOTTOM_BORDER); ++m_scanline)
				{
					while (m_scanlineTstates < 224)
					{
						tstates = m_pZ80->SingleStep();
						m_scanlineTstates += tstates;
						UpdateTape(tstates);
					}
					m_scanlineTstates -= 224;

					UpdateScanline();
				}

				++m_frameNumber;

#if defined(SHOW_FRAMERATE)
				static double startTime = currentTime;
				static uint32 startFrame = m_frameNumber;
				if ((currentTime - startTime) > 1.0)
				{
					double framerate = (double)(m_frameNumber - startFrame) / (currentTime - startTime);
					fprintf(stdout, "[ZX Spectrum]: average frame rate is %.02f\n", framerate);
					startFrame = m_frameNumber;
					startTime = currentTime;
				}
#endif // defined(SHOW_FRAMERATE)
			}

			ret &= m_pDisplay->Update(this) && !CKeyboard::IsKeyPressed(GLFW_KEY_ESC);
		}
	}

	return ret;
}

//=============================================================================

void CZXSpectrum::DisplayHelp(void) const
{
	fprintf(stderr, "[ZX Spectrum]: Help keys:\n");
	fprintf(stderr, "[ZX Spectrum]:      [F1]     Show help\n");
	fprintf(stderr, "[ZX Spectrum]:      [F2]     Toggle debug mode\n");
	fprintf(stderr, "[ZX Spectrum]:      [F3]     Toggle status output\n");
	fprintf(stderr, "[ZX Spectrum]:      [F5]     Toggle unattended debug mode\n");
	fprintf(stderr, "[ZX Spectrum]:      [F7]     Toggle enable break points\n");
	fprintf(stderr, "[ZX Spectrum]:      [F8]     Toggle enable program flow break points\n");
	fprintf(stderr, "[ZX Spectrum]:      [F9/F10] Single step\n");
	fprintf(stderr, "[ZX Spectrum]:      [PgUp]   Start/stop tape\n");
	fprintf(stderr, "[ZX Spectrum]:      [Home]   Rewind tape\n");
	fprintf(stderr, "[ZX Spectrum]:      [Up]     Increase emulation speen\n");
	fprintf(stderr, "[ZX Spectrum]:      [Down]   Decrease emulation speen\n");
	fprintf(stderr, "[ZX Spectrum]:      [ESC]    Quit\n");
}

//=============================================================================

void CZXSpectrum::WriteMemory(uint16 address, uint8 byte)
{
	if (address >= 0x4000)
	{
		m_memory[address] = byte;
	}
	else
	{
		m_pZ80->HitBreakpoint("write to ROM");
	}
}

//=============================================================================

uint8 CZXSpectrum::ReadMemory(uint16 address) const
{
	return m_memory[address];
}

//=============================================================================

void CZXSpectrum::WritePort(uint16 address, uint8 byte)
{
	switch (address & 0x00FF)
	{
		case 0x00FE:
			// +---+---+---+---+---+---+---+---+
			// |   |   |   | E | M |  Border   |
			// +---+---+---+---+---+---+---+---+
			m_writePortFE = byte & PC_OUTPUT_MASK;
			break;

		default:
			fprintf(stderr, "[ZX Spectrum]: WritePort for unhandled address %04X, data %02X\n", address, byte);
			break;
	}
}

//=============================================================================

uint8 CZXSpectrum::ReadPort(uint16 address) const
{
	if (address & 0x0001)
	{
		return 0xFF;
	}

	// +---+---+---+---+---+---+---+---+
	// | 1 | E | 1 | <-half row keys-> |
	// +---+---+---+---+---+---+---+---+
	// keys: 0=pressed; 1=not pressed

	m_readPortFE |= 0xBF;
	uint8 mask = 0x00;
	uint8 line = ~(address >> 8);

	if (line & 0x01)
	{
		// SHIFT, Z, X, C, V
		if (glfwGetKey(GLFW_KEY_LSHIFT) == GLFW_PRESS) mask |= 0x01;
		if (glfwGetKey('Z') == GLFW_PRESS) mask |= 0x02;
		if (glfwGetKey('X') == GLFW_PRESS) mask |= 0x04;
		if (glfwGetKey('C') == GLFW_PRESS) mask |= 0x08;
		if (glfwGetKey('V') == GLFW_PRESS) mask |= 0x10;
	}

	if (line & 0x02)
	{
		// A, S, D, F, G
		if (glfwGetKey('A') == GLFW_PRESS) mask |= 0x01;
		if (glfwGetKey('S') == GLFW_PRESS) mask |= 0x02;
		if (glfwGetKey('D') == GLFW_PRESS) mask |= 0x04;
		if (glfwGetKey('F') == GLFW_PRESS) mask |= 0x08;
		if (glfwGetKey('G') == GLFW_PRESS) mask |= 0x10;
	}

	if (line & 0x04) // Q, W, E, R, T
	{
		if (glfwGetKey('Q') == GLFW_PRESS) mask |= 0x01;
		if (glfwGetKey('W') == GLFW_PRESS) mask |= 0x02;
		if (glfwGetKey('E') == GLFW_PRESS) mask |= 0x04;
		if (glfwGetKey('R') == GLFW_PRESS) mask |= 0x08;
		if (glfwGetKey('T') == GLFW_PRESS) mask |= 0x10;
	}

	if (line & 0x08) // 1, 2, 3, 4, 5
	{
		if (glfwGetKey('1') == GLFW_PRESS) mask |= 0x01;
		if (glfwGetKey('2') == GLFW_PRESS) mask |= 0x02;
		if (glfwGetKey('3') == GLFW_PRESS) mask |= 0x04;
		if (glfwGetKey('4') == GLFW_PRESS) mask |= 0x08;
		if (glfwGetKey('5') == GLFW_PRESS) mask |= 0x10;
	}

	if (line & 0x10) // 0, 9, 8, 7, 6
	{
		if (glfwGetKey('0') == GLFW_PRESS) mask |= 0x01;
		if (glfwGetKey('9') == GLFW_PRESS) mask |= 0x02;
		if (glfwGetKey('8') == GLFW_PRESS) mask |= 0x04;
		if (glfwGetKey('7') == GLFW_PRESS) mask |= 0x08;
		if (glfwGetKey('6') == GLFW_PRESS) mask |= 0x10;
	}

	if (line & 0x20) // P, O, I, U, Y
	{
		if (glfwGetKey('P') == GLFW_PRESS) mask |= 0x01;
		if (glfwGetKey('O') == GLFW_PRESS) mask |= 0x02;
		if (glfwGetKey('I') == GLFW_PRESS) mask |= 0x04;
		if (glfwGetKey('U') == GLFW_PRESS) mask |= 0x08;
		if (glfwGetKey('Y') == GLFW_PRESS) mask |= 0x10;
	}

	if (line & 0x40) // ENTER, L, K, J, H
	{
		if (glfwGetKey(GLFW_KEY_ENTER) == GLFW_PRESS) mask |= 0x01;
		if (glfwGetKey('L') == GLFW_PRESS) mask |= 0x02;
		if (glfwGetKey('K') == GLFW_PRESS) mask |= 0x04;
		if (glfwGetKey('J') == GLFW_PRESS) mask |= 0x08;
		if (glfwGetKey('H') == GLFW_PRESS) mask |= 0x10;
	}

	if (line & 0x80) // SPACE, SYM SHIFT, M, N, B
	{
		if (glfwGetKey(GLFW_KEY_SPACE) == GLFW_PRESS) mask |= 0x01;
		if (glfwGetKey(GLFW_KEY_RSHIFT) == GLFW_PRESS) mask |= 0x02;
		if (glfwGetKey('M') == GLFW_PRESS) mask |= 0x04;
		if (glfwGetKey('N') == GLFW_PRESS) mask |= 0x08;
		if (glfwGetKey('B') == GLFW_PRESS) mask |= 0x10;
	}

	m_readPortFE &= ~mask;
//	if ((m_readPortFE & 0x1F) != 0x1F)
//	{
//		printf("[ZX Spectrum]: Port %04X is %02X\n", address, m_readPortFE);
//	}

	return m_readPortFE;
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

bool CZXSpectrum::LoadROM(const char* fileName)
{
	memset(m_memory, 0, sizeof(m_memory));
	m_pFile = fopen(fileName, "rb");
	bool success = false;

	if (m_pFile != NULL)
	{
		size_t result = fread(m_memory, sizeof(m_memory), 1, m_pFile);
		fclose(m_pFile);
		m_pFile = NULL;

		fprintf(stdout, "[ZX Spectrum]: loaded rom [%s] successfully\n", fileName);

		success = true;
	}
	else
	{
		fprintf(stderr, "[ZX Spectrum]: failed to load [%s]\n", fileName);
	}

	return success;
}

//=============================================================================

bool CZXSpectrum::LoadSNA(const char* fileName)
{
	m_pFile = fopen(fileName, "rb");
	bool success = false;

	char scratch[49179];
	if (m_pFile != NULL)
	{
		size_t result = fread(scratch, sizeof(scratch), 1, m_pFile);
		fclose(m_pFile);
		m_pFile = NULL;

		fprintf(stdout, "[ZX Spectrum]: loaded SNA [%s] successfully\n", fileName);

		memcpy(&m_memory[SC_SCREEN_START_ADDRESS], &scratch[27], SC_48K_SPECTRUM - SC_SCREEN_START_ADDRESS);
		m_pZ80->LoadSNA(reinterpret_cast<uint8*>(scratch));

		success = true;
	}
	else
	{
		fprintf(stderr, "[ZX Spectrum]: failed to load [%s]\n", fileName);
	}

	return success;
}

//=============================================================================

bool CZXSpectrum::LoadRAW(const char* fileName)
{
	m_pFile = fopen(fileName, "rb");
	bool success = false;

	if (m_pFile != NULL)
	{
		fprintf(stdout, "[ZX Spectrum]: opened RAW [%s] successfully\n", fileName);
		success = true;
		m_tapeFormat = TC_FORMAT_RAW;
	}
	else
	{
		fprintf(stderr, "[ZX Spectrum]: failed to load [%s]\n", fileName);
	}

	return success;
}

//=============================================================================

bool CZXSpectrum::LoadTAP(const char* fileName)
{
	m_pFile = fopen(fileName, "rb");
	bool success = false;

	if (m_pFile != NULL)
	{
		fprintf(stdout, "[ZX Spectrum]: opened TAP [%s] successfully\n", fileName);
		success = true;
		m_tapeFormat = TC_FORMAT_TAP;
	}
	else
	{
		fprintf(stderr, "[ZX Spectrum]: failed to load [%s]\n", fileName);
	}

	return success;
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

	uint32 borderRGB = 0xFF000000;
	if (m_writePortFE & CC_BLUE) borderRGB |= 0x00CD0000;
	if (m_writePortFE & CC_RED) borderRGB |= 0x000000CD;
	if (m_writePortFE & CC_GREEN) borderRGB |= 0x0000CD00;

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
				bool flash = (pScreenMemory[attributeByte + offset] & 0x80) ? true : false;

				uint32 paperRGB = 0xFF000000;
				if (paper & CC_BLUE) paperRGB |= 0x00FF0000;
				if (paper & CC_RED) paperRGB |= 0x000000FF;
				if (paper & CC_GREEN) paperRGB |= 0x0000FF00;
				paperRGB &= bright;
				uint32 inkRGB = 0xFF000000;
				if (ink & CC_BLUE) inkRGB |= 0x00FF0000;
				if (ink & CC_RED) inkRGB |= 0x000000FF;
				if (ink & CC_GREEN) inkRGB |= 0x0000FF00;
				inkRGB &= bright;

				bool pixel = ((pScreenMemory[pixelByte + offset] & (1 << (7 - (x & 0x07))))) ? true : false;
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

void CZXSpectrum::UpdateTape(uint32 tstates)
{
	int tapeError = 0;
	bool stopTape = false;

	if (!m_tapePlaying)
		return;
	
	// RAW images are 44100 Hz => 44100 bytes/sec (1 byte in file = 1 bit)
	// Screen refresh is (64+192+56)*224=69888 T states long
	// 3.5Mhz/69888=50.080128205Hz refresh rate (let's call it 50Hz)
	// 44100bps/50.080128205Hz=880.588800002 bits per screen refresh
	// (44100bps/50Hz=882 bits per screen refresh)
	// 69888/880.588800002=79.365079365 tstates per bit
	// (69888/882=79.238095238 tstates per bit)
	// rounded to 79 tstates for simplicity

	m_tapeTstates += tstates;

	switch (m_tapeFormat)
	{
		case TC_FORMAT_RAW:
			if (m_tapeTstates >= 79)
			{
				m_tapeTstates -= 79;

				if (fread(&m_tapeByte, sizeof(m_tapeByte), 1, m_pFile))
				{
					m_readPortFE &= ~PC_EAR_IN;
					m_readPortFE |= (m_tapeByte & 0x80) ? PC_EAR_IN : 0;
				}
				else
				{
					if (feof(m_pFile))
					{
						fprintf(stdout, "[ZX Spectrum]: tape reached end - rewound and stopped\n");
						m_clockRate = 1.0f;
						m_frameTime = (1.0 / 50.0) / m_clockRate;
						fprintf(stdout, "[ZX Spectrum]: emulation speed set to %.02f\n", m_clockRate);
					}
					else
					{
						tapeError = ferror(m_pFile);
					}

					stopTape = true;
				}
			}
			break;

		case TC_FORMAT_TAP:
			switch (m_tapeState)
			{
				case TC_STATE_READING_FORMAT:
					printf("TC_STATE_READING_FORMAT\n");
					if (fread(&m_tapeByte, sizeof(m_tapeByte), 1, m_pFile))
					{
						m_tapeBlockSize = m_tapeByte;
						if (fread(&m_tapeByte, sizeof(m_tapeByte), 1, m_pFile))
						{
							m_tapeBlockSize |= m_tapeByte << 8;
							if (fread(&m_tapeBlockType, sizeof(m_tapeBlockType), 1, m_pFile))
							{
								switch (m_tapeBlockType)
								{
									case TC_BLOCK_TYPE_HEADER:
										m_tapePulseCounter = 8063;
										break;
									case TC_BLOCK_TYPE_DATA:
										m_tapePulseCounter = 3223;
										break;
								}
							}
							else
							{
								tapeError = ferror(m_pFile);
							}
						}
						else
						{
							tapeError = ferror(m_pFile);
						}
					}
					else
					{
						tapeError = ferror(m_pFile);
					}
					m_tapeState = TC_STATE_GENERATING_PILOT;
					printf("TC_STATE_GENERATING_PILOT\n");
					// intentional fall-through

				case TC_STATE_GENERATING_PILOT:
					if (m_tapeTstates >= 2168)
					{
						m_tapeTstates -= 2168;
						m_readPortFE ^= PC_EAR_IN;
						if (m_tapePulseCounter-- == 0)
						{
							m_tapeState = TC_STATE_GENERATING_SYNC_PULSE_0;
							printf("TC_STATE_GENERATING_SYNC_PULSE_0\n");
						}
					}
					break;

				case TC_STATE_GENERATING_SYNC_PULSE_0:
					if (m_tapeTstates >= 667)
					{
						m_tapeTstates -= 667;
						m_readPortFE ^= PC_EAR_IN;
						m_tapeState = TC_STATE_GENERATING_SYNC_PULSE_1;
						printf("TC_STATE_GENERATING_SYNC_PULSE_1\n");
					}
					break;

				case TC_STATE_GENERATING_SYNC_PULSE_1:
					if (m_tapeTstates >= 735)
					{
						m_tapeTstates -= 735;
						m_readPortFE ^= PC_EAR_IN;

						m_tapeByte = m_tapeBlockType;
						m_tapeBitMask = 0x80;
						m_tapeState = TC_STATE_GENERATING_DATA;
						printf("TC_STATE_GENERATING_DATA\n");
					}
					break;

				case TC_STATE_GENERATING_DATA:
					if (m_tapeBitMask == 0)
					{
						if (m_tapeBlockSize-- > 0)
						{
							if (fread(&m_tapeByte, sizeof(m_tapeByte), 1, m_pFile))
							{
								m_tapeBitMask = 0x80;
								m_tapePulseCounter = 2;
							}
							else
							{
								tapeError = ferror(m_pFile);
								break;
							}
						}
						else
						{
							m_tapeState = TC_STATE_READING_FORMAT;
							break;
						}
					}

					if (m_tapeByte & m_tapeBitMask)
					{
						// Generate a 1
						if (m_tapeTstates >= 1710)
						{
							m_tapeTstates -= 1710;
							m_readPortFE ^= PC_EAR_IN;
							if (m_tapePulseCounter-- == 0)
							{
								m_tapePulseCounter = 2;
								m_tapeBitMask >>= 1;
							}
						}
					}
					else
					{
						// Generate a 0
						if (m_tapeTstates >= 855)
						{
							m_tapeTstates -= 855;
							m_readPortFE ^= PC_EAR_IN;
							if (m_tapePulseCounter-- == 0)
							{
								m_tapePulseCounter = 2;
								m_tapeBitMask >>= 1;
							}
						}
					}
					break;
			}
			break;

		default:
			fprintf(stdout, "[ZX Spectrum]: unknown tape format - rewound and stopped\n");
			stopTape = true;
			break;
	}

	if (tapeError != 0)
	{
		m_tapeFormat = TC_FORMAT_UNKNOWN;
		fprintf(stdout, "[ZX Spectrum]: tape error %d - rewound and stopped\n", tapeError);
		stopTape = true;
	}

	if (stopTape)
	{
		fseek(m_pFile, 0, SEEK_SET);
		m_tapePlaying	= false;
	}
}
//=============================================================================

