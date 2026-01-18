#pragma once

#include <array>
#include <string_view>
#include <vector>

#include "Utility/EngineDefines.hpp"
#include <glm/glm.hpp>

//forward declaration
struct SDL_Window;
struct SDL_Gamepad;
typedef struct SDL_GLContextState *SDL_GLContext;

enum class Key : uint16_t {
    eUP          = 82,   // SDL_SCANCODE_UP
    eDOWN        = 81,   // SDL_SCANCODE_DOWN
    eLEFT        = 80,   // SDL_SCANCODE_LEFT
    eRIGHT       = 79,   // SDL_SCANCODE_RIGHT

    eW           = 26,   // SDL_SCANCODE_W
    eA           = 4,    // SDL_SCANCODE_A
    eS           = 22,   // SDL_SCANCODE_S
    eD           = 7,    // SDL_SCANCODE_D
    eQ           = 20,   // SDL_SCANCODE_Q
    eE           = 8,    // SDL_SCANCODE_E
    eR           = 21,   // SDL_SCANCODE_R
    eT           = 23,   // SDL_SCANCODE_T
    eY           = 28,   // SDL_SCANCODE_Y
    eU           = 24,   // SDL_SCANCODE_U
    eI           = 12,   // SDL_SCANCODE_I
    eO           = 18,   // SDL_SCANCODE_O
    eP           = 19,   // SDL_SCANCODE_P
    eF           = 9,    // SDL_SCANCODE_F
    eG           = 10,   // SDL_SCANCODE_G
    eH           = 11,   // SDL_SCANCODE_H
    eJ           = 13,   // SDL_SCANCODE_J
    eK           = 14,   // SDL_SCANCODE_K
    eL           = 15,   // SDL_SCANCODE_L
    eZ           = 29,   // SDL_SCANCODE_Z
    eX           = 27,   // SDL_SCANCODE_X
    eC           = 6,    // SDL_SCANCODE_C
    eV           = 25,   // SDL_SCANCODE_V
    eB           = 5,    // SDL_SCANCODE_B
    eN           = 17,   // SDL_SCANCODE_N
    eM           = 16,   // SDL_SCANCODE_M

    eSpace       = 44,   // SDL_SCANCODE_SPACE
    eEnter       = 40,   // SDL_SCANCODE_RETURN
    eEscape      = 41,   // SDL_SCANCODE_ESCAPE

    e0           = 39,   // SDL_SCANCODE_0
    e1           = 30,   // SDL_SCANCODE_1
    e2           = 31,   // SDL_SCANCODE_2
    e3           = 32,   // SDL_SCANCODE_3
    e4           = 33,   // SDL_SCANCODE_4
    e5           = 34,   // SDL_SCANCODE_5
    e6           = 35,   // SDL_SCANCODE_6
    e7           = 36,   // SDL_SCANCODE_7
    e8           = 37,   // SDL_SCANCODE_8
    e9           = 38,   // SDL_SCANCODE_9

    eLeftShift   = 225,  // SDL_SCANCODE_LSHIFT
    eRightShift  = 229,  // SDL_SCANCODE_RSHIFT
    eLeftControl = 224,  // SDL_SCANCODE_LCTRL
    eRightControl= 228,  // SDL_SCANCODE_RCTRL
    eLeftAlt     = 226,  // SDL_SCANCODE_LALT
    eRightAlt    = 230,  // SDL_SCANCODE_RALT

    eTab         = 43    // SDL_SCANCODE_TAB
};

enum class MouseButton : uint32_t {
    eRight = 0,
    eLeft = 1,
    eMiddle = 2
};

enum class GamepadButton : uint8_t {
    eUp = 0,
    eDown = 1,
    eRight = 2,
    eLeft = 3,
    eRightShoulder = 4,
    eLeftShoulder = 5,
    eRightStick = 6,
    eLeftStick = 7,
    eView = 8,
    eMenu = 9,
    eDpadUp = 10,
    eDpadDown = 11,
    eDpadRight = 12,
    eDpadLeft = 13,
};

class ENGINE_API Window {
public:
    Window(std::string_view window_title, glm::uvec2 windowExtent);
    ~Window();

    void HideCursor(bool value) const;

    [[nodiscard]] glm::vec2 GetExtent() const { return m_extent; }
    [[nodiscard]] SDL_Window* GetSDLWindow() const { return m_window; };

    //keyboard
    [[nodiscard]] bool IsKeyPressed(Key key) const;
    [[nodiscard]] bool IsKeyReleased(Key key) const;
    [[nodiscard]] bool IsKeyHeld(Key key) const;
    [[nodiscard]] bool CurrentKeyState(Key key) const;
    [[nodiscard]] bool PrevKeyState(Key key) const;

