// servo pwm pin
#ifndef PIN_SERVO

#define PIN_SERVO GPIO_NUM_10

// potentiometer. Analog
#define PIN_POTENTIOMETER GPIO_NUM_4

// Load cell pins and settings
#define PIN_LC_DOUT GPIO_NUM_8 // mcu > HX711 dout pin
#define PIN_LC_SCK GPIO_NUM_6  // mcu > HX711 sck pin

#define LOAD_CELL_ENABLE_TARE_ON_START false
#define LOAD_CELL_STABILIZE_TIME LOAD_CELL_ENABLE_TARE_ON_START ? 1000 : 0

// RPM sense pin.
#define PIN_RPM GPIO_NUM_5

// bat measurements
#define PIN_BAT_VOLTAGE GPIO_NUM_0
#define PIN_BAT_CURRENT GPIO_NUM_2

// wifi & server settings
#define POWERMETER_mDNS_NAME "zmotor_tester"

#define AP true
// #define ENABLE_DNS true

#ifdef AP
#define WIFI_SSID "zmotor_tester.local"
#define WIFI_PASS "12345678"
#endif

#ifndef AP
#define WIFI_SSID "H294"
#define WIFI_PASS "12384344"
#endif


// SD card stuff
#define PIN_SD_CS GPIO_NUM_21
#define PIN_SD_MOSI GPIO_NUM_20
#define PIN_SD_CLK GPIO_NUM_1
#define PIN_SD_MISO GPIO_NUM_3

typedef struct Measurement
{
    float mtime;
    float potentiometer_percent;
    float setValue_percent;
    float rpm;
    int32_t loadCell_raw;
    float loadCell;
    float batCurrent_ma;
    float batVoltage_v;
    float power;

    float loadCellZeroOffset = 0;

    char formattedMessage[500];


    const char *jsonlFormat = "{\"mtime\":%f,\"potentiometer_percent\":%f, \"setValue_percent\":%f, \"rpm\":%f, \"loadCell_raw\":%ld, \"batCurrent_ma\":%f, \"batVoltage_v\":%f, \"power\":%f, \"loadCell\":%f}\n";
    const char *csvFormat = "%f,%f,%f,%f,%ld,%f,%f,%f,%f\n";
    const char *csvHeader = "mtime, potentiometer_percent, setValue_percent, rpm, loadCell_raw, batCurrent_ma, batVoltage_v, power, loadCell\n";

    const char *arduinoFormat = "mtime:%f,potentiometer_percent:%f, setValue_percent:%f, rpm:%f, loadCell_raw:%ld, batCurrent_ma:%f, batVoltage_v:%f, power:%f, loadCell:%f\n";

    char *formatMeasurement(const char *formatString)
    {
        sprintf(formattedMessage, formatString, mtime,potentiometer_percent, setValue_percent, rpm, loadCell_raw, batCurrent_ma, batVoltage_v, power, loadCell);
        return formattedMessage;
    }
    char *formatMeasurementJSONL(){
        return formatMeasurement(jsonlFormat);
    }
    char *formatMeasurementCSV(){
        return formatMeasurement(csvFormat);
    }
    char *formatMeasurementArduinoPlot(){
        return formatMeasurement(arduinoFormat);
    }


} Measurement_t;

typedef struct Settings
{
    unsigned int magicNumber = 1;
    bool sdLoggingEnabled = true;
    unsigned int lastFilenumber;

    float loadCellRawOfset = -11800;
    float loadCellScaleFactor = 146.717455079556635;
} Settings_t;

void setupStorage(void);
void setupLoadCell(void);
void dataReadyISR();
void saveToFile(char *s);
void ARDUINO_ISR_ATTR rpmInterruptHandler();
#endif
