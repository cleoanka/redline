#ifndef ATG_ENGINE_SIM_GAMEPAD_INPUT_H
#define ATG_ENGINE_SIM_GAMEPAD_INPUT_H

// Lightweight SDL2 GameController wrapper for redline. Polls the first attached
// controller directly each frame (SDL_GameControllerUpdate + GetAxis/GetButton), so it
// works regardless of how the host pumps the SDL event queue, and handles hot-plug by
// re-scanning when no controller is open / the open one detaches.

struct _SDL_GameController;

class GamepadInput {
public:
    enum class Button {
        A, B, X, Y,
        LeftBumper, RightBumper,
        Back, Start, Guide,
        LeftStick, RightStick,
        DpadUp, DpadDown, DpadLeft, DpadRight,
        Count
    };

    GamepadInput();
    ~GamepadInput();

    void initialize();
    void shutdown();

    // Call once per frame before reading state.
    void update();

    bool isConnected() const { return m_controller != nullptr; }
    const char *getName() const;

    // Triggers, normalized 0..1 (rest = 0).
    float getRightTrigger() const;
    float getLeftTrigger() const;

    // Sticks, normalized -1..1 with a radial dead zone applied.
    float getLeftStickX() const;
    float getLeftStickY() const;
    float getRightStickX() const;
    float getRightStickY() const;

    bool isDown(Button b) const;      // currently held
    bool wasPressed(Button b) const;  // transitioned to down this frame (edge)

private:
    void tryOpen();
    void close();
    float axis01(int sdlAxis) const;
    float axisStick(int sdlAxis) const;

    _SDL_GameController *m_controller = nullptr;
    int m_instanceId = -1;
    bool m_initialized = false;

    static constexpr int kButtonCount = (int)Button::Count;
    bool m_current[kButtonCount] = {};
    bool m_previous[kButtonCount] = {};
};

#endif /* ATG_ENGINE_SIM_GAMEPAD_INPUT_H */
