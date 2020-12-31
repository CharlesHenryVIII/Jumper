#pragma once
#include "Math.h"

#include <string>


typedef uint64 AudioID;
typedef float Sample;

#define AUDIO_FADEIN	BIT(0)
#define AUDIO_FADEOUT	BIT(1)
#define AUDIO_DURATION	BIT(2)
#define AUDIO_REPEAT	BIT(3)
#define AUDIO_MAXLOOPS	INT_MAX

struct Audio {
	std::string nameOfSound;
	float fadeOutDuration = 0;
	float fadeInDuration = 0;
	int32 loopCount = 0;
	float secondsToPlay = 0;
	uint16 flags = 0;
};

enum class Volume {
	None,
	Effect,
	Music,
	Count,
};

AudioID PlayAudio(Audio audio);
AudioID PlayAudio(const std::string& nameOfSound);
AudioID StopAudio(AudioID ID);
void InitilizeAudio();
