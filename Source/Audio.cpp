#include "Audio.h"
#include "Console.h"

#include <unordered_map>
#include <cassert>

AudioDriverData audioData;

std::unordered_map<std::string, AudioFile> audioList;


AudioFile LoadWavFile(const char* fileLocation)
{
    AudioFile data = {};
    if (SDL_LoadWAV(fileLocation, &data.spec, &data.buffer, &data.length) == NULL)
        ConsoleLog("Could not open %s: %s\n", fileLocation, SDL_GetError());

    if (data.spec.format != audioData.driverSpec.format)
    {

		// Change 1024 stereo sample frames at 48000Hz from Float32 to Int16.
		SDL_AudioCVT cvt;
		if (SDL_BuildAudioCVT(&cvt, data.spec.format, data.spec.channels, data.spec.freq, 
                                audioData.driverSpec.format, audioData.driverSpec.channels, audioData.driverSpec.freq) < 0)
            ConsoleLog("Failed at SDL_BuildAudioCVT for %s: %s\n", fileLocation, SDL_GetError());
        else
        {
			SDL_assert(cvt.needed); // obviously, this one is always needed.
			cvt.len = data.length;//1024 * 2 * 4;  // 1024 stereo float32 sample frames.
			cvt.buf = (uint8*)SDL_malloc(cvt.len * cvt.len_mult);
			// read your float32 data into cvt.buf here.
			uint8* writeBuffer = cvt.buf;
			for (int32 i = 0; i < cvt.len; i++)
			{
				*writeBuffer = *(data.buffer + i);
				writeBuffer++;
			}
			if (SDL_ConvertAudio(&cvt))
				ConsoleLog("Could not change format on %s: %s\n", fileLocation, SDL_GetError());
			// cvt.buf has cvt.len_cvt bytes of converted data now.

			SDL_free(data.buffer);
			data.buffer = cvt.buf;
            data.length = cvt.len;
            data.spec.channels = audioData.driverSpec.channels;
            data.spec.format= audioData.driverSpec.format;
            data.spec.freq= audioData.driverSpec.freq;
        }
    }

    assert(data.spec.channels == audioData.driverSpec.channels);
    assert(data.spec.format == audioData.driverSpec.format);
    assert(data.spec.freq == audioData.driverSpec.freq);
    assert(data.spec.samples == audioData.driverSpec.samples);
    return data;
}

double previousTime = 0;
void CSH_AudioCallback(void* userdata, Uint8* stream, int len)
{
    uint16* writeBuff = reinterpret_cast<uint16*>(stream);
    int32 count = len / sizeof(*writeBuff);
    AudioDriverData* audioDataInfo = reinterpret_cast<AudioDriverData*>(userdata);
	AudioFile audioFile = audioList["Halo"];

#if 1
	for (int32 i = 0; i < count; i++)
	{
        
        *writeBuff = 0;//USHRT_MAX / 2;
		writeBuff++;
	}

    if (audioFile.buffer)
    {

		uint8* readBuffer = audioFile.buffer + audioDataInfo->samplesTaken;

        audioDataInfo->samplesTaken = (audioDataInfo->samplesTaken + len) % audioFile.length;
		SDL_MixAudioFormat(stream, readBuffer, audioData.driverSpec.format, len, 20);
    }

    assert((uintptr_t)writeBuff == uintptr_t(stream + len));

#else

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
	want.userdata = &audioData;
	want.callback = CSH_AudioCallback;
	//should I allow any device driver or should I prefer one like directsound?
	audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	audioData.frequency = (float)have.freq;
	audioData.driverSpec = have;

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
	audioList["Halo"] = LoadWavFile("C:\\Projects\\Jumper\\Assets\\10. This is the Hour.wav");
}
