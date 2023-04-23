#include <Windows.h>
#include <assert.h>
#include <atomic>
#include "audio.h"

#pragma comment(lib, "winmm.lib")

namespace Audio
{
static WAVEFORMATEX waveFormat;
static HWAVEOUT waveOut;
static bool opened = false;
static WAVEHDR audioBuffers[NumBuffers];
static std::function<void(int16_t*)> fillBufferCallback = nullptr;
static std::atomic<bool> isFinished = false;

static void CALLBACK WaveCallback(HWAVEOUT waveOut, UINT msg, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR param2)
{
	if (msg == WOM_DONE && !isFinished)
	{
		WAVEHDR* waveHdr = (WAVEHDR*)param1;
		assert(waveHdr);
		int finishedIndex = (int)(waveHdr - audioBuffers);
		assert(finishedIndex >= 0 && finishedIndex < NumBuffers);
		int16_t* buffer = (int16_t*)audioBuffers[finishedIndex].lpData;
		if (fillBufferCallback)
			fillBufferCallback(buffer);
		MMRESULT result = waveOutWrite(waveOut, &audioBuffers[finishedIndex], sizeof(WAVEHDR));
		assert(result == MMSYSERR_NOERROR);
	}
}

bool Init()
{
	ZeroMemory(&waveFormat, sizeof(WAVEFORMATEX));
	ZeroMemory(audioBuffers, sizeof(audioBuffers));

	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nChannels = NumChannels;
	waveFormat.nSamplesPerSec = SampleRate;
	waveFormat.wBitsPerSample = BitDepth;
	waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	MMRESULT result = waveOutOpen(&waveOut, WAVE_MAPPER, &waveFormat, (DWORD_PTR)WaveCallback, NULL, CALLBACK_FUNCTION);
	if (result != MMSYSERR_NOERROR)
	{
		return false;
	}

	opened = true;

	DWORD bufferLength = waveFormat.nAvgBytesPerSec * BufferSizeSeconds;
	assert(bufferLength == BufferSizeSamples * (BitDepth / 8));

	for (int i = 0; i < NumBuffers; i++)
	{
		WAVEHDR& waveHdr = audioBuffers[i];
		waveHdr.dwBufferLength = bufferLength;
		waveHdr.lpData = (LPSTR)malloc(waveHdr.dwBufferLength);
		assert(waveHdr.lpData);
		memset(waveHdr.lpData, 0, waveHdr.dwBufferLength);
		result = waveOutPrepareHeader(waveOut, &waveHdr, sizeof(WAVEHDR));
		if (result != MMSYSERR_NOERROR)
		{
			return false;
		}
	}

	return true;
}

void Shutdown()
{
	if (opened)
	{
		isFinished = true;
		waveOutReset(waveOut);
	}

	for (int i = 0; i < NumBuffers; i++)
	{
		WAVEHDR& waveHdr = audioBuffers[i];
		if (waveHdr.lpData)
		{
			assert(opened);
			MMRESULT result = waveOutUnprepareHeader(waveOut, &waveHdr, sizeof(WAVEHDR));
			assert(result == MMSYSERR_NOERROR);
			free(waveHdr.lpData);
		}
	}

	ZeroMemory(audioBuffers, sizeof(audioBuffers));

	if (opened)
	{
		waveOutClose(waveOut);
		opened = false;
	}
}

void Start()
{
	assert(opened);
	for (int i = 0; i < NumBuffers; i++)
	{
		MMRESULT result = waveOutWrite(waveOut, &audioBuffers[i], sizeof(WAVEHDR));
		assert(result == MMSYSERR_NOERROR);
	}
}

void SetFillBufferCallback(std::function<void(int16_t*)> callback)
{
	fillBufferCallback = callback;
}

}
