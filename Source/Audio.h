#pragma once
#include "Math.h"

#include <string>


typedef uint64 AudioID;

AudioID PlayAudio(const std::string& nameOfSound, float fadeOutTime = 0, int32 loopCount = 0, float secondsToPlay = 0);
AudioID StopAudio(AudioID ID);
void InitilizeAudio();
