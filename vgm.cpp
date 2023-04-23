#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "vgm.h"

namespace VGM
{
static std::function<void(uint8_t, uint8_t)> oplWriteFunction = nullptr;

void SetOPLWriteFunction(std::function<void(uint8_t, uint8_t)> func)
{
    oplWriteFunction = func;
}

VGM* Load(const char* filename)
{
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "rb");
    if (!fp)
    {
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    VGM* vgm = (VGM*)malloc(sizeof(VGM) + fileSize);
    assert(vgm);
    fread(vgm + 1, 1, fileSize, fp);

    vgm->header = (VGM::Header*)(vgm + 1);
    vgm->commands = ((uint8_t*)&vgm->header->vgmDataOffset) + vgm->header->vgmDataOffset;
    vgm->currentSample = vgm->currentPlayedSample = vgm->currentCommand = 0;

    assert(vgm->header->ident == VGM::IDENT);
    assert(vgm->header->eofOffset == fileSize - 4);
    assert(vgm->header->version >= 0x00000151);

    fclose(fp);

    return vgm;
}

void Destroy(VGM* vgm)
{
    assert(vgm);
    free(vgm);
}

void Update(VGM* vgm)
{
    assert(vgm);
    bool updateCurrentSample = true;

    while (vgm->currentSample <= vgm->currentPlayedSample)
    {
        uint8_t command = vgm->commands[vgm->currentCommand];
        if (command >= 0x70 && command < 0x80)
        {
            // 0x7n - wait n+1 samples.
            uint32_t sampleCount = ((uint32_t)command & 0x0f) + 1;
            vgm->currentSample += sampleCount;
            vgm->currentCommand++;
        }
        else
        {
            assert(command != 0);
            switch (command)
            {
                case 0x5a:
                {
                    // YM3812 write command.
                    uint8_t reg = vgm->commands[vgm->currentCommand + 1];
                    uint8_t val = vgm->commands[vgm->currentCommand + 2];
                    if (oplWriteFunction)
                        oplWriteFunction(reg, val);
                    vgm->currentCommand += 3;
                    break;
                }
                case 0x61:
                {
                    // Wait N samples command.
                    uint16_t sampleCount = *(uint16_t*)(&vgm->commands[vgm->currentCommand + 1]);
                    vgm->currentSample += sampleCount;
                    vgm->currentCommand += 3;
                    break;
                }
                case 0x62:
                {
                    // Wait 735 samples (1/60th of a second).
                    vgm->currentSample += 735;
                    vgm->currentCommand++;
                    break;
                }
                case 0x63:
                {
                    // Wait 882 samples (1/50th of a second).
                    vgm->currentSample += 882;
                    vgm->currentCommand++;
                    break;
                }
                case 0x66:
                {
                    // End of sound data command (this will cause the song to loop).
                    vgm->currentSample = 0;
                    vgm->currentCommand = 0;
                    vgm->currentPlayedSample = 0;
                    updateCurrentSample = false;
                    break;
                }
                case 0x67:
                {
                    // Data block - skip this as we don't use it.
                    vgm->currentCommand++;
                    vgm->currentCommand++; // skip 0x66
                    vgm->currentCommand++; // skip data type
                    uint32_t size = *(uint32_t*)(&vgm->commands[vgm->currentCommand++]);
                    vgm->currentCommand += size;
                    break;
                }
                default:
                {
                    assert(!"Unknown command");
                };
            }
        }
    }

    if (updateCurrentSample)
    {
        vgm->currentPlayedSample++;
    }
}
}
