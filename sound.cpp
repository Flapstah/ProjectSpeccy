//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sound.h"

//=============================================================================

static const ALenum g_format = AL_FORMAT_MONO8;
//static const BUFFER_TYPE g_levelHigh = 0x3F;
static const BUFFER_TYPE g_levelHigh = 0x0F;
static const BUFFER_TYPE g_levelLow = 0x00;

//=============================================================================

CSound::CSound(void)
	: m_pOpenALDevice(NULL)
	, m_pOpenALContext(NULL)
	, m_soundCycles(0)
	, m_currentSourceBufferIndex(0)
	, m_sourceBufferToQueueIndex(0)
	, m_buffersUsed(0)
	, m_initialised(false)
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
#if defined(DEBUG)
	static uint32 stallTime = 0;
#endif // defined(DEBUG)

	m_soundCycles += (tstates << TSTATE_BITSHIFT);
	if (m_soundCycles >= TSTATE_FIXED_FLOATING_POINT)
	{
		m_soundCycles -= TSTATE_FIXED_FLOATING_POINT;

		// TODO: need to fix how volume works when using 16 bit samples
		BUFFER_TYPE data = static_cast<BUFFER_TYPE>(volume * static_cast<float>(g_levelHigh));
		if (m_source[m_currentSourceBufferIndex].AddSample(data))
		{
			//			printf("source buffer %d is full (full index %d)\n", m_currentSourceBufferIndex, m_fullSourceBufferIndex);
			uint32 oldBuffer = m_currentSourceBufferIndex;
			++m_currentSourceBufferIndex %= NUM_SOURCE_BUFFERS;
			if (m_source[m_currentSourceBufferIndex].IsFull())
			{
				m_currentSourceBufferIndex = oldBuffer;
				//fprintf(stderr, "[Sound]: CSound::Update() all source buffers full! (%d destination buffers in use)\n", m_buffersUsed);
				printf("[Sound]: CSound::Update() all source buffers full! (%d destination buffers in use)\n", m_buffersUsed);
#if defined(DEBUG)
				stallTime += tstates;
#endif // defined(DEBUG)
			}
			else
			{
				m_source[m_currentSourceBufferIndex].AddSample(data);
			}
		}
	}

	while (m_source[m_sourceBufferToQueueIndex].IsFull())
	{
		ALuint nextBuffer = 0;
		if (FindFreeBufferIndex(nextBuffer))
		{
			uint32 size = m_source[m_sourceBufferToQueueIndex].Size();
			if (size > 0)
			{
#if defined(DEBUG)
				if (stallTime > 0)
				{
					fprintf(stderr, "[Sound]: CSound::Update() queueing a buffer but stalled for %d tstates\n", stallTime);
					stallTime = 0;
				}
#endif // defined(DEBUG)

				alBufferData(nextBuffer, g_format, m_source[m_sourceBufferToQueueIndex].m_buffer, size, FREQUENCY);
				alSourceQueueBuffers(m_alSource, 1, &nextBuffer);
				m_source[m_sourceBufferToQueueIndex].Reset();
				++m_sourceBufferToQueueIndex %= NUM_SOURCE_BUFFERS;
				SetBufferInUse(nextBuffer, true, size);

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
					printf("forcing buffer to play\n");
					alSourcePlay(m_alSource);
				}
			}
		}
	}
}

//=============================================================================

void CSound::Uninitialise(void)
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

bool CSound::FindFreeBufferIndex(ALuint& bufferId)
{
	bool found = false;

	if (m_buffersUsed > 0)
	{
		// Dequeue any buffers that have been processed
		ALint processed = 0;
		alGetSourcei(m_alSource, AL_BUFFERS_PROCESSED, &processed);
		while (processed > 0)
		{
			alSourceUnqueueBuffers(m_alSource, 1, &bufferId);
			SetBufferInUse(bufferId, false, 0);
			found = true;
			--processed;
		}
	}

	if (!found)
	{
		for (uint32 index = 0; index < NUM_DESTINATION_BUFFERS; ++index)
		{
			if (m_bufferInUse[index] == false)
			{
				bufferId = m_alBuffer[index];
				found = true;
				break;
			}
		}
	}

	return found;
}

//=============================================================================

void CSound::SetBufferInUse(ALuint bufferId, bool inUse, uint32 count)
{
	for (uint32 index = 0; index < NUM_DESTINATION_BUFFERS; ++index)
	{
		if (m_alBuffer[index] == bufferId)
		{
			m_bufferInUse[index] = inUse;
			if (inUse)
			{
				++m_buffersUsed;
			}
			else
			{
				if (m_buffersUsed > 0)
				{
					--m_buffersUsed;
				}
			}
			break;
		}
	}
}

//=============================================================================

