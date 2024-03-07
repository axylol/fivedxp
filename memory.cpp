#include "memory.h"
#include <csignal>
#include <sys/mman.h>
#include <cstring>

void patchMemory(void* address, std::vector<uint8_t> data) {
    int page_size = sysconf(_SC_PAGESIZE);
    int aligned_address = ((int)address & ~(page_size - 1));
    int end = (int)address + data.size();
    int new_size = end - aligned_address;

    mprotect((void*)aligned_address, new_size, PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy(address, data.data(), data.size());
}

void patchMemoryString0(void* address, std::string data) {
    std::vector<uint8_t> p;
    p.resize(data.size());
    memcpy(p.data(), data.c_str(), data.size());

    patchMemory(address, p);
    patchMemory((uint8_t*)address + data.size(),  { 0x00 });
}