    //mouse
    [[nodiscard]] bool IsMouseButtonPressed(MouseButton button) const;
    [[nodiscard]] bool IsMouseButtonReleased(MouseButton button) const;
    [[nodiscard]] bool IsMouseButtonHeld(MouseButton button) const;
    [[nodiscard]] bool CurrentMouseButtonState(MouseButton button) const;
    [[nodiscard]] bool PrevMouseButtonState(MouseButton button) const;
    [[nodiscard]] glm::vec2 GetMousePos() const { return m_mouse_pos; }
    [[nodiscard]] glm::vec2 GetMouseOffset() const { return m_mouse_offset; }

    //gamepad
    [[nodiscard]] bool GamepadExists() const { return m_gamepad != nullptr; }
    [[nodiscard]] glm::vec2 GetGamepadLeftStick() const;
    [[nodiscard]] glm::vec2 GetGamepadRightStick() const;
    [[nodiscard]] float GetGamepadRightTrigger() const;
    [[nodiscard]] float GetGamepadLeftTrigger() const;
    [[nodiscard]] bool GetGamepadButtonPressed(GamepadButton button) const;
    [[nodiscard]] bool GetGamepadButtonReleased(GamepadButton button) const;
    [[nodiscard]] bool GetGamepadButtonHeld(GamepadButton button) const;
    [[nodiscard]] bool CurrentGamepadButtonState(GamepadButton button) const;
    [[nodiscard]] bool PrevGamepadButtonState(GamepadButton button) const;

    static std::vector<const char*> GetVulkanExtensions();

    bool PollEvents();
private:
    SDL_Window* m_window = nullptr;
    SDL_Gamepad* m_gamepad = nullptr;
    glm::vec2 m_extent{};

    glm::vec2 m_mouse_pos{};
    glm::vec2 m_mouse_offset{};

    const bool* m_pressed_keys{nullptr};
    std::array<bool, 256> m_pressed_keys_prev{};

    std::array<bool, 14> m_gamepad_buttons{};
    std::array<bool, 14> m_gamepad_buttons_prev{};

    std::array<bool, 3> m_mouse_buttons{};
    std::array<bool, 3> m_mouse_buttons_prev{};
};

inline bool Window::IsKeyPressed(const Key key) const {
    if (CurrentKeyState(key) == true && PrevKeyState(key) == false) {
        return true;
    }
    return false;
}

inline bool Window::IsKeyReleased(const Key key) const {
    if (CurrentKeyState(key) == false && PrevKeyState(key) == true) {
        return true;
    }
    return false;
}

inline bool Window::IsKeyHeld(const Key key) const {
    return CurrentKeyState(key);
}

inline bool Window::CurrentKeyState(const Key key) const {
    return m_pressed_keys[static_cast<uint16_t>(key)];
}

inline bool Window::PrevKeyState(const Key key) const {
    return m_pressed_keys_prev[static_cast<uint16_t>(key)];
}

inline bool Window::IsMouseButtonPressed(const MouseButton button) const {
    if (CurrentMouseButtonState(button) == false && PrevMouseButtonState(button) == true) {
        return true;
    }
    return false;
}

inline bool Window::IsMouseButtonReleased(const MouseButton button) const {
    if (CurrentMouseButtonState(button) == false && PrevMouseButtonState(button) == true) {
        return true;
    }
    return false;
}

inline bool Window::IsMouseButtonHeld(const MouseButton button) const {
    return CurrentMouseButtonState(button);
}

inline bool Window::CurrentMouseButtonState(const MouseButton button) const {
    return m_mouse_buttons[static_cast<uint16_t>(button)];
}

inline bool Window::PrevMouseButtonState(const MouseButton button) const {
    return m_mouse_buttons_prev[static_cast<uint16_t>(button)];
}

inline bool Window::GetGamepadButtonPressed(const GamepadButton button) const {
    if (CurrentGamepadButtonState(button) == true && PrevGamepadButtonState(button) == false) {
        return true;
    }
    return false;
}

inline bool Window::GetGamepadButtonReleased(const GamepadButton button) const {
    if (CurrentGamepadButtonState(button) == false && PrevGamepadButtonState(button) == true) {
        return true;
    }
    return false;
}

inline bool Window::GetGamepadButtonHeld(const GamepadButton button) const {
    return CurrentGamepadButtonState(button);
}

inline bool Window::CurrentGamepadButtonState(const GamepadButton button) const {
    return m_gamepad_buttons[static_cast<uint16_t>(button)];
}

inline bool Window::PrevGamepadButtonState(const GamepadButton button) const {
    return m_gamepad_buttons_prev[static_cast<uint16_t>(button)];
}