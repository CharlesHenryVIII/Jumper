#include "Audio.h"
#include "Console.h"

#include "SDL.h"

#include <mutex>
#include <unordered_map>
#include <cassert>


struct FileData {
	uint8* buffer = nullptr;
	uint32 length = 0;
	Volume audioType = Volume::None;
};

enum class AudioState {
	None,
	Playing,
	FadingIn,
	FadingOut,
	Repeating,
	EndOfFile,
	Ending,
	Count,
};

float g_volumes[(size_t)Volume::Count];


AudioID s_audioID = 0;
struct AudioInstance {
	AudioID ID = ++s_audioID;
	std::string name;
	float endDuration = 0;
	float playedDuration = 0;
	float fadeOutDuration = 0;
	float fadeInDuration = 0;
    float fade = 1;
	Volume audioType = Volume::None;
};

SDL_AudioSpec s_driverSpec;
std::unordered_map<std::string, FileData> s_audioFiles;
std::vector<AudioInstance> s_audioPlaying;
std::vector<AudioID> s_audioMarkedForDeletion;
SDL_mutex* s_playingAudioMutex = nullptr;
SDL_mutex* s_deletionQueueMutex = nullptr;
std::mutex s_playingAudioMutex2;
std::mutex s_deletionQueueMutex2;


uint32 BytesToSamples(uint32 bytes)
{
	assert((bytes % (sizeof(Sample))) == 0);
	assert(s_driverSpec.format == AUDIO_S16);
	return bytes / (sizeof(Sample));
}

uint32 SamplesToBytes(uint32 samples)
{
	return samples * sizeof(Sample);
}

uint32 Round(uint32 v, uint32 bound)
{
	uint32 mod = v % bound;
	if (mod > bound / 2)
		return v + (bound - mod);
	else
		return v - mod;
}

uint32 SecondsToSamples(float seconds, uint32 boundary, uint32 channels = s_driverSpec.channels)
{
	//todo fix floating point error
	uint32 result = static_cast<uint32>(s_driverSpec.freq * seconds * channels + 0.5f);
	result = Round(result, boundary);
	assert(!(result & BIT(0)));
	return result;
}

float SamplesToSeconds(uint32 samples, uint32 channels = s_driverSpec.channels)
{
	return static_cast<float>(samples) / (static_cast<float>(s_driverSpec.freq) * channels);
}

AudioInstance* GetAudioInstance(AudioID ID)
{
	std::lock_guard<std::mutex> guard(s_playingAudioMutex2);
	for (uint32 i = 0; i < s_audioPlaying.size(); i++)
	{
		if (s_audioPlaying[i].ID == ID)
			return &(s_audioPlaying[i]);
	}
	return nullptr;
}

void LockMutex(SDL_mutex* mutex)
{
	if (SDL_LockMutex(mutex))
	{
        ConsoleLog("%s\n", SDL_GetError());
		assert(false);
	}
}

void UnlockMutex(SDL_mutex* mutex)
{
	if (SDL_UnlockMutex(mutex))
	{
        ConsoleLog("%s\n", SDL_GetError());
		assert(false);
	}
}

void ConsolePrintFileLine()
{
	ConsoleLog("    File: %s", __FILE__);
	ConsoleLog("    Line: %i", __LINE__);
}

