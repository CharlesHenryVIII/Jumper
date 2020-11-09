#include "Audio.h"
#include "Console.h"

#include "SDL.h"

#include <unordered_map>
#include <cassert>


struct FileData {
	uint8* buffer = nullptr;
	uint32 length = 0;
	uint32 incrimenter = 0;
};

AudioID s_audioID = 0;
struct AudioFile {
	AudioID ID = ++s_audioID;
	std::string name;
	float duration;
	uint32 repeat;
};

SDL_AudioSpec s_driverSpec;
std::unordered_map<std::string, FileData> s_audioFiles;
//std::vector<AudioFile> s_audioQueue;
std::vector<AudioFile> s_audioPlaying;
//uint64 s_samplesTaken = 0;

AudioID PlayAudio(const std::string& nameOfSound, int32 loopCount, float secondsToPlay)
{

	AudioFile file;
	file.name = nameOfSound;
	file.duration = secondsToPlay;
	file.repeat = loopCount;
	s_audioPlaying.push_back(file);

	return file.ID;
}

AudioID StopAudio(AudioID ID)
{
	std::erase_if(s_audioPlaying, [ID](AudioFile a) { return a.ID == ID; });
	return 0;
}

FileData LoadWavFile(const char* fileLocation)
{
    //AudioFile data = {};
    SDL_AudioSpec spec = {};
    uint8* buffer = nullptr;
    uint32 length = 0;

    const SDL_AudioSpec& driverSpec = s_driverSpec;

	if (SDL_LoadWAV(fileLocation, &spec, &buffer, &length) == NULL)
	{
        ConsoleLog("%s\n", SDL_GetError());
        //ConsoleLog("Could not open %s: %s\n", fileLocation, SDL_GetError());
	}

    if (spec.format != driverSpec.format || 
		spec.channels != driverSpec.channels || 
		spec.freq != driverSpec.freq)
    {

		// Change 1024 stereo sample frames at 48000Hz from Float32 to Int16.
		SDL_AudioCVT cvt;
		if (SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq,
							  driverSpec.format, driverSpec.channels, driverSpec.freq) < 0)
		{
            ConsoleLog("Failed at SDL_BuildAudioCVT for %s: %s\n", fileLocation, SDL_GetError());
		}
        else
        {
			SDL_assert(cvt.needed); // obviously, this one is always needed.
			cvt.len = length;//1024 * 2 * 4;  // 1024 stereo float32 sample frames.
			cvt.buf = (uint8*)SDL_malloc(cvt.len * cvt.len_mult);
			// read your float32 data into cvt.buf here.
			uint8* writeBuffer = cvt.buf;
			for (int32 i = 0; i < cvt.len; i++)
			{
				*writeBuffer = *(buffer + i);
				writeBuffer++;
			}
			if (SDL_ConvertAudio(&cvt))
				ConsoleLog("Could not change format on %s: %s\n", fileLocation, SDL_GetError());
			// cvt.buf has cvt.len_cvt bytes of converted data now.

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
	s_streamBuffer.reserve(len);
	uint8* realStream = reinterpret_cast<uint8*>(stream);
	uint8* bufferStream = reinterpret_cast<uint8*>(s_streamBuffer.data());
	uint8* writeBuffer = bufferStream;
    int32 count = len / sizeof(*writeBuffer);
	FileData* audioFile = &s_audioFiles["Halo"];

	//Clear Buffer
	for (int32 i = 0; i < len; i++)
	{

		*writeBuffer = 0;
		*realStream = 0;
		writeBuffer++;
		realStream++;
	}
    assert((uintptr_t)writeBuffer == uintptr_t(s_streamBuffer.data() + len));


	//Blend Audio into Buffer
	std::vector<AudioID> audioMarkedForDeletion;
	for (int32 i = 0; i < s_audioPlaying.size(); i++)
	{

		AudioFile* audio = &s_audioPlaying[i];
		FileData* file = &s_audioFiles[audio->name];
		uint8* readBuffer = file->buffer + file->incrimenter;
		uint8* writeBuffer = bufferStream;
		int32 length = len;
		if (file->incrimenter + len > file->length)
			length = file->length - file->incrimenter;

		for (int32 j = 0; j < length; j++)
		{

			*writeBuffer += *readBuffer;
			writeBuffer++;
			readBuffer++;
		}
		file->incrimenter += length;
		if (file->incrimenter >= file->length)
		{
			if (audio->repeat)
			{
				if (audio->repeat != UINT_MAX)
					audio->repeat--;
				length = len - length;
				readBuffer = file->buffer;
				for (int32 j = 0; j < length; j++)
				{

					*writeBuffer += *readBuffer;
					writeBuffer++;
					readBuffer++;
				}
				file->incrimenter = length;
			}
			else
				audioMarkedForDeletion.push_back(audio->ID);
		}
	}
	for (int32 i = 0; i < audioMarkedForDeletion.size(); i++)
	{
		StopAudio(audioMarkedForDeletion[i]);
	}
	audioMarkedForDeletion.clear();

	//Fill Stream with Buffer
	SDL_MixAudioFormat(stream, bufferStream, s_driverSpec.format, len, 20);


#if 0
    //LRLRLR ordering
    //Should I use a buffer method or audio queing?
    ConsoleLog("length: %d, count: %d, timeStamp: %0.5f\n", len, count, audioDataInfo->totalTime - previousTime);
    previousTime = audioDataInfo->totalTime;
    for (int32 i = 0; i < count / 2; i++)
    {
        
        float sampleFreq = 1 / audioDataInfo->frequency;
        uint16 information = (uint16)(sinf((audioDataInfo->samplesTaken++) * sampleFreq * tau * 100) * 20000 + 20000);

        *buf = information;
        buf += 1;
        *buf = information;
        buf++;
    }

    //audioDataInfo->samplesTaken += count / 2;
    assert((uintptr_t)writeBuff == uintptr_t(stream + len));
#endif
}

void InitilizeAudio()
{
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
	}
}

