#include <dlfcn.h>
#include "hook.h"

defineHook(int, nsAdrv_init, const char *_default, const char *surround51, int unk6, int unk32, int frequency, int unk256, int unk) {
    return callOld(nsAdrv_init, _default, "default", unk6, unk32, frequency, unk256, unk);
}

defineHook(int, snd_mixer_selem_set_playback_volume)
{
    printf("snd_mixer_selem_set_playback_volume\n");
    return 0;
}

defineHook(int, snd_mixer_selem_set_playback_volume_all)
{
    printf("snd_mixer_selem_set_playback_volume_all\n");
    return 0;
}

defineHook(int, snd_mixer_selem_set_capture_volume)
{
    printf("snd_mixer_selem_set_capture_volume\n");
    return 0;
}

defineHook(int, snd_mixer_selem_set_capture_volume_all)
{
    printf("snd_mixer_selem_set_capture_volume_all\n");
    return 0;
}

void initNsadrv() {
    void* nsAdrv_dll = dlopen("./nsAdrv.dll", 2);

    enableHook(nsAdrv_init, dlsym(nsAdrv_dll, "nsAdrv_init"));

    void *libasound = dlopen("libasound.so.2", 2);
    enableHook(snd_mixer_selem_set_playback_volume, dlsym(libasound, "snd_mixer_selem_set_playback_volume"));
    enableHook(snd_mixer_selem_set_playback_volume_all, dlsym(libasound, "snd_mixer_selem_set_playback_volume_all"));
    enableHook(snd_mixer_selem_set_capture_volume, dlsym(libasound, "snd_mixer_selem_set_capture_volume"));
    enableHook(snd_mixer_selem_set_capture_volume_all, dlsym(libasound, "snd_mixer_selem_set_capture_volume_all"));
}