AudioID PlayAudio(const Audio audio)
{

	AudioInstance instance;
	instance.name = audio.nameOfSound;
	instance.audioType = s_audioFiles[audio.nameOfSound].audioType;

	float filePlaySeconds = SamplesToSeconds(BytesToSamples(s_audioFiles[audio.nameOfSound].length));
	float loopDurationInSeconds = ((float)audio.loopCount * filePlaySeconds);
	if (audio.loopCount == AUDIO_MAXLOOPS)
		loopDurationInSeconds = inf;

	//Ending Time
	if (audio.secondsToPlay && loopDurationInSeconds) //AUDIO_DURATION && AUDIO_REPEAT
	{
		instance.endDuration = Min<float>(audio.secondsToPlay, loopDurationInSeconds);
	}
	else if (audio.secondsToPlay) //AUDIO_DURATION
	{
		instance.endDuration = audio.secondsToPlay;
	}
	else if (loopDurationInSeconds) //AUDIO_REPEAT
	{
		instance.endDuration = loopDurationInSeconds;
	}
	else //NEITHER
	{
		instance.endDuration = filePlaySeconds;
	}


	//Fading
	instance.fade = 0;
	if (audio.fadeInDuration && audio.fadeOutDuration)
	{

		if (audio.fadeInDuration + audio.fadeOutDuration > instance.endDuration)
		{
			float fadeTotal = audio.fadeInDuration + audio.fadeOutDuration;
			float diff = fadeTotal - instance.endDuration;
			float frac = diff / fadeTotal;

			instance.fadeInDuration = audio.fadeInDuration - (audio.fadeInDuration * frac);
			instance.fadeOutDuration = audio.fadeOutDuration - (audio.fadeOutDuration * frac);

			ConsoleLog("Audio fade in and fadeout larger than endDuration");
			ConsoleLog("    Audio: %s", instance.name);
			ConsolePrintFileLine();
		}
		else
		{
			instance.fadeInDuration = audio.fadeInDuration;
			instance.fadeOutDuration = audio.fadeOutDuration;
		}
	}
	else if (audio.fadeInDuration)
	{

		if (audio.fadeInDuration > instance.endDuration)
			instance.fadeInDuration = audio.fadeInDuration;
		else
			instance.fadeInDuration = audio.fadeInDuration;
	}
	else if (audio.fadeOutDuration)
	{

		instance.fade = 1.0f;
		if (audio.fadeOutDuration > instance.endDuration)
			instance.fadeOutDuration = audio.fadeOutDuration;
		else
			instance.fadeOutDuration = audio.fadeOutDuration;
	}
	else
	{
		instance.fadeInDuration = instance.fadeOutDuration = 0;
	}

	std::lock_guard<std::mutex> guard(s_playingAudioMutex2);
	s_audioPlaying.push_back(instance);

	return instance.ID;
}

AudioID PlayAudio(const std::string& nameOfSound)
{

	Audio audio;
	audio.nameOfSound = nameOfSound;
	return PlayAudio(audio);
}

AudioID StopAudio(AudioID ID)
{
	if (AudioInstance* instance = GetAudioInstance(ID))
	{
		if (instance->fadeOutDuration)
		{

			instance->endDuration = instance->playedDuration + instance->fadeOutDuration;
		}
		else
		{

			std::lock_guard<std::mutex> guard(s_deletionQueueMutex2);
			s_audioMarkedForDeletion.push_back(ID);
		}
	}
	else
	{
		ConsoleLog("AudioID %i not found when running StopAudio()", ID);
		ConsolePrintFileLine();
	}
	return 0;
}


void EraseFile(AudioID ID)
{
	std::erase_if(s_audioPlaying, [ID](AudioInstance a) { return a.ID == ID; });
}

FileData LoadWavFile(const char* fileLocation)
{
    SDL_AudioSpec spec = {};
    uint8* buffer = nullptr;
    uint32 length = 0;
    const SDL_AudioSpec& driverSpec = s_driverSpec;

	if (SDL_LoadWAV(fileLocation, &spec, &buffer, &length) == NULL)
        ConsoleLog("%s\n", SDL_GetError());

    if (spec.format != driverSpec.format ||
		spec.channels != driverSpec.channels ||
		spec.freq != driverSpec.freq)
    {
		SDL_AudioCVT cvt;
		if (SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq,
							  driverSpec.format, driverSpec.channels, driverSpec.freq) < 0)
		{
            ConsoleLog("Failed at SDL_BuildAudioCVT for %s: %s\n", fileLocation, SDL_GetError());
		}
        else
        {
			SDL_assert(cvt.needed);
			cvt.len = length;
			cvt.buf = (uint8*)SDL_malloc(cvt.len * cvt.len_mult);
			memcpy(cvt.buf, buffer, cvt.len);
			if (SDL_ConvertAudio(&cvt))
				ConsoleLog("Could not change format on %s: %s\n", fileLocation, SDL_GetError());

			SDL_free(buffer);
			buffer = cvt.buf;
            length = cvt.len;
            spec.channels = driverSpec.channels;
            spec.format= driverSpec.format;
            spec.freq= driverSpec.freq;
        }
    }

    assert(spec.channels == driverSpec.channels);
    assert(spec.format == driverSpec.format);
    assert(spec.freq == driverSpec.freq);
    assert(spec.samples == driverSpec.samples);
	return { buffer, length};
}

