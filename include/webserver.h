#include <main.h>
#include <ESPAsyncWebServer.h>

void processDNS(void);

AsyncWebServer* setupWebserver(void);

void sendWsMessage(char * message);