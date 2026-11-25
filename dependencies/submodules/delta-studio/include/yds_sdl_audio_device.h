#ifndef YDS_SDL_AUDIO_DEVICE_H
#define YDS_SDL_AUDIO_DEVICE_H

#include "yds_audio_device.h"

#include <SDL_audio.h>

class ysSdlAudioSystem;

class ysSdlAudioDevice : public ysAudioDevice {
    friend ysSdlAudioSystem;

public:
    ysSdlAudioDevice();
    virtual ~ysSdlAudioDevice() override;

    virtual ysAudioBuffer *CreateBuffer(const ysAudioParameters *parameters, SampleOffset size) override;
    virtual ysAudioSource *CreateSource(const ysAudioParameters *parameters, SampleOffset size) override;
    virtual ysAudioSource *CreateSource(ysAudioBuffer *sourceBuffer) override;
    virtual void UpdateAudioSources() override;

private:
    void FillBuffer(int16_t *audio, int frames);

private:
    SDL_AudioDeviceID m_deviceId;
    // Value-initialized: when SDL_OpenAudioDevice fails the device stays listed with
    // m_deviceId == 0 and this zeroed format, so CreateSource() reliably returns nullptr
    // instead of comparing against indeterminate values.
    SDL_AudioSpec m_outputFormat{};
};

#endif /* YDS_SDL_AUDIO_DEVICE_H */