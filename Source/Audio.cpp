#include "Audio.h"
#include "Console.h"

#include "SDL.h"

#include <mutex>
#include <unordered_map>
#include <cassert>

#define AUDIO_ERROR(fmt, ...)							            \
ConsoleLog(LogLevel_Error, "Audio: " fmt, __VA_ARGS__);				\
ConsoleLog(LogLevel_Error, "    At: %s(%d)", __FILE__, __LINE__);   \

#define AUDIO_WARNING(fmt, ...)							            \
ConsoleLog(LogLevel_Warning, "Audio: " fmt, __VA_ARGS__);			\
ConsoleLog(LogLevel_Warning, "    At: %s(%d)", __FILE__, __LINE__);	\


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
AudioID noID = 0;

AudioID s_audioID = 0;

struct AudioInstance {
	AudioID ID = ++s_audioID;
	std::string name;
	uint32 playedSamples = 0;
	uint32 endSamples = 0;
	uint32 fadeOutSamples = 0;
	uint32 fadeInSamples = 0;
	float volumes[AUDIO_MAX_CHANNELS];
    float fade = 1;
	Volume audioType = Volume::None;
    bool playing = true;
};

//volumes and AUDIO_CHANNELS are:
// For mono, stereo, and Quad:
// 0 = (front) left or mono
// 1 = (front) right
// 2 = rear left
// 3 = rear right
//
// For 5.1:
// 0 = front left
// 1 = front right
// 2 = center
// 3 = low freq
// 4 = rear left
// 5 = rear right


SDL_AudioSpec s_driverSpec;
std::unordered_map<std::string, FileData> s_audioFiles;
std::vector<AudioInstance> s_audioPlaying;
std::vector<AudioID> s_audioMarkedForDeletion;
std::mutex s_playingAudioMutex;
std::mutex s_deletionQueueMutex;


uint32 BytesToSamples(uint32 bytes)
{
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

void PlayAudio(const AudioParams& audio, AudioID& ID)
{

    if (AudioInstance* i = GetAudioInstance(ID))
    {
        if(i->playing)
            i->playedSamples = 0;
        i->playing = true;
    }
    else
    {
        AudioInstance instance;
        instance.name = audio.nameOfSound;
        instance.audioType = s_audioFiles[audio.nameOfSound].audioType;
		ID = instance.ID;
        for (int32 i = 0; i < s_driverSpec.channels; i++)
            instance.volumes[i] = 1.0f;

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

        AUDIO_ERROR("Playing audio: %s", instance.name.c_str());


        //Fading
        instance.fade = 1.0f;
        assert(audio.fadeInDuration >= 0);
        assert(audio.fadeOutDuration >= 0);
        uint32 fadeInDuration = SecondsToSamples(audio.fadeInDuration);
        uint32 fadeOutDuration = SecondsToSamples(audio.fadeOutDuration);
        if (audio.fadeInDuration && audio.fadeOutDuration)
        {

            if (fadeInDuration + fadeOutDuration > instance.endSamples)
            {
                float fadeTotal = (float)fadeInDuration + fadeOutDuration;
                float diff = fadeTotal - (float)instance.endSamples;
                float frac = diff / fadeTotal;

                instance.fadeInSamples  = (uint32)(fadeInDuration  - (fadeInDuration  * frac) + 0.5f);
                instance.fadeOutSamples = (uint32)(fadeOutDuration - (fadeOutDuration * frac) + 0.5f);

                AUDIO_ERROR("Invalid fade values for: %s", instance.name.c_str());
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

    }
}

void PlayAudio(const std::string& nameOfSound, AudioID& ID)
{

	AudioParams audio;
	audio.nameOfSound = nameOfSound;
	PlayAudio(audio, ID);
}

void PlayAudio(AudioID& ID)
{
    AudioInstance* instance = GetAudioInstance(ID);
    if (instance)
        instance->playing = true;
}

void PauseAudio(const AudioID& ID)
{
    AudioInstance* instance = GetAudioInstance(ID);
    if (instance)
        instance->playing = false;
}

void StopAudio(AudioID& ID)
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
	}
	ID = 0;
}

void EraseInstance(AudioID ID)
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

	SDL_AudioCVT cvt = {};
	int32 result = SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, driverSpec.format, driverSpec.channels, driverSpec.freq);
	if (result < 0)
	{
		ConsoleLog("Failed at SDL_BuildAudioCVT for %s: %s\n", fileLocation, SDL_GetError());
	}
	else if (result == 0)
	{
		//No Conversion Required
	}
	else
	{
		SDL_assert(cvt.needed);

		cvt.len = (int32)length;
		cvt.buf = (uint8*)SDL_malloc(cvt.len * cvt.len_mult);
		memcpy(cvt.buf, buffer, cvt.len);
		if (SDL_ConvertAudio(&cvt))
			ConsoleLog("Could not change format on %s: %s\n", fileLocation, SDL_GetError());

		SDL_free(buffer);
		buffer = cvt.buf;

		//cvt.len_ratio is always garbage unless we build it ourselves
		length = cvt.len_cvt;

		spec.channels = driverSpec.channels;
		spec.format = driverSpec.format;
		spec.freq = driverSpec.freq;
	}

    assert(spec.channels == driverSpec.channels);
    assert(spec.format == driverSpec.format);
    assert(spec.freq == driverSpec.freq);
    assert(spec.samples == driverSpec.samples);
	return { buffer, length};
}

