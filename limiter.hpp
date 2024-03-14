#include <dlfcn.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include "config.h"
#include "hook.h"

// based on old https://gitlab.com/torkel104/libstrangle

long targetFrameTime = -1;

struct timespec oldTimestamp,
        newTimestamp,
        sleepyTime,
        remainingTime;

const clockid_t clockType = CLOCK_MONOTONIC_RAW;
void limiter() {
    if ( clock_gettime( clockType, &newTimestamp ) == 0 ) {
        sleepyTime.tv_nsec = targetFrameTime - newTimestamp.tv_nsec + oldTimestamp.tv_nsec;
        /*while( sleepyTime.tv_nsec > 0 && sleepyTime.tv_nsec < targetFrameTime ) {

            // sleep in smaller and smaller intervals
            sleepyTime.tv_nsec /= 2;
            nanosleep( &sleepyTime, &remainingTime );
            clock_gettime( clockType, &newTimestamp );
            sleepyTime.tv_nsec = targetFrameTime - newTimestamp.tv_nsec + oldTimestamp.tv_nsec;

            // For FPS == 1 this is needed as tv_nsec cannot exceed 999999999
            sleepyTime.tv_nsec += newTimestamp.tv_sec*1000000000 - oldTimestamp.tv_sec*1000000000;
        }*/

        if (sleepyTime.tv_nsec > 0 && sleepyTime.tv_nsec < targetFrameTime) {
            nanosleep( &sleepyTime, &remainingTime );
        }

        clock_gettime( clockType, &oldTimestamp );
    }
}

defineHook(void, glXWaitVideoSyncSGI) {
    limiter();
}

defineHook(void, initGlx, int* a1) {
    callOld(initGlx, a1);
    a1[56] = (int)jmp_glXWaitVideoSyncSGI;
}

void init_limiter() {
    if (isMt4) {
        enableHook(initGlx, 0x89BA530);
    } else {
        enableHook(initGlx, 0xa826310);
    }

    targetFrameTime = 1000000000 / 60;
}