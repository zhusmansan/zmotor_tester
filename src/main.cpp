#include <Arduino.h>
#include <ESP32Servo.h>

#include "EEPROM.h"

#include <main.h>

#include "HX711.h"

#include <Wire.h>
#include "SPI.h"
#include "SD.h"
#include "FS.h"

#include "webserver.h"
#include <ESPAsyncWebServer.h>

HX711 scale;

Servo myservo; // create servo object to control a servo

// object that stores all measured values
Measurement_t m;

Settings_t settings;
char fName[30];

// EEPROM
#define EEPROM_SIZE sizeof(Settings_t)

File dataFile;

unsigned long lastIsrTime = 0;
long rpmPeriod = 0;

long sweepCurrentTm = 0;
long sweepMaxTm = 1000;

void taskServoLoop(void *pvParameters)
{
  while (true)
  {
    if (sweepCurrentTm == 0)
    {
      m.setValue_percent = m.potentiometer_percent;
    }
    else
    {
      m.setValue_percent = constrain(map(sweepCurrentTm, sweepMaxTm, 0, 0, 100), 0, 100);
      sweepCurrentTm--;
    }
    // write value to servo motor
    myservo.write(map(m.setValue_percent, 0, 100, 0, 180));
    delay(10);
  }
}

void setup(void)

{
  Serial.begin(115200);

  myservo.setPeriodHertz(50); // Standard 50hz servo
  myservo.attach(PIN_SERVO, 995, 2020);
  myservo.write(0);

  // configure rpm meter pin and interrupt handling
  pinMode(PIN_RPM, INPUT_PULLDOWN);
  attachInterrupt(PIN_RPM, rpmInterruptHandler, FALLING);

  setupLoadCell();

  setupStorage();

  AsyncWebServer *server =
      setupWebserver();

  // curl -X POST zmotor_tester.local/sweep?t=5
  server->on("/sweep", HTTP_POST, [](AsyncWebServerRequest *request)
             {
    
    int params = request->params();
    
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      long sweepTime = atol(p->value().c_str());
      if(sweepTime > 0){
        sweepMaxTm = constrain(sweepTime* 100, 100, 10000);
        sweepCurrentTm = sweepMaxTm;
      }else{
        sweepCurrentTm = 0;    
      }
    }
    
    
    request->send(200); });

  server->serveStatic("/sd", SD, "/").setDefaultFile(fName);

  xTaskCreate(
      taskServoLoop, "taskServoLoop" // A name just for humans
      ,
      1024 // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
      ,
      NULL // Task parameter which can modify the task behavior. This must be passed as pointer to void.
      ,
      1 // Priority
      ,
      NULL // Task handle is not used here - simply pass NULL
  );
}

int i = 0;

void loop()
{
  processDNS();

  m.mtime = millis();

  // read potentiometer value
  m.potentiometer_percent = constrain(map(analogRead(PIN_POTENTIOMETER), 0, 3250, 0, 100), 0, 100);

  // read voltage and current
  m.batVoltage_v = map(analogRead(PIN_BAT_VOLTAGE), 1885, 2860, 10000, 15000) / 1000.0;
  m.batCurrent_ma = map(analogRead(PIN_BAT_CURRENT), 2395, 2680, 0, 3000);
  m.power = m.batVoltage_v * m.batCurrent_ma / 1000;

  // condition RPM values
  if (micros() - lastIsrTime > 250000)
    rpmPeriod = 0;
  if (rpmPeriod > 500 | rpmPeriod <= 0)
  {
    m.rpm = 60000000 / (rpmPeriod == 0 ? -60000000 : rpmPeriod);
  }

  i++;
  i %= 2;
  if (i == 0)
  {
    sendWsMessage(m.formatMeasurementJSONL());
    Serial.print(m.formatMeasurementArduinoPlot());
    saveToFile(m.formatMeasurementCSV());
  }
  delay(100);
}

void saveToFile(char *s)
{
  if (settings.sdLoggingEnabled)
  {
    dataFile.print(s);
    dataFile.flush();
  }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.print(file.name());
      time_t t = file.getLastWrite();
      struct tm *tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      if (levels)
      {
        listDir(fs, file.path(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.print(file.size());
      time_t t = file.getLastWrite();
      struct tm *tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    file = root.openNextFile();
  }
}

void setupStorage(void)
{
  delay(1000);
  SPI.begin(PIN_SD_CLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  if (!SD.begin(PIN_SD_CS))
  {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC)
  {
    Serial.println("MMC");
  }
  else if (cardType == CARD_SD)
  {
    Serial.println("SDSC");
  }
  else if (cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
  }
  else
  {
    Serial.println("UNKNOWN");
  }

  // listDir(SD, "/", 0);
  EEPROM.begin(EEPROM_SIZE);

  EEPROM.readBytes(0, &settings, EEPROM_SIZE);

  Settings_t mn;

  if (settings.magicNumber != mn.magicNumber)
  {
    settings = mn;
  }

  settings.lastFilenumber++;
  EEPROM.writeBytes(0, &settings, EEPROM_SIZE);
  EEPROM.commit();

  sprintf(fName, "/raw_data_%05u.csv", settings.lastFilenumber);
  Serial.print("fName:");
  Serial.println(fName);
  dataFile = SD.open(fName, FILE_WRITE);

  dataFile.print(m.csvHeader);
  dataFile.flush();
}

void taskReadScale(void *pvParameters)
{
  scale.tare(30);
  while (true)
  {
    m.loadCell_raw = scale.get_value(1);
    m.loadCell = (m.loadCell_raw ) / (settings.loadCellScaleFactor == 0 ? 1 : settings.loadCellScaleFactor);
    delay(50);
  }
}

void setupLoadCell(void)
{
  scale.begin(PIN_LC_DOUT, PIN_LC_SCK);
  
  // Read scale periodically
  xTaskCreate(
      taskReadScale, "TaskReadScale" // A name just for humans
      ,
      1024 // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
      ,
      NULL // Task parameter which can modify the task behavior. This must be passed as pointer to void.
      ,
      1 // Priority
      ,
      NULL // Task handle is not used here - simply pass NULL
  );
}

void ARDUINO_ISR_ATTR rpmInterruptHandler()
{
  unsigned long currentIsrTime = micros();

  rpmPeriod = currentIsrTime - lastIsrTime < 10 ? rpmPeriod : currentIsrTime - lastIsrTime;
  rpmPeriod = rpmPeriod == 0 ? -60000000 : rpmPeriod;
  lastIsrTime = currentIsrTime;
}
