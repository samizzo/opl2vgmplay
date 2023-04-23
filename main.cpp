#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <stdio.h>
#include <assert.h>
#include "audio.h"
#include "opal.h"
#include "vgm.h"

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("usage: opl2vgmplay <filename>\n");
        return 0;
    }

    if (!Audio::Init())
    {
        printf("error: failed to initialise audio\n");
        return 0;
    }

    VGM::VGM* vgm = VGM::Load(argv[1]);
    assert(vgm);

    Opal adlib(Audio::SampleRate, false);
    auto adlibWrite = [&adlib](uint8_t reg, uint8_t val)
    {
        adlib.Port(reg, val);
    };
    VGM::SetOPLWriteFunction(adlibWrite);

    auto fillBufferCallback = [&vgm, &adlib](int16_t* buffer)
    {
        for (int i = 0; i < Audio::BufferSizeSamples; i++)
        {
            VGM::Update(vgm);

            int16_t left, right;
            adlib.Sample(&left, &right);

            buffer[i] = (left + right) >> 1;
        }
    };
    Audio::SetFillBufferCallback(fillBufferCallback);

    Audio::Start();

    printf("playing '%s', press a key to quit...\n", argv[1]);
    while (true)
    {
        bool isConsoleWindowFocussed = (GetConsoleWindow() == GetForegroundWindow());
        if (isConsoleWindowFocussed && (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0)
            break;
    }

    Audio::Shutdown();
    VGM::Destroy(vgm);
}
