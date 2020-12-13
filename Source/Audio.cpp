#include "Audio.h"
#include "Console.h"

#include "SDL.h"

#include <mutex>
#include <unordered_map>
#include <cassert>


struct FileData {
	uint8* buffer = nullptr;
	uint32 length = 0;
};


enum class AudioState {
	None,
	Playing,
	FadingIn,
	FadingOut,
	Repeating,
	Ending,
	Count,
};

#if 1
AudioID s_audioID = 0;
struct AudioInstance {
	AudioID ID = ++s_audioID;
	std::string name;
	double duration = 0;
	double playedDuration = 0;
	float fadeoutTime = 0;
	float fadeinTime = 0;
    float fade = 1;
	uint32 repeat = 0;
	uint32 playOffset = 0;
	bool b_duration = false;
	bool b_fadeout = false;
	bool b_fadein = false;
	bool b_repeat = false;

};
#else

AudioID s_audioID = 0;
struct AudioInstance {
	AudioID ID = ++s_audioID;
	std::string name;
	double duration = 0;
	double playedDuration = 0;
	float fadeoutTime = 0;
	float fadeinTime = 0;
    float fade = 1;
	uint32 flags = 0;
	uint32 repeat = 0;
	uint32 playOffset = 0;
	AudioState audioState = AudioState::None;
};

#endif
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

uint32 SecondsToSamples(double seconds, uint32 channels = s_driverSpec.channels)
{
	return static_cast<uint32>(s_driverSpec.freq * seconds * channels);
}

