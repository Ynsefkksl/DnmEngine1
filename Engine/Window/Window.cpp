#include "Engine.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "imgui/imgui_impl_sdl3.h"
#include "Utility/Log.hpp"

Window::Window(const std::string_view window_title, const glm::uvec2 windowExtent) {
    SDL_SetHint("SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS", "1");
    SDL_SetHint("SDL_JOYSTICK_RAWINPUT", "0");

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);

    uint64_t flags = SDL_WINDOW_VULKAN;

    if (windowExtent == glm::uvec2(0, 0)) {
        const SDL_DisplayID display = SDL_GetPrimaryDisplay();
        if (const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
            mode != nullptr) {
            m_extent = {mode->w, mode->h};
            flags |= SDL_WINDOW_FULLSCREEN;
        }
        else {
            LogError("cannot find display");
        }
    }
    else {
        m_extent = windowExtent;
    }

    m_window = SDL_CreateWindow(window_title.data(), m_extent.x, m_extent.y, flags);

    m_pressed_keys = SDL_GetKeyboardState(nullptr);

    int gamepad_count;
    const auto gamepads = SDL_GetGamepads(&gamepad_count);
    if (gamepad_count != 0)
        m_gamepad = SDL_OpenGamepad(gamepads[0]);
}

Window::~Window() {
    SDL_DestroyWindow(m_window);
}

void Window::HideCursor(const bool value) const {
    if (value) {
        //SDL_HideCursor();
        SDL_SetWindowRelativeMouseMode(m_window, true);
    }
    else {
        //SDL_ShowCursor();
        SDL_SetWindowRelativeMouseMode(m_window, false);
    }
}

bool Window::PollEvents() {
    memcpy(m_pressed_keys_prev.data(), m_pressed_keys, 256);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
            SDL_CloseGamepad(m_gamepad);
            m_gamepad = nullptr;
        }
        else if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
            m_gamepad = SDL_OpenGamepad(event.gdevice.which);
        }

        if (event.type == SDL_EVENT_QUIT)
            return false;
    }

    {
        m_mouse_buttons_prev = m_mouse_buttons;

        const uint32_t buttons = SDL_GetRelativeMouseState(&m_mouse_offset.x, &m_mouse_offset.y);
        SDL_GetMouseState(&m_mouse_pos.x, &m_mouse_pos.y);

        m_mouse_buttons[static_cast<uint8_t>(MouseButton::eRight)] = buttons & SDL_BUTTON_RMASK;
        m_mouse_buttons[static_cast<uint8_t>(MouseButton::eLeft)] = buttons & SDL_BUTTON_LMASK;
        m_mouse_buttons[static_cast<uint8_t>(MouseButton::eMiddle)] = buttons & SDL_BUTTON_MMASK;
    }

    // update gamepad buttons
    if (GamepadExists()) {
        m_gamepad_buttons_prev = m_gamepad_buttons;

        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eUp)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_NORTH);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eDown)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eRight)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_EAST);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eLeft)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_WEST);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eRightShoulder)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eLeftShoulder)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eRightStick)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eLeftStick)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_LEFT_STICK);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eView)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_BACK);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eMenu)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_GUIDE);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eDpadUp)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eDpadDown)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eDpadRight)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
        m_gamepad_buttons[static_cast<uint8_t>(GamepadButton::eDpadLeft)] = SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    }

    return true;
}

glm::vec2 Window::GetGamepadLeftStick() const {
    if (!GamepadExists()) return glm::vec2(0.f);
    return {SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTX)/32768.f, -SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTY)/32768.f};
}

glm::vec2 Window::GetGamepadRightStick() const {
    if (!GamepadExists()) return glm::vec2(0.f);
    return {SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHTX)/32768.f, -SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHTY)/32768.f};
}

float Window::GetGamepadRightTrigger() const {
    if (!GamepadExists()) return 0.f;
    return SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)/32768.f;
}

float Window::GetGamepadLeftTrigger() const {
    if (!GamepadExists()) return 0.f;
    return SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER)/32768.f;
}

inline std::vector<const char*> Window::GetVulkanExtensions() {
    uint32_t count;
    SDL_Vulkan_GetInstanceExtensions(&count);

    std::vector<const char*> extensions(count);

    std::memcpy(extensions.data(), SDL_Vulkan_GetInstanceExtensions(nullptr), sizeof(char*) * count);

    return extensions;
}
