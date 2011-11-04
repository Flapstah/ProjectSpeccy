//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glfw.h>
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
, m_currentSourceBufferIndex(0)
, m_fullSourceBufferIndex(0)
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
		if (m_source[m_currentSourceBufferIndex].AddSample(data))
		{
			++m_currentSourceBufferIndex %= NUM_SOURCE_BUFFERS;
			if (m_source[m_currentSourceBufferIndex].IsFull())
			{
				fprintf(stderr, "[Sound]: CSound::Update() all source buffers full!\n");
			}
		}
	}

	if (m_source[m_fullSourceBufferIndex].IsFull())
	{
		ALuint nextBuffer;
		ALint processed;
		alGetSourcei(m_alSource, AL_BUFFERS_PROCESSED, &processed);

		if (processed > 0)
		{
			static double lastProcessed = glfwGetTime();
			double timeNow = glfwGetTime();
			double diff = timeNow - lastProcessed;
			lastProcessed = timeNow;
			if (processed > 2)
			{
				printf("processed %d, %f\n", processed, diff);
			}

			while (processed--)
			{
				alSourceUnqueueBuffers(m_alSource, 1, &nextBuffer);
				for (uint32 index = 0; index < NUM_DESTINATION_BUFFERS; ++index)
				{
					if (m_alBuffer[index] == nextBuffer)
					{
						m_bufferInUse[index] = false;
					}
				}
			}
		}

		bool freeBufferFound = false;
		for (uint32 index = 0; index < NUM_DESTINATION_BUFFERS; ++index)
		{
			if (m_bufferInUse[index] == false)
			{
				m_bufferInUse[index] = true;
				nextBuffer = m_alBuffer[index];
				freeBufferFound = true;
				break;
			}
		}

		if (freeBufferFound)
		{
			alBufferData(nextBuffer, g_format, m_source[m_fullSourceBufferIndex].m_buffer, BUFFER_SIZE * BUFFER_ELEMENT_SIZE, g_frequency);
			alSourceQueueBuffers(m_alSource, 1, &nextBuffer);
			m_source[m_fullSourceBufferIndex].Reset();
			++m_currentSourceBufferIndex %= NUM_SOURCE_BUFFERS;

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
				// TODO: why does this keep happening? This is what makes the
				// pops/clicks.
				// Happens even when we've not freed a buffer so perhaps we're not
				// adding new buffers quickly enough?  Need to log how many full source
				// buffers there were when forcing buffer to play...
				printf("forcing buffer to play (processed %d)\n", processed);
				alSourcePlay(m_alSource);
			}
		}
		else
		{
			printf("no free buffer\n");
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

