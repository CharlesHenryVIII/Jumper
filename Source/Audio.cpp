#include "Audio.h"
#include "Console.h"

#include "SDL.h"

#include <unordered_map>
#include <cassert>


struct FileData {
	uint8* buffer = nullptr;
	uint32 length = 0;
};

AudioID s_audioID = 0;
struct AudioInstance {
	AudioID ID = ++s_audioID;
	std::string name;
	double duration = 0;
	float fadeoutTime = 0;
    float fade = 1;
	uint32 repeat = 0;
	uint32 incrimenter = 0;
	uint16 flags = 0;
};

SDL_AudioSpec s_driverSpec;
std::unordered_map<std::string, FileData> s_audioFiles;
std::vector<AudioInstance> s_audioPlaying;
std::vector<AudioID> s_audioMarkedForDeletion;
SDL_mutex* s_playingAudioMutex = nullptr;
SDL_mutex* s_deletionQueueMutex = nullptr;

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
	LockMutex(s_playingAudioMutex);
	s_audioPlaying.push_back(instance);
	UnlockMutex(s_playingAudioMutex);
	return instance.ID;
}

AudioID PlayAudio(Audio audio)
{

	AudioInstance instance;
	instance.name = audio.nameOfSound;
	instance.duration = audio.secondsToPlay;
	instance.repeat = audio.loopCount;
	instance.fadeoutTime = audio.fadeOutTime;
	instance.flags = audio.flags;
	LockMutex(s_playingAudioMutex);
	s_audioPlaying.push_back(instance);
	UnlockMutex(s_playingAudioMutex);

	return instance.ID;
}

AudioID StopAudio(AudioID ID)
{

	LockMutex(s_deletionQueueMutex);
	s_audioMarkedForDeletion.push_back(ID);
	UnlockMutex(s_deletionQueueMutex);
	return 0;
}

void EraseFile(AudioID ID)
{

	LockMutex(s_playingAudioMutex);
	std::erase_if(s_audioPlaying, [ID](AudioInstance a) { return a.ID == ID; });
	UnlockMutex(s_playingAudioMutex);
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

//int32 SecondsToBytes(double seconds)
//{
//	return static_cast<int32>(s_driverSpec.samples * seconds * s_driverSpec.freq);
//}
//
//double BytesToSeconds(int32 bytes = 1)
//{//(bytes / samples) / frequency = seconds
//
//	return (static_cast<double>(bytes) / s_driverSpec.samples) / s_driverSpec.freq;
//}

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

std::vector<uint8> s_streamBuffer;
void CSH_AudioCallback(void* userdata, Uint8* stream, int len)
{
	s_streamBuffer.resize(len, 0);
	memset(s_streamBuffer.data(), 0, len);
	memset(stream, 0, len);
	Sample* streamBuffer = reinterpret_cast<Sample*>(s_streamBuffer.data());
	uint32 samples = BytesToSamples(len);
	double seconds = SamplesToSeconds(samples);

	//Blend Audio into Buffer
	LockMutex(s_playingAudioMutex);
	LockMutex(s_deletionQueueMutex);
	for (int32 i = 0; i < s_audioPlaying.size(); i++)
	{
		AudioInstance* instance = &s_audioPlaying[i];
		const FileData* file = &s_audioFiles[instance->name];
		Sample* readBuffer = reinterpret_cast<Sample*>(file->buffer) + instance->incrimenter;
		Sample* writeBuffer = streamBuffer;
		const uint32 fileSamples = BytesToSamples(file->length);

		uint32 lengthToFill = samples;
		if (instance->incrimenter + samples > fileSamples)
			lengthToFill = fileSamples - instance->incrimenter;
		if (instance->flags & AUDIO_DURATION)
		{

			if (uint32 instanceSecondsToSamples = SecondsToSamples(instance->duration) <= samples)
				lengthToFill = instanceSecondsToSamples;

			double instanceSeconds = SamplesToSeconds(lengthToFill);
			instance->duration -= instanceSeconds;
			if (instance->duration <= 0)
				s_audioMarkedForDeletion.push_back(instance->ID);

		}
		//if (instance->flags & AUDIO_FADEOUT)
		//{
		//	double repeatSeconds = SamplesToSeconds((fileSamples - instance->incrimenter) + instance->repeat * fileSamples);

		//	if ((instance->flags & AUDIO_DURATION && instance->duration <= instance->fadeoutTime) ||
		//		(instance->flags & AUDIO_REPEAT && (instance->fadeoutTime >= repeatSeconds)))
		//	{
		//		if (instance->flags & AUDIO_REPEAT)
		//		{
		//			if (instance->flags & AUDIO_DURATION)
		//			{

		//				if (repeatSeconds > instance->duration)
		//				{
		//					//do audio_repeat fade

		//				}
		//				else
		//				{
		//					//do audio_duration fade

		//				}

		//			}
		//			else
		//			{
		//				//do audio_Repeat fade

		//			}
		//		}
		//		else
		//		{

		//			if (instance->flags & AUDIO_DURATION)
		//			{
		//				//do audio_duration fade

		//			}
		//			else
		//			{
		//				//impossible?
		//				assert(false);

		//			}
		//		}
		//	}
		//}

		for (uint32 i = 0; i < lengthToFill; i++)
		{
			writeBuffer[i] += readBuffer[i];
		}
		//memcpy(writeBuffer, readBuffer, SamplesToBytes(lengthToFill));

		instance->incrimenter += lengthToFill;
		//loop while there is more to write
		if (instance->incrimenter >= fileSamples)
		{
			if (instance->flags & AUDIO_REPEAT && instance->repeat)
			{
				if (instance->repeat != UINT_MAX)
					instance->repeat--;
				lengthToFill = samples - lengthToFill;
				readBuffer = reinterpret_cast<Sample*>(file->buffer);

				memcpy(writeBuffer, readBuffer, lengthToFill);
				instance->incrimenter = lengthToFill;
			}
			else
				s_audioMarkedForDeletion.push_back(instance->ID);
		}
	}
	UnlockMutex(s_playingAudioMutex);

	for (int32 i = 0; i < s_audioMarkedForDeletion.size(); i++)
	{
		EraseFile(s_audioMarkedForDeletion[i]);
	}
	s_audioMarkedForDeletion.clear();
	UnlockMutex(s_deletionQueueMutex);

	//Fill Stream with Buffer
	SDL_MixAudioFormat(stream, reinterpret_cast<uint8*>(streamBuffer), s_driverSpec.format, len, 20);
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
		//snprintf();
		//sprintf();
	}
}