double SamplesToSeconds(uint32 samples, uint32 channels = s_driverSpec.channels)
{
	return static_cast<double>(samples) / (static_cast<double>(s_driverSpec.freq) * channels);
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

AudioID PlayAudio(const std::string& nameOfSound)
{

	AudioInstance instance;
	instance.name = nameOfSound;
	std::lock_guard<std::mutex> guard(s_playingAudioMutex2);
	s_audioPlaying.push_back(instance);
	//UnlockMutex(s_playingAudioMutex);
	return instance.ID;
}

#if 1
AudioID PlayAudio(Audio audio)
{

	AudioInstance instance;
	instance.name = audio.nameOfSound;
	instance.duration = audio.secondsToPlay;
	instance.repeat = audio.loopCount;
	instance.b_duration = audio.flags & AUDIO_DURATION;
	instance.b_fadeout = audio.flags & AUDIO_FADEOUT;
	instance.b_fadein = audio.flags & AUDIO_FADEIN;
	instance.b_repeat = audio.flags & AUDIO_REPEAT;
	instance.fadeoutTime = audio.fadeOutTime;
	instance.fadeinTime = audio.fadeInTime;
	if (instance.b_fadein)
		instance.fade = 0;
	else if (instance.b_fadeout)
		instance.fade = 1;
	std::lock_guard<std::mutex> guard(s_playingAudioMutex2);
	s_audioPlaying.push_back(instance);
	//UnlockMutex(s_playingAudioMutex);

	return instance.ID;
}

#else

AudioID PlayAudio(Audio audio)
{

	AudioInstance instance;
	instance.name = audio.nameOfSound;
	instance.duration = audio.secondsToPlay;
	instance.repeat = audio.loopCount;
	instance.fadeoutTime = audio.fadeOutTime;
	instance.fadeinTime = audio.fadeInTime;
	instance.flags = audio.flags;
	if (instance.flags & AUDIO_FADEIN)
		instance.fade = 0;
	else if (instance.flags & AUDIO_FADEOUT)
		instance.fade = 1;
	std::lock_guard<std::mutex> guard(s_playingAudioMutex2);
	s_audioPlaying.push_back(instance);

	return instance.ID;
}
#endif

#if 1

AudioID StopAudio(AudioID ID)
{
	//LockMutex(s_playingAudioMutex);
	if (AudioInstance* instance = GetAudioInstance(ID))
	{
		if (instance->b_fadeout)
		{
			instance->duration = instance->fadeoutTime;
			instance->b_duration = true;
		}
		else
		{
			std::lock_guard<std::mutex> guard(s_deletionQueueMutex2);
			s_audioMarkedForDeletion.push_back(ID);
			//UnlockMutex(s_deletionQueueMutex);
		}
	}
	//UnlockMutex(s_playingAudioMutex);
	return 0;
}

#else

AudioID StopAudio(AudioID ID)
{
	//LockMutex(s_playingAudioMutex);
	if (AudioInstance* instance = GetAudioInstance(ID))
	{
		if (instance->flags & AUDIO_FADEOUT)
		{
			instance->duration = instance->fadeoutTime;
			instance->flags |= AUDIO_DURATION;
		}
		else
		{
			std::lock_guard<std::mutex> guard(s_deletionQueueMutex2);
			s_audioMarkedForDeletion.push_back(ID);
			//UnlockMutex(s_deletionQueueMutex);
		}
	}
	//UnlockMutex(s_playingAudioMutex);
	return 0;
}


#endif

void EraseFile(AudioID ID)
{

	//std::lock_guard<std::mutex> guard(s_playingAudioMutex2);
	std::erase_if(s_audioPlaying, [ID](AudioInstance a) { return a.ID == ID; });
	//UnlockMutex(s_playingAudioMutex);
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

//void AudioFade(char* dataStream, int32 length, float currentValue, float totalValue)
//{
//
//	float fade = FadeAmount(currentValue, totalValue);
//	for (int32 i = 0; i < length; i++)
//		dataStream[i] = dataStream[i] * fade;
//
//}


#if 1

std::vector<uint8> s_streamBuffer;
void CSH_AudioCallback(void* userdata, Uint8* stream, int len)
{
	s_streamBuffer.resize(len, 0);
	memset(s_streamBuffer.data(), 0, len);
	memset(stream, 0, len);
	Sample* streamBuffer = reinterpret_cast<Sample*>(s_streamBuffer.data());
	uint32 samples = BytesToSamples(len);

	//Blend Audio into Buffer

	std::lock_guard<std::mutex> playingGuard(s_playingAudioMutex2);
	std::lock_guard<std::mutex> deletionGuard(s_deletionQueueMutex2);
	for (int32 i = 0; i < s_audioPlaying.size(); i++)
	{
		AudioInstance* instance = &s_audioPlaying[i];
		const FileData* file = &s_audioFiles[instance->name];
		Sample* readBuffer = reinterpret_cast<Sample*>(file->buffer) + instance->playOffset;
		const uint32 fileSamples = BytesToSamples(file->length);

		uint32 lengthToFill = samples;
		if (instance->playOffset + samples > fileSamples)
			lengthToFill = fileSamples - instance->playOffset;

		if (instance->b_duration)
		{

			if (uint32 instanceSecondsToSamples = SecondsToSamples(instance->duration) <= samples)
				lengthToFill = instanceSecondsToSamples;

			double instanceSeconds = SamplesToSeconds(lengthToFill);
			instance->duration -= instanceSeconds;
			if (instance->duration <= 0)
				s_audioMarkedForDeletion.push_back(instance->ID);

		}

		if (instance->b_fadein)
		{
			if (instance->fadeinTime)
			{
				instance->fade = FadeAmount((float)instance->playedDuration, instance->fadeinTime);
			}
		}
		if (instance->b_fadeout)
		{

			double repeatSeconds = SamplesToSeconds((fileSamples - instance->playOffset) + instance->repeat * fileSamples);
			if ((instance->b_duration && instance->duration <= instance->fadeoutTime) ||
				(instance->b_repeat && (instance->fadeoutTime >= repeatSeconds)))
			{

				if (instance->b_repeat && instance->b_duration)
				{
					if (repeatSeconds > instance->duration)
					{//do audio_repeat fade

						instance->fade = FadeAmount((float)repeatSeconds, instance->fadeoutTime);
					}
					else
					{//do audio_duration fade

						//instance->fade = ((float)instance->duration / instance->fadeoutTime);
						instance->fade = FadeAmount((float)instance->duration, instance->fadeoutTime);
					}
				}
				else
				{

					if (instance->b_repeat)
						instance->fade = FadeAmount((float)repeatSeconds, instance->fadeoutTime);
					else if (instance->b_duration)
						instance->fade = FadeAmount((float)instance->duration, instance->fadeoutTime);
				}
			}
		}


		for (uint32 i = 0; i < lengthToFill; i++)
		{
			streamBuffer[i] += Sample((float)readBuffer[i] * instance->fade);
		}

		instance->playOffset += lengthToFill;
		instance->playedDuration += SamplesToSeconds(lengthToFill);
		//loop while there is more to write
		if (instance->playOffset >= fileSamples - 1)
		{
			if (instance->b_repeat && instance->repeat)
			{
				if (instance->repeat != UINT_MAX)
					instance->repeat--;
				lengthToFill = samples - lengthToFill;
				readBuffer = reinterpret_cast<Sample*>(file->buffer);

				memcpy(streamBuffer, readBuffer, lengthToFill);
				instance->playOffset = lengthToFill;
			}
			else
				s_audioMarkedForDeletion.push_back(instance->ID);
		}
	}
	//UnlockMutex(s_playingAudioMutex);

	for (int32 i = 0; i < s_audioMarkedForDeletion.size(); i++)
	{
		EraseFile(s_audioMarkedForDeletion[i]);
	}
	s_audioMarkedForDeletion.clear();
	//UnlockMutex(s_deletionQueueMutex);

	//Fill Stream with Buffer
	SDL_MixAudioFormat(stream, reinterpret_cast<uint8*>(streamBuffer), s_driverSpec.format, len, 20);
}

#else

std::vector<uint8> s_streamBuffer;
void CSH_AudioCallback(void* userdata, Uint8* stream, int len)
{
	//play audio
	//fade in and out
	//end based on duration
	//end based on seperate function call

	s_streamBuffer.resize(len, 0);
	memset(s_streamBuffer.data(), 0, len);
	memset(stream, 0, len);
	Sample* streamBuffer = reinterpret_cast<Sample*>(s_streamBuffer.data());
	uint32 samples = BytesToSamples(len);

	//Blend Audio into Buffer

	std::lock_guard<std::mutex> playingGuard(s_playingAudioMutex2);
	std::lock_guard<std::mutex> deletionGuard(s_deletionQueueMutex2);
	for (int32 i = 0; i < s_audioPlaying.size(); i++)
	{
		AudioInstance& instance = s_audioPlaying[i];
		const FileData* file = &s_audioFiles[instance.name];
		Sample* readBuffer = reinterpret_cast<Sample*>(file->buffer) + instance.playOffset;
		const uint32 fileSamples = BytesToSamples(file->length);
		const uint32 fileSeconds = SamplesToSeconds(fileSamples);

		bool repeat = true;
		while (repeat)
		{
			repeat = false;
			//Update state of audio instance
			AudioState audioState = AudioState::Playing;
			if (instance.playedDuration >= instance.fadeoutTime)
				audioState = AudioState::FadingIn;
			else if (instance.fadeinTime >= instance.playedDuration)
				audioState = AudioState::FadingOut;
			else if (instance.playedDuration >= fileSeconds)
				audioState = AudioState::Ending;

			//Audio Does its thing
			switch (audioState)
			{
			case AudioState::None:
			{
				assert(false);
				break;
			}
			case AudioState::Playing:
			{

				assert(false);
				break;
			}
			case AudioState::FadingIn:
			{

				assert(false);
				break;
			}
			case AudioState::FadingOut:
			{

				assert(false);
				break;
			}
			case AudioState::Repeating:
			{

				assert(false);
				break;
			}
			case AudioState::Ending:
			{

				assert(false);
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
}

#endif

void InitilizeAudio()
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
			ConsoleLog("We didn't get Float32 audio format.");

		assert(have.format == want.format);
		SDL_PauseAudioDevice(audioDevice, 0); /* start audio playing. */
		//SDL_Delay(1000); /* let the audio callback play some sound for 1 seconds. */

		//SDL_CloseAudioDevice(audioDevice);
	}

	//SDL_AudioQuit(); //used only for "shutting down audio" that was initilized with SDL_AudioInit()
	//audioList["Halo"] = LoadWavFile("C:\\Projects\\Jumper\\Assets\\Audio\\10. This is the Hour.wav");
	std::string audioFiles[] = { "Button_Confirm", "Button_Hover", "Grass", "Halo", "Jump", "Punched" };
	std::string name;
	for (int32 i = 0; i < sizeof(audioFiles) / sizeof(audioFiles[0]); i++)
	{
		name = audioFiles[i];
		audioFiles[i] = "C:\\Projects\\Jumper\\Assets\\Audio\\" + audioFiles[i] + ".wav";
		s_audioFiles[name] = LoadWavFile(audioFiles[i].c_str());
		//snprintf();
		//sprintf();
	}
}

