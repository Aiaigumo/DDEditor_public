#define MINIAUDIO_IMPLEMENTATION
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c" 
#include "miniaudio.h"
#include "AudioPlayer.h"
#include <iostream>

AudioPlayer::AudioPlayer()
{
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
        std::cerr << "Failed to initialize audio engine.\n";
    }
}

AudioPlayer::~AudioPlayer()
{
    if (loaded) {
        ma_sound_uninit(&sound);
    }
    ma_engine_uninit(&engine);
}

bool AudioPlayer::load(const std::string& path)
{
    if (loaded)
    {
        ma_sound_uninit(&sound);
        loaded = false;
    }

    ma_result result = ma_sound_init_from_file(&engine, path.c_str(), 0, NULL, NULL, &sound);

    if (result != MA_SUCCESS)
    {
        const char* error_desc = ma_result_description(result);

        std::cerr << "Failed to load: " << path << "\n";
        std::cerr << "ma_result code: " << result << "\n";
        std::cerr << "Error description: " << error_desc << "\n"; 
        return false;
    }

    loaded = true;
    return true;
}

void AudioPlayer::play()
{
    if (loaded) {
        ma_sound_start(&sound);
    }
}

void AudioPlayer::playFromStart()
{
    if (loaded) {
        setSeek(0.0);
        ma_sound_start(&sound);
    }
}

void AudioPlayer::stop()
{
    if (loaded) {
        ma_sound_stop(&sound);
    }
}

bool AudioPlayer::isPlaying() const
{
    if (!loaded) return false;
    return ma_sound_is_playing(&sound);
}

void AudioPlayer::setVolume(float volume)
{
    if (loaded) {
        ma_sound_set_volume(&sound, volume);
    }
}

double AudioPlayer::getCurrentTime() const {
    if (!loaded) return 0.0;

    ma_uint64 cursor = 0;
    ma_sound_get_cursor_in_pcm_frames(&sound, &cursor);

    ma_uint32 sampleRate;
    ma_uint32 channels;
    ma_format format;
    ma_sound_get_data_format(&sound, &format, &channels, &sampleRate, nullptr, 0);

    return static_cast<double>(cursor) / static_cast<double>(sampleRate);
}

double AudioPlayer::getLength() const {
    if (!loaded) return 0.0;
    ma_uint64 totalFrames = 0;
    ma_sound_get_length_in_pcm_frames(&sound, &totalFrames);

    ma_uint32 sampleRate;
    ma_uint32 channels;
    ma_format format;
    ma_sound_get_data_format(&sound, &format, &channels, &sampleRate, nullptr, 0);

    return static_cast<double>(totalFrames) / static_cast<double>(sampleRate);
}

void AudioPlayer::setSeek(double seconds)
{
    if (!loaded) return;

    ma_uint32 sampleRate;
    ma_uint32 channels;
    ma_format format;
    ma_sound_get_data_format(&sound, &format, &channels, &sampleRate, nullptr, 0);

    ma_uint64 frameToSeek = static_cast<ma_uint64>(seconds * sampleRate);
    ma_sound_seek_to_pcm_frame(&sound, frameToSeek);
}