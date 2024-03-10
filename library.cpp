#include <iostream>
#include "hasp.hpp"
#include "hook.h"
#include <fcntl.h>
#include <cstring>
#include <cstdarg>
#include <termios.h>
#include <csignal>
#include <thread>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "jvs.h"
#include "touch.h"
#include "sysmonitor.hpp"
#include "nsadrv.hpp"
#include "bana.h"
#include "config.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <fstream>
#include "json.hpp"
#include "memory.h"
#include "ssl.hpp"
#include "input.h"

using namespace nlohmann;

int jvsFd = 144444443;
int touchFd = 144444444;
int strFd = 144444445;

#include <GL/gl.h>
#include <GL/glx.h>
#include <sys/stat.h>


void jvsThread() {
    init_input();
    initJvs();

    while (!inputStopped) {
        jvsMutex.lock();
        update_input();
        jvsMutex.unlock();
        usleep(1 * 1000);
    }
}

defineHook(int, system, const char* command) {
    printf("system(\"%s\")\n", command);
    return -1;
}

#define YA_CARD_EMU_FIFO "/tmp/yacardemu-mt4"

defineHook(int, open, const char *pathname, int flags, ...) {
    //printf("open(\"%s\", %d)\n", pathname, flags);

    int mode = 0;
    bool twoArgs = true;

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);

        twoArgs = false;
    }

    if (strstr(pathname, "/dev/tty") == pathname) {
        printf("open(\"%s\", %d)\n", pathname, flags);
    }

    if (isMt4 && strcmp(pathname, "/dev/ttyS1") == 0 && !redirectMagneticCard.empty())
        return callOld(open, redirectMagneticCard.c_str(), flags);

    if (strcmp(pathname, "/dev/ttyS2") == 0)
        return jvsFd;

    if (strcmp(pathname, "/dev/ttyS0") == 0) {
        if (isTerminal) {
            initTouch();
            return touchFd;
        }
        return strFd;
    }

    if(twoArgs)
        return callOld(open, pathname, flags);
    return callOld(open, pathname, flags, mode);
}

defineHook(int, close, int fd) {
    if (fd == jvsFd)
        return 0;
    if (fd == touchFd)
        return 0;
    if (fd == strFd)
        return 0;
    return callOld(close, fd);
}

defineHook(int, ioctl, int fd, unsigned long request) {
    if (fd == jvsFd) {
        switch (request) {
            case 0x545D: {
                return -1;
            }
            case 0x5415: {
                return -1;
            }
            case 0x540b: {
                return -1;
            }
        }
        return 0;
    }
    if (fd == touchFd)
        return 0;
    if (fd == strFd)
        return 0;

    return callOld(ioctl, fd, request);
}

defineHook(int, fcntl, int fd, int cmd, void* arg) {
    if (fd == touchFd)
        return 0;
    if (fd == strFd)
        return 0;
    return callOld(fcntl, fd, cmd, arg);
}

defineHook(int, fcntl64, int fd, int cmd, void* arg) {
    if (fd == touchFd)
        return 0;
    if (fd == strFd)
        return 0;
    return callOld(fcntl, fd, cmd, arg);
}

defineHook(int, tcgetattr, int fildes, struct termios *termios_p) {
    if (fildes == jvsFd)
        return 0;
    if (fildes == touchFd)
        return 0;
    if (fildes == strFd)
        return 0;
    return callOld(tcgetattr, fildes, termios_p);
}

defineHook(int, tcsetattr, int fildes, int optional_actions,
           const struct termios *termios_p) {
    if (fildes == jvsFd)
        return 0;
    if (fildes == touchFd)
        return 0;
    if (fildes == strFd)
        return 0;
    return callOld(tcsetattr, fildes, optional_actions, termios_p);
}

defineHook(int, epoll_ctl, int epfd, int op, int fd, struct epoll_event *event) {
    if (fd == touchFd)
        return 0;
    return callOld(epoll_ctl, epfd, op, fd, event);
}

defineHook(int, read, int fd, void* buf, size_t count) {
    if (fd == jvsFd) {
        jvsMutex.lock();
        updateJvs();
        jvsMutex.unlock();

        int size;
        if (!readJvs(buf, &size)) {
            printf("no packet jvs\n");
            errno = EINVAL;
            return -1;
        }
        return size;
    }

    if (fd == strFd) {
        errno = EINVAL;
        return -1;
    }

    if (fd == touchFd) {
        int size;
        if (!readTouch(buf, &size))
            return 0;
        return size;
    }

    return callOld(read, fd, buf, count);
}

defineHook(int, write, int fd, const void* buf, size_t count) {
    if (fd == jvsFd) {
        writeJvs((void*)buf, count);

        jvsMutex.lock();
        updateJvs();
        jvsMutex.unlock();
        return count;
    }

    if (fd == strFd)
        return count;

    if (fd == touchFd) {
        writeTouch((void*)buf, count);
        return count;
    }

    return callOld(write, fd, buf, count);
}

defineHook(ssize_t, writev, int fd, const struct iovec *iov, int iovcnt) {
    if (fd == touchFd) {
        writeTouch((void*)iov->iov_base, iov->iov_len);
        return iov->iov_len;
    }
    return callOld(writev, fd, iov, iovcnt);
}

defineHook(ssize_t, readv, int fd, const struct iovec *iov, int iovcnt) {
    if (fd == touchFd) {
        int size;
        if (!readTouch(iov->iov_base, &size))
            return 0;
        return size;
    }
    return callOld(writev, fd, iov, iovcnt);
}

defineHook(FILE*, popen, const char* command, const char* type) {
    printf("popen %s\n", command);
    if (strstr(command, "LANG=C;/usr/bin/sudo ") == command) {
        return callOld(popen, command + 21, type);
    }
    return callOld(popen, command, type);
}

char logBuffer[0x10000];

defineHook(void, log, void* a1, int type, const char* msg, ...) {
    va_list va;
    va_start(va, msg);
    int len = vsnprintf(logBuffer, sizeof(logBuffer), msg, va);
    va_end(va);
    if (len < 1)
        return;
    switch (type) {
        case 1: {
            printf("[INF] ");
            break;
        } case 2: {
            printf("[WRN] ");
            break;
        } case 3: {
            printf("[SYS] ");
            break;
        } case 4: {
            printf("[ERR] ");
            break;
        }
    }
    printf("%s", logBuffer);
}

defineHook(void, logmt4, int type, const char* msg, ...) {
    va_list va;
    va_start(va, msg);
    int len = vsnprintf(logBuffer, sizeof(logBuffer), msg, va);
    va_end(va);
    if (len < 1)
        return;
    switch (type) {
        case 1: {
            printf("[INF] ");
            break;
        } case 2: {
            printf("[WRN] ");
            break;
        } case 3: {
            printf("[SYS] ");
            break;
        } case 4: {
            printf("[ERR] ");
            break;
        }
    }
    printf("%s", logBuffer);
}

defineHook(int, getContentRouter)
{
    return inet_addr("192.168.92.254");
}

defineHook(int, isTerminal)
{
    return 1;
}

defineHook(int, isTerminalMt4, int* a1)
{
    a1[1] = 0x841B840;
    return 0;
}

defineHook(int, touchPanelFix) {
    return 1;
}

defineHook(int, bind, int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    struct sockaddr_in* in = ((struct sockaddr_in*) addr);
    int port = ntohs(in->sin_port);

    if (port == 50765) {
        int one = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

        // bind on 0.0.0.0 for terminal
        in->sin_addr.s_addr = 0;
    }

    return callOld(bind, sockfd, addr, addrlen);
}

defineHook(int, connect, int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    struct sockaddr_in* in = ((struct sockaddr_in*) addr);
    int port = ntohs(in->sin_port);

    char ip[20];
    inet_ntop(AF_INET, &in->sin_addr, ip, sizeof(ip));

    printf("connect %s %d\n", ip, port);

    if (port == 50765) {
        in->sin_addr.s_addr = inet_addr("127.0.0.1");
    }

    return callOld(connect, sockfd, addr, addrlen);
}

defineHook(int, decryptToken, char* dest, int* destSize, char* src, void* a4) {
    // custom solution for priv server (too lazy to reverse enc)
    if (src[0] == '@') {
        memcpy(dest, src + 1, strlen(src + 1));
        return 0;
    }
    return callOld(decryptToken, dest, destSize, src, a4);
}

int lastType = 0;
defineHook(void, updateTest, uint8_t* a1, int a2) {
    int type = *(int*)(a1 + 568);
    if (type != lastType)
        printf("test=%d\n", type);

    switch (type) {
        case 9: {
            *(int *)(a1 + 568) = 22;
            *(int *)(a1 + 1096) = 1;
            break;
        }
        case 20: {
            *(int *)(a1 + 568) = 22;
            break;
        }
    }

    lastType = type;
    callOld(updateTest, a1, a2);
}

defineHook(int, AA75120) {
    return 0;
}

defineHook(int, A393BE0) {
    return 0;
}

defineHook(void, 84EC560, int * a1) {
    *a1 = 0;
}

defineHook(void, billingSave) {

}

uint8_t ourPcb = 255;
defineHook(ssize_t, recvmsg, int fd, struct msghdr *msg, int flags) {
    int ret = callOld(recvmsg, fd, msg, flags);
    if (ret < 0)
        return ret;

    if (msg->msg_name) {
        struct sockaddr_in* in = ((struct sockaddr_in*) msg->msg_name);
        int port = ntohs(in->sin_port);

        uint8_t* data = (uint8_t*)msg->msg_iov->iov_base;
        if (port == 50765 && msg->msg_iov->iov_len >= 2) {
            if (data[0] == isMt4 ? 1 : 2) {
                //printf("%d %d\n", data[1], ourPcb);
                if (data[1] != ourPcb) {
                    switch (data[1]) {
                        case 0: {
                            in->sin_addr.s_addr = inet_addr("192.168.92.11");
                            break;
                        }
                        case 1: {
                            in->sin_addr.s_addr = inet_addr("192.168.92.12");
                            break;
                        }
                        case 2: {
                            in->sin_addr.s_addr = inet_addr("192.168.92.13");
                            break;
                        }
                        case 3: {
                            in->sin_addr.s_addr = inet_addr("192.168.92.14");
                            break;
                        }
                        case 4: {
                            in->sin_addr.s_addr = inet_addr("192.168.92.20");
                            break;
                        }
                    }
                }
            }
        }
    }

    return ret;
}

defineHook(int, alin0) {
    return 0;
}

defineHook(int, sendAlthmand) {
    return 0;
}

defineHook(int, XCreatePixmapCursor) {
    return 0;
}

defineHook(Status, XGetWindowAttributes, Display *display, Window w, XWindowAttributes *window_attributes_return) {
    Status ret = callOld(XGetWindowAttributes, display, w, window_attributes_return);
    window_attributes_return->width = 1360;
    window_attributes_return->height = 768;
    return ret;
}

defineHook(Display*, XOpenDisplay, char* name) {
    Display* ret = callOld(XOpenDisplay, NULL);
    if (!ret)
        ret = callOld(XOpenDisplay, name);
    if (!ret)
        printf("cant open any display\n");
    return ret;
}

defineHook(ssize_t, sendmsg, int fd, struct msghdr *msg, int flags) {
    if (msg->msg_name) {
        struct sockaddr_in* in = ((struct sockaddr_in*) msg->msg_name);
        int port = ntohs(in->sin_port);
        if (port == 50765 && msg->msg_iov->iov_len >= 2) {
            uint8_t* data = (uint8_t*)msg->msg_iov->iov_base;
            if (data[0] == isMt4 ? 1 : 2) {
                ourPcb = data[1];
            }
        }
    }
    return callOld(sendmsg, fd, msg, flags);
}

defineHook(int, refreshNetwork, int a1) {
    int ret = callOld(refreshNetwork, a1);
    printf("refreshNetwork=%d\n", ret);

    if (ret != 1)
        return ret;
    *(int*)(a1 + 20) = htons(inet_addr("192.168.92.254"));
    return 1;
}

defineHook(int, str400Receive, int a1, int* a2) {
    *a2 = 0;
    return 0;
}

defineHook(int, str400_reset_status_wait, int* param1) {
    printf("str400_reset_status_wait\n");

    *param1 = 0x835e5c0; // 0
    param1[1] = 0; // 4
    return 1;
}

__attribute__((constructor))
void initialize_wlldr() {
    std::ifstream f("./config.json");
    if (!f.is_open()) {
        printf("can't open file config.json\n");
        return;
    }

    try {
        json config = json::parse(f);
        f.close();

        isTerminal = config.at("terminal").get<bool>();

        if (config.contains("access_code") && config.contains("chip_id")) {
            // crashses if i do accessCode = config
            std::string acc = config.at("access_code").get<std::string>();
            std::string cip = config.at("chip_id").get<std::string>();

            accessCode = acc;
            chipID = cip;

            if (config.contains("redirect_magnetic_card")) {
                std::string rdmc = config.at("redirect_magnetic_card").get<std::string>();
                redirectMagneticCard = rdmc;
            }
        }

        if (config.contains("mt4"))
            isMt4 = config.at("mt4").get<bool>();

        if (config.contains("surround51"))
            useSurround51 = config.at("surround51").get<bool>();
    } catch (json::exception e) {
        f.close();

        printf("%s\n", e.what());
        return;
    } catch (...) {
        printf("a exception uhh\n");
        return;
    }

    printf("terminal=%d, mt4=%d\n", isTerminal, isMt4);

    initHasp();
    printf("hasp\n");
    if (!useSurround51) {
        initNsadrv();
        printf("nsadrv\n");
    }
    initSysMonitor();
    printf("monitor\n");
    initBana();
    printf("bana\n");

    // disable ssl verification for mucha
    disableSSLCert();
    printf("ssl\n");

    if (isMt4) {
        patchMemoryString0((void*)0x8c11004, "mucha.local");

        enableHook(logmt4, 0x809ddb0);

       if (isTerminal)
           enableHook(isTerminalMt4, 0x841BF00);

        // content router fix
        enableHook(refreshNetwork, 0x821D4F0);

        patchMemory((void*)0x80a0920, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });

        enableHook(sendAlthmand, 0x89E5C80);

        enableHook(str400_reset_status_wait, 0x8361450);
        enableHook(str400Receive, 0x8371440);

        // fix resolution
        enableHook(XGetWindowAttributes, XGetWindowAttributes);

        enableHook(XOpenDisplay, 0x8057d28);
    } else {
        patchMemoryString0((void*)0xaafaa88, "mucha.local");
        //patchMemory((void*)0x81de9fc, { 0x66, 0xc7, 0x85, 0x62, 0xfe, 0xff, 0xff, 0xBB, 0x01 }); // port 443 mucha patch
        enableHook(decryptToken, 0xAA77780);

        enableHook(log, 0x80bc980);
        enableHook(log, 0x80bca60);
        enableHook(log, 0x80bcb40);

        // content router
        enableHook(getContentRouter, 0x82519d0);
        patchMemory((void*)0x827f7e3, { 0x0f, 0x85 });
        patchMemory((void*)0x827f9fc, { 0x0f, 0x84 });

        if (isTerminal) {
            enableHook(touchPanelFix, 0x83C17E0);
            enableHook(isTerminal, 0x80eeea0);
        } else {
            // local network disconnect fix
            enableHook(84EC560, 0x84EC560);
            enableHook(AA75120, 0xAA75120);
            enableHook(A393BE0, 0xA393BE0);

            // terminal check bypass
            enableHook(updateTest, 0x84FA120);
        }

        // dont save billing = dont crash
        enableHook(billingSave, 0x8401A70);

        enableHook(sendAlthmand, 0xa851320);

        enableHook(XOpenDisplay, 0x805504c);
    }

    if (isTerminal)
        ourPcb = 4;
    enableHook(recvmsg, recvmsg);
    enableHook(sendmsg, sendmsg);

    enableHook(system, system);
    enableHook(open, open);
    enableHook(close, close);
    enableHook(tcgetattr, tcgetattr);
    enableHook(tcsetattr, tcsetattr);
    enableHook(epoll_ctl, epoll_ctl);
    enableHook(read, read);
    enableHook(write, write);
    enableHook(writev, writev);
    enableHook(readv, readv);
    enableHook(ioctl, ioctl);
    enableHook(fcntl, fcntl);
    enableHook(fcntl64, fcntl64);
    enableHook(popen, popen);

    enableHook(bind, bind);
    enableHook(connect, connect);

    void *alin_dll = dlopen("./alin.dll", 2);
    enableHook(alin0, dlsym(alin_dll, "alin_init"));
    enableHook(alin0, dlsym(alin_dll, "alin_keyboard"));
    enableHook(alin0, dlsym(alin_dll, "alin_mouse"));
    enableHook(alin0, dlsym(alin_dll, "alin_pad"));
    enableHook(alin0, dlsym(alin_dll, "alin_term"));

    // dont hide cursor
    enableHook(XCreatePixmapCursor, XCreatePixmapCursor);

    std::thread t(jvsThread);
    t.detach();
}