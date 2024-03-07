#include <dlfcn.h>
#include "hook.h"

defineHook(int, nsAdrv_init, const char *_default, const char *surround51, int unk6, int unk32, int frequency, int unk256, int unk) {
    return callOld(nsAdrv_init, _default, "default", unk6, unk32, frequency, unk256, unk);
}

void initNsadrv() {
    void* nsAdrv_dll = dlopen("./nsAdrv.dll", 2);

    enableHook(nsAdrv_init, dlsym(nsAdrv_dll, "nsAdrv_init"));
}
