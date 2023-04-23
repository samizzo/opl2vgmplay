#pragma once

#include <functional>

namespace Audio
{
constexpr int NumBuffers = 2;
constexpr int NumChannels = 1;
constexpr int SampleRate = 44100;
constexpr int BitDepth = 16;
constexpr int BufferSizeSeconds = 1;
constexpr int BufferSizeSamples = SampleRate * BufferSizeSeconds;

bool Init();
void Shutdown();
void Start();
void SetFillBufferCallback(std::function<void(int16_t*)> callback);
}
