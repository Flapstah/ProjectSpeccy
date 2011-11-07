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

static uint32 g_buffersUsed = 0;

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
		if (m_source[m_currentSourceBufferIndex].AddSample(data))
		{
			++m_currentSourceBufferIndex %= NUM_SOURCE_BUFFERS;
			if (m_source[m_currentSourceBufferIndex].IsFull())
			{
				fprintf(stderr, "[Sound]: CSound::Update() all source buffers full!\n");
			}
		}
	}

	ALuint nextBuffer;
	ALint processed;
	bool freeBufferFound = false;
	alGetSourcei(m_alSource, AL_BUFFERS_PROCESSED, &processed);

	if ((m_source[m_fullSourceBufferIndex].IsFull()) || (processed > 0))
	{
		if (processed > 0)
		{
			printf("%d destination buffers processed %f (tstates %d)\n", processed, glfwGetTime(), tstates);

			if (processed == NUM_DESTINATION_BUFFERS)
			{
				printf("processed ALL destination buffers\n");
			}

			while (processed--)
			{
				--g_buffersUsed;

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
			uint32 size = m_source[m_fullSourceBufferIndex].Size();
			if (size > 0)
			{
				++g_buffersUsed;

				printf("buffering %d elements in buffer %d\n", size, nextBuffer);
				alBufferData(nextBuffer, g_format, m_source[m_fullSourceBufferIndex].m_buffer, size, FREQUENCY);
				alSourceQueueBuffers(m_alSource, 1, &nextBuffer);
				m_source[m_fullSourceBufferIndex].Reset();
				++m_fullSourceBufferIndex %= NUM_SOURCE_BUFFERS;
				SetBufferInUse(nextBuffer, true);

				ALuint error = alGetError();
				if (error != AL_NO_ERROR)
				{
					fprintf(stderr, "[Sound]: CSound::Update() OpenAL error %X\n", error);
					exit(0);
				}

				ALint state;
				alGetSourcei(m_alSource, AL_SOURCE_STATE, &state);
				if (state != AL_PLAYING)
				{
					printf("forcing buffer to play %f\n", glfwGetTime());
					// TODO: why does this keep happening? This is what makes the
					// pops/clicks.
					// Happens even when we've not freed a buffer so perhaps we're not
					// adding new buffers quickly enough?  Need to log how many full source
					// buffers there were when forcing buffer to play...
					//
					// Perhaps we need to fix the buffering of data to be dynamically sized
					// and do the sound the other way round, i.e. when a buffer is freed by
					// OpenAL, just buffer up what we have accumulated in the source buffer.
					alSourcePlay(m_alSource);
				}
				else
				{
					//				printf("still playing\n");
				}
			}

		}

		printf("buffers used %d\n", g_buffersUsed);
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
//			printf("found free buffer id %d\n", bufferId);
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
//			printf("set buffer in use id %d, set %s\n", bufferId, (inUse) ? "TRUE" : "FALSE");
			m_bufferInUse[index] = inUse;
			break;
		}
	}
}

//=============================================================================

