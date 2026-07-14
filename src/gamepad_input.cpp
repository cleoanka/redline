#include "../include/gamepad_input.h"

#include <SDL.h>

#include <cmath>

namespace {
    // SDL button per GamepadInput::Button (indices must match the enum order).
    const SDL_GameControllerButton kSdlButtons[] = {
        SDL_CONTROLLER_BUTTON_A,
        SDL_CONTROLLER_BUTTON_B,
        SDL_CONTROLLER_BUTTON_X,
        SDL_CONTROLLER_BUTTON_Y,
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_BACK,
        SDL_CONTROLLER_BUTTON_START,
        SDL_CONTROLLER_BUTTON_GUIDE,
        SDL_CONTROLLER_BUTTON_LEFTSTICK,
        SDL_CONTROLLER_BUTTON_RIGHTSTICK,
        SDL_CONTROLLER_BUTTON_DPAD_UP,
        SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT,
        SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    };
    constexpr float kStickDeadZone = 8000.0f / 32767.0f;
    constexpr float kTriggerFloor = 1500.0f / 32767.0f;  // ignore trigger rattle at rest
}

GamepadInput::GamepadInput() {}
GamepadInput::~GamepadInput() { shutdown(); }

void GamepadInput::initialize() {
    if (m_initialized) return;
    // The window/audio systems init their own SDL subsystems; add game controller here.
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) == 0) {
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
    // Keep receiving controller state even when the window is not focused, so a paused
    // background window still reads the pad; harmless otherwise.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    m_initialized = true;
    tryOpen();
}

void GamepadInput::shutdown() {
    close();
    if (m_initialized) {
        if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) != 0) SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        m_initialized = false;
    }
}

void GamepadInput::tryOpen() {
    const int n = SDL_NumJoysticks();
    for (int i = 0; i < n; ++i) {
        if (SDL_IsGameController(i)) {
            SDL_GameController *c = SDL_GameControllerOpen(i);
            if (c != nullptr) {
                m_controller = c;
                SDL_Joystick *j = SDL_GameControllerGetJoystick(c);
                m_instanceId = (j != nullptr) ? SDL_JoystickInstanceID(j) : -1;
                for (int b = 0; b < kButtonCount; ++b) { m_current[b] = m_previous[b] = false; }
                return;
            }
        }
    }
}

void GamepadInput::close() {
    if (m_controller != nullptr) {
        SDL_GameControllerClose(m_controller);
        m_controller = nullptr;
        m_instanceId = -1;
    }
}

void GamepadInput::update() {
    if (!m_initialized) return;
    SDL_GameControllerUpdate();

    // Hot-plug: drop a detached controller, and (re)scan when none is open.
    if (m_controller != nullptr && !SDL_GameControllerGetAttached(m_controller)) {
        close();
    }
    if (m_controller == nullptr) {
        tryOpen();
        if (m_controller == nullptr) return;
    }

    for (int b = 0; b < kButtonCount; ++b) {
        m_previous[b] = m_current[b];
        m_current[b] = SDL_GameControllerGetButton(m_controller, kSdlButtons[b]) != 0;
    }
}

const char *GamepadInput::getName() const {
    return (m_controller != nullptr) ? SDL_GameControllerName(m_controller) : "";
}

float GamepadInput::axis01(int sdlAxis) const {
    if (m_controller == nullptr) return 0.0f;
    const int raw = SDL_GameControllerGetAxis(m_controller, (SDL_GameControllerAxis)sdlAxis);
    float v = (float)raw / 32767.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    if (v < kTriggerFloor) v = 0.0f;
    return v;
}

float GamepadInput::axisStick(int sdlAxis) const {
    if (m_controller == nullptr) return 0.0f;
    const int raw = SDL_GameControllerGetAxis(m_controller, (SDL_GameControllerAxis)sdlAxis);
    float v = (float)raw / 32767.0f;
    if (v > 1.0f) v = 1.0f;
    if (v < -1.0f) v = -1.0f;
    const float mag = std::fabs(v);
    if (mag < kStickDeadZone) return 0.0f;
    // Rescale past the dead zone so motion starts smoothly from 0.
    const float scaled = (mag - kStickDeadZone) / (1.0f - kStickDeadZone);
    return (v < 0.0f ? -scaled : scaled);
}

float GamepadInput::getRightTrigger() const { return axis01(SDL_CONTROLLER_AXIS_TRIGGERRIGHT); }
float GamepadInput::getLeftTrigger() const { return axis01(SDL_CONTROLLER_AXIS_TRIGGERLEFT); }
float GamepadInput::getLeftStickX() const { return axisStick(SDL_CONTROLLER_AXIS_LEFTX); }
float GamepadInput::getLeftStickY() const { return axisStick(SDL_CONTROLLER_AXIS_LEFTY); }
float GamepadInput::getRightStickX() const { return axisStick(SDL_CONTROLLER_AXIS_RIGHTX); }
float GamepadInput::getRightStickY() const { return axisStick(SDL_CONTROLLER_AXIS_RIGHTY); }

bool GamepadInput::isDown(Button b) const {
    return m_controller != nullptr && m_current[(int)b];
}

bool GamepadInput::wasPressed(Button b) const {
    return m_controller != nullptr && m_current[(int)b] && !m_previous[(int)b];
}
