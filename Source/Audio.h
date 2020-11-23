#pragma once
#include "Math.h"

#include <string>


typedef uint64 AudioID;
typedef int16 Sample;
#define AUDIO_FADEOUT	BIT(0)
#define AUDIO_DURATION	BIT(1)
#define AUDIO_REPEAT	BIT(2)

struct Audio {
	std::string nameOfSound;
	float fadeOutTime = 0;
	int32 loopCount = 0;
	float secondsToPlay = 0;
	uint16 flags = 0;
};

AudioID PlayAudio(Audio audio);
AudioID PlayAudio(const std::string& nameOfSound);
AudioID StopAudio(AudioID ID);
void InitilizeAudio();
