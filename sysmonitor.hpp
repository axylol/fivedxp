#include "hook.h"
#include <mqueue.h>
#include <cstdio>
#include <cstring>

mqd_t replyFd = 1;
mqd_t commandFd = 2;

bool redirectSysMonitor = false;

defineHook(mqd_t, mq_open, const char *name, int oflag, int mode, void* arg) {
    printf("mq_open %s\n", name);

    if (!redirectSysMonitor) {
        if (strcmp(name, "/Sys.Monitor.Command") == 0)
            return commandFd;
        if (strcmp(name, "/Sys.Monitor.Reply") == 0)
            return replyFd;
    } else {
        if (strcmp(name, "/Sys.Monitor.Command") == 0) {
            commandFd = callOld(mq_open, "/sysmonitoremu.command2", O_WRONLY | O_NONBLOCK, 0, NULL);
            return commandFd;
        }
        if (strcmp(name, "/Sys.Monitor.Reply") == 0) {
            replyFd = callOld(mq_open, "/sysmonitoremu.reply2", O_RDONLY | O_NONBLOCK, 0, NULL);
            return replyFd;
        }
    }
    return -1;
}

defineHook(int, mq_getattr, mqd_t fd, struct mq_attr *attr) {
    if (!redirectSysMonitor)
        return 0;
    return callOld(mq_getattr, fd, attr);
}

defineHook(int, mq_send, mqd_t fd, const char* msg_ptr, size_t msg_len, unsigned int msg_prio) {
    if (!redirectSysMonitor || fd == replyFd)
        return msg_len;
    return callOld(mq_send, fd, msg_ptr, msg_len, msg_prio);
}

defineHook(int, mq_receive, mqd_t fd, char* msg_ptr, size_t msg_len, unsigned int *msg_prio) {
    if (!redirectSysMonitor || fd == commandFd) {
        errno = EAGAIN;
        return -1;
    }
    return callOld(mq_receive, fd, msg_ptr, msg_len, msg_prio);
}

void initSysMonitor() {
    enableHook(mq_open, mq_open);

    enableHook(mq_getattr, mq_getattr);
    enableHook(mq_send, mq_send);
    enableHook(mq_receive, mq_receive);
}