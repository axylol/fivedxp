#include <dlfcn.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <GL/gl.h>
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
        while( sleepyTime.tv_nsec > 0 && sleepyTime.tv_nsec < targetFrameTime ) {

            // sleep in smaller and smaller intervals
            sleepyTime.tv_nsec /= 2;
            nanosleep( &sleepyTime, &remainingTime );
            clock_gettime( clockType, &newTimestamp );
            sleepyTime.tv_nsec = targetFrameTime - newTimestamp.tv_nsec + oldTimestamp.tv_nsec;

            // For FPS == 1 this is needed as tv_nsec cannot exceed 999999999
            sleepyTime.tv_nsec += newTimestamp.tv_sec*1000000000 - oldTimestamp.tv_sec*1000000000;
        }
        clock_gettime( clockType, &oldTimestamp );
    }
}

struct timespec oldTimestamp2,
        newTimestamp2,
        sleepyTime2,
        remainingTime2;

void limiter2() {
    if ( clock_gettime( clockType, &newTimestamp2 ) == 0 ) {
        sleepyTime2.tv_nsec = targetFrameTime - newTimestamp2.tv_nsec + oldTimestamp2.tv_nsec;
        while( sleepyTime2.tv_nsec > 0 && sleepyTime2.tv_nsec < targetFrameTime ) {

            // sleep in smaller and smaller intervals
            sleepyTime2.tv_nsec /= 2;
            nanosleep( &sleepyTime2, &remainingTime2 );
            clock_gettime( clockType, &newTimestamp2 );
            sleepyTime2.tv_nsec = targetFrameTime - newTimestamp2.tv_nsec + oldTimestamp2.tv_nsec;

            // For FPS == 1 this is needed as tv_nsec cannot exceed 999999999
            sleepyTime2.tv_nsec += newTimestamp2.tv_sec*1000000000 - oldTimestamp2.tv_sec*1000000000;
        }
        clock_gettime( clockType, &oldTimestamp2 );
    }
}

defineHook(void, glXSwapBuffers, int dpy, int drawable ) {
    callOld(glXSwapBuffers, dpy, drawable);

    limiter();
}

defineHook(void, glXWaitVideoSyncSGI) {
    limiter2();
}

defineHook(void, initGlx, int* a1) {
    callOld(initGlx, a1);
    a1[56] = (int)jmp_glXWaitVideoSyncSGI;
}

void init_limiter() {
    if (isMt4) {
        enableHook(glXSwapBuffers, 0x8059038);
        enableHook(initGlx, 0x89BA530);
    }

    targetFrameTime = 1000000000 / 60;
}