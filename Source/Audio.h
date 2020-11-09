#pragma once
#include "Math.h"

#include <string>


typedef uint64 AudioID;

AudioID PlayAudio(const std::string& nameOfSound, int32 loopCount = 0, float secondsToPlay = inf);
AudioID StopAudio(AudioID ID);
void InitilizeAudio();
