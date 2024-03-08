#include <stdio.h>
#include "jvs.h"
#include "touch.h"
#include "input.h"

#include <string.h>

#include <vector>
#include <mutex>
#include <string>

//#define JVS_DEBUG

// TODO: rewrite this mess
// based on https://github.com/OpenJVS/OpenJVS

std::mutex jvsMutex;

uint8_t analogBytes[32];
uint8_t inputBuffer[JVS_MAX_PACKET_SIZE];
uint8_t outputBuffer[JVS_MAX_PACKET_SIZE];

bool testSwitched = false;
bool startValue = false;
bool serviceValue = false;
bool upPressed = false;
bool downPressed = false;
bool leftPressed = false;
bool rightPressed = false;
bool interuptionSwitch = false;
bool perspectiveSwitch = false;
bool button1Pressed = false;
bool button2Pressed = false;
bool button3Pressed = false;
bool button4Pressed = false;
bool button5Pressed = false;
bool button6Pressed = false;

bool cardPressed = true;

int wmmtGear = 0;

void updateGear()
{
    if (wmmtGear == 0)
    {
        button1Pressed = false;
        button2Pressed = false;
        button3Pressed = false;
        button4Pressed = false;
        button5Pressed = false;
        button6Pressed = false;
    }
    else if (wmmtGear == 1)
    {
        button1Pressed = false;
        button2Pressed = false;
        button3Pressed = true;
        button4Pressed = false;
        button5Pressed = true;
        button6Pressed = false;
    }
    else if (wmmtGear == 2)
    {
        button1Pressed = false;
        button2Pressed = false;
        button3Pressed = false;
        button4Pressed = true;
        button5Pressed = true;
        button6Pressed = false;
    }
    else if (wmmtGear == 3)
    {
        button1Pressed = false;
        button2Pressed = false;
        button3Pressed = true;
        button4Pressed = false;
        button5Pressed = false;
        button6Pressed = false;
    }
    else if (wmmtGear == 4)
    {
        button1Pressed = false;
        button2Pressed = false;
        button3Pressed = false;
        button4Pressed = true;
        button5Pressed = false;
        button6Pressed = false;
    }
    else if (wmmtGear == 5)
    {
        button1Pressed = false;
        button2Pressed = false;
        button3Pressed = true;
        button4Pressed = false;
        button5Pressed = false;
        button6Pressed = true;
    }
    else if (wmmtGear == 6)
    {
        button1Pressed = false;
        button2Pressed = false;
        button3Pressed = false;
        button4Pressed = true;
        button5Pressed = false;
        button6Pressed = true;
    }
}

JVSPacket inputPacket, outputPacket;
int deviceID;
std::string jvsName = "namco ltd.;NA-JV;Ver4.00;JPN,Multipurpose"; //"namco ltd.;I/O CYBER LEAD;Ver2.02;JPNLED-0100";

std::vector<std::vector<uint8_t>> readQueue = {};
std::vector<std::vector<uint8_t>> writeQueue = {};

int readBytes(unsigned char *buffer, int amount)
{
    if (readQueue.empty())
        return -1;
    std::vector<uint8_t> data = readQueue[0];
    readQueue.erase(readQueue.begin());

    if (data.size() > amount) {
        printf("readBytes overflow %d %d\n", data.size(), amount);
    }

    memcpy(buffer, data.data(), data.size());
    return data.size();
}

int writeBytes(unsigned char *buffer, int amount)
{
    std::vector<uint8_t> data = {};
    data.resize(amount);
    memcpy(data.data(), buffer, amount);
    writeQueue.push_back(data);
    return amount;
}

JVSStatus readPacket(int pipe, JVSPacket *packet)
{
    int bytesAvailable = 0;
    int escape = 0;
    int phase = 0;
    int index = 0;
    int dataIndex = 0;
    int finished = 0;
    uint8_t checksum = 0;

    while (!finished)
    {
        int bytesRead = readBytes(inputBuffer + bytesAvailable, JVS_MAX_PACKET_SIZE - bytesAvailable);
        if (bytesRead < 0)
        {
            return JVS_STATUS_ERROR_TIMEOUT;
        }

        bytesAvailable += bytesRead;

        while ((index < bytesAvailable) && !finished)
        {
            if (!escape && inputBuffer[index] == SYNC)
            {
                phase = 0;
                dataIndex = 0;
                index++;
                continue;
            }

            if (!escape && inputBuffer[index] == ESCAPE)
            {
                escape = 1;
                index++;
                continue;
            }

            if (escape)
            {
                inputBuffer[index]++;
                escape = 0;
            }

            switch (phase)
            {
                case 0:
                {
                    packet->destination = inputBuffer[index];
                    checksum = packet->destination & 0xFF;
                    phase++;
                    break;
                }
                case 1:
                {
                    packet->length = inputBuffer[index];
                    checksum = (checksum + packet->length) & 0xFF;
                    phase++;
                    break;
                }
                case 2:
                {
                    if (dataIndex == (packet->length - 1))
                    {
                        if (checksum != inputBuffer[index])
                        {
                            printf("invalid checksum\n");
                            return JVS_STATUS_ERROR_CHECKSUM;
                        }
                        finished = 1;
                        break;
                    }
                    packet->data[dataIndex++] = inputBuffer[index];
                    checksum = (checksum + inputBuffer[index]) & 0xFF;
                    break;
                }
                default:
                {
                    printf("got weird phase\n");
                    return JVS_STATUS_ERROR;
                }
            }
            index++;
        }
    }
    return JVS_STATUS_SUCCESS;
}

JVSStatus writePacket(int pipe, JVSPacket *packet)
{
    /* Don't return anything if there isn't anything to write! */
    if (packet->length < 2)
        return JVS_STATUS_SUCCESS;

    /* Get pointer to raw data in packet */
    uint8_t *packetPointer = (uint8_t *)packet;

    /* Add SYNC and reset buffer */
    int checksum = 0;
    int outputIndex = 1;
    outputBuffer[0] = SYNC;

    packet->length++;

    /* Write out entire packet */
    for (int i = 0; i < packet->length + 1; i++)
    {
        if (packetPointer[i] == SYNC || packetPointer[i] == ESCAPE)
        {
            outputBuffer[outputIndex++] = ESCAPE;
            outputBuffer[outputIndex++] = (packetPointer[i] - 1);
        }
        else
        {
            outputBuffer[outputIndex++] = (packetPointer[i]);
        }
        checksum = (checksum + packetPointer[i]) & 0xFF;
    }

    /* Write out escaped checksum */
    if (checksum == SYNC || checksum == ESCAPE)
    {
        outputBuffer[outputIndex++] = ESCAPE;
        outputBuffer[outputIndex++] = (checksum - 1);
    }
    else
    {
        outputBuffer[outputIndex++] = checksum;
    }

    int written = 0, timeout = 0;
    while (written < outputIndex)
    {
        if (written != 0)
        {
            timeout = 0;
        }

        if (timeout > JVS_RETRY_COUNT)
        {
            return JVS_STATUS_ERROR_WRITE_FAIL;
        }

        written += writeBytes(outputBuffer + written, outputIndex - written);
        timeout++;
    }

    return JVS_STATUS_SUCCESS;
}

void update_jvs()
{

    JVSStatus readPacketStatus = readPacket(0, &inputPacket);
    if (readPacketStatus != JVS_STATUS_SUCCESS)
    {
        return;
    }

    if (inputPacket.destination != BROADCAST)
    {
        if (inputPacket.destination != deviceID)
        {
            printf("sending jvs to not us?\n");
            return;
        }
    }

    if (inputPacket.data[0] == CMD_RETRANSMIT)
    {
        printf("we need to retransit\n");
        writePacket(0, &outputPacket);
        return;
    }

    outputPacket.length = 0;
    outputPacket.destination = BUS_MASTER;
    memset(outputPacket.data, 0, sizeof(outputPacket.data));
    int index = 0;

    /* Set the entire packet success line */
    outputPacket.data[outputPacket.length++] = STATUS_SUCCESS;

    while (index < inputPacket.length)
    {
        int size = 1;
        switch (inputPacket.data[index])
        {
            case 0:
            {
                // SKIP
                break;
            }
            case CMD_RESET:
            {
#ifdef JVS_DEBUG
                printf("CMD_RESET\n");
#endif

                size = 2;
                deviceID = -1;
                break;
            }
            case CMD_ASSIGN_ADDR:
            {
#ifdef JVS_DEBUG
                printf("CMD_ASSIGN_ADDR\n");
#endif
                size = 2;
                deviceID = inputPacket.data[index + 1];
#ifdef JVS_DEBUG
                printf("device id %d\n", deviceID);
#endif
                outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
                break;
            }
            case CMD_REQUEST_ID:
            {
#ifdef JVS_DEBUG
                printf("CMD_REQUEST_ID\n");
#endif
                outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
                memcpy(&outputPacket.data[outputPacket.length + 1], jvsName.c_str(), jvsName.size() + 1);
                outputPacket.length += 1 + jvsName.size() + 1;
                break;
            }

                /*    break;
                          case 0x11:
                          case 0x12:
                          case 0x13:
                            pcVar13 = "Command Revision";
                            if ((bVar10 != 0x11) && (pcVar13 = "Jv Revision", bVar10 != 0x12)) {
                              pcVar13 = "Communication Revision";
                            }*/

                /*iVar2 = snprintf(param_1 + uVar6,param_2 - uVar6," revision: command=%x, jv=%x, comm=%x\n",
                         (uint)(byte)(&DAT_0c00b50c)[iVar1],(uint)(byte)(&DAT_0c00b50d)[iVar1],
                         (uint)(byte)(&DAT_0c00b50e)[iVar1]);
        uVar6 = uVar6 + iVar2;
        if (param_2 <= uVar6) {
          return uVar6;
        }*/

            case CMD_COMMAND_VERSION:
            {
#ifdef JVS_DEBUG
                printf("CMD_COMMAND_VERSION\n");
#endif
                outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
                outputPacket.data[outputPacket.length + 1] = 0x13;
                outputPacket.length += 2;
                break;
            }
            case CMD_JVS_VERSION:
            {
#ifdef JVS_DEBUG
                printf("CMD_JVS_VERSION\n");
#endif
                outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
                outputPacket.data[outputPacket.length + 1] = 0x30;
                outputPacket.length += 2;
                break;
            }
            case CMD_COMMS_VERSION:
            {
#ifdef JVS_DEBUG
                printf("CMD_COMMS_VERSION\n");
#endif
                outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
                outputPacket.data[outputPacket.length + 1] = 100;
                outputPacket.length += 2;
                break;
            }
            case CMD_CAPABILITIES:
            {
#ifdef JVS_DEBUG
                printf("CMD_CAPABILITIES\n");
#endif

                outputPacket.data[outputPacket.length] = REPORT_SUCCESS;
                outputPacket.length += 1;

                // CAP_PLAYERS
                outputPacket.data[outputPacket.length] = 0x01;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x01;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x18;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x00;
                outputPacket.length++;

                outputPacket.data[outputPacket.length] = 0x02;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x02;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x00;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x00;
                outputPacket.length++;

                outputPacket.data[outputPacket.length] = 0x03;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x08;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x10;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x00;
                outputPacket.length++;

                outputPacket.data[outputPacket.length] = 0x04;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x04;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x00;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x00;
                outputPacket.length++;

                outputPacket.data[outputPacket.length] = 0x12;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x12;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x00;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x00;
                outputPacket.length++;

                outputPacket.data[outputPacket.length] = 0x13;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x02;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x00;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x00;
                outputPacket.length++;

                outputPacket.data[outputPacket.length] = 0x14;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x50;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x19;
                outputPacket.length++;
                outputPacket.data[outputPacket.length] = 0x00;
                outputPacket.length++;

                outputPacket.data[outputPacket.length] = CAP_END;
                outputPacket.length += 1;
                break;
            }
            case CMD_CONVEY_ID:
            {
#ifdef JVS_DEBUG
                printf("CMD_CONVEY_ID\n");
#endif
                outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
                outputPacket.data[outputPacket.length++] = 0x01;
                outputPacket.data[outputPacket.length++] = 0x05;

                size = 1;
                for (int i = 1; i < 100; i++)
                {
                    size++;
                    if (inputPacket.data[index + i] == 0)
                        break;
                }
                break;
            }
            case CMD_READ_ANALOGS:
            {
#ifdef JVS_DEBUG
                printf("CMD_READ_ANALOGS\n");
#endif
                uint8_t channelCount = inputPacket.data[index + 1];
#ifdef JVS_DEBUG
                printf("%d\n", channelCount);
#endif
                size = 2;

                outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
                for (int i = 0; i < channelCount; i++)
                {
                    outputPacket.data[outputPacket.length++] = analogBytes[i * 2];
                    outputPacket.data[outputPacket.length++] = analogBytes[i * 2 + 1];
                }
                break;
            }
            case CMD_WRITE_GPO:
            {
                size = 2 + inputPacket.data[index + 1];
#ifdef JVS_DEBUG
                printf("CMD_WRITE_GPO %d\n", size);

                if (inputPacket.data[index + 1] == 3)
                {
                    uint8_t flags1 = inputPacket.data[index + 2];

                    if (flags1 == 0x00)
                    {
                        // idk what 0
                    }
                    else if (flags1 == 0x01)
                    {
                    }
                    else if (flags1 == 0x09)
                    {
                        printf("Flash the green button\n");
                    }
                    else if (flags1 == 0x0D)
                    {
                        printf("Flash both the green and red button\n");
                    }
                    else
                    {
                        printf("GPO Got flag %d\n", flags1);
                    }

                    uint8_t flags2 = inputPacket.data[index + 3];
                    if (flags2 == 0xF0)
                    {
                        // We can go to the test menu?
                    }
                    else if (flags2 == 0)
                    {
                        // We are on test menu or cant go to test menu
                    }
                    else
                    {
                        printf("GPO Got flag2 %d\n", flags2);
                    }

                    uint8_t flags3 = inputPacket.data[index + 4];
                    if (flags3 != 0)
                    {
                        printf("GPO Got flag3 %d\n", flags3);
                    }
                }
#endif

                outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
                break;
            }
            case CMD_WRITE_ANALOG:
            {
#ifdef JVS_DEBUG
                printf("CMD_WRITE_ANALOG\n");
#endif
                uint8_t byteCount = inputPacket.data[index + 1];
#ifdef JVS_DEBUG
                printf("CMD_WRITE_ANALOG %d\n", byteCount);
#endif
                size = 2;
                size += byteCount * 2;
                outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
                break;
            }
            case CMD_READ_COINS:
            {
#ifdef JVS_DEBUG
                printf("CMD_READ_COINS\n");
#endif
                uint8_t slotCount = inputPacket.data[index + 1];
#ifdef JVS_DEBUG
                printf("%d\n", slotCount);
#endif
                size = 2;

                outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;

                for (int i = 0; i < slotCount; i++)
                {
                    outputPacket.data[outputPacket.length++] = 0;
                    outputPacket.data[outputPacket.length++] = 0;
                }
                break;
            }
            case CMD_READ_SWITCHES:
            {
#ifdef JVS_DEBUG
                printf("CMD_READ_SWITCHES\n");
#endif
                uint8_t players = inputPacket.data[index + 1];
#ifdef JVS_DEBUG
                printf("Players %d\n", players);
#endif
                size = 3;

                uint8_t bytesToRead = inputPacket.data[index + 2];
                outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
                outputPacket.data[outputPacket.length++] = testSwitched ? 0x80 : 0;

                if (players > 0)
                {

                    uint8_t playerControls = 0;
                    if (startValue)
                        playerControls |= 0x80;
                    if (serviceValue)
                        playerControls |= 0x40;
                    if (upPressed)
                        playerControls |= 0x20;
                    if (downPressed)
                        playerControls |= 0x10;
                    if (leftPressed)
                        playerControls |= 0x08;
                    if (rightPressed)
                        playerControls |= 0x04;
                    if (button1Pressed)
                        playerControls |= 0x02;
                    if (button2Pressed)
                        playerControls |= 0x01;
                    outputPacket.data[outputPacket.length++] = playerControls;
                    bytesToRead--;

                    if (bytesToRead != 0)
                    {
                        uint8_t ext = 0;
                        if (button3Pressed)
                            ext |= 0x80;
                        if (button4Pressed)
                            ext |= 0x40;
                        if (button5Pressed)
                            ext |= 0x20;
                        if (button6Pressed)
                            ext |= 0x10;
                        if (interuptionSwitch)
                            ext |= 0x01;
                        if (perspectiveSwitch)
                            ext |= 0x02;
                        outputPacket.data[outputPacket.length++] = ext;
                        bytesToRead--;
                    }

                    while (bytesToRead > 0)
                    {
                        outputPacket.data[outputPacket.length++] = 0;
                        bytesToRead--;
                    }

                    if (players > 1)
                    {
                        bytesToRead = inputPacket.data[index + 2];

                        outputPacket.data[outputPacket.length++] = 0;
                        bytesToRead--;

                        if (bytesToRead != 0)
                        {
                            outputPacket.data[outputPacket.length++] = 0;
                            bytesToRead--;
                        }

                        while (bytesToRead > 0)
                        {
                            outputPacket.data[outputPacket.length++] = 0;
                            bytesToRead--;
                        }
                    }
                }

                break;
            }
            case CMD_NAMCO_SPECIFIC:
            {
#ifdef JVS_DEBUG
                printf("CMD_NAMCO_SPECIFIC\n");
#endif
                size = 2;
                outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;

                switch (inputPacket.data[index + 1])
                {
                    case 3:
                    {
                        // dip switch?
#ifdef JVS_DEBUG
                        printf("namco 0x03\n");
#endif
                        size = 2;
                        outputPacket.data[outputPacket.length++] = 0x0; // dip switch
                        break;
                    }
                    case 5: { // card vender?
                        printf("namco cmd 5%d\n", inputPacket.data[index + 2]);
                        size = 3 + inputPacket.data[index + 2];

                        outputPacket.data[outputPacket.length++] = 0x01;
                        break;
                    }
                    case 0x15: {
#ifdef JVS_DEBUG
                        printf("namco 0x15\n");
#endif
                        size  = 4;
                        outputPacket.data[outputPacket.length++] = 0x00;
                        break;
                    }
                    case 0x16:
                    {
#ifdef JVS_DEBUG
                        printf("namco 0x16\n");
#endif
                        size = 4;
                        outputPacket.data[outputPacket.length++] = 0x00;
                        break;
                    }
                    case 0x18:
                    {
#ifdef JVS_DEBUG
                        printf("namco 0x18\n");
#endif
                        size = 6;
                        outputPacket.data[outputPacket.length++] = 0x01; // start?
                        break;
                    }
                    default:
                    {
                        printf("unhandled namco command %d %p\n", inputPacket.data[index + 1], inputPacket.data[index + 1]);
                        system("pause");
                        break;
                    }
                }
                break;
            }

            case CMD_SET_PAYOUT: {
                printf("CMD_SET_PAYOUT\n");
                size = 4;

                outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
                break;
            }
            case CMD_DECREASE_COINS:
            {
                printf("CMD_DECREASE_COINS\n");
                size = 4;
                outputPacket.data[outputPacket.length++] = REPORT_SUCCESS;
                break;
            }
            case 0x50: {

                break;
            }
            default:
            {
                printf("unhandled command %d %p\n", inputPacket.data[index], inputPacket.data[index]);
                // exit(1);
                break;
            }
        }
        index += size;
    }

#ifdef JVS_DEBUG
    printf("flush\n");
#endif

    writePacket(0, &outputPacket);
}

void initJvs() {
    memset(analogBytes, 0, 32);
}

void writeJvs(void* packet, int size) {
    jvsMutex.lock();
    std::vector<uint8_t> data = {};
    data.resize(size);
    memcpy(data.data(), packet, size);
    readQueue.push_back(data);
    jvsMutex.unlock();
}

bool readJvs(void* packet, int* size) {
    jvsMutex.lock();
    if (writeQueue.empty()) {
        jvsMutex.unlock();
        return false;
    }

    std::vector<uint8_t> data = writeQueue[0];
    writeQueue.erase(writeQueue.begin());

    memcpy(packet, data.data(), data.size());
    *size = data.size();

    jvsMutex.unlock();
    return true;
}

void updateJvs()
{
    jvsMutex.lock();
    update_input();
    update_jvs();
    jvsMutex.unlock();
}

void jvs_test(bool state) {
    testSwitched = state;
}

void jvs_service(bool down) {
    serviceValue = down;
}

void jvs_perspective(bool down) {
    perspectiveSwitch = down;
}

void jvs_interrupt(bool down) {
    interuptionSwitch = down;
}

void jvs_test_up(bool down) {
    upPressed = down;
}

void jvs_test_down(bool down) {
    downPressed = down;
}

void jvs_test_enter(bool down) {
    if (down) {
        wmmtGear = 0;
        updateGear();
        button1Pressed = true;
    } else {
        wmmtGear = 0;
        updateGear();
        button1Pressed = false;
    }
}

void jvs_wheel(uint8_t value) {
    analogBytes[0] = value;
}

void jvs_gas(uint8_t value) {
    analogBytes[2] = value;
}

void jvs_brakes(uint8_t value) {
    analogBytes[4] = value;
}

void jvs_gear(uint8_t gear) {
    if (gear > 6)
        gear = 6;
    if (gear < 0)
        gear = 0;
    wmmtGear = gear;
    updateGear();
}

void jvs_shift_up() {
    if (wmmtGear > 6)
        return;
    wmmtGear++;
    updateGear();
}
void jvs_shift_down() {
    if (wmmtGear < 0)
        return;
    wmmtGear--;
    updateGear();
}