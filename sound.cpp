//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sound.h"

// Sound buffers are played at 44100Hz
// Screen refresh is (64+192+56)*224=69888 T states long
// 3.5Mhz/69888=50.080128205128205128205128205128Hz refresh rate
// 44100/50.080128205128205128205128205128=880.5888 bytes per screen refresh
// 69888/880.5888=79.365079365079365079365079365079 tstates per byte
// 79.365079365079365079365079365079*65536=5201269.8412698412698412698412698
// 5201269/65536=79.3650665283203125 => nearly 5 decimal places of accuracy

#define TSTATE_BITSHIFT (16)
#define TSTATE_MULTIPLIER (1 << TSTATE_BITSHIFT)
#define TSTATE_FIXED_FLOATING_POINT (79.365079365079365079365079365079*TSTATE_MULTIPLIER)

//=============================================================================

static const ALuint g_frequency = 44100;
static const ALenum g_format = AL_FORMAT_MONO8;
static const BUFFER_TYPE g_levelHigh = 0x3F;
static const BUFFER_TYPE g_levelLow = 0x00;

//=============================================================================

CSound::CSound(void)
: m_initialised(false)
, m_sourceBufferIndex(0)
, m_soundCycles(0)
{
}

//=============================================================================

CSound::~CSound(void)
{
	if (m_initialised)
	{
		Uninitialise();
	}
}

//=============================================================================

bool CSound::Initialise(void)
{
	m_pOpenALDevice = alcOpenDevice(NULL);
	if (m_pOpenALDevice != NULL)
	{
		m_pOpenALContext = alcCreateContext(m_pOpenALDevice, NULL);
		alcMakeContextCurrent(m_pOpenALContext);
		if (m_pOpenALContext != NULL)
		{
			alGetError();
			alGenBuffers(NUM_DESTINATION_BUFFERS, m_alBuffer);
			if (alGetError() == AL_NO_ERROR)
			{
				alGenSources(1, &m_alSource);
				for (uint32 index = 0; index < NUM_DESTINATION_BUFFERS; ++index)
				{
					m_bufferInUse[index] = false;
				}
				m_initialised = true;
			}
		}
	}

	return m_initialised;
}

//=============================================================================

void CSound::Update(uint32 tstates, float volume)
{
	// need to rewrite all this - too much duplication of work

	m_soundCycles += (tstates << TSTATE_BITSHIFT);
	if (m_soundCycles >= TSTATE_FIXED_FLOATING_POINT)
	{
		m_soundCycles -= TSTATE_FIXED_FLOATING_POINT;

		// TODO: need to fix how volume works when using 16 bit samples
		BUFFER_TYPE data = (volume * g_levelHigh);
		if (m_source[m_sourceBufferIndex].AddSample(data))
		{
			++m_sourceBufferIndex %= NUM_SOURCE_BUFFERS;
		}
	}

	bool sourceBufferFull = false;
	//printf("source buffer ");
	for (uint32 index = 0; index < NUM_SOURCE_BUFFERS; ++index)
	{
		if (m_source[index].IsFull())
		{
			sourceBufferFull = true;
	//		printf("[full]  ");
		}
		else
		{
	//		printf("[empty] ");
		}
	}
	//printf("\n");

	if (sourceBufferFull)
	{
		ALuint nextBuffer;
		ALint processed;
		alGetSourcei(m_alSource, AL_BUFFERS_PROCESSED, &processed);

		if (processed > 0)
		{
			printf("processed %d\n", processed);
			while (processed--)
			{
				alSourceUnqueueBuffers(m_alSource, 1, &nextBuffer);
				for (uint32 index = 0; index < NUM_DESTINATION_BUFFERS; ++index)
				{
					if (m_alBuffer[index] == nextBuffer)
					{
						printf("destination buffer %d free\n", index);
						m_bufferInUse[index] = false;
					}
				}
			}
		}
		else
		{
			printf("processed 0\n");
		}

		bool freeBufferFound = false;
		printf("destination buffer ");
		if (!freeBufferFound)
		{
			for (uint32 index = 0; index < NUM_DESTINATION_BUFFERS; ++index)
			{
				if (m_bufferInUse[index] == false)
				{
					printf("_");
					m_bufferInUse[index] = true;
					nextBuffer = m_alBuffer[index];
					freeBufferFound = true;
					break;
				}
				else
				{
					printf("X");
				}
			}
			printf("\n");
		}

		if (freeBufferFound)
		{
			uint32 sourceBufferToUse = (m_sourceBufferIndex - 1) % NUM_SOURCE_BUFFERS;
			alBufferData(nextBuffer, g_format, m_source[sourceBufferToUse].m_buffer, BUFFER_SIZE * BUFFER_ELEMENT_SIZE, g_frequency);
			alSourceQueueBuffers(m_alSource, 1, &nextBuffer);
			m_source[sourceBufferToUse].Reset();

			ALuint error = alGetError();
			if (error != AL_NO_ERROR)
			{
				fprintf(stderr, "[Sound]: CSound::Update() OpenAL error %d\n", error);
				exit(0);
			}

			ALint state;
			alGetSourcei(m_alSource, AL_SOURCE_STATE, &state);
			if (state != AL_PLAYING)
			{
				printf("forcing buffer to play\n");
				alSourcePlay(m_alSource);
			}
		}
		else
		{
			printf("no free buffer found\n");
		}
	}
}

//=============================================================================

bool CSound::Uninitialise(void)
{
	if (m_initialised)
	{
		ALint state;

		do
		{
			alGetSourcei(m_alSource, AL_SOURCE_STATE, &state);
		} while (state == AL_PLAYING);

		alDeleteSources(1, &m_alSource);
		alDeleteBuffers(NUM_DESTINATION_BUFFERS, m_alBuffer);
		alcMakeContextCurrent(NULL);
		alcDestroyContext(m_pOpenALContext);
		alcCloseDevice(m_pOpenALDevice);

		m_initialised = false;
	}
}

//=============================================================================

