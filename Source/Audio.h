#pragma once
#include "Math.h"
#include "Console.h"

#include <string>

typedef uint64 AudioID;
typedef int16 Sample;

#define AUDIO_MAXLOOPS	INT_MAX
#define AUDIO_MAX_CHANNELS	2

enum class Volume {
	None,
	Master,
	Effect,
	Music,
	Count,
};

struct AudioParams {
	std::string nameOfSound;
	float fadeInDuration = 0;
	float fadeOutDuration = 0;
	int32 loopCount = 0;
	float secondsToPlay = 0;
};

struct AudioFileMetaData {
	std::string name;
	Volume volumeType = Volume::None;
};


extern float g_volumes[(size_t)Volume::Count];

AudioID PlayAudio(const AudioParams& audio);
AudioID PlayAudio(const std::string& nameOfSound);

void PauseAudio(AudioID ID);
void UnPauseAudio(AudioID ID);

void StopAudio(AudioID& ID);

void EraseAudioIDs();
bool AudioIDValid(AudioID ID);
void SetAudioPan(AudioID id, float volume);
float GetAudioVolume(const AudioID& ID);
void SetAudioVolume(const AudioID& ID, float volume);
void InitializeAudio();

CONSOLE_FUNCTIONA(c_PlayAudio);
CONSOLE_FUNCTION(c_PauseAudio);
CONSOLE_FUNCTION(c_StopAudio);
