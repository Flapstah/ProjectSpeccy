#if !defined(__SOUND_H__)
#define __SOUND_H__

//=============================================================================

#include "common/platform_types.h"

#include <AL/al.h>
#include <AL/alc.h>

#define BUFFER_TYPE int8
#define BUFFER_ELEMENT_SIZE (sizeof(BUFFER_TYPE))
#define NUM_DESTINATION_BUFFERS (5)
#define NUM_SOURCE_BUFFERS (2)

// Sound buffers are played at 44100Hz
// Screen refresh is (64+192+56)*224=69888 T states long
// 3.5Mhz/69888=50.080128205128205128205128205128Hz refresh rate
// 44100/50.080128205128205128205128205128=880.5888 bytes per screen refresh
// 69888/880.5888=79.365079365079365079365079365079 tstates per byte
// 79.365079365079365079365079365079*65536=5201269.8412698412698412698412698
// 5201269/65536=79.3650665283203125 => nearly 5 decimal places of accuracy

#define FREQUENCY (44100)
#define FRAME_RATE (3500000 / 69888)
#define FRAME_SIZE (FREQUENCY/FRAME_RATE)
//#define TSTATE_COUNT (69888/FRAME_SIZE)
#define TSTATE_COUNT (79.365079365079365079365079365079)
//#define SOURCE_BUFFER_SIZE (uint32)(FRAME_SIZE)
#define SOURCE_BUFFER_SIZE (uint32)(882)

#define TSTATE_BITSHIFT (16)
#define TSTATE_MULTIPLIER (1 << TSTATE_BITSHIFT)
#define TSTATE_FIXED_FLOATING_POINT ((TSTATE_COUNT-1)*TSTATE_MULTIPLIER)


//=============================================================================

class CSound
{
	public:
		CSound();
		virtual ~CSound();

		bool				Initialise(void);
		void				Update(uint32 tstates, float volume);
		bool				Uninitialise(void);

	protected:
		bool FindFreeBufferIndex(ALuint& bufferId) const;
		void SetBufferInUse(ALuint bufferId, bool inUse, uint32 count);

		struct SSourceBuffer
		{
			SSourceBuffer()
			{
				Reset();
			}

			void Reset()
			{
				m_pos = 0;
			}

			bool IsFull(void) const
			{
				return (m_pos == SOURCE_BUFFER_SIZE);
			}

			bool AddSample(BUFFER_TYPE data)
			{
				if (m_pos < SOURCE_BUFFER_SIZE)
				{
					m_buffer[m_pos++] = data;
				}

				return IsFull();
			}

			uint32 Size(void) const
			{
				return (m_pos * BUFFER_ELEMENT_SIZE);
			}

			BUFFER_TYPE m_buffer[SOURCE_BUFFER_SIZE];
			uint32 m_pos;
		} m_source[NUM_SOURCE_BUFFERS];

		ALCdevice* m_pOpenALDevice;
		ALCcontext* m_pOpenALContext;

		uint64 m_soundCycles;
		uint32 m_currentSourceBufferIndex;
		uint32 m_fullSourceBufferIndex;
		uint32 m_buffersUsed;
		ALuint m_alBuffer[NUM_DESTINATION_BUFFERS];
		ALuint m_alSource;
		uint32 m_bufferInUseCapacity[NUM_DESTINATION_BUFFERS];
		bool m_bufferInUse[NUM_DESTINATION_BUFFERS];
		bool m_initialised;
};

#endif // !defined(__SOUND_H__)

//=============================================================================

