#include "input.h"
#include "jvs.h"
#include "bana.h"
#include <csignal>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <X11/keysym.h>
#include "hook.h"
#include "config.h"
#include "touch.h"
#include "toml.hpp"
#include <map>
#include <SDL2/SDL.h>

std::map<const char*, KeySym> xKeys = {
        { "XK_F1", XK_F1 },
        { "XK_F2", XK_F2 },
        { "XK_F3", XK_F3 },
        { "XK_F4", XK_F4 },
        { "XK_F5", XK_F5 },
        { "XK_F6", XK_F6 },
        { "XK_F7", XK_F7 },
        { "XK_F8", XK_F8 },
        { "XK_F9", XK_F9 },
        { "XK_F10", XK_F10 },
        { "XK_F11", XK_F11 },
        { "XK_F12", XK_F12 },
        { "XK_0", XK_0 },
        { "XK_1", XK_1 },
        { "XK_2", XK_2 },
        { "XK_3", XK_3 },
        { "XK_4", XK_4 },
        { "XK_5", XK_5 },
        { "XK_6", XK_6 },
        { "XK_7", XK_7 },
        { "XK_8", XK_8 },
        { "XK_9", XK_9 },
        { "XK_A", XK_a },
        { "XK_B", XK_b },
        { "XK_C", XK_c },
        { "XK_D", XK_d },
        { "XK_E", XK_e },
        { "XK_F", XK_f },
        { "XK_G", XK_g },
        { "XK_H", XK_h },
        { "XK_I", XK_i },
        { "XK_J", XK_j },
        { "XK_K", XK_k },
        { "XK_L", XK_l },
        { "XK_M", XK_m },
        { "XK_N", XK_n },
        { "XK_O", XK_o },
        { "XK_P", XK_p },
        { "XK_Q", XK_q },
        { "XK_R", XK_r },
        { "XK_S", XK_s },
        { "XK_T", XK_t },
        { "XK_U", XK_u },
        { "XK_V", XK_v },
        { "XK_W", XK_w },
        { "XK_X", XK_x },
        { "XK_Y", XK_y },
        { "XK_Z", XK_z },
        { "XK_leftarrow", XK_Left },
        { "XK_uparrow", XK_Up },
        { "XK_rightarrow", XK_Right },
        { "XK_downarrow", XK_Down }
};

std::map<const char*, SDL_GameControllerButton> sdlButtons = {
        { "SDL_START", SDL_CONTROLLER_BUTTON_START },
        { "SDL_BACK", SDL_CONTROLLER_BUTTON_BACK },

        { "SDL_LEFTSTICK", SDL_CONTROLLER_BUTTON_LEFTSTICK },
        { "SDL_RIGHTSTICK", SDL_CONTROLLER_BUTTON_RIGHTSTICK },

        {"SDL_B", SDL_CONTROLLER_BUTTON_B},
        {"SDL_X", SDL_CONTROLLER_BUTTON_X},
        {"SDL_A", SDL_CONTROLLER_BUTTON_A},
        {"SDL_Y", SDL_CONTROLLER_BUTTON_Y},

        { "SDL_RIGHTSHOULDER", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER },
        { "SDL_LEFTSHOULDER", SDL_CONTROLLER_BUTTON_LEFTSHOULDER },

        { "SDL_DPAD_LEFT", SDL_CONTROLLER_BUTTON_DPAD_LEFT },
        { "SDL_DPAD_UP", SDL_CONTROLLER_BUTTON_DPAD_UP },
        { "SDL_DPAD_DOWN", SDL_CONTROLLER_BUTTON_DPAD_DOWN },
        { "SDL_DPAD_RIGHT", SDL_CONTROLLER_BUTTON_DPAD_RIGHT }
};

