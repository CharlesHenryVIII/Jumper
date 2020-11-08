#pragma once
#include "SDL.h"
#include "Math.h"

struct AudioFile {
    SDL_AudioSpec spec = {};
    uint8* buffer = nullptr;
    uint32 length = 0;
};

struct AudioDriverData {
    //double totalTime = 0;
    //float loudness = 0;
#if 1
    uint64 samplesTaken = 0;
#else 
    float samplesTaken = 0;
#endif
    float frequency = 0;
    SDL_AudioSpec driverSpec;
    //AudioFileData fileToPlay;
};


void InitilizeAudio();
