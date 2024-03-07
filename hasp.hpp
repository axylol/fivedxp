#include <string>
#include <cstring>
#include "hook.h"
#include "config.h"

uint32_t haspSize = 0xD40;
uint8_t haspBuffer[0xD40];

defineHook(int, hasp_login)
{
    return 0;
}

defineHook(int, hasp_logout)
{
    return 0;
}

defineHook(int, hasp_encrypt)
{
    return 0;
}

defineHook(int, hasp_decrypt)
{
    return 0;
}

defineHook(int, hasp_get_size, int a1, int a2, int *a3)
{
    *a3 = haspSize;
    return 0;
}

defineHook(int, hasp_read, uint32_t handle, uint32_t fileID, uint32_t offset, uint32_t length, uint8_t *buffer)
{
    memcpy(buffer, haspBuffer + offset, length);
    return 0;
}

defineHook(int, hasp_write, uint32_t handle, uint32_t fileID, uint32_t offset, uint32_t length, uint8_t *buffer)
{
    memcpy(haspBuffer + offset, buffer, length);
    return 0;
}

void generateHaspDongleData(const std::string& serial)
{
    memset(haspBuffer, 0, sizeof(haspBuffer));

    haspBuffer[0] = 0x01;
    haspBuffer[0x13] = 0x01;
    haspBuffer[0x17] = 0x0A;
    haspBuffer[0x1B] = 0x04;
    haspBuffer[0x1C] = 0x3B;
    haspBuffer[0x1D] = 0x6B;
    haspBuffer[0x1E] = 0x40;
    haspBuffer[0x1F] = 0x87;
    haspBuffer[0x23] = 0x01;
    haspBuffer[0x27] = 0x0A;
    haspBuffer[0x2B] = 0x04;
    haspBuffer[0x2C] = 0x3B;
    haspBuffer[0x2D] = 0x6B;
    haspBuffer[0x2E] = 0x40;
    haspBuffer[0x2F] = 0x87;
    memcpy(haspBuffer + 0xD00, serial.c_str(), 12);

    uint8_t crc = 0;
    for (int i = 0; i < 62; i++)
        crc += haspBuffer[0xD00 + i];

    haspBuffer[0xD3E] = crc & 0xFF;
    haspBuffer[0xD3F] = haspBuffer[0xD3E] ^ 0xFF;
}

void initHasp() {
    if (isTerminal) {
        generateHaspDongleData("267621542069"); // terminal
    } else {
        generateHaspDongleData("267620542069"); // drive
    }

    enableHook(hasp_login, 0xa982740);
    enableHook(hasp_login, 0xa982740);
    enableHook(hasp_logout, 0xa9827e0);
    enableHook(hasp_encrypt, 0xa9828cc);
    enableHook(hasp_decrypt, 0xa9829b8);
    enableHook(hasp_get_size, 0xa9836d0);
    enableHook(hasp_read, 0xa983538);
    enableHook(hasp_write, 0xa983604);
}