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


AudioID s_audioID = 0;
struct AudioInstance {
	AudioID ID = ++s_audioID;
	std::string name;
	double endDuration = 0;
	double playedDuration = 0;
	float fadeoutTime = 0;
	float fadeinTime = 0;
    float fade = 1;
	uint32 flags = 0;
	uint32 repeat = 0;
	uint32 playOffset = 0;
	AudioState audioState = AudioState::None;
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
	return instance.ID;
}


AudioID PlayAudio(const Audio audio)
{

	AudioInstance instance;
	instance.name = audio.nameOfSound;
	//instance.repeat = audio.loopCount;
	instance.fadeoutTime = audio.fadeOutTime;
	instance.fadeinTime = audio.fadeInTime;
	instance.flags = audio.flags;

	if (instance.flags & AUDIO_FADEIN)
		instance.fade = 0;
	else if (instance.flags & AUDIO_FADEOUT)
		instance.fade = 1;

	double filePlaySeconds = SamplesToSeconds(BytesToSamples(s_audioFiles[audio.nameOfSound].length));
	double loopDurationInSeconds = ((double)audio.loopCount * filePlaySeconds);

	if (audio.secondsToPlay && loopDurationInSeconds) //AUDIO_DURATION && AUDIO_REPEAT
	{
		instance.endDuration = Min<double>(audio.secondsToPlay, loopDurationInSeconds);
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
	// if inifinite repeat then we got problems

	




	if (instance.flags & AUDIO_REPEAT && audio.loopCount != AUDIO_MAXLOOPS)
	{
		double filePlaySeconds = SamplesToSeconds(BytesToSamples(s_audioFiles[audio.nameOfSound].length));
		double loopDurationInSeconds = ((double)audio.loopCount * filePlaySeconds);
		instance.flags |= AUDIO_DURATION;

	}

	if (instance.flags & AUDIO_FADEIN && instance.flags & AUDIO_FADEOUT)
	{
		if (instance.flags & AUDIO_REPEAT && audio.loopCount != AUDIO_MAXLOOPS)
		{	
			assert(audio.fadeInTime < audio.secondsToPlay);
			assert(audio.fadeOutTime < audio.secondsToPlay);
			if (instance.fadeinTime + instance.fadeoutTime > instance.endDuration)
			{
				instance.fadeinTime = audio.fadeOutTime + (audio.fadeInTime - audio.fadeOutTime);
				instance.fadeoutTime = instance.endDuration - instance.fadeinTime;
			}
		}
		else
		{
			
		}
	}
	std::lock_guard<std::mutex> guard(s_playingAudioMutex2);
	s_audioPlaying.push_back(instance);

	return instance.ID;
}

AudioID StopAudio(AudioID ID)
{
	if (AudioInstance* instance = GetAudioInstance(ID))
	{
		if (instance->flags & AUDIO_FADEOUT)
		{
			instance->endDuration = instance->fadeoutTime;
			instance->flags |= AUDIO_DURATION;
		}
		else
		{
			std::lock_guard<std::mutex> guard(s_deletionQueueMutex2);
			s_audioMarkedForDeletion.push_back(ID);
		}
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


std::vector<Sample> s_streamBuffer;
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
        float callbackSeconds = SamplesToSeconds(BytesToSamples(len));
        int32 lengthToWrite = samples;

		bool updateAudio = true;
		while (updateAudio)
		{
			updateAudio = false;
			//Update state of audio instance
			AudioState audioState = AudioState::None;

			if ((instance.flags & AUDIO_DURATION) && instance.playedDuration >= instance.endDuration)
				audioState = AudioState::Ending;

			else if ((instance.flags & AUDIO_FADEOUT) && (instance.playedDuration >= (fileSeconds - instance.fadeoutTime)))
				audioState = AudioState::FadingOut;

			else if ((instance.flags & AUDIO_FADEIN) && instance.playedDuration >= instance.fadeoutTime)
				audioState = AudioState::FadingIn;

			else if ((instance.flags & AUDIO_REPEAT) && instance.playedDuration >= fileSeconds)
				audioState = AudioState::Repeating;

			else if (instance.playedDuration >= fileSeconds)
				audioState = AudioState::Ending;

			else
				audioState = AudioState::Playing;

            if (instance.playedDuration + callbackSeconds > fileSeconds)
            {
                lengthToWrite = fileSamples - SecondsToSamples(instance.playedDuration);
                updateAudio = true;
            }



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

				for (uint32 i = 0; i < lengthToWrite; i++)
					streamBuffer[i] = (Sample)((float)(streamBuffer[i]) + (float)(readBuffer[i]));

				break;
			}
			case AudioState::FadingIn:
			{

				for (uint32 i = 0; i < lengthToWrite; i++)
				{
					UpdateFade(instance.fade, callbackSeconds, instance.fadeinTime);
					streamBuffer[i] = streamBuffer[i] + (instance.fade * readBuffer[i]);
				}
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
	}
}

