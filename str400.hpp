#pragma once
#include "hook.h"
#include "config.h"
#include <vector>
#include <cstring>
#include <iostream>

std::vector<std::vector<uint8_t>> str400queue = {};
void sendStr400(uint8_t command, std::vector<uint8_t> extraData) {

    std::vector<uint8_t> t = {};
    uint8_t hash = 0;
    uint8_t dataLength = 0;

    t.push_back(2); // Start Transaction

    hash += command;
    t.push_back(command);

    // Data Length
    t.push_back(0);

    hash += extraData.size();
    t.push_back(extraData.size()); // 3

    for (int i = 0; i < extraData.size(); i++) {
        hash += extraData[i];
        t.push_back(extraData[i]);
    }

    t.push_back(hash); // 5
    t.push_back(3); // 6 End Transaction

    str400queue.push_back(t);
}

defineHook(int, str400Receive, void* data, int length) {
    if (!str400queue.empty()) {
        auto q = str400queue[0];
        str400queue.erase(str400queue.begin());

        memcpy(data, q.data(), q.size());
        return q.size();
    }
    return 0;
}

bool vendReady = false;

defineHook(void, str400Send, uint8_t a1, uint8_t* data, int size) {
    if (data[0] != 0x02) // not stx
        return;
    if (data[size - 1] != 0x03) // not etx
        return;

    switch (data[1]) {
        case 0x30: { // Reset
            //023000003003
            printf("[VENDING] reset\n");
            vendReady = true; // really hacky shit

            sendStr400(0x30, { '1' }); // OK
            break;
        }
        case 0x31: { // carry
            printf("[VENDING] carry[0x31]\n");
            sendStr400(0x31, { '0' }); // OK
            break;
        }
        case 0x32: { // Status
            // printf("[VENDING] status\n");

            std::vector<uint8_t> e = { '1' };

            // 0x4 = Jammed
            // 0x8 = Eject
            // 0x20 UNK
            // 0x40 UNK
            uint8_t flags = 0;

            // not ready yet
            if (!vendReady) {
                flags |= 0x20;
            }
            else {
                flags |= 0x40; // idk

                if (isTerminal)
                    flags |= 0x8; // we have cards
            }

            e.push_back(flags);
            e.push_back('1');
            e.push_back(0);

            sendStr400(0x32, e);
            break;
        }
        case 0x34: { // Eject card
            printf("[VENDING] eject\n");

            // stx type zero length data hash etx
            // 02 34 00 01 30 65 03

            sendStr400(0x34, { '1' }); // OK
            break;
        }
        case 0x41: { // Carry (Drive cab)
            printf("[VENDING] carry[0x41]\n");
            sendStr400(0x41, { '1' }); // carrying
            break;
        }
    }

    /*printf("str400Send\n");
    for (uint32_t x = 0; x < size; x++)
        printf("%02X", data[x]);
    printf("\n");*/
}

defineHook(int, str400Receive5dxp, int a1, void* data, int length) {
    return jmp_str400Receive(data, length);
}

defineHook(int, str400Send5dxp, int a1, void* data, int length) {
    jmp_str400Send(1, (uint8_t*)data, length);
    return length;
}

void init_str400() {
    if (isMt4) {
        enableHook(str400Send, 0x83713D0);
        enableHook(str400Receive, 0x8371440);
    } else {
        enableHook(str400Send5dxp, 0x80EEFC0);
        enableHook(str400Receive5dxp, 0x80EEF50);
    }
}