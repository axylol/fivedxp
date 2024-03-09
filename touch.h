#pragma once

void update_touch(bool state, int x, int y);

void initTouch();
void writeTouch(void* packet, int size);
bool readTouch(void* packet, int* size);