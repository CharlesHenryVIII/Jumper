#pragma once
#include "Math.h"
#include "Console.h"

#include <string>

typedef uint64 AudioID;
typedef int16 Sample;

#define AUDIO_MAXLOOPS	INT_MAX
#define AUDIO_MAX_CHANNELS	2

struct AudioParams {
	std::string nameOfSound;
	float fadeInDuration = 0;
	float fadeOutDuration = 0;
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
extern AudioID noID;

void PlayAudio(const AudioParams& audio, AudioID& ID = noID);
void PlayAudio(const std::string& nameOfSound, AudioID& ID = noID);
void PlayAudio(AudioID& ID = noID);
void PauseAudio(const AudioID& ID);
void StopAudio(AudioID& ID);
void InitializeAudio();

CONSOLE_FUNCTIONA(c_PlayAudio);
CONSOLE_FUNCTION(c_PauseAudio);
CONSOLE_FUNCTION(c_StopAudio);