float FadeAmount(float current, float total)
{
	return Clamp<float>((current / total), 0.0f, 1.0f);
}

float UpdateFade(const float& currentFade, float incriment, float total)
{
	return Clamp<float>(currentFade + (incriment / total), 0.0f, 1.0f);
}

void UpdateStreamBuffer(float& streamBuffer, const Sample readBuffer, Volume volumeType, float fade = 1.0f)
{
	streamBuffer += (((float)readBuffer) * g_volumes[(size_t)volumeType] * fade);
}

std::vector<float> s_streamBuffer;
void CSH_AudioCallback(void* userdata, Uint8* stream, int len)
{
	//play audio
	//fade in and out
	//end based on duration
	//end based on seperate function call

	Sample* writeBuffer = reinterpret_cast<Sample*>(stream);
	uint32 samples = BytesToSamples(len);

	memset(stream, 0, len);
	s_streamBuffer.clear();
	s_streamBuffer.resize(samples, 0);
	//memset(s_streamBuffer.data(), 0, samples);

	float callbackSeconds = SamplesToSeconds(samples);

	//Blend Audio into Buffer

	std::lock_guard<std::mutex> playingGuard(s_playingAudioMutex2);
	std::lock_guard<std::mutex> deletionGuard(s_deletionQueueMutex2);
	for (int32 i = 0; i < s_audioPlaying.size(); i++)
	{
		AudioInstance& instance = s_audioPlaying[i];
		const FileData* file = &s_audioFiles[instance.name];
		const uint32 fileSamples = BytesToSamples(file->length);
		const float fileSeconds = SamplesToSeconds(fileSamples);
		uint32 samplesToWrite = samples;
		uint32 samplesWritten = 0;

		bool updateAudio = true;
		while (updateAudio)
		{
			updateAudio = false;
			float playDurationThisLoop = fmod(instance.playedDuration, fileSeconds);
		    Sample* readBuffer = reinterpret_cast<Sample*>(file->buffer) + SecondsToSamples(playDurationThisLoop, samples);
			AudioState audioState = AudioState::None;

			//Update state of audio instance
			if (instance.playedDuration >= instance.endDuration)
				audioState = AudioState::Ending;

			else if (instance.playedDuration >= instance.endDuration - instance.fadeOutDuration)//bug on equal sign?
				audioState = AudioState::FadingOut;

			else if (instance.playedDuration < instance.fadeInDuration)//bug on no equal sign?
				audioState = AudioState::FadingIn;

			else if (instance.playedDuration >= instance.endDuration)
				audioState = AudioState::Ending;

			else if (playDurationThisLoop + (SamplesToSeconds(samplesToWrite)) >= fileSeconds)
				audioState = AudioState::EndOfFile;

			//else if (fmod(instance.playedDuration, fileSeconds) + SamplesToSeconds(lengthToWrite) > fileSeconds)
			//	audioState = AudioState::Repeating;

			else
				audioState = AudioState::Playing;


			//Audio Does its thing
			//assert(instance.playedDuration + SamplesToSeconds(samplesToWrite) <= instance.endDuration);
			switch (audioState)
			{
			case AudioState::None:
			{
				assert(false);
				break;
			}
			case AudioState::Playing:
			{

				for (uint32 i = 0; i < samplesToWrite; i++)
				{
					UpdateStreamBuffer(s_streamBuffer[i], readBuffer[i], instance.audioType);
				}
				instance.playedDuration += SamplesToSeconds(samplesToWrite);
				samplesWritten += samplesToWrite;
				if (samplesWritten != samples)
					updateAudio = true;
				break;
			}
			case AudioState::FadingIn:
			{

				float oneSampleToSeconds = SamplesToSeconds(1);
				instance.fade = 1.0f;
				for (uint32 i = 0; i < samplesToWrite; i++)
				{
					//instance.fade = Clamp<float>(instance.fade + oneSampleToSeconds, 0.0f, 1.0f);

					float fade = Clamp<float>(instance.playedDuration / instance.fadeInDuration + (oneSampleToSeconds * i), 0.0f, 1.0f);
					UpdateStreamBuffer(s_streamBuffer[i], readBuffer[i], instance.audioType, fade);
				}
				instance.playedDuration += SamplesToSeconds(samplesToWrite);
				samplesWritten += samplesToWrite;
				if (samplesWritten != samples)
					updateAudio = true;
				break;
			}
			case AudioState::FadingOut:
			{

				float oneSampleToSeconds = SamplesToSeconds(1) / instance.fadeInDuration;
				for (uint32 i = 0; i < samplesToWrite; i++)
				{
					instance.fade = Clamp<float>(instance.fade - oneSampleToSeconds, 0.0f, 1.0f);
					UpdateStreamBuffer(s_streamBuffer[i], readBuffer[i], instance.audioType, instance.fade);
				}
				instance.playedDuration += SamplesToSeconds(samplesToWrite);
				samplesWritten += samplesToWrite;
				if (samplesWritten != samples)
					updateAudio = true;
				break;
			}
			case AudioState::Repeating:
			{
				//Theoretically dont do anything?
				assert(false);
				break;
			}
			case AudioState::EndOfFile:
			{

				samplesToWrite = fileSamples - SecondsToSamples(instance.playedDuration, samples);
				break;
			}
			case AudioState::Ending:
			{

				s_audioMarkedForDeletion.push_back(instance.ID);
				break;
			}
			default:
			{
				assert(false);
				break;
			}

			}
		}
	}

	for (uint32 i = 0; i < samples; i++)
	{
		writeBuffer[i] = (int16)((s_streamBuffer[i] * g_volumes[(size_t)Volume::Master]));
	}

	for (int32 i = 0; i < s_audioMarkedForDeletion.size(); i++)
	{
		EraseFile(s_audioMarkedForDeletion[i]);
	}
	s_audioMarkedForDeletion.clear();

}

struct AudioFileMetaData {
	std::string name;
	Volume volume = Volume::None;
};

void InitializeAudio()
{
	s_playingAudioMutex = SDL_CreateMutex();
	s_deletionQueueMutex = SDL_CreateMutex();
	SDL_AudioSpec want, have;
	SDL_AudioDeviceID audioDevice;

	SDL_memset(&want, 0, sizeof(want));
	want.freq = 48000;
	want.format = AUDIO_S16;
	want.channels = 2;
	want.samples = 4096;
	want.userdata = &s_driverSpec;
	want.callback = CSH_AudioCallback;
	//should I allow any device driver or should I prefer one like directsound?
	audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	s_driverSpec = have;

	if (audioDevice == 0)
	{
		ConsoleLog("Failed to open audio: %s", SDL_GetError());
	}
	else
	{
		if (have.format != want.format) // we let this one thing change.
			ConsoleLog("Audio format is not what was requested");

		assert(have.format == want.format);
		SDL_PauseAudioDevice(audioDevice, 0); /* start audio playing. */
		//SDL_Delay(1000); /* let the audio callback play some sound for 1 seconds. */

		//SDL_CloseAudioDevice(audioDevice);
	}

	//SDL_AudioQuit(); //used only for "shutting down audio" that was initilized with SDL_AudioInit()
	//audioList["Halo"] = LoadWavFile("C:\\Projects\\Jumper\\Assets\\Audio\\10. This is the Hour.wav");
	AudioFileMetaData audioFiles[] = {
		{ "Button_Confirm", Volume::Effect},
		{ "Button_Hover", Volume::Effect},
		{ "Grass", Volume::Effect},
		{ "Halo", Volume::Music},
		{ "Jump", Volume::Effect},
		{ "Punched", Volume::Effect} };

	std::string name;
	for (int32 i = 0; i < sizeof(audioFiles) / sizeof(audioFiles[0]); i++)
	{
		name = "C:\\Projects\\Jumper\\Assets\\Audio\\" + audioFiles[i].name + ".wav";
		s_audioFiles[audioFiles[i].name] = LoadWavFile(name.c_str());

		s_audioFiles[audioFiles[i].name].audioType = audioFiles[i].volume;
	}
	for (int32 i = 1; i < (size_t)Volume::Count; i++)
		g_volumes[i] = 1.0f;
	g_volumes[(size_t)Volume::Master] = 0.5f;
}

