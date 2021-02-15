#include "Audio.h"
#include "WinUtilities.h"
#include "Misc.h"
#include "JSONInterop.h"

#include "SDL.h"

#include <mutex>
#include <atomic>
#include <unordered_map>
#include <cassert>

#define AUDIO_ERROR(fmt, ...)							            \
ConsoleLog(LogLevel_Error, "Audio: " fmt, __VA_ARGS__);				\
ConsoleLog(LogLevel_Error, "    At: %s(%d)", __FILE__, __LINE__)

#define AUDIO_WARNING(fmt, ...)							            \
ConsoleLog(LogLevel_Warning, "Audio: " fmt, __VA_ARGS__);			\
ConsoleLog(LogLevel_Warning, "    At: %s(%d)", __FILE__, __LINE__)

struct FileData {
	uint8* buffer = nullptr;
	uint32 length = 0;
	Volume audioType = Volume::None;
};

float g_volumes[(size_t)Volume::Count];

AudioID s_audioID = 0;
struct AudioInstance {
	AudioID ID = ++s_audioID;
	std::string name;
	uint32 playedSamples = 0;
	uint32 fadeOutSamples = 0;
	uint32 fadeInSamples = 0;
    float fade = 1;
	Volume audioType = Volume::None;

	std::atomic<uint32> endSamples = 0;
    std::atomic<bool> playing = true;
	std::atomic<float> volume = 0;
	std::atomic<float> pan = 1.0f;
	std::atomic<bool> toDelete = false;
};

// Volumes and AUDIO_CHANNELS are:
// For mono, stereo, and Quad:
// 0 = (front) left or mono
// 1 = (front) right
// 2 = rear left
// 3 = rear right

// For 5.1:
// 0 = front left
// 1 = front right
// 2 = center
// 3 = low freq
// 4 = rear left
// 5 = rear right


