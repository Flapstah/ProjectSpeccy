#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glfw.h>

#include "zxspectrum.h"
#include "display.h"
#include "keyboard.h"
#include "sound.h"
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
	: m_frameStart(0.0)
 	, m_frameRate(3500000 / 69888)
 	, m_frameTime(1.0 / m_frameRate)
	, m_clockRate(1.0f)
	, m_pDisplay(NULL)
	, m_pZ80(NULL)
	, m_pSound(NULL)
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

	if (m_pSound != NULL)
	{
		delete m_pSound;
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

bool CZXSpectrum::Initialise(int argc, char* argv[])
{
	bool initialised = false;

	m_pDisplay = new CDisplay(SC_VIDEO_MEMORY_WIDTH * DISPLAY_SCALE, SC_VIDEO_MEMORY_HEIGHT * DISPLAY_SCALE, "ZX Spectrum");
	if (m_pDisplay != NULL)
	{
		m_pDisplay->SetDisplayScale(DISPLAY_SCALE);
		CKeyboard::Initialise();

		m_pSound = new CSound();
		if (m_pSound != NULL)
		{
			m_pSound->Initialise();

			if (m_pZ80 = new CZ80(this))
			{
				const char* rom = "roms/48.rom";
				const char* tape = NULL;
				int arg = 0;

				// Parse arguments
				while (arg < argc)
				{
					if (strcmp(argv[arg], "-rom") == 0)
					{
						if (++arg < argc)
						{
							rom = argv[arg];
						}
						else
						{
							fprintf(stderr, "[ZX Spectrum]: missing parameter for '-rom'\n");
						}
					}
					else
					{
						// assume any other argument is a tape
						tape = argv[arg++];
					}
				}

				LoadROM(rom);
				if (tape != NULL)
				{
					LoadTape(tape);
				}

				fprintf(stdout, "[ZX Spectrum]: Initialised\n");
				initialised = true;
			}
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
				m_frameTime = (1.0 / m_frameRate) / m_clockRate;
				fprintf(stdout, "[ZX Spectrum]: increased emulation speed to %.02f\n", m_clockRate);
			}
		}

		if (CKeyboard::IsKeyPressed(GLFW_KEY_DOWN))
		{
			if (m_clockRate > MIN_CLOCKRATE_MULTIPLIER)
			{
				m_clockRate /= 2.0f;
				m_frameTime = (1.0 / m_frameRate) / m_clockRate;
				fprintf(stdout, "[ZX Spectrum]: decreased emulation speed to %.02f\n", m_clockRate);
			}
		}

		// Timings:
		// Each line is 224 tstates (24 tstates of left border, 128 tstates of
		// screen, 24 tstates of right border and 48 tstates of flyback
		// Each frame is 64 + 192 + 56 lines
	
		double currentTime = glfwGetTime();
		double elapsedTime = currentTime - m_frameStart;

		bool updateZ80 = !m_pZ80->GetEnableDebug() || m_pZ80->GetEnableUnattendedDebug() || CKeyboard::IsKeyPressed(GLFW_KEY_F9) || CKeyboard::IsKeyPressed(GLFW_KEY_F10);

		if (updateZ80)
		{
			uint32 tstates = 0;
			if (m_scanline >= (SC_TOP_BORDER + SC_PIXEL_SCREEN_HEIGHT + SC_BOTTOM_BORDER))
			{
				if (elapsedTime >= m_frameTime)
				{
					ret &= m_pDisplay->Update(this);
					tstates = m_pZ80->ServiceInterrupts();
					m_frameStart = currentTime;
					++m_frameNumber;
					m_scanline = 0;
				}
			}
			else
			{
				tstates = m_pZ80->SingleStep();
			}

			if (tstates > 0)
			{
				UpdateScanline(tstates);
				UpdateTape(tstates);
				m_pSound->Update(tstates, ((m_writePortFE & PC_EAR_OUT) | (m_readPortFE & PC_EAR_IN)) ? 1.0f : 0.0f);
			}
		}
		else
		{
			if (elapsedTime >= m_frameTime)
			{
				// Needed to update the keyboard...
				ret &= m_pDisplay->Update(this);
				m_frameStart = currentTime;
			}
		}

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

	ret &= !CKeyboard::IsKeyPressed(GLFW_KEY_ESC);
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
//			fprintf(stdout, "ear %d mic %d\n", (m_writePortFE & PC_EAR_OUT) ? 1 : 0, (m_writePortFE & PC_MIC_OUT) ? 1 : 0);
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
		if (CKeyboard::IsKeyDown(GLFW_KEY_LSHIFT)) mask |= 0x01;
		if (CKeyboard::IsKeyDown('Z')) mask |= 0x02;
		if (CKeyboard::IsKeyDown('X')) mask |= 0x04;
		if (CKeyboard::IsKeyDown('C')) mask |= 0x08;
		if (CKeyboard::IsKeyDown('V')) mask |= 0x10;
	}

	if (line & 0x02)
	{
		// A, S, D, F, G
		if (CKeyboard::IsKeyDown('A')) mask |= 0x01;
		if (CKeyboard::IsKeyDown('S')) mask |= 0x02;
		if (CKeyboard::IsKeyDown('D')) mask |= 0x04;
		if (CKeyboard::IsKeyDown('F')) mask |= 0x08;
		if (CKeyboard::IsKeyDown('G')) mask |= 0x10;
	}

	if (line & 0x04) // Q, W, E, R, T
	{
		if (CKeyboard::IsKeyDown('Q')) mask |= 0x01;
		if (CKeyboard::IsKeyDown('W')) mask |= 0x02;
		if (CKeyboard::IsKeyDown('E')) mask |= 0x04;
		if (CKeyboard::IsKeyDown('R')) mask |= 0x08;
		if (CKeyboard::IsKeyDown('T')) mask |= 0x10;
	}

	if (line & 0x08) // 1, 2, 3, 4, 5
	{
		if (CKeyboard::IsKeyDown('1')) mask |= 0x01;
		if (CKeyboard::IsKeyDown('2')) mask |= 0x02;
		if (CKeyboard::IsKeyDown('3')) mask |= 0x04;
		if (CKeyboard::IsKeyDown('4')) mask |= 0x08;
		if (CKeyboard::IsKeyDown('5')) mask |= 0x10;
	}

	if (line & 0x10) // 0, 9, 8, 7, 6
	{
		if (CKeyboard::IsKeyDown('0')) mask |= 0x01;
		if (CKeyboard::IsKeyDown('9')) mask |= 0x02;
		if (CKeyboard::IsKeyDown('8')) mask |= 0x04;
		if (CKeyboard::IsKeyDown('7')) mask |= 0x08;
		if (CKeyboard::IsKeyDown('6')) mask |= 0x10;
	}

	if (line & 0x20) // P, O, I, U, Y
	{
		if (CKeyboard::IsKeyDown('P')) mask |= 0x01;
		if (CKeyboard::IsKeyDown('O')) mask |= 0x02;
		if (CKeyboard::IsKeyDown('I')) mask |= 0x04;
		if (CKeyboard::IsKeyDown('U')) mask |= 0x08;
		if (CKeyboard::IsKeyDown('Y')) mask |= 0x10;
	}

	if (line & 0x40) // ENTER, L, K, J, H
	{
		if (CKeyboard::IsKeyDown(GLFW_KEY_ENTER)) mask |= 0x01;
		if (CKeyboard::IsKeyDown('L')) mask |= 0x02;
		if (CKeyboard::IsKeyDown('K')) mask |= 0x04;
		if (CKeyboard::IsKeyDown('J')) mask |= 0x08;
		if (CKeyboard::IsKeyDown('H')) mask |= 0x10;
	}

	if (line & 0x80) // SPACE, SYM SHIFT, M, N, B
	{
		if (CKeyboard::IsKeyDown(GLFW_KEY_SPACE)) mask |= 0x01;
		if (CKeyboard::IsKeyDown(GLFW_KEY_RSHIFT)) mask |= 0x02;
		if (CKeyboard::IsKeyDown('M')) mask |= 0x04;
		if (CKeyboard::IsKeyDown('N')) mask |= 0x08;
		if (CKeyboard::IsKeyDown('B')) mask |= 0x10;
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

bool CZXSpectrum::LoadTape(const char* fileName)
{
	// Find extension
	const char* extension = strrchr(fileName, '.');

	if (extension != NULL)
	{
		if (strcmp(extension, ".sna") == 0)
		{
			return LoadSNA(fileName);
		}

		struct STapeFormat
		{
			eTapeConstant m_formatID;
			const char* m_formatExtension;
		} types[] = { { TC_FORMAT_RAW, ".raw" }, { TC_FORMAT_TAP, ".tap" }, { TC_FORMAT_TZX, ".tzx" } };
		
		for (uint32 format = 0; format < (sizeof(types) / sizeof(STapeFormat)); ++format)
		{
			if (strcmp(extension, types[format].m_formatExtension) == 0)
			{
				m_pFile = fopen(fileName, "rb");
				if (m_pFile != NULL)
				{
					fprintf(stdout, "[ZX Spectrum]: tape opened [%s] successfully\n", fileName);
					m_tapeFormat = types[format].m_formatID;
					return true;
				}
			}
		}
	}

	fprintf(stderr, "[ZX Spectrum]: failed to load [%s]\n", fileName);
	return false;
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

void CZXSpectrum::UpdateScanline(uint32 tstates)
{
	m_scanlineTstates += tstates;
	if (m_scanlineTstates < 224)
	{
		return;
	}
	m_scanlineTstates -= 224;

	if ((m_scanline < (SC_TOP_BORDER - SC_VISIBLE_BORDER_SIZE)) || (m_scanline >= (SC_TOP_BORDER + SC_PIXEL_SCREEN_HEIGHT + SC_VISIBLE_BORDER_SIZE)))
	{
		//printf("CZXSpectrum::UpdateScanline() outside visible bounds %d\n", m_scanline);
	}
	else
	{
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

	++m_scanline;
}

//=============================================================================

void CZXSpectrum::UpdateTape(uint32 tstates)
{
	// TODO: refactor this out into a tape class
	uint16 blockSize = 0;
	uint16 pauseMS = 0;
	bool stopTape = false;
	m_tapeError = 0;

	if (!m_tapePlaying)
	{
		return;
	}
	
	// RAW images are 44100 Hz => 44100 bytes/sec (1 byte in file = 1 bit)
	// Screen refresh is (64+192+56)*224=69888 T states long
	// 3.5Mhz/69888=50.080128205128205128205128205128Hz refresh rate
	// 44100/50.080128205128205128205128205128=880.5888 bytes per screen refresh
	// 69888/880.5888=79.365079365079365079365079365079 tstates per bit
	// 79.365079365079365079365079365079*65536=5201269.8412698412698412698412698
	// 5201269/65536=79.3650665283203125 => nearly 5 decimal places of accuracy

	m_tapeTstates += (tstates << 16);

	switch (m_tapeFormat)
	{
		case TC_FORMAT_RAW:
			if (m_tapeTstates >= 5201269)
			{
				m_tapeTstates -= 5201269;

				if (ReadTapeByte(m_tapeByte))
				{
					m_readPortFE &= ~PC_EAR_IN;
					m_readPortFE |= (m_tapeByte & 0x80) ? PC_EAR_IN : 0;
				}
				else
				{
					stopTape = true;
				}
			}
			break;

		case TC_FORMAT_TAP:
			switch (m_tapeState)
			{
				case TC_STATE_READING_FORMAT:
					m_readPortFE &= ~PC_EAR_IN;
					m_tapeBlockInfo.Reset();

					if (ReadTapeWord(blockSize))
					{
						m_tapeBlockInfo.m_blockSize = blockSize;
						if (ReadTapeByte(m_tapeByte))
						{
							m_tapeState = TC_STATE_GENERATING_PILOT;

							switch (m_tapeByte)
							{
								case TC_BLOCK_TYPE_HEADER:
									// Pilot pulse count already set by Reset() above
									break;
								case TC_BLOCK_TYPE_DATA:
									m_tapeBlockInfo.m_pilotPulseCount = 3223;
									break;
								default:
									fprintf(stderr, "Unknown blocktype %02X\n", m_tapeByte);
									m_tapeState = TC_STATE_STOP_TAPE;
									break;
							}

							m_tapePulseCounter = m_tapeBlockInfo.m_pilotPulseCount;
						}
					}
					// intentional fall-through

				case TC_STATE_GENERATING_PILOT:
				case TC_STATE_GENERATING_SYNC_PULSE_0:
				case TC_STATE_GENERATING_SYNC_PULSE_1:
				case TC_STATE_GENERATING_DATA:
				case TC_STATE_GENERATING_PAUSE:
					if (UpdateBlock() && (m_tapeState != TC_STATE_STOP_TAPE))
					{
						m_tapeState = TC_STATE_READING_FORMAT;
					}
					break;

				case TC_STATE_STOP_TAPE:
					stopTape = true;
					break;
			}
			break;

		case TC_FORMAT_TZX:
			switch (m_tapeState)
			{
				case TC_STATE_READING_FORMAT:
					{
						m_readPortFE &= ~PC_EAR_IN;
						uint8 buffer[10];
						if (fread(buffer, sizeof(buffer), 1, m_pFile))
						{
							if ((memcmp(buffer, "ZXTape!", 7) == 0) && (buffer[7] == 0x1A))
							{
								fprintf(stdout, "[ZX Spectrum]: TZX version %d:%d\n", buffer[8], buffer[9]);
								m_tapeState = TC_STATE_READING_BLOCK;
							}
							else
							{
								fprintf(stdout, "[ZX Spectrum]: not [ZXTape!] format\n");
								m_tapeState = TC_STATE_STOP_TAPE;
								break;
							}
						}
						else
						{
							m_tapeError = ferror(m_pFile);
							m_tapeState = TC_STATE_STOP_TAPE;
							break;
						}
					}
					break;

				case TC_STATE_READING_BLOCK:
					if (ReadTapeByte(m_tapeByte))
					{
						m_tapeBlockInfo.Reset();

						switch (m_tapeByte)
						{
							case 0x10:
								fprintf(stdout, "[ZX Spectrum]: TZX block ID 10 (Standard Speed Data Block)\n");
								if (ReadTapeWord(m_tapeBlockInfo.m_pauseLength))
								{
									if (ReadTapeWord(blockSize))
									{
										m_tapeBlockInfo.m_blockSize = blockSize;

										if (ReadTapeByte(m_tapeByte))
										{
//											m_tapeBlockInfo.Log();
											m_tapeState = TC_STATE_GENERATING_PILOT;

											switch (m_tapeByte)
											{
												case TC_BLOCK_TYPE_HEADER:
													// Pilot pulse count already set by Reset() above
													break;
												case TC_BLOCK_TYPE_DATA:
													m_tapeBlockInfo.m_pilotPulseCount = 3223;
													break;
												default:
													fprintf(stderr, "Unknown blocktype %02X\n", m_tapeByte);
													m_tapeState = TC_STATE_STOP_TAPE;
													break;
											}

											m_tapePulseCounter = m_tapeBlockInfo.m_pilotPulseCount;
										}
									}
								}
								break;

							case 0x11:
								fprintf(stdout, "[ZX Spectrum]: TZX block ID 11 (Turbo Speed Data Block)\n");
								if (ReadTapeWord(m_tapeBlockInfo.m_pilotPulseLength))
								{
									if (ReadTapeWord(m_tapeBlockInfo.m_sync0PulseLength))
									{
										if (ReadTapeWord(m_tapeBlockInfo.m_sync1PulseLength))
										{
											if (ReadTapeWord(m_tapeBlockInfo.m_bit0PulseLength))
											{
												if (ReadTapeWord(m_tapeBlockInfo.m_bit1PulseLength))
												{
													if (ReadTapeWord(m_tapeBlockInfo.m_pilotPulseCount))
													{
														if (ReadTapeByte(m_tapeBlockInfo.m_lastByteBitMask))
														{
															m_tapeBlockInfo.m_lastByteBitMask = ~(0xFF >> m_tapeBlockInfo.m_lastByteBitMask);
															if (ReadTapeWord(m_tapeBlockInfo.m_pauseLength))
															{
																if (ReadTapeWord(blockSize))
																{
																	m_tapeBlockInfo.m_blockSize = blockSize;
																	if (ReadTapeByte(m_tapeByte))
																	{
																		m_tapeBlockInfo.m_blockSize |= (m_tapeByte << 16);

																		if (ReadTapeByte(m_tapeByte))
																		{
																			// m_tapeByte now has the block type byte
																			// (which is the first byte of data)
//																			m_tapeBlockInfo.Log();
																			m_tapeState = TC_STATE_GENERATING_PILOT;

																			m_tapePulseCounter = m_tapeBlockInfo.m_pilotPulseCount;
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
								break;

							case 0x12:
								fprintf(stdout, "[ZX Spectrum]: TZX block ID 12 (Pure Tone)\n");
								if (ReadTapeWord(m_tapeBlockInfo.m_pilotPulseLength))
								{
									if (ReadTapeWord(m_tapeBlockInfo.m_pilotPulseCount))
									{
										m_tapeState = TC_STATE_GENERATING_TONE;
										m_tapePulseCounter = m_tapeBlockInfo.m_pilotPulseCount;
									}
								}
								break;

							case 0x20:
								fprintf(stdout, "[ZX Spectrum]: TZX block ID 20 (Pause/Stop the Tape)\n");
								if (ReadTapeWord(m_tapeBlockInfo.m_pauseLength))
								{
									m_readPortFE &= ~PC_EAR_IN;
									m_tapeState = TC_STATE_GENERATING_PAUSE;
								}
								break;

							case 0x21:
								{
									uint8 buffer[256];
									memset(buffer, 0, sizeof(buffer));

									if (ReadTapeByte(m_tapeByte))
									{
										if (fread(buffer, m_tapeByte, 1, m_pFile))
										{
											fprintf(stdout, "[ZX Spectrum]: TZX block ID 21 (Group start) %s\n", buffer);
										}
										else
										{
											m_tapeError = ferror(m_pFile);
											m_tapeState = TC_STATE_STOP_TAPE;
										}
									}
								}
								break;

							case 0x22:
								{
									fprintf(stdout, "[ZX Spectrum]: TZX block ID 22 (Group end)\n");
								}
								break;


							case 0x30:
								{
									uint8 buffer[256];
									memset(buffer, 0, sizeof(buffer));

									if (ReadTapeByte(m_tapeByte))
									{
										if (fread(buffer, m_tapeByte, 1, m_pFile))
										{
											fprintf(stdout, "[ZX Spectrum]: TZX block ID 30 (Text description) %s\n", buffer);
										}
										else
										{
											m_tapeError = ferror(m_pFile);
											m_tapeState = TC_STATE_STOP_TAPE;
										}
									}
								}
								break;

							default:
								fprintf(stdout, "[ZX Spectrum]: Unhandled TZX block ID %02X\n", m_tapeByte);
								m_tapeState = TC_STATE_STOP_TAPE;
								break;
						}
					}
					break;

				case TC_STATE_GENERATING_PILOT:
				case TC_STATE_GENERATING_SYNC_PULSE_0:
				case TC_STATE_GENERATING_SYNC_PULSE_1:
				case TC_STATE_GENERATING_DATA:
				case TC_STATE_GENERATING_PAUSE:
				case TC_STATE_GENERATING_TONE:
					if (UpdateBlock() && (m_tapeState != TC_STATE_STOP_TAPE))
					{
						m_tapeState = TC_STATE_READING_BLOCK;
					}
					break;

				case TC_STATE_STOP_TAPE:
					stopTape = true;
					break;
			}
			break;

		default:
			stopTape = true;
			break;
	}

	if (m_tapeError != 0)
	{
		fprintf(stdout, "[ZX Spectrum]: tape error %d\n", m_tapeError);
		stopTape = true;
	}

	if (stopTape)
	{
		if (feof(m_pFile))
		{
			fprintf(stdout, "[ZX Spectrum]: tape reached end\n");
			m_clockRate = 1.0f;
			m_frameTime = (1.0 / m_frameRate) / m_clockRate;
			fprintf(stdout, "[ZX Spectrum]: emulation speed set to %.02f\n", m_clockRate);
		}
		fprintf(stdout, "[ZX Spectrum]: tape rewound and stopped\n");
		fseek(m_pFile, 0, SEEK_SET);
		m_tapePlaying	= false;
	}
}

//=============================================================================

bool CZXSpectrum::UpdatePulse(uint32 length)
{
	bool finished = false;
	length <<= 16;

	if (m_tapeTstates >= length)
	{
		m_readPortFE ^= PC_EAR_IN;
		m_tapeTstates -= length;
		if (--m_tapePulseCounter == 0)
		{
			finished = true;
		}
	}

	return finished;
}

//=============================================================================

bool CZXSpectrum::UpdateBlock(void)
{
	bool blockDone = false;

	switch (m_tapeState)
	{
		case TC_STATE_GENERATING_PILOT:
			if (UpdatePulse(m_tapeBlockInfo.m_pilotPulseLength))
			{
				m_tapeState = TC_STATE_GENERATING_SYNC_PULSE_0;
				m_tapePulseCounter = 1;
			}
			break;

		case TC_STATE_GENERATING_SYNC_PULSE_0:
			if (UpdatePulse(m_tapeBlockInfo.m_sync0PulseLength))
			{
				m_tapeState = TC_STATE_GENERATING_SYNC_PULSE_1;
				m_tapePulseCounter = 1;
			}
			break;

		case TC_STATE_GENERATING_SYNC_PULSE_1:
			if (UpdatePulse(m_tapeBlockInfo.m_sync1PulseLength))
			{
				m_tapePulseCounter = 2;
				m_tapeDataBitMask = 0x80;
				m_tapeState = TC_STATE_GENERATING_DATA;
			}
			break;

		case TC_STATE_GENERATING_DATA:
			if (m_tapeDataBitMask == 0)
			{
				if (--m_tapeBlockInfo.m_blockSize > 0)
				{
					if (ReadTapeByte(m_tapeByte))
					{
						m_tapeDataBitMask = 0x80;
						m_tapeDataByteMask = (m_tapeBlockInfo.m_blockSize == 1) ? m_tapeBlockInfo.m_lastByteBitMask : 0xFF;
						m_tapePulseCounter = 2;
					}
					else
					{
						blockDone = true;
						break;
					}
				}
				else
				{
					if (m_tapeBlockInfo.m_pauseLength == 0)
					{
						blockDone = true;
					}
					else
					{
						m_tapeState = TC_STATE_GENERATING_PAUSE;
					}
					break;
				}
			}

			if (m_tapeByte & m_tapeDataBitMask & m_tapeDataByteMask)
			{
				// Generate a 1
				if (UpdatePulse(m_tapeBlockInfo.m_bit1PulseLength))
				{
					m_tapePulseCounter = 2;
					m_tapeDataBitMask >>= 1;
				}
			}
			else
			{
				// Generate a 0
				if (UpdatePulse(m_tapeBlockInfo.m_bit0PulseLength))
				{
					m_tapePulseCounter = 2;
					m_tapeDataBitMask >>= 1;
				}
			}
			break;

		case TC_STATE_GENERATING_PAUSE:
			{
				uint64 cycleCount = (3500 << 16);
				if (m_tapeTstates >= cycleCount)
				{
					m_readPortFE &= ~PC_EAR_IN;

					cycleCount *= m_tapeBlockInfo.m_pauseLength;
					if (m_tapeTstates >= cycleCount)
					{
						printf("1 m_tapeTstates %llu, pause len %d, %llu\n", m_tapeTstates, m_tapeBlockInfo.m_pauseLength, cycleCount);
						m_tapeTstates -= cycleCount;
						printf("2 m_tapeTstates %llu, pause len %d, %llu\n", m_tapeTstates, m_tapeBlockInfo.m_pauseLength, cycleCount);
						blockDone = true;
					}
				}
			}
			break;

		case TC_STATE_GENERATING_TONE:
			if (UpdatePulse(m_tapeBlockInfo.m_pilotPulseLength))
			{
				blockDone = true;
			}
			break;
	}

	return blockDone;
}

//=============================================================================

bool CZXSpectrum::ReadTapeByte(uint8& value)
{
	bool readSuccessfully = false;
	uint8 byte = 0;

	if (fread(&byte, sizeof(byte), 1, m_pFile))
	{
		value = byte;
		readSuccessfully = true;
	}
	else
	{
		m_tapeError = ferror(m_pFile);
		m_tapeState = TC_STATE_STOP_TAPE;
	}

	return readSuccessfully;
}

//=============================================================================

bool CZXSpectrum::ReadTapeWord(uint16& value)
{
	bool readSuccessfully = false;
	uint8 byte = 0;

	if (ReadTapeByte(byte))
	{
		value = byte;
		if (ReadTapeByte(byte))
		{
			readSuccessfully = true;
			value |= (byte << 8);
		}
	}

	return readSuccessfully;
}

//=============================================================================