std::map<const char*, SDL_GameControllerAxis> sdlAxis = {
        { "SDL_TRIGGERRIGHT", SDL_CONTROLLER_AXIS_TRIGGERRIGHT },
        { "SDL_TRIGGERLEFT", SDL_CONTROLLER_AXIS_TRIGGERLEFT },

        {"SDL_LEFTX", SDL_CONTROLLER_AXIS_LEFTX },
        {"SDL_RIGHTX", SDL_CONTROLLER_AXIS_RIGHTX },

        {"SDL_LEFTY", SDL_CONTROLLER_AXIS_LEFTY },
        {"SDL_RIGHTY", SDL_CONTROLLER_AXIS_RIGHTY }
};

SDL_GameControllerButton ctrlKeybindTestSwitch = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
SDL_GameControllerButton ctrlKeybindTestUp = SDL_CONTROLLER_BUTTON_DPAD_UP;
SDL_GameControllerButton ctrlKeybindTestDown = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
SDL_GameControllerButton ctrlKeybindTestEnter = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
SDL_GameControllerButton ctrlKeybindShiftUp = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
SDL_GameControllerButton ctrlKeybindShiftDown = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
SDL_GameControllerButton ctrlKeybindPerspective = SDL_CONTROLLER_BUTTON_Y;
SDL_GameControllerButton ctrlKeybindInterrupt = SDL_CONTROLLER_BUTTON_A;
SDL_GameControllerButton ctrlKeybindCard = SDL_CONTROLLER_BUTTON_B;
SDL_GameControllerButton ctrlKeybindService = SDL_CONTROLLER_BUTTON_X;

SDL_GameControllerAxis controllerAxisSteer = SDL_CONTROLLER_AXIS_LEFTX;
SDL_GameControllerAxis controllerAxisGas = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
SDL_GameControllerAxis controllerAxisBrake = SDL_CONTROLLER_AXIS_TRIGGERLEFT;

bool gasReversed = false;
bool gasFull = false;
bool brakesReversed = false;
bool brakesFull = false;
float deadzone = 0.05f;
bool useLegacyPedalsAxis = false;

KeySym kbKeybindTestSwitch = XK_Left;
KeySym kbKeybindTestUp = XK_Up;
KeySym kbKeybindTestDown = XK_Down;
KeySym kbKeybindTestEnter = XK_Right;
KeySym kbKeybindShiftUp = XK_p;
KeySym kbKeybindShiftDown = XK_o;
KeySym kbKeybindPerspective = XK_e;
KeySym kbKeybindInterrupt = XK_q;
KeySym kbKeybindGas = XK_w;
KeySym kbKeybindLeft = XK_a;
KeySym kbKeybindBrakes = XK_s;
KeySym kbKeybindRight = XK_d;
KeySym kbKeybindCard = XK_c;
KeySym kbKeybindServiceSwitch = XK_v;

bool kbSteerLeft = false;
bool kbSteerRight = false;

bool testState = false;

bool inputStopped = false;

Display* display = NULL;
Window window = 0;

#define SDL_WINDOW

defineHook(int, initWindow, int a1) {
    int ret = callOld(initWindow, a1);
    if (ret == 0) {
        display = *(Display**)(a1 + 4);
        window = *(Window*)(a1 + 12);

        XSelectInput(display, window, ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask);
    }
    return ret;
}

void tryLoadCtrlAxis(toml::node_view<toml::node> table, SDL_GameControllerAxis* btn, std::string name) {
    std::string configKey = table[name].as_string()->get();

    for (const auto& [key, value] : sdlAxis) {
        if (configKey != key)
            continue;
        *btn = value;
        return;
    }
    printf("cant find sdl axis %s for %s\n", configKey.c_str(), name.c_str());
}

void tryLoadCtrl(toml::node_view<toml::node> table, SDL_GameControllerButton * btn, std::string name) {
    std::string configKey = table[name].as_string()->get();

    for (const auto& [key, value] : sdlButtons) {
        if (configKey != key)
            continue;
        *btn = value;
        return;
    }
    printf("cant find sdl button %s for %s\n", configKey.c_str(), name.c_str());
}

void tryLoadKb(toml::node_view<toml::node> table, KeySym* sym, std::string name) {
    std::string configKey = table[name].as_string()->get();

    for (const auto& [key, value] : xKeys) {
        if (configKey != key)
            continue;
        *sym = value;
        return;
    }
    printf("cant find key %s for %s\n", configKey.c_str(), name.c_str());
}

bool loadKeybinds() {
    try {
        auto config = toml::parse_file( "keybinds.toml" );

        auto controllerConfig = config["controller"];
        tryLoadCtrlAxis(controllerConfig, &controllerAxisGas, "gas");
        tryLoadCtrlAxis(controllerConfig, &controllerAxisSteer, "wheel");
        tryLoadCtrlAxis(controllerConfig, &controllerAxisBrake, "brakes");
        tryLoadCtrl(controllerConfig, &ctrlKeybindCard, "card");
        tryLoadCtrl(controllerConfig, &ctrlKeybindPerspective, "perspective");
        tryLoadCtrl(controllerConfig, &ctrlKeybindInterrupt, "interrupt");
        tryLoadCtrl(controllerConfig, &ctrlKeybindService, "service");
        tryLoadCtrl(controllerConfig, &ctrlKeybindShiftUp, "shift_up");
        tryLoadCtrl(controllerConfig, &ctrlKeybindShiftDown, "shift_down");
        tryLoadCtrl(controllerConfig, &ctrlKeybindTestSwitch, "test_switch");
        tryLoadCtrl(controllerConfig, &ctrlKeybindTestUp, "test_up");
        tryLoadCtrl(controllerConfig, &ctrlKeybindTestDown, "test_down");
        tryLoadCtrl(controllerConfig, &ctrlKeybindTestEnter, "test_enter");

        auto keyboardConfig = config["keyboard"];
        tryLoadKb(keyboardConfig, &kbKeybindGas, "gas");
        tryLoadKb(keyboardConfig, &kbKeybindLeft, "left");
        tryLoadKb(keyboardConfig, &kbKeybindBrakes, "brakes");
        tryLoadKb(keyboardConfig, &kbKeybindRight, "right");
        tryLoadKb(keyboardConfig, &kbKeybindCard, "card");
        tryLoadKb(keyboardConfig, &kbKeybindPerspective, "perspective");
        tryLoadKb(keyboardConfig, &kbKeybindInterrupt, "interrupt");
        tryLoadKb(keyboardConfig, &kbKeybindServiceSwitch, "service");
        tryLoadKb(keyboardConfig, &kbKeybindShiftUp, "shift_up");
        tryLoadKb(keyboardConfig, &kbKeybindShiftDown, "shift_down");
        tryLoadKb(keyboardConfig, &kbKeybindTestSwitch, "test_switch");
        tryLoadKb(keyboardConfig, &kbKeybindTestUp, "test_up");
        tryLoadKb(keyboardConfig, &kbKeybindTestDown, "test_down");
        tryLoadKb(keyboardConfig, &kbKeybindTestEnter, "test_enter");

        if (controllerConfig["gas_reversed"].is_boolean())
            gasReversed = controllerConfig["gas_reversed"].as_boolean()->get();
        if (controllerConfig["brakes_reversed"].is_boolean())
            brakesReversed = controllerConfig["brakes_reversed"].as_boolean()->get();

        if (controllerConfig["gas_full"].is_boolean())
            gasFull = controllerConfig["gas_full"].as_boolean()->get();
        if (controllerConfig["brakes_full"].is_boolean())
            brakesFull = controllerConfig["brakes_full"].as_boolean()->get();

        if (controllerConfig["deadzone"].is_floating_point())
            deadzone = controllerConfig["deadzone"].as_floating_point()->get();
        if (controllerConfig["use_legacy_pedals_axis"].is_boolean())
            useLegacyPedalsAxis = controllerConfig["use_legacy_pedals_axis"].as_boolean()->get();
    } catch (toml::ex::parse_error& e) {
        printf("error reading keybinds.toml\n%s\n", e.what());
        return false;
    } catch (...) {
        printf("unknown exception while reading keybinds\n");
        return false;
    }
    return true;
}

void init_input() {
    if (isMt4) {
        enableHook(initWindow, 0x89B8680);
    } else {
        enableHook(initWindow, 0xa824460);
    }

    SDL_SetMainReady();

    // hacky fix for ctrl+c
    struct sigaction action;
    sigaction(SIGINT, NULL, &action);

    if (SDL_Init( SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS | SDL_INIT_VIDEO) != 0) {
        sigaction(SIGINT, &action, NULL);

        printf("cant initialize sdl %s\n", SDL_GetError());
        return;
    }

    sigaction(SIGINT, &action, NULL);

    if (SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt") == -1)
        printf("[warn] cant load gamecontrollerdb.txt, get it at https://github.com/mdqinc/SDL_GameControllerDB/blob/master/gamecontrollerdb.txt\n");

    SDL_GameControllerEventState(SDL_ENABLE);

    loadKeybinds();
}

float maxAxis = SDL_JOYSTICK_AXIS_MAX -1 ;

void update_input() {
    bool focused = false;

    if (display != NULL) {
        Window retWindow;
        int rtt;
        XGetInputFocus(display, &retWindow, &rtt);

        focused = retWindow == window;

        XEvent e;
        while (XPending(display))
        {
            XNextEvent(display, &e);

            //TODO: add keyboard events
            // https://tronche.com/gui/x/xlib/events/types.html
            switch (e.type)
            {
                case ButtonPress:
                case ButtonRelease: {
                    bool down = e.type == ButtonPress;
                    update_touch(down, e.xbutton.x, e.xbutton.y);
                    break;
                }
                case KeyPress:
                case KeyRelease: {
                    if (!useKeyboard)
                        break;
                    if (!focused)
                        break;

                    bool down = e.type == KeyPress;
                    KeySym code = XKeycodeToKeysym(display, e.xkey.keycode, 0);

                    if (code == kbKeybindTestSwitch && !down) {
                        testState = !testState;
                        jvs_test(testState);
                    }
                    if (code == kbKeybindServiceSwitch)
                        jvs_service(down);
                    if (code == kbKeybindTestUp)
                        jvs_test_up(down);
                    if (code == kbKeybindTestDown)
                        jvs_test_down(down);
                    if (code == kbKeybindTestEnter)
                        jvs_test_enter(down);
                    if (code == kbKeybindShiftUp && down)
                        jvs_shift_up();
                    if (code == kbKeybindShiftDown && down)
                        jvs_shift_down();
                    if (code == kbKeybindPerspective)
                        jvs_perspective(down);
                    if (code == kbKeybindInterrupt)
                        jvs_interrupt(down);
                    if (code == kbKeybindGas)
                        jvs_gas(down ? (useLegacyPedalsAxis ? 128 : 80) : 0);
                    if (code == kbKeybindBrakes)
                        jvs_brakes(down ? (useLegacyPedalsAxis ? 128 : 80) : 0);
                    if (code == kbKeybindLeft || code == kbKeybindRight) {
                        if (code == kbKeybindLeft)
                            kbSteerLeft = down;
                        else
                            kbSteerRight = down;

                        uint8_t val = 127;
                        if (kbSteerRight)
                            val += 127;
                        if (kbSteerLeft)
                            val -= 127;
                        jvs_wheel(val);
                    }
                    if (code == kbKeybindCard)
                        bana_enter_card(down);
                }
            }
        }
    }

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {

        switch (event.type) {
            case SDL_CONTROLLERDEVICEADDED: {
                SDL_GameControllerOpen(event.cdevice.which);
                break;
            }
            case SDL_CONTROLLERDEVICEREMOVED: {
                auto controller = SDL_GameControllerFromInstanceID(event.cdevice.which);
                if (!controller)
                    break;
                SDL_GameControllerClose(controller);
                break;
            }

            case SDL_CONTROLLERAXISMOTION: {
                if (!focused)
                    break;

                if (event.caxis.axis == controllerAxisSteer) {
                    int axisFull = int(event.caxis.value) + maxAxis;
                    float axisRange = float(axisFull) / maxAxis;

                    if (axisRange > 2.f)
                        axisRange = 2.f;
                    if (axisRange < 0.f)
                        axisRange = 0.f;

                    if (axisRange <= deadzone + 1.f && axisRange >= 1.f - deadzone)
                        axisRange = 1.f;

                    jvs_wheel(axisRange * 127.f);
                }

                if (event.caxis.axis == controllerAxisGas) {
                    float axisRange = float(event.caxis.value) / maxAxis;

                    if (gasFull) {
                        axisRange += 1.f; // 0 - 2
                        axisRange /= 2.f; // 0 - 1
                        if (gasReversed)
                            axisRange = 1.f - axisRange;
                    } else {
                        if (gasReversed)
                            axisRange *= -1.f;
                    }

                    if (axisRange > 1.f)
                        axisRange = 1.f;
                    if (axisRange < 0.f)
                        axisRange = 0.f;

                    if (axisRange <= deadzone && axisRange >= -deadzone)
                        axisRange = 0.f;

                    if (!useLegacyPedalsAxis)
                        axisRange *= 0.625f;

                    jvs_gas(axisRange * 128.f);
                }

                if (event.caxis.axis == controllerAxisBrake) {
                    float axisRange = float(event.caxis.value) / maxAxis;

                    if (brakesFull) {
                        axisRange += 1.f; // 0 - 2
                        axisRange /= 2.f; // 0 - 1
                        if (brakesReversed)
                            axisRange = 1.f - axisRange;
                    } else {
                        if (brakesReversed)
                            axisRange *= -1.f;
                    }

                    if (axisRange > 1.f)
                        axisRange = 1.f;
                    if (axisRange < 0.f)
                        axisRange = 0.f;

                    if (axisRange <= deadzone && axisRange >= -deadzone)
                        axisRange = 0.f;

                    if (!useLegacyPedalsAxis)
                        axisRange *= 0.625f;

                    jvs_brakes(axisRange * 128.f);
                }
                break;
            }

            case SDL_CONTROLLERBUTTONUP:
            case SDL_CONTROLLERBUTTONDOWN: {
                if (!focused)
                    break;

                auto button = (SDL_GameControllerButton)event.cbutton.button;
                bool down = event.type == SDL_CONTROLLERBUTTONDOWN;

                if (button == ctrlKeybindTestSwitch && down)  {
                    testState = !testState;
                    jvs_test(testState);
                }

                if (button == ctrlKeybindService)
                    jvs_service(down);

                if (button == ctrlKeybindTestUp)
                    jvs_test_up(down);
                if (button == ctrlKeybindTestDown)
                    jvs_test_down(down);
                if (button == ctrlKeybindTestEnter)
                    jvs_test_enter(down);
                if (button == ctrlKeybindShiftUp && down)
                    jvs_shift_up();
                if (button == ctrlKeybindShiftDown && down)
                    jvs_shift_down();
                if (button == ctrlKeybindPerspective)
                    jvs_perspective(down);
                if (button == ctrlKeybindInterrupt)
                    jvs_interrupt(down);
                if (button == ctrlKeybindCard)
                    bana_enter_card(down);

                break;
            }
            case SDL_QUIT: {
                printf("sdl quit\n");
                inputStopped = true;
                break;
            }
        }
    }
}