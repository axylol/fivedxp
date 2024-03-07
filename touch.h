#pragma once

extern bool sendTouch;

void initTouch();
void writeTouch(void* packet, int size);
bool readTouch(void* packet, int* size);