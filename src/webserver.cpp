#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <ESPmDNS.h>
#include <webserver.h>
#include "SD.h"

#ifdef AP
#include <DNSServer.h>
#ifdef ENABLE_DNS
const byte DNS_PORT = 53;
IPAddress apIP(8, 8, 4, 4); // The default android DNS
DNSServer dnsServer;
#endif
#ifndef ENABLE_DNS
IPAddress apIP(10, 9, 0, 1);
#endif

#endif

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

AsyncWebServer server(80);
AsyncWebSocket liveDataWebSocket("/liveData");

void onWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

AsyncWebServer* setupWebserver(void)
{
#ifdef AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

#ifdef ENABLE_DNS
  dnsServer.start(DNS_PORT, "*", apIP);
#endif
#endif
#ifndef AP
  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

#endif
  // initialize SPIFFS
  SPIFFS.begin();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              Serial.println(request->url());

              AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.html", "text/html");

              response->addHeader("Access-Control-Allow-Origin","*");
              request->send(response);

              // request->send(SPIFFS, "/index.html", "text/html"); 
            });

  server.serveStatic("/", SPIFFS, "/");

  server.on("/flist", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              Serial.println(request->url());
              request->send(200, "text/json", "[\"raw_data_00067.jsonl\",\"raw_data_00071.jsonl\",\"raw_data_00072.jsonl\"]"); });


  server.serveStatic("/sd", SD, "/");


  liveDataWebSocket.onEvent(onWSEvent);
  server.addHandler(&liveDataWebSocket);

  server.begin();

  if (!MDNS.begin(POWERMETER_mDNS_NAME))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  return &server;
}

void notifyClients()
{
  liveDataWebSocket.textAll(String("ledState"));
}

void sendWsMessage(char *message)
{

  liveDataWebSocket.textAll(message);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    if (strcmp((char *)data, "toggle") == 0)
    {
      // ledState = !ledState;
      notifyClients();
    }
  }
}

void onWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    server->cleanupClients();
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void processDNS(void)
{
#ifdef ENABLE_DNS
  dnsServer.processNextRequest();
#endif
}