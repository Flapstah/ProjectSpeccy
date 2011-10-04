#if !defined(__ZXSPECTRUM_H__)
#define __ZXSPECTRUM_H__

#include "common/platform_types.h"

#include "imemory.h"
#include "iscreenmemory.h"

class CDisplay;
class CZ80;

class CZXSpectrum : public IMemory, public IScreenMemory
{
	public:
		CZXSpectrum(void);
		virtual ~CZXSpectrum(void);

		// IMemory
		virtual void WriteMemory(uint16 address, uint8 byte);
		virtual uint8 ReadMemory(uint16 address) const;
		virtual void WritePort(uint16 address, uint8 byte);
		virtual uint8 ReadPort(uint16 address) const;
		// ~IMemory

		// IScreenMemory
		virtual	const void*	GetScreenMemory(void) const;
		virtual	uint32			GetScreenWidth(void) const;
		virtual	uint32			GetScreenHeight(void) const;
		// ~IScreenMemory

						bool				Initialise(void);
						bool				Update(void);

						bool				LoadROM(const char* fileName);
						bool				LoadSNA(const char* fileName);
						bool				LoadRAW(const char* fileName);
						bool				LoadTAP(const char* fileName);
						bool				LoadTZX(const char* fileName);

	protected:
						void				DisplayHelp(void) const;

		enum eSpectrumConstant
		{
			SC_TOP_BORDER = 64,
			SC_BOTTOM_BORDER = 56,
			SC_LEFT_BORDER = 48,
			SC_RIGHT_BORDER = 48,
			SC_VISIBLE_BORDER_SIZE = 16,
			SC_PIXEL_SCREEN_WIDTH = 256,
			SC_PIXEL_SCREEN_HEIGHT = 192,
			SC_ATTRIBUTE_SCREEN_WIDTH = 32,
			SC_ATTRIBUTE_SCREEN_HEIGHT = 24,
			SC_SCREEN_START_ADDRESS = 16384,
			SC_PIXEL_SCREEN_BYTES = (SC_PIXEL_SCREEN_WIDTH * SC_PIXEL_SCREEN_HEIGHT) >> 3,
			SC_ATTRIBUTES_START_ADDRESS = SC_SCREEN_START_ADDRESS + SC_PIXEL_SCREEN_BYTES,
			SC_ATTRIBUTES_SCREEN_BYTES = SC_ATTRIBUTE_SCREEN_WIDTH * SC_ATTRIBUTE_SCREEN_HEIGHT,
			SC_SCREEN_SIZE_BYTES = SC_PIXEL_SCREEN_BYTES + SC_ATTRIBUTES_SCREEN_BYTES,

			SC_VIDEO_MEMORY_WIDTH = SC_PIXEL_SCREEN_WIDTH + (SC_VISIBLE_BORDER_SIZE << 1),
			SC_VIDEO_MEMORY_HEIGHT = SC_PIXEL_SCREEN_HEIGHT + (SC_VISIBLE_BORDER_SIZE << 1),

			SC_TOTAL_VIDEO_WIDTH = SC_LEFT_BORDER + SC_PIXEL_SCREEN_WIDTH + SC_RIGHT_BORDER,
			SC_TOTAL_VIDEO_HEIGHT = SC_TOP_BORDER + SC_PIXEL_SCREEN_HEIGHT + SC_BOTTOM_BORDER,

			SC_FRAME_TSTATES = 69888, // (24+128+24+48)*(64+192+56)

			SC_16K_SPECTRUM = 32768,
			SC_48K_SPECTRUM = 65536
		};

		// The ZX Spectrum screen starts at memory address 16384 and is 256*192
		// pixels.  It is layed out in memory thusly:
		// The pixel data comes first, and is a single bitplane.  Each line of 32
		// bytes corresponds to a horizontal row, but rows are not mapped linearly.
		// The first 8 blocks of 32 bytes correspond to rows 0, 8, 16, 24, 32, 40,
		// 48 and 56.  The next 8 blocks correspond to rows 1, 9, 17, 25, 33, 41, 49
		// and 57 (meaning row 1 is 256 bytes away from row 0).  This repeats until
		// row 63, whereupon the cycle repeats starting at row 64, and then again at
		// row 128 making 3 cycles in total.
		//
		// Following the pixel data is an attribute map which is not a 1:1 mapping
		// per pixel.  Instead, there is an ink and paper colour per 8x8 pixel cell.
		// The ink and paper colour are 3 bits each in size (8 colours), and then
		// there's one bit for brightness (making 15 colours in all as bright black
		// is the same as normal black), and one bit for flash (which swaps the ink
		// and paper colour on a timer).  This means one attribute byte per cell,
		// and there are 32*24 cells, so the attribute map is 768 bytes, starting at
		// address 22528.
		//
		// So, in order to work out which byte a particular pixel corresponds to,
		// the following steps need to be taken:
		//	(1) work out which 3rd of the screen you're in (0 - 63, 64 - 127, 128 -
		//			191), each 3rd being 2048 bytes in size
		//					(y & 0xC0) isolates the bits we need, >> 6 to get into a 0
		//					based range, and then << 11 to multiply by 2048
		//	(2) work out which 'cell' row you're in (0 - 7, 8 - 15, etc), relative
		//			to the screen 3rd, each cell row being 32 bytes from the next cell
		//			row.
		//					(y & 0x38) isolates the bits we need, >> 3 to get into a 0
		//					based range, and then << 5 to multiply by 32
		//	(3) work out which pixel row you're in relative to the cell row and
		//			multiply it by 256 (as pixel row 1 is 256 bytes away from row 0)
		//					(y & 0x07) is already in a 0 based range, so just << 8
		//	(4) work out which 'cell' column you're in
		//					(x >> 3) to divide by 8
		//
		// Calculating the attribute cell is trivially calculated by ((y / 8) * 32)
		// + (x / 8)
		inline	uint32	PixelByteIndex(uint8 x, uint8 y) const { return ((y & 0xC0) << 5) + ((y & 0x38) << 2) + ((y & 0x07) << 8) + (x >> 3); };
		inline	uint32	AttributeByteIndex(uint8 x, uint8 y) const { return (SC_PIXEL_SCREEN_BYTES + ((y >> 3) * SC_ATTRIBUTE_SCREEN_WIDTH) + (x >> 3)); }
						void		UpdateScanline(uint32 tstates);
						void		UpdateTape(uint32 tstates);
						bool		UpdatePulse(uint32 length);
						bool		UpdateBlock(void);
						bool		ReadTapeByte(uint8& value);
						bool		ReadTapeWord(uint16& value);
		
		enum ColourConstants
		{
			CC_BLUE			= 0x01,
			CC_RED			= 0x02,
			CC_MAGENTA	= 0x03,
			CC_GREEN		= 0x04,
			CC_CYAN			= 0x05,
			CC_YELLOW		= 0x06,
			CC_WHITE		= 0x07
		};

		enum PortConstants
		{
			PC_EAR_IN				= 0x40,
			PC_EAR_OUT			= 0x10,
			PC_MIC_OUT			= 0x08,
			PC_BORDER_MASK	= 0x07,
			PC_OUTPUT_MASK	= PC_EAR_OUT | PC_MIC_OUT | PC_BORDER_MASK
		};

		enum eTapeConstant
		{
			TC_FORMAT_UNKNOWN = 64,
			TC_FORMAT_RAW = 65,
			TC_FORMAT_TAP = 66,
			TC_FORMAT_TZX = 67,

			TC_STATE_READING_FORMAT = 32,
			TC_STATE_GENERATING_PILOT = 33,
			TC_STATE_GENERATING_SYNC_PULSE_0 = 34,
			TC_STATE_GENERATING_SYNC_PULSE_1 = 35,
			TC_STATE_GENERATING_DATA = 36,
			TC_STATE_GENERATING_PAUSE = 37,
			TC_STATE_STOP_TAPE = 38,
			TC_STATE_READING_BLOCK = 39,

			TC_BLOCK_TYPE_HEADER = 0x00,
			TC_BLOCK_TYPE_DATA = 0xFF,

			TC_DATA_TYPE_PROGRAM = 0,
			TC_DATA_TYPE_NUMBER_ARRAY = 1,
			TC_DATA_TYPE_CHARACTER_ARRAY = 2,
			TC_DATA_TYPE_CODE = 3,
		};

		struct STapeBlock
		{
			void Reset(void)
			{
				m_pilotPulseLength = 2168;
				m_sync0PulseLength = 667;
				m_sync1PulseLength = 735;
				m_bit0PulseLength = 855;
				m_bit1PulseLength = 1710;
				m_pilotPulseCount = 8063;
				m_lastByteBitMask = 0xFF;
				m_pauseLength = 3500000 / 1000;
				m_dataByteLength = 0;
			};

			uint16		m_pilotPulseLength;
			uint16		m_sync0PulseLength;
			uint16		m_sync1PulseLength;
			uint16		m_bit0PulseLength;
			uint16		m_bit1PulseLength;
			uint16		m_pilotPulseCount;
			uint8			m_lastByteBitMask;
			uint16		m_pauseLength;
			uint32		m_dataByteLength;
		};

		// Memory for the OpenGL texture used to represent the ZX Spectrum screen
		uint32			m_videoMemory[SC_VIDEO_MEMORY_WIDTH * SC_VIDEO_MEMORY_HEIGHT];
		// Main memory for the 48K ZX Spectrum
		uint8				m_memory[SC_48K_SPECTRUM];
		double			m_frameStart;
		double			m_frameTime;
		float				m_clockRate;
		CDisplay*		m_pDisplay;
		CZ80*				m_pZ80;
		FILE*				m_pFile;
		uint32			m_scanline;
		uint32			m_xpos;
		uint32			m_frameNumber;
		uint32			m_scanlineTstates;
		uint32			m_tapeTstates;
		uint8				m_writePortFE;
		mutable uint8				m_readPortFE;
		bool				m_tapePlaying;
		eTapeConstant m_tapeFormat;
		eTapeConstant m_tapeState;
		uint16			m_tapeBlockSize;
		uint8				m_tapeBlockType;
		uint8				m_tapeDataBitMask;
		uint8				m_tapeDataByteMask;
		uint8				m_tapeByte;
		uint16			m_tapePulseCounter;
		STapeBlock	m_tapeBlockInfo;
		int					m_tapeError;

	private:
};

#endif // !defined(__ZXSPECTRUM_H__)
