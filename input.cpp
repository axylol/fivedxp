#include "input.h"
#include "jvs.h"
#include "bana.h"
#include <SDL2/SDL.h>
#include <csignal>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <dlfcn.h>
#include "hook.h"
#include "config.h"
#include "touch.h"

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

SDL_Keycode kbKeybindTestSwitch = SDLK_LEFT;
SDL_Keycode kbKeybindTestUp = SDLK_UP;
SDL_Keycode kbKeybindTestDown = SDLK_DOWN;
SDL_Keycode kbKeybindTestEnter = SDLK_RIGHT;
SDL_Keycode kbKeybindShiftUp = SDLK_EQUALS;
SDL_Keycode kbKeybindShiftDown = SDLK_MINUS;
SDL_Keycode kbKeybindPerspective = SDLK_e;
SDL_Keycode kbKeybindInterrupt = SDLK_q;
SDL_Keycode kbKeybindGas = SDLK_w;
SDL_Keycode kbKeybindLeft = SDLK_a;
SDL_Keycode kbKeybindBrakes = SDLK_s;
SDL_Keycode kbKeybindRight = SDLK_d;
SDL_Keycode kbKeybindCard = SDLK_c;
SDL_Keycode kbKeybindServiceSwitch = SDLK_f;

bool kbSteerLeft = false;
bool kbSteerRight = false;

bool testState = false;

bool inputStopped = false;

Display* display = NULL;
Window window = 0;

defineHook(int, initWindow, int a1) {
    int ret = callOld(initWindow, a1);
    if (ret == 0) {
        display = *(Display**)(a1 + 4);
        window = *(Window*)(a1 + 12);

        XSelectInput(display, window, ButtonPressMask | ButtonReleaseMask);
    }
    return ret;
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

    if (SDL_Init( SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) {
        sigaction(SIGINT, &action, NULL);

        printf("cant initialize sdl %s\n", SDL_GetError());
        return;
    }

    sigaction(SIGINT, &action, NULL);

    if (SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt") == -1)
        printf("[warn] cant load gamecontrollerdb.txt, get it at https://github.com/mdqinc/SDL_GameControllerDB/blob/master/gamecontrollerdb.txt\n");

    SDL_GameControllerEventState(SDL_ENABLE);
}

float maxAxis = SDL_JOYSTICK_AXIS_MAX -1 ;

void update_input() {
    if (display != NULL) {
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
                if (event.caxis.axis == controllerAxisSteer) {
                    int axisFull = int(event.caxis.value) + maxAxis;
                    float axisRange = float(axisFull) / maxAxis;

                    if (axisRange > 2.f)
                        axisRange = 2.f;
                    if (axisRange < 0.f)
                        axisRange = 0.f;

                    jvs_wheel(axisRange * 128.f);
                }

                if (event.caxis.axis == controllerAxisGas) {
                    float axisRange = float(event.caxis.value) / maxAxis;

                    if (axisRange > 1.f)
                        axisRange = 1.f;
                    if (axisRange < 0.f)
                        axisRange = 0.f;

                    jvs_gas(axisRange * 128.f);
                }

                if (event.caxis.axis == controllerAxisBrake) {
                    float axisRange = float(event.caxis.value) / maxAxis;

                    if (axisRange > 1.f)
                        axisRange = 1.f;
                    if (axisRange < 0.f)
                        axisRange = 0.f;

                    jvs_brakes(axisRange * 128.f);
                }
                break;
            }

            case SDL_CONTROLLERBUTTONUP:
            case SDL_CONTROLLERBUTTONDOWN: {
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

            case SDL_MOUSEMOTION: {
                printf("mouse motion\n");
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                printf("mouse up\n");
                break;
            }

            // TODO: FIX KEYBOARD EVENTS
            case SDL_KEYUP:
            case SDL_KEYDOWN: {
                SDL_Keycode code = event.key.keysym.sym;
                bool down = event.key.state != SDL_RELEASED;

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
                    jvs_gas(down ? 128 : 0);
                if (code == kbKeybindBrakes)
                    jvs_brakes(down ? 128 : 0);
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