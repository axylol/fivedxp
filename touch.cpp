#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "touch.h"
#include "config.h"

bool touchReady = false;

void initTouch() {
    touchReady = false;
}

int8_t touchBuffer[5];

uint8_t touchOk[3] = { 0x01, 0x30, 0x0d };


bool touchPressed = false;
bool lastTouchPressed = false;
int touchX, touchY;

int touchOkResponse = 0;
void writeTouch(void* packet, int size) {
    uint8_t* data = (uint8_t*)packet;
    if (data[0] == 1) {
        if (data[1] == 68) {
            printf("touch should be ready\n");
            touchReady = true;
            touchPressed = false;
            lastTouchPressed = false;
        }
        if (data[1] == 82) {
            printf("reset touch\n");
            touchReady = false;
        }
    }
    touchOkResponse++;
}

void update_touch(bool state, int x, int y) {
    touchPressed = state;
    touchBuffer[1] = x & 0x7f;
    touchBuffer[2] = (x / 0x80) & 0x7f;
    touchBuffer[3] = y & 0x7f;
    touchBuffer[4] = ( y / 0x80) & 0x7f;
}

bool readTouch(void* packet, int* size) {
    if (isMt4) {
        if (touchReady) {
            bool sendTouch = false;
            if (!touchPressed) {
                if (lastTouchPressed) {
                    touchBuffer[0] = -128;
                    sendTouch = true;
                    lastTouchPressed = false;
                }
            } else {
                touchBuffer[0] = -64;
                lastTouchPressed = true;
                sendTouch = true;
            }

            if (sendTouch) {
                memcpy(packet, touchBuffer, 5);
                *size = 5;
                return true;
            }
        }

        if (isMt4) {
            // not sure how to deal with this yet
            memcpy(packet, touchOk, 3);
            *size = 3;
            return true;
        }

        return false;
    }

    touchOkResponse--;
    memcpy(packet, touchOk, 3);
    *size = 3;
    return true;
}