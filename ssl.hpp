#include "hook.h"

int sslCallback() {
    return 1;
}

defineHook(void, SSL_CTX_set_verify, int a1, int mode, void* callback) {
    callOld(SSL_CTX_set_verify, a1, 0, (void*)sslCallback);
}

defineHook(int, curl_easy_setopt, void *handle, int option, int param) {
    if (option == 10004)
        return 0;

    if (option == 64) { // verifypeer
        // tls v1.3
        callOld(curl_easy_setopt, handle, 32, 7);

        return callOld(curl_easy_setopt, handle, option, 0);
    }

    if (option == 81) { // verifyhost
        return callOld(curl_easy_setopt, handle, option, 0);
    }

    return callOld(curl_easy_setopt, handle, option, param);
}

defineHook(void, SSLv2totls, int* a1, int a2) {
    callOld(SSLv2totls, a1, 6); //tlsv1
}

typedef void* (*t_TLSv1_method)();

// they tried to load TLSv1_method from here :sob:
t_TLSv1_method TLSv1_method_fake = NULL;

defineHook(void*, SSLv23_method) {
    if (!TLSv1_method_fake)
        TLSv1_method_fake = (t_TLSv1_method)dlsym(dlopen("libssl.so.0.9.8", 2), "TLSv1_method");
    return TLSv1_method_fake();
}

void disableSSLCert() {
    if (isMt4) {
        enableHook(SSL_CTX_set_verify, 0x8059628);
        enableHook(SSLv23_method, 0x8058718);
    } else {
        enableHook(SSL_CTX_set_verify, 0x80576BC);
        enableHook(curl_easy_setopt, 0x80560EC);
        enableHook(SSLv2totls, 0xA764E00);
    }
}