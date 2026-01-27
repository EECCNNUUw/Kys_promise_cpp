#pragma once
#include <string>
#include <vector>
#include <map>
#include <SDL3/SDL.h>

// 简易音频管理器 - 使用 SDL3 原生音频
// 对应 Pascal 原版 kys_engine.pas 中的音频部分
class SoundManager {
public:
    static SoundManager& getInstance();

    bool Init();
    void Quit();

    // 播放音乐 (Music)
    // Pascal: PlayMusic / PlayCD
    // 目前使用 SDL3 占位，原版使用 MP3/MIDI
    void PlayMusic(int musicId);
    void StopMusic();

    // 播放音效 (Sound Effect)
    // Pascal: PlayWave / PlaySound
    // 对应 e.grp (音效资源)
    void PlaySound(int soundId);

    // 更新循环 (清理流等)
    void Update();

private:
    SoundManager();
    ~SoundManager();
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    struct AudioData {
        Uint8* buffer;
        Uint32 length;
        SDL_AudioSpec spec;
    };

    // 音频资源缓存
    std::map<int, AudioData> m_soundCache;
    
    // 活动音频流
    std::vector<SDL_AudioStream*> m_activeStreams;

    SDL_AudioDeviceID m_deviceId;
    int m_currentMusicId;
};
