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
	float duration = 0;
	float fadeoutTime = 0;
    float fade = 1;
	uint32 repeat = 0;
	uint32 incrimenter = 0;
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

AudioID PlayAudio(const std::string& nameOfSound, float fadeOutTime, int32 loopCount, float secondsToPlay)
{

	LockMutex(s_playingAudioMutex);
	AudioInstance instance;
	instance.name = nameOfSound;
	instance.duration = secondsToPlay;
	instance.repeat = loopCount;
	instance.fadeoutTime = fadeOutTime;
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

std::vector<uint8> s_streamBuffer;
void CSH_AudioCallback(void* userdata, Uint8* stream, int len)
{
	s_streamBuffer.resize(len, 0);
	memset(s_streamBuffer.data(), 0, len);
	memset(stream, 0, len);
	uint8* streamBuffer = s_streamBuffer.data();
    int32 count = len / sizeof(*streamBuffer);

	//Blend Audio into Buffer
	LockMutex(s_playingAudioMutex);
	LockMutex(s_deletionQueueMutex);
	for (int32 i = 0; i < s_audioPlaying.size(); i++)
	{

		AudioInstance* instance = &s_audioPlaying[i];
		FileData* file = &s_audioFiles[instance->name];
		uint8* readBuffer = file->buffer + instance->incrimenter;
		uint8* writeBuffer = streamBuffer;

		int32 lengthToFill = len;
		if (instance->incrimenter + len > file->length)
			lengthToFill = file->length - instance->incrimenter;
		if (instance->duration && instance->duration < lengthToFill)
		{
			lengthToFill = static_cast<int32>(instance->duration);
			instance->duration -= lengthToFill;
			if (instance->duration <= 0)
				s_audioMarkedForDeletion.push_back(instance->ID);
		}

#if 0

		if (instance->incrimenter + instance->fadeoutTime + lengthToFill >= file->length)
		{//Do fadeout

			uint8* currentWriteBuffer = writeBuffer;
			double secondsPerByte = (1.0f / s_driverSpec.samples) / s_driverSpec.freq;
			double totalSecondsPerCallback = (static_cast<float>(len) / s_driverSpec.samples) / s_driverSpec.freq;


			for (int32 i = 0; i < lengthToFill; i++)
			{
				//(bytes / samples) / frequency = seconds
				//if ()

				//SDL_MixAudioFormat(stream, streamBuffer, s_driverSpec.format, len, 20);
				*currentWriteBuffer = *readBuffer;
				currentWriteBuffer++;
				readBuffer++;
			}
		}
		else
			memcpy(writeBuffer, readBuffer, lengthToFill);
		writeBuffer += lengthToFill;
#else

		memcpy(writeBuffer, readBuffer, lengthToFill);
#endif

		instance->incrimenter += lengthToFill;
		//loop while there is more to write
		if (instance->incrimenter >= file->length)
		{
			if (instance->repeat)
			{
				if (instance->repeat != UINT_MAX)
					instance->repeat--;
				lengthToFill = len - lengthToFill;
				readBuffer = file->buffer;

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
	SDL_MixAudioFormat(stream, streamBuffer, s_driverSpec.format, len, 20);
}

void InitilizeAudio()
{
	s_playingAudioMutex = SDL_CreateMutex();
	s_deletionQueueMutex = SDL_CreateMutex();
	SDL_AudioSpec want, have;
	SDL_AudioDeviceID audioDevice;

	SDL_memset(&want, 0, sizeof(want));
	want.freq = 44100;
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

