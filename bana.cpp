#include <cstring>
#include "bana.h"
#include "hook.h"
#include "jvs.h"
#include "config.h"

bool cardEntered = false;

defineHook(void, BngRwInit) {
    printf("BngRwInit\n");
}

defineHook(int, BngRwAttach, int a1, int a2, int a3, int a4,int cb, uint8_t* bn) {
    printf("BngRwAttach\n");

    *(int*)(bn + 8) = 0;
    return 1;
}

defineHook(int, BngRwIsCmdExec) {
    printf("BngRwIsCmdExec\n");
    return 0;
}

defineHook(int, BngRwReqLed, int a1, int a2, int cb, uint8_t* bn) {
    printf("BngRwReqLed\n");

    *(int*)(bn + 8) = 0;
    return 1;
}

defineHook(int, BngRwReqAction, int a1, int a2, int cb, uint8_t* bn) {
    printf("BngRwReqAction\n");

    *(int*)(bn + 8) = 0;
    return 1;
}

defineHook(int, BngRwReqBeep, int a1, int a2, int cb, uint8_t* bn) {
    printf("BngRwReqBeep\n");

    *(int*)(bn + 8) = 0;
    return 1;
}

defineHook(int, BngRwReqCancel) {
    printf("BngRwReqCancel\n");
    return 1;
}

defineHook(int, BngRwReqSendUrlTo, int a1, int a2, int a3, int a4, int a5, int a6, int cb, uint8_t* bn) {
    printf("BngRwReqSendUrlTo\n");

    *(int*)(bn + 8) = 0;
    return 1;
}

struct CardData {
    char pad1[44]; // 2
    char chipID[36]; // 44
    char accessCode[40]; // 80
    char cardType[6]; // 120
    char pad4[42];

    CardData() {
        memset(this, 0, sizeof(*this));
    }
};

CardData* g_cardData = NULL;

defineHook(int, BngRwReqWaitTouch, int c, int a2, uint32_t flags, void* callback, uint8_t* bn)
{
    if (c == 0) {
        if (!cardEntered)
            return -1;

        *(int*)(bn + 8) = 0;
        memcpy(bn + 16, g_cardData, sizeof(*g_cardData));
        return 1;
    }
    return -1;
}

defineHook(int, BngRwReset, int a1, int cb, uint8_t* bn) {
    printf("BngRwReset\n");

    *(int*)(bn + 8) = 0;
    return 1;
}

defineHook(int, bngRwAttachMt4, int a1, int a2, int a3, int a4, int cb, int d)
{
    printf("bngRwAttach\n");

    *(int *)(d + 0x24) = 1;
    *(int *)(d + 0x28) = 0;
    return 1;
}

defineHook(int, bngRwReqLedMt4, int a1, int a2, int cb, int d)
{
    printf("bngRwReqLed\n");

    *(int *)(d + 0x24) = 1;
    *(int *)(d + 0x28) = 0;
    return 1;
}

defineHook(int, bngRwResetMt4, int a1, int cb, int d)
{
    printf("bngRwReqReset\n");

    *(int *)(d + 0x24) = 1;
    *(int *)(d + 0x28) = 0;
    return 1;
}

defineHook(int, bngRwWaitTouchMt4, int a1, int a2, int a3, int cb, uint8_t* bn)
{
    if (!cardEntered) { // super hacky solution plz fix me
        *(int*)(bn + 12) = 0x836EDE0;
        *(int*)(bn + 220) = *(int*)0x9213994; // dont timeout
        return 0;
    }
    printf("bngRwWaitTouch %p\n", (void *)bn);

    *(int *)(bn + 36) = 1;
    *(int *)(bn + 40) = 0;
    memcpy(bn + 44, g_cardData, sizeof(*g_cardData));
    return 1;
}

void initBana() {
    if (isMt4) {
        enableHook(bngRwAttachMt4, 0x8AC3ECC);
        enableHook(bngRwResetMt4, 0x8AC39B0);
        enableHook(bngRwReqLedMt4, 0x8AC3774);
        enableHook(bngRwWaitTouchMt4, 0x8AC387E);
    } else {
        enableHook(BngRwInit, 0xAA62C34);
        enableHook(BngRwAttach, 0xAA62764);
        enableHook(BngRwIsCmdExec, 0x80EAD50);
        enableHook(BngRwReqLed, 0xAA6200C);
        enableHook(BngRwReqAction, 0xAA61DE6);
        enableHook(BngRwReqBeep, 0xAA61EEC);
        enableHook(BngRwReqCancel, 0xAA61A1A);
        enableHook(BngRwReqSendUrlTo, 0xAA6236E);
        enableHook(BngRwReqWaitTouch, 0xAA62116);
        enableHook(BngRwReset, 0x0aa62248);
    }

    g_cardData = new CardData();
    if (accessCode)
        memcpy(g_cardData->accessCode, accessCode, strlen(accessCode) + 1);
    if (chipID)
        memcpy(g_cardData->chipID, chipID, strlen(chipID) + 1);
    memcpy(g_cardData->cardType, "NBGIC6", 6);
}

void bana_enter_card(bool state) {
    cardEntered = state;
}