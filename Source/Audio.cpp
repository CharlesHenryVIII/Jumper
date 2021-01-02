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
	EndOfFile,
	Ending,
	Count,
};

float g_volumes[(size_t)Volume::Count];


AudioID s_audioID = 0;
struct AudioInstance {
	AudioID ID = ++s_audioID;
	std::string name;
	uint32 playedSamples = 0;
	uint32 endSamples = 0;
	uint32 fadeOutSamples = 0;
	uint32 fadeInSamples = 0;
    float fade = 1;
	Volume audioType = Volume::None;
};

SDL_AudioSpec s_driverSpec;
std::unordered_map<std::string, FileData> s_audioFiles;
std::vector<AudioInstance> s_audioPlaying;
std::vector<AudioID> s_audioMarkedForDeletion;
std::mutex s_playingAudioMutex;
std::mutex s_deletionQueueMutex;


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

uint32 SecondsToSamples(float seconds, uint32 channels = s_driverSpec.channels)
{
	return static_cast<uint32>(s_driverSpec.freq * seconds * channels);
}

float SamplesToSeconds(uint32 samples, uint32 channels = s_driverSpec.channels)
{
	return static_cast<float>(samples) / (static_cast<float>(s_driverSpec.freq) * channels);
}

AudioInstance* GetAudioInstance(AudioID ID)
{
	std::lock_guard<std::mutex> guard(s_playingAudioMutex);
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

	uint32 filePlaySamples = (BytesToSamples(s_audioFiles[audio.nameOfSound].length));
	uint32 loopDuration = audio.loopCount * filePlaySamples;
	if (audio.loopCount == AUDIO_MAXLOOPS)
		loopDuration = AUDIO_MAXLOOPS;
	uint32 endSamples = SecondsToSamples(audio.secondsToPlay);

	//Ending Time
	if (audio.secondsToPlay && loopDuration) //AUDIO_DURATION && AUDIO_REPEAT
	{
		instance.endSamples = Min<uint32>(endSamples, loopDuration);
	}
	else if (audio.secondsToPlay) //AUDIO_DURATION
	{
		instance.endSamples = endSamples;
	}
	else if (loopDuration) //AUDIO_REPEAT
	{
		instance.endSamples = loopDuration;
	}
	else //NEITHER
	{
		instance.endSamples = filePlaySamples;
	}


	//Fading
	instance.fade = 0;
	uint32 fadeInDuration = SecondsToSamples(audio.fadeInDuration);
	uint32 fadeOutDuration = SecondsToSamples(audio.fadeOutDuration);
	if (audio.fadeInDuration && audio.fadeOutDuration)
	{

		if (fadeInDuration + fadeOutDuration > instance.endSamples)
		{
			float fadeTotal = audio.fadeInDuration + audio.fadeOutDuration;
			float diff = fadeTotal - (float)instance.endSamples;
			float frac = diff / fadeTotal;

			instance.fadeInSamples  = (uint32)(fadeInDuration  - (fadeInDuration  * frac) + 0.5f);
			instance.fadeOutSamples = (uint32)(fadeOutDuration - (fadeOutDuration * frac) + 0.5f);

			ConsoleLog("Audio fade in and fadeout larger than endSamples");
			ConsoleLog("    Audio: %s", instance.name);
			ConsolePrintFileLine();
		}
		else
		{
			instance.fadeInSamples = fadeInDuration;
			instance.fadeOutSamples = fadeOutDuration;
		}
	}
	else if (audio.fadeInDuration)
	{

		if (fadeInDuration > instance.endSamples)
			instance.fadeInSamples = fadeInDuration;
		else
			instance.fadeInSamples = fadeInDuration;
	}
	else if (audio.fadeOutDuration)
	{

		instance.fade = 1.0f;
		if (fadeOutDuration > instance.endSamples)
			instance.fadeOutSamples = fadeOutDuration;
		else
			instance.fadeOutSamples = fadeOutDuration;
	}
	else
	{
		instance.fadeInSamples = instance.fadeOutSamples = 0;
	}

	std::lock_guard<std::mutex> guard(s_playingAudioMutex);
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
		if (instance->fadeOutSamples)
		{

			instance->endSamples = instance->playedSamples + instance->fadeOutSamples;
		}
		else
		{

			std::lock_guard<std::mutex> guard(s_deletionQueueMutex);
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
	assert(fade >= 0.0f && fade <= 1.0f);
	streamBuffer += (((float)readBuffer) * g_volumes[(size_t)volumeType] * fade);
}

std::vector<float> s_streamBuffer;
void CSH_AudioCallback(void* userdata, Uint8* stream, int len)
{
	Sample* writeBuffer = reinterpret_cast<Sample*>(stream);
	uint32 samples = BytesToSamples(len);

	memset(stream, 0, len);
	s_streamBuffer.clear();
	s_streamBuffer.resize(samples, 0);

	std::lock_guard<std::mutex> playingGuard(s_playingAudioMutex);
	std::lock_guard<std::mutex> deletionGuard(s_deletionQueueMutex);
	for (int32 i = 0; i < s_audioPlaying.size(); i++)
	{
		AudioInstance& instance = s_audioPlaying[i];
		const uint32 fileSamples = BytesToSamples(s_audioFiles[instance.name].length);
		uint32 samplesToWrite = samples;
		uint32 samplesWritten = 0;

		bool updateAudio = true;
		while (updateAudio)
		{
			updateAudio = false;
			uint32 playedSamplesThisLoop = instance.playedSamples % fileSamples;
			float* writeBuffer = s_streamBuffer.data() + samplesWritten;
		    Sample* readBuffer = reinterpret_cast<Sample*>(s_audioFiles[instance.name].buffer) + playedSamplesThisLoop;
			AudioState audioState = AudioState::None;

			if (instance.playedSamples >= instance.endSamples)
				audioState = AudioState::Ending;

			else if (playedSamplesThisLoop + samplesToWrite > fileSamples)
				audioState = AudioState::EndOfFile;

			else if (instance.playedSamples >= instance.endSamples - instance.fadeOutSamples)
				audioState = AudioState::FadingOut;

			else if (instance.playedSamples < instance.fadeInSamples)
				audioState = AudioState::FadingIn;

			else if (instance.playedSamples >= instance.endSamples)
				audioState = AudioState::Ending;

			else
				audioState = AudioState::Playing;


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
					UpdateStreamBuffer(writeBuffer[i], readBuffer[i], instance.audioType);
					instance.playedSamples++;
					samplesWritten++;
				}
				if (samplesWritten != samples)
				{
					samplesToWrite = samples - samplesWritten;
					updateAudio = true;
				}
				break;
			}
			case AudioState::FadingIn:
			{

				for (uint32 i = 0; i < samplesToWrite; i++)
				{
					instance.fade = Clamp<float>((float)(instance.playedSamples) / instance.fadeInSamples, 0.0f, 1.0f);
					UpdateStreamBuffer(writeBuffer[i], readBuffer[i], instance.audioType, instance.fade);
					instance.playedSamples++;
					samplesWritten++;
				}
				if (samplesWritten != samples)
				{
					samplesToWrite = samples - samplesWritten;
					updateAudio = true;
				}
				break;
			}
			case AudioState::FadingOut:
			{

				for (uint32 i = 0; i < samplesToWrite; i++)
				{
					instance.fade = Clamp<float>(instance.fade - (1.0f / instance.fadeInSamples), 0.0f, 1.0f);
					UpdateStreamBuffer(writeBuffer[i], readBuffer[i], instance.audioType, instance.fade);
					instance.playedSamples++;
					samplesWritten++;
				}
				if (samplesWritten != samples)
				{
					samplesToWrite = samples - samplesWritten;
					updateAudio = true;
				}
				break;
			}
			case AudioState::EndOfFile:
			{

				samplesToWrite = fileSamples - playedSamplesThisLoop;
				updateAudio = true;
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