void UpdateStreamBuffer(float* streamBuffer, const Sample* readBuffer, const AudioInstance& instance)
{
	assert(instance.fade >= 0.0f && instance.fade <= 1.0f);
    for (int32 i = 0; i < s_driverSpec.channels; i++)
    {
        streamBuffer[i] += (((float)readBuffer[i]) * g_volumes[(size_t)instance.audioType] * instance.fade * instance.volumes[i]);
    }
}


float NoFade(const AudioInstance& i)
{
	return 1.0f;
}

float FadeIn(const AudioInstance& i)
{
	return Clamp<float>((float)(i.playedSamples) / i.fadeInSamples, 0.0f, 1.0f);
}

float FadeOut(const AudioInstance& i)
{
	return Clamp<float>((((float)i.endSamples - i.playedSamples) / i.fadeOutSamples), 0.0f, 1.0f);
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

            if (!instance.playing)
                continue;
			else if (instance.playedSamples >= instance.endSamples)
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

			float (*fadeFunc)(const AudioInstance&) = nullptr;
			switch (audioState)
			{
			case AudioState::None:
			{
				assert(false);
				break;
			}
			case AudioState::Playing:
			{

				fadeFunc = NoFade;
				break;
			}
			case AudioState::FadingIn:
			{

				fadeFunc = FadeIn;
				break;
			}
			case AudioState::FadingOut:
			{

				fadeFunc = FadeOut;
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

			if (fadeFunc)
			{

				for (uint32 i = 0; i < samplesToWrite; i += s_driverSpec.channels)
				{
					instance.fade = fadeFunc(instance);
                    UpdateStreamBuffer(&writeBuffer[i], &readBuffer[i], instance);
					instance.playedSamples += s_driverSpec.channels;
					samplesWritten += s_driverSpec.channels;
				}

				if (samplesWritten != samples)
				{
					samplesToWrite = samples - samplesWritten;
					updateAudio = true;
				}
			}
		}
	}

	for (uint32 i = 0; i < samples; i++)
	{
		writeBuffer[i] = (Sample)((s_streamBuffer[i] * g_volumes[(size_t)Volume::Master]));
	}

	for (int32 i = 0; i < s_audioMarkedForDeletion.size(); i++)
	{
		EraseInstance(s_audioMarkedForDeletion[i]);
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
	want.freq = 44100;
	want.format = AUDIO_S16;
	want.channels = AUDIO_MAX_CHANNELS;
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
		//name = g_workingDirectory;
		name = "C:\\Projects\\Jumper\\Assets\\Audio\\" + audioFiles[i].name + ".wav";
		s_audioFiles[audioFiles[i].name] = LoadWavFile(name.c_str());

		s_audioFiles[audioFiles[i].name].audioType = audioFiles[i].volume;
	}
	for (int32 i = 1; i < (size_t)Volume::Count; i++)
		g_volumes[i] = 1.0f;
	g_volumes[(size_t)Volume::Master] = 0.5f;
}

