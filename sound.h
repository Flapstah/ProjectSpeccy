#if !defined(__SOUND_H__)
#define __SOUND_H__

//=============================================================================

#include "common/platform_types.h"

#include <AL/al.h>
#include <AL/alc.h>

// 5 destination buffers works on the laptop... but why?
#define NUM_DESTINATION_BUFFERS (4)
#define BUFFER_SIZE (44100 / 50)
#define BUFFER_ELEMENT_SIZE (1)
#define BUFFER_TYPE int8
#define NUM_SOURCE_BUFFERS (2)

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
				return (m_pos == BUFFER_SIZE);
			}

			bool AddSample(BUFFER_TYPE data)
			{
				if (m_pos < BUFFER_SIZE)
				{
					m_buffer[m_pos++] = data;
				}

				return IsFull();
			}

			BUFFER_TYPE m_buffer[BUFFER_SIZE];
			uint32 m_pos;
		} m_source[NUM_SOURCE_BUFFERS];

		ALCdevice* m_pOpenALDevice;
		ALCcontext* m_pOpenALContext;

		ALuint m_alBuffer[NUM_DESTINATION_BUFFERS];
		bool m_bufferInUse[NUM_DESTINATION_BUFFERS];
		bool m_initialised;
		ALuint m_alSource;

		uint32 m_currentSourceBufferIndex;
		uint32 m_fullSourceBufferIndex;
		uint32 m_soundCycles;
};

#endif // !defined(__SOUND_H__)

//=============================================================================

