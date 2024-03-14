#pragma once
#include <string>

extern bool isTerminal;
extern char* accessCode;
extern char* chipID;
extern bool isMt4;
extern bool useSurround51;
extern char* redirectMagneticCard;
extern bool useJvs;
extern bool useStr400;
extern bool useStr3;
extern bool useTouch;
extern bool useKeyboard;
extern bool useBana;
extern bool useLimiter;

bool loadConfig();