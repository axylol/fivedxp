#include "hook.h"
#include <mqueue.h>
#include <cstdio>
#include <cstring>

mqd_t replyFd = 1;
mqd_t commandFd = 2;

defineHook(mqd_t, mq_open, const char *name, int oflag, int mode, void* arg) {
    printf("mq_open %s\n", name);

    if (!isOnline) {
        if (strcmp(name, "/Sys.Monitor.Command") == 0) {
            return commandFd;
        }

        if (strcmp(name, "/Sys.Monitor.Reply") == 0) {
            return replyFd;
        }
    } else {
        if (strcmp(name, "/Sys.Monitor.Command") == 0) {
            name = "/sysmonitoremu.command2";

            return callOld(mq_open, name, O_WRONLY | O_NONBLOCK, 0, NULL);
        }

        if (strcmp(name, "/Sys.Monitor.Reply") == 0) {
            name = "/sysmonitoremu.reply2";

            return callOld(mq_open, name, O_RDONLY | O_NONBLOCK, 0, NULL);
        }
    }
    return -1;
}

defineHook(int, mq_getattr) {
    return 0;
}

defineHook(int, mq_send, mqd_t fd, const char* msg_ptr, size_t msg_len, unsigned int msg_prio) {
    return msg_len;
}

defineHook(int, mq_receive, mqd_t fd, char* msg_ptr, size_t msg_len, unsigned int *msg_prio) {
    errno = EAGAIN;
    return -1;
}

void initSysMonitor() {
    enableHook(mq_open, mq_open);

    if (!isOnline) {
        enableHook(mq_getattr, mq_getattr);
        enableHook(mq_send, mq_send);
        enableHook(mq_receive, mq_receive);
    }
}