#include "config.h"
#include "toml.hpp"

bool isTerminal = false;
char* accessCode = NULL;
char* chipID = NULL;
bool isMt4 = false;
bool useSurround51 = false;
char* redirectMagneticCard = NULL;
bool useJvs = true;
bool useStr400 = true;
bool useStr3 = false;
bool useTouch = true;
bool useKeyboard = true;
bool useBana = true;
bool useLimiter = true;
char* redirectBanaReader = NULL;

char* copyCharString(char* str) {
    size_t len = strlen(str);
    char* p = new char[len + 1];
    if (len > 0)
        memcpy(p, str, len);
    p[len] = 0;
    return p;
}

char* createCharString(std::string str) {
    return copyCharString((char*)str.c_str());
}

bool loadConfig() {
    try {
        auto config = toml::parse_file( "config.toml" );

        auto gameConfig = config["game"];
        isMt4 = gameConfig["mt4"].as_boolean()->get();
        isTerminal = gameConfig["terminal"].as_boolean()->get();

        if (gameConfig["limiter"].is_boolean())
            useLimiter = gameConfig["limiter"].as_boolean()->get();

        auto banaConfig = config["bana"];
        useBana = banaConfig["enabled"].as_boolean()->get();
        if (useBana) {
            accessCode = createCharString(banaConfig["access_code"].as_string()->get());
            chipID = createCharString(banaConfig["chip_id"].as_string()->get());
        }

        auto banaRedirConfig = config["bana_redir"];
        if (banaRedirConfig["redirect"].as_boolean()->get())
            redirectBanaReader = createCharString(banaRedirConfig["path"].as_string()->get());

        auto emuConfig = config["emu"];
        useJvs = emuConfig["jvs"].as_boolean()->get();
        useStr400 = emuConfig["str400"].as_boolean()->get();
        useStr3 = emuConfig["str3"].as_boolean()->get();
        useTouch = emuConfig["touch"].as_boolean()->get();

        auto soundConfig = config["sound"];
        useSurround51 = soundConfig["surround51"].as_boolean()->get();

        auto mgConfig = config["magnetic_card"];
        if (mgConfig["redirect"].as_boolean()->get())
            redirectMagneticCard = createCharString(mgConfig["path"].as_string()->get());
    } catch (toml::ex::parse_error& e) {
        printf("error reading config.toml\n%s\n", e.what());
        return false;
    } catch (...) {
        printf("unknown exception while reading config\n");
        return false;
    }
    return true;
}