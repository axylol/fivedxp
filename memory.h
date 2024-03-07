#pragma once
#include <cstdint>
#include <vector>
#include <string>

void patchMemory(void* address, std::vector<uint8_t> data);
void patchMemoryString0(void* address, std::string data);