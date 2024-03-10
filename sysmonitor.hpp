#include "hook.h"
#include <mqueue.h>
#include <cstdio>
#include <cstring>

mqd_t replyFd = 1;
mqd_t commandFd = 2;

defineHook(mqd_t, mq_open, const char *name, int oflag, int mode, void* arg) {
    printf("mq_open %s\n", name);
    if (strcmp(name, "/Sys.Monitor.Command") == 0)
        return commandFd;
    if (strcmp(name, "/Sys.Monitor.Reply") == 0)
        return replyFd;
    return -1;
}

defineHook(int, mq_getattr, mqd_t fd, struct mq_attr *attr) {
    return 0;
}

defineHook(int, mq_notify) {
    return 0;
}

defineHook(int, mq_send, mqd_t fd, const char* msg_ptr, size_t msg_len, unsigned int msg_prio) {
    return 0;
}

defineHook(int, mq_receive, mqd_t fd, char* msg_ptr, size_t msg_len, unsigned int *msg_prio) {
    errno = EAGAIN;
    return -1;
}

/*sub_80A5F20*/

defineHook(int, updateSysMonitorMt4, int a1, int a2) {
    int type = *(int*)(a1 + 628);

    if (type == 0) {
        type = 1;
    } else if (type == 1) { // checking cable
        type = 2;
        *(int*)(a1 + 552) = 3; // cable is good
    } else if (type == 3) {
        *(int*)(a1 + 556) = 3; // gateway is good
    } else if (type == 4) {
        *(int*)(a1 + 436) = 3; // checking shop router
    } else if (type == 5) {
        *(int*)(a1 + 576) = 1;
        *(int*)(a1 + 568) = 3; // shop router hops = 1
    } else if (type == 7) { // ntp
        *(int*)(a1 + 548) = 3; // synchronized date
    } else if (type == 9) { // renew
        *(int*)(a1 + 544) = 3;
    } else if (type == 0xC) { // set date
        *(int*)(a1 + 588) = 0;
    } else if (type == 0xD) { // online network check
        *(int*)(a1 + 592) = 0;
    } else if (type == 0xE) { // check cable
        *(int*)(a1 + 552) = 0;
    } else if (type == 0x0F) { // ntp
        *(int*)(a1 + 592) = 0;
    }

    *(int*)(a1 + 628) = type;

    return callOld(updateSysMonitorMt4, a1, a2);
}


void initSysMonitor() {
    if (isMt4) {
        enableHook(updateSysMonitorMt4, 0x80A9B70);
    } else {
        // TODO: 5dx+ before release please
    }

    enableHook(mq_open, mq_open);
    enableHook(mq_getattr, mq_getattr);
    enableHook(mq_notify, mq_notify);
    enableHook(mq_send, mq_send);
    enableHook(mq_receive, mq_receive);
}