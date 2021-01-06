#pragma once
#include "Math.h"

#include <string>

typedef uint64 AudioID;
typedef int16 Sample;

#define AUDIO_MAXLOOPS	INT_MAX
#define AUDIO_MAX_CHANNELS	2

struct AudioParams {
	std::string nameOfSound;
	float fadeOutDuration = 0;
	float fadeInDuration = 0;
	int32 loopCount = 0;
	float secondsToPlay = 0;
};

enum class Volume {
	None,
	Master,
	Effect,
	Music,
	Count,
};

extern float g_volumes[(size_t)Volume::Count];

AudioID PlayAudio_(AudioParams audio, const char* file, int line);
AudioID PlayAudio_(const std::string& nameOfSound, const char* file, int line);
void StopAudio(AudioID& ID);
void InitializeAudio();

#define PlayAudio(arg) PlayAudio_(arg, __FILE__, __LINE__)