SDL_AudioSpec s_driverSpec;
SDL_AudioDeviceID s_audioDevice;
std::unordered_map<std::string, FileData> s_audioFiles;
std::vector<AudioInstance*> s_audioPlaying;
std::vector<AudioID> s_audioMarkedForDeletion;
std::vector<std::string> s_audioFileNames;
std::mutex s_audioMutex;


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
	//TODO:
	//assert(Main_Thread);
	for (uint32 i = 0; i < s_audioPlaying.size(); i++)
	{
		if (s_audioPlaying[i]->ID == ID)
			return (s_audioPlaying[i]);
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

AudioID PlayAudio(const AudioParams& audio)
{
	if (s_audioFiles.find(audio.nameOfSound) == s_audioFiles.end())
	{
		ConsoleLog(LogLevel::LogLevel_Error, "Audio file '%s' doesn't exist", audio.nameOfSound.c_str());
		return 0;
	}
	else if (s_audioFiles[audio.nameOfSound].buffer == nullptr)
	{
		ConsoleLog(LogLevel::LogLevel_Error, "Audio file '%s' buffer not valid", audio.nameOfSound.c_str());
		return 0;
	}
	else
	{
		AudioInstance* instance = new AudioInstance();
		instance->name = audio.nameOfSound;
		instance->audioType = s_audioFiles[audio.nameOfSound].audioType;
		instance->volume = 1.0f;

		uint32 filePlaySamples = (BytesToSamples(s_audioFiles[audio.nameOfSound].length));
		uint32 loopDuration = audio.loopCount * filePlaySamples;
		if (audio.loopCount == AUDIO_MAXLOOPS)
			loopDuration = AUDIO_MAXLOOPS;
		uint32 endSamples = SecondsToSamples(audio.secondsToPlay);

		//Ending Time
		if (audio.secondsToPlay && loopDuration) //AUDIO_DURATION && AUDIO_REPEAT
		{
			instance->endSamples = Min<uint32>(endSamples, loopDuration);
		}
		else if (audio.secondsToPlay) //AUDIO_DURATION
		{
			instance->endSamples = endSamples;
		}
		else if (loopDuration) //AUDIO_REPEAT
		{
			instance->endSamples = loopDuration;
		}
		else //NEITHER
		{
			instance->endSamples = filePlaySamples;
		}


		//Fading
		instance->fade = 1.0f;
		assert(audio.fadeInDuration >= 0);
		assert(audio.fadeOutDuration >= 0);
		uint32 fadeInDuration = SecondsToSamples(audio.fadeInDuration);
		uint32 fadeOutDuration = SecondsToSamples(audio.fadeOutDuration);
		if (audio.fadeInDuration && audio.fadeOutDuration)
		{

			if (fadeInDuration + fadeOutDuration > instance->endSamples)
			{
				float fadeTotal = (float)fadeInDuration + fadeOutDuration;
				float diff = fadeTotal - (float)instance->endSamples;
				float frac = diff / fadeTotal;

				instance->fadeInSamples = (uint32)(fadeInDuration - (fadeInDuration * frac) + 0.5f);
				instance->fadeOutSamples = (uint32)(fadeOutDuration - (fadeOutDuration * frac) + 0.5f);

				AUDIO_WARNING("Invalid fade values for: %s", instance->name.c_str());
			}
			else
			{
				instance->fadeInSamples = fadeInDuration;
				instance->fadeOutSamples = fadeOutDuration;
			}
		}
		else if (audio.fadeInDuration)
		{

			if (fadeInDuration > instance->endSamples)
				instance->fadeInSamples = fadeInDuration;
			else
				instance->fadeInSamples = fadeInDuration;
		}
		else if (audio.fadeOutDuration)
		{

			instance->fade = 1.0f;
			if (fadeOutDuration > instance->endSamples)
				instance->fadeOutSamples = fadeOutDuration;
			else
				instance->fadeOutSamples = fadeOutDuration;
		}
		else
		{
			instance->fadeInSamples = instance->fadeOutSamples = 0;
		}

		std::lock_guard<std::mutex> guard(s_audioMutex);
		s_audioPlaying.push_back(instance);
		return instance->ID;
	}
}

AudioID PlayAudio(const std::string& nameOfSound)
{

	AudioParams audio;
	audio.nameOfSound = nameOfSound;
	return PlayAudio(audio);
}

void UnPauseAudio(AudioID ID)
{
    AudioInstance* instance = GetAudioInstance(ID);
    if (instance)
        instance->playing = true;
}

AudioID s_consoleAudio = 0;
CONSOLE_FUNCTIONA(c_PlayAudio)
{
    switch (args.size())
    {
    case 0:
    {
        UnPauseAudio(s_consoleAudio);
        break;
    }
    case 1:
    {

		for (int32 i = 0; i < s_audioFileNames.size(); i++)
		{
			if (StringsMatchi(s_audioFileNames[i], args[0]))
			{
				StopAudio(s_consoleAudio);
				s_consoleAudio = PlayAudio(s_audioFileNames[i]);
				ConsoleLog(LogLevel::LogLevel_Info, "Playing audio: %s", s_audioFileNames[i].c_str());
				break;
			}
		}
        break;
    }
    case 5:
    {

        for (int32 i = 0; i < s_audioFileNames.size(); i++)
        {
            if(StringsMatchi(s_audioFileNames[i], args[0]))
            {

                AudioParams params = {
                .nameOfSound = s_audioFileNames[i],
                .fadeInDuration = static_cast<float>(stoi(args[1])),
                .fadeOutDuration = static_cast<float>(stoi(args[2])),
                .loopCount = stoi(args[3]),
                .secondsToPlay = static_cast<float>(stoi(args[4])),
                };

                StopAudio(s_consoleAudio);
                s_consoleAudio = PlayAudio(params);
				ConsoleLog(LogLevel::LogLevel_Info, "Playing audio: %s", s_audioFileNames[i].c_str());
                break;
            }
        }
        break;
	}
    default:
    {
        ConsoleLog(LogLevel::LogLevel_Warning, "Wrong number of args for 'Play': %i", args.size());
    }
    }
}

void PauseAudio(AudioID ID)
{
    AudioInstance* instance = GetAudioInstance(ID);
    if (instance)
        instance->playing = false;
}

CONSOLE_FUNCTION(c_PauseAudio)
{
    PauseAudio(s_consoleAudio);
}

void StopAudio(AudioID& ID)
{
	if (AudioInstance* instance = GetAudioInstance(ID))
	{
		if (instance->fadeOutSamples)
		{

			if (instance->playedSamples < instance->endSamples - instance->fadeOutSamples)
				instance->endSamples = instance->playedSamples + instance->fadeOutSamples;
		}
		else
		{

			std::lock_guard<std::mutex> guard(s_audioMutex);
			s_audioMarkedForDeletion.push_back(ID);
		}
	}
	else
	{
		ConsoleLog("AudioID %i not found when running StopAudio()", ID);
	}
}

CONSOLE_FUNCTION(c_StopAudio)
{
    StopAudio(s_consoleAudio);
}

FileData LoadWavFile(const char* fileLocation)
{
    SDL_AudioSpec spec = {};
    uint8* buffer = nullptr;
    uint32 length = 0;
    const SDL_AudioSpec& driverSpec = s_driverSpec;

	if (SDL_LoadWAV(fileLocation, &spec, &buffer, &length) == NULL)
	{
		ConsoleLog("%s\n", SDL_GetError());
	}
	else
	{
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
		//assert(spec.samples == driverSpec.samples);
	}
	return { buffer, length };
}

void UpdateStreamBuffer(float* streamBuffer, const Sample* readBuffer, const AudioInstance& instance)
{
	assert(instance.fade >= 0.0f && instance.fade <= 1.0f);
	assert(instance.pan >= -1.0f && instance.pan  <= 1.0f);

#if AUDIO_MAX_CHANNELS == 2

	float lvls[AUDIO_MAX_CHANNELS] = {};
	float v = fabs(instance.pan);
	assert(v <= 1.0f && v >= 0.0f);

	if (instance.pan > 0.0f)	//Right SIde
	{
		lvls[0] = v;
		lvls[1] = v * v;
	}
	else						//Left Side
	{
		lvls[0] = v * v;
		lvls[1] = v;
	}

	for (int32 i = 0; i < s_driverSpec.channels; i++)
	{
		streamBuffer[i] += (((float)readBuffer[i]) * g_volumes[size_t(Volume::Master)] *
							g_volumes[(size_t)instance.audioType] * instance.fade * instance.volume * lvls[i]);
	}


#elif AUDIO_MAX_CHANNELS == 1

	float v = fabs(instance.pan);
	streamBuffer[0] += (((float)readBuffer[0]) * g_volumes[size_t(Volume::Master)] * g_volumes[(size_t)instance.audioType] * instance.fade * instance.volumes[0] * v);

#else

	FAIL;
#endif
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

void CloseAudioThread()
{
	SDL_CloseAudioDevice(s_audioDevice);
}

std::vector<float> s_streamBuffer;
void CSH_AudioCallback(void* userdata, Uint8* stream, int len)
{
	Sample* writeBuffer = reinterpret_cast<Sample*>(stream);
	uint32 samples = BytesToSamples(len);

	memset(stream, 0, len);
	s_streamBuffer.clear();
	s_streamBuffer.resize(samples, 0);

	std::lock_guard<std::mutex> deletionGuard(s_audioMutex);
	for (int32 i = 0; i < s_audioPlaying.size(); i++)
	{
		AudioInstance& instance = *s_audioPlaying[i];
		if (!instance.playing)
			continue;
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
			float (*fadeFunc)(const AudioInstance&) = nullptr;

			if (instance.playedSamples >= instance.endSamples)				// ENDING
			{
				s_audioMarkedForDeletion.push_back(instance.ID);
			}
			else if (playedSamplesThisLoop + samplesToWrite > fileSamples)	// END OF FILE
			{
				samplesToWrite = fileSamples - playedSamplesThisLoop;
				updateAudio = true;
			}
			else if (instance.playedSamples >= instance.endSamples - instance.fadeOutSamples) //FADING OUT
			{
				fadeFunc = FadeOut;
			}
			else if (instance.playedSamples < instance.fadeInSamples)		// FADING IN
			{
				fadeFunc = FadeIn;
			}
			else // PLAYING
			{
				fadeFunc = NoFade;
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
}

void EraseAudioIDs()
{
	//TODO: Assert(Main_Thread);
	std::lock_guard<std::mutex> deletionGuard(s_audioMutex);
	for (auto& ID : s_audioMarkedForDeletion)
	{
		std::erase_if(s_audioPlaying, [ID](AudioInstance* a)
		{
			if (a->ID == ID)
			{
				delete a;
				return true;
			}
			return false;
		});
	}
	s_audioMarkedForDeletion.clear();
}

bool AudioIDValid(AudioID ID)
{
	if (GetAudioInstance(ID) == nullptr)
		return false;
	return true;
}

void SetAudioPan(AudioID ID, float v)
{
	v = Clamp(v, -1.0f, 1.0f);
	if (AudioInstance* ai = GetAudioInstance(ID))
		ai->pan = v;
}

float GetAudioVolume(const AudioID& ID)
{
	if (AudioInstance* ai = GetAudioInstance(ID))
		return ai->volume;
	return 0.0f;
}

void SetAudioVolume(const AudioID& ID, float volume)
{
	if (AudioInstance* ai = GetAudioInstance(ID))
		ai->volume = volume;
}

void InitializeAudio()
{
	SDL_AudioSpec want, have;

	SDL_memset(&want, 0, sizeof(want));
	want.freq = 48000;
	want.format = AUDIO_S16;
	want.channels = AUDIO_MAX_CHANNELS;
	want.samples = 1024;//4096;
	want.userdata = &s_driverSpec;
	want.callback = CSH_AudioCallback;
	//should I allow any device driver or should I prefer one like directsound?
	s_audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	s_driverSpec = have;

    assert(want.channels == have.channels);
    assert(want.format	 == have.format  );
    assert(want.freq	 == have.freq	 );
    assert(want.samples  == have.samples );

	if (s_audioDevice == 0)
	{
		ConsoleLog("Failed to open audio: %s", SDL_GetError());
	}
	else
	{
		if (have.format != want.format) // we let this one thing change.
			ConsoleLog("Audio format is not what was requested");

		assert(have.format == want.format);
		SDL_PauseAudioDevice(s_audioDevice, 0); /* start audio playing. */
		//SDL_Delay(1000); /* let the audio callback play some sound for 1 seconds. */

		//SDL_CloseAudioDevice(audioDevice);
	}

	//SDL_AudioQuit(); //used only for "shutting down audio" that was initilized with SDL_AudioInit()

	std::vector<AudioFileMetaData> afmd;
	LoadAudioFiles(&afmd);

	std::string name;
	for (AudioFileMetaData a : afmd)
	{
		name = "Assets\\Audio\\" + a.name + ".wav";
		s_audioFiles[a.name] = LoadWavFile(name.c_str());

		s_audioFiles[a.name].audioType = a.volumeType;

		if (s_audioFiles[a.name].buffer == nullptr)
			ConsoleLog(LogLevel_Error, "Failed to load audio: %s", a.name.c_str());
        s_audioFileNames.push_back(a.name);
	}

	for (int32 i = 1; i < (size_t)Volume::Count; i++)
		g_volumes[i] = 1.0f;
	g_volumes[(size_t)Volume::Master] = 0.5f;

    ConsoleAddCommand("audio_play", c_PlayAudio);
    ConsoleAddCommand("audio_stop", c_StopAudio);
    ConsoleAddCommand("audio_pause", c_PauseAudio);

	ConsoleAddCommand("audio_master", [](const std::vector<std::string>& args)
	{
		if (args.size() == 1)
			g_volumes[(size_t)Volume::Master] = Clamp(static_cast<float>(atof(args[0].c_str())), 0.0f, 1.0f);
		ConsoleLog(LogLevel::LogLevel_Info, "Master Audio Level is set to %.02f", g_volumes[(size_t)Volume::Master]);
	});

	//TODO: Add case insensitive string compare here
	ConsoleAddCommand("audio_debug", [](const std::vector<std::string>& args)
	{
		if (args.size() == 1)
		{
			if (StringsMatchi(args[0], "true"))
				g_debugList[DebugOptions::ShowAudio] = true;
			else if (StringsMatchi(args[0], "false"))
				g_debugList[DebugOptions::ShowAudio] = false;
		}

		ConsoleLog(LogLevel::LogLevel_Info, "Debug Option: Show Audio is now %i", g_debugList[DebugOptions::ShowAudio]);
	});
}
