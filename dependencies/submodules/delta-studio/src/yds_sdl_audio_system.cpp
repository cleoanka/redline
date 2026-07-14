#include "../include/yds_sdl_audio_system.h"

#include "../include/yds_sdl_audio_device.h"

#include <SDL_audio.h>
#include <SDL.h>

ysSdlAudioSystem::ysSdlAudioSystem() : ysAudioSystem(API::Sdl) {
    // Make sure that the audio system is up
    SDL_Init(SDL_INIT_AUDIO);
}

ysSdlAudioSystem::~ysSdlAudioSystem() {
    /* void */
}

void ysSdlAudioSystem::EnumerateDevices() {
    ysAudioSystem::EnumerateDevices();

    // The output format we want
    SDL_AudioSpec desired{};
    desired.freq = 44100;
    desired.format = AUDIO_S16;
    desired.channels = 1; // TODO: multi-channel support
    desired.samples = 512;
    desired.callback = [](void *ud, Uint8 *stream, int bytes) {
        static_cast<ysSdlAudioDevice *>(ud)->FillBuffer((int16_t *)stream, bytes / sizeof(int16_t));
    };

    auto CreateDevice = [&](const char *name) {
        // Create a new device to track it
        auto *newDevice = m_devices.NewGeneric<ysSdlAudioDevice>();

        // Set the callbacks to match the new device
        desired.userdata = newDevice;

        // Try and open the device
        SDL_AudioSpec obtained;
        SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(name, 0, &desired, &obtained, 0);
        if (deviceId == 0) {
            // TODO: destroy newDevice somehow
            return;
        }

        // Device is ready
        newDevice->m_outputFormat = obtained;
        newDevice->m_deviceId = deviceId;
    };

    // SDL_GetNumAudioDevices() detects devices under the hood
    const int numDevices = SDL_GetNumAudioDevices(0);

    // Add our primary device
    CreateDevice(nullptr);

    // We could iterate over all available devices, however we only need
    // the primary one at the moment
#if 0
    for (int i = 0; i < numDevices; i++) {
        // Try and open the device
        const char *name = SDL_GetAudioDeviceName(i, 0);
        CreateDevice(name);
    }
#endif
}

void ysSdlAudioSystem::ConnectDevice(ysAudioDevice *device, ysWindow *windowAssociation) {
    ysAudioSystem::ConnectDevice(device, windowAssociation);
}

void ysSdlAudioSystem::ConnectDeviceConsole(ysAudioDevice *device) {
    ysAudioSystem::ConnectDeviceConsole(device);
}

void ysSdlAudioSystem::DisconnectDevice(ysAudioDevice *device) {
    // Stop the SDL audio callback before anything frees the device's source list. The
    // callback thread runs ysSdlAudioDevice::FillBuffer(), which iterates m_audioSources;
    // closing the device here — while it is still fully alive — blocks until any in-flight
    // callback finishes and no new ones start, preventing a use-after-free during teardown.
    if (device != nullptr) {
        auto *sdlDevice = static_cast<ysSdlAudioDevice *>(device);
        if (sdlDevice->m_deviceId != 0) {
            SDL_CloseAudioDevice(sdlDevice->m_deviceId);
            sdlDevice->m_deviceId = 0;
        }
    }
    ysAudioSystem::DisconnectDevice(device);
}