#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "touch.h"

bool sendTouch = false;
bool touchReady = false;

void initTouch() {
    touchReady = false;
}

int8_t touchBuffer[5];

uint8_t touchOk[3] = { 0x01, 0x30, 0x0d };

int touchOkResponse = 0;
void writeTouch(void* packet, int size) {
    uint8_t* data = (uint8_t*)packet;
    if (data[0] == 1) {
        if (data[1] == 68) {
            printf("touch should be ready\n");
            touchReady = true;
        }
        if (data[1] == 82) {
            printf("reset touch\n");
            touchReady = false;
        }
    }
    touchOkResponse++;
}

bool readTouch(void* packet, int* size) {
    if (touchOkResponse < 1) {
        if (!touchReady)
            return false;
        if (sendTouch) {
            int touchX = rand() % 1000;
            int touchY = rand() % 720;

            touchBuffer[0] = -128;
            touchBuffer[1] = touchX & 0x7f;
            touchBuffer[2] = (touchX / 0x80) & 0x7f;
            touchBuffer[3] = touchY & 0x7f;
            touchBuffer[4] = (touchY / 0x80) & 0x7f;

            memcpy(packet, touchBuffer, 5);
            *size = 5;
            return true;
        }

        return false;
    }

    touchOkResponse--;
    memcpy(packet, touchOk, 3);
    *size = 3;
    return true;
}