//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glfw.h>
#include "sound.h"

//=============================================================================

static const ALenum g_format = AL_FORMAT_MONO8;
static const BUFFER_TYPE g_levelHigh = 0x3F;
static const BUFFER_TYPE g_levelLow = 0x00;

//=============================================================================

CSound::CSound(void)
: m_initialised(false)
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
					printf("allocated buffer id %d\n", m_alBuffer[index]);
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
	m_soundCycles += (tstates << TSTATE_BITSHIFT);
	if (m_soundCycles >= TSTATE_FIXED_FLOATING_POINT)
	{
		m_soundCycles -= TSTATE_FIXED_FLOATING_POINT;

		// TODO: need to fix how volume works when using 16 bit samples
		BUFFER_TYPE data = (volume * g_levelHigh);
		bool isFull = m_source.AddSample(data);

		ALuint nextBuffer;
		ALint processed;
		bool freeBufferFound = false;
		alGetSourcei(m_alSource, AL_BUFFERS_PROCESSED, &processed);

		if (isFull || (processed > 0))
		{
			if (processed > 0)
			{
				if (processed == NUM_DESTINATION_BUFFERS)
				{
					printf("processed ALL destination buffers\n");
				}

				while (processed--)
				{
					alSourceUnqueueBuffers(m_alSource, 1, &nextBuffer);
					SetBufferInUse(nextBuffer, false);
					freeBufferFound = true;
				}
			}
			else
			{
				freeBufferFound = FindFreeBufferIndex(nextBuffer);
			}

			if (freeBufferFound)
			{
				printf("buffering %d elements in buffer %d, %d\n", m_source.Size(), nextBuffer, SOURCE_BUFFER_SIZE);
				alBufferData(nextBuffer, g_format, m_source.m_buffer, m_source.Size(), FREQUENCY);
				alSourceQueueBuffers(m_alSource, 1, &nextBuffer);
				m_source.Reset();
				SetBufferInUse(nextBuffer, true);

				ALuint error = alGetError();
				if (error != AL_NO_ERROR)
				{
					fprintf(stderr, "[Sound]: CSound::Update() OpenAL error %X\n", error);
					exit(0);
				}
			}

			ALint state;
			alGetSourcei(m_alSource, AL_SOURCE_STATE, &state);
			if (state != AL_PLAYING)
			{
				printf("forcing buffer to play (processed %d)\n", processed);
				alSourcePlay(m_alSource);
			}
			else
			{
//				printf("still playing\n");
			}
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

bool CSound::FindFreeBufferIndex(ALuint& bufferId) const
{
	bool found = false;

	for (uint32 index = 0; index < NUM_DESTINATION_BUFFERS; ++index)
	{
		if (m_bufferInUse[index] == false)
		{
			bufferId = m_alBuffer[index];
			printf("found free buffer id %d\n", bufferId);
			found = true;
			break;
		}
	}

	if (!found)
	{
		printf("no free buffer found\n");
	}

	return found;
}

//=============================================================================

void CSound::SetBufferInUse(ALuint bufferId, bool inUse)
{
	for (uint32 index = 0; index < NUM_DESTINATION_BUFFERS; ++index)
	{
		if (m_alBuffer[index] == bufferId)
		{
			printf("set buffer in use id %d, set %s\n", bufferId, (inUse) ? "TRUE" : "FALSE");
			m_bufferInUse[index] = inUse;
			break;
		}
	}
}

//=============================================================================

