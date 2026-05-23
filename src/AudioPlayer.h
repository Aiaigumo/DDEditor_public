#pragma once
#include "miniaudio.h"
#include <string>

// Owns a miniaudio engine and sound instance for loading, seeking, and playing one audio file.
class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    bool load(const std::string& path);
    void play();
    void playFromStart();
    void stop();
    bool isPlaying() const;
    bool isLoaded() const { return loaded; }
    void setVolume(float volume);
    double getCurrentTime() const;
    double getLength() const;
    void setSeek(double seconds);

private:
    ma_engine engine;
    ma_sound sound;
    bool loaded = false;
};
