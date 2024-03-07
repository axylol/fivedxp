#include "dobby.h"

#define defineHook(type, name, ...) typedef type (*__type_old_##name)(__VA_ARGS__); \
__type_old_##name old_##name = 0; \
type jmp_##name(__VA_ARGS__)

#define enableHook(name, address) DobbyHook((void*)address, (void*)jmp_##name, (void**)&old_##name);

#define callOld(name, ...) old_##name(__VA_ARGS__)