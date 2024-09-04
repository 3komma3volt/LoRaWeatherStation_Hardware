#include <Arduino.h>
#include <LoRaWan_APP.h>
#include <Wire.h>

#include "Light_VEML7700.h"
#include "PCF8574.h"
#include "Seeed_BME280.h"
#include "ttnparams.h"

uint32_t appTxDutyCycle = 5 * 60000;  // the frequency of readings, in minutes * 60000

LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t loraWanClass = LORAWAN_CLASS;
bool overTheAirActivation = LORAWAN_NETMODE;
bool loraWanAdr = LORAWAN_ADR;
bool keepNet = LORAWAN_NET_RESERVE;
bool isTxConfirmed = LORAWAN_UPLINKMODE;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

int temperature, humidity, batteryVoltage, batteryLevel;

long pressure;

BME280 bme280;

#if ENABLE_WINDSPEEDSENSOR == 1
volatile uint16_t windspeed;
#define WINDPIN GPIO1
#endif

#if ENABLE_BRIGHTNESSENSOR == 1
uint8_t sensor_available_brightness = 0;
uint16_t brightness;
Light_VEML7700 veml7700 =
    Light_VEML7700();  // Or change to use the BH1750, but this sensor did not
                       // work very well
#endif

#if ENABLE_WINDDIRSENSOR == 1
uint8_t sensor_winddir_available = 0;
uint8_t wind_direction = 0;
#endif

#if ENABLE_RAINSENSOR == 1
#define RAIN_PIN GPIO5
volatile uint8_t rain;
#endif

static void prepareTxFrame(uint8_t port) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);

#if ENABLE_WINDSPEEDSENSOR == 1
  windspeed = 0;
  pinMode(WINDPIN, INPUT);
  attachInterrupt(WINDPIN, windcounter, FALLING);
#endif

  delay(500);
  // Wire.begin(SDA, SCL);
  if (!bme280.init()) {
#if DEBUG_UART == 1

    Serial.println("Device error!");
#endif
  }

#if ENABLE_BRIGHTNESSENSOR == 1
  if (veml7700.begin()) {
#if DEBUG_UART == 1
    Serial.println(F("VEML7700 Advanced begin"));
#endif
    veml7700.setGain(VEML7700_GAIN_1_8);
    veml7700.setIntegrationTime(VEML7700_IT_25MS);
    sensor_available_brightness = 1;
  } else {
#if DEBUG_UART == 1
    Serial.println(F("Error initialising VEML7700"));
#endif
    appData[12] = 0xFF;  // brightness high
    appData[13] = 0xFF;  // bightness low
    sensor_available_brightness = 0;
  }
#endif

#if ENABLE_WINDDIRSENSOR == 1
  if (PCF8574_Init(0x20)) {
    sensor_winddir_available = 1;
#if DEBUG_UART == 1
    Serial.println("Init Winddirsensor PCF8574");
#endif
  } else {
    sensor_winddir_available = 0;
#if DEBUG_UART == 1
    Serial.println("Failed Winddirsensor PCF8574");
    appData[15] = 0;
#endif
  }

#endif

  delay(500);  // Time for sensor init
#if ENABLE_WINDSPEEDSENSOR == 1
  detachInterrupt(WINDPIN);
#endif

  temperature = bme280.getTemperature() * 100;
  humidity = bme280.getHumidity();
  pressure = bme280.getPressure();

#if ENABLE_BRIGHTNESSENSOR == 1
  if (sensor_available_brightness == 1) {
    brightness = veml7700.readALS();
  }
#endif

#if ENABLE_WINDDIRSENSOR == 1
  if (sensor_winddir_available == 1) {
    wind_direction = PCF8574_Read(0x20);
  }
#endif

  Wire.end();

  digitalWrite(Vext, HIGH);

  batteryVoltage = getBatteryVoltage();
  // batteryLevel = (BoardGetBatteryLevel() / 254) * 100;

  appDataSize = 17;

  appData[0] = highByte(temperature);
  appData[1] = lowByte(temperature);

  appData[2] = highByte(humidity);
  appData[3] = lowByte(humidity);

  appData[4] = (byte)((pressure & 0xFF000000) >> 24);
  appData[5] = (byte)((pressure & 0x00FF0000) >> 16);
  appData[6] = (byte)((pressure & 0x0000FF00) >> 8);
  appData[7] = (byte)((pressure & 0X000000FF));

  appData[8] = highByte(batteryVoltage);
  appData[9] = lowByte(batteryVoltage);

  appData[10] = 0xFF;  // old batterylevel
  appData[11] = 0xFF;  // old batterylevel
#if ENABLE_BRIGHTNESSENSOR == 1
  appData[12] = highByte(brightness);  // brightness high
  appData[13] = lowByte(brightness);   // bightness low
#else
  appData[12] = 0xFF;
  appData[13] = 0xFF;
#endif
#if ENABLE_RAINSENSOR == 1
  appData[14] = rain;  // rain value
#else
  appData[14] = 0xFF;
#endif
#if ENABLE_WINDDIRSENSOR == 1
  appData[15] = wind_direction;
#else
  appData[15] = 0;
#endif
#if ENABLE_WINDSPEEDSENSOR == 1
  appData[16] = windspeed;
#else
  appData[16] = 0xFF;
#endif

#if DEBUG_UART == 1
  Serial.print("Temperature: ");
  Serial.print(temperature / 100);
  Serial.print("C, Humidity: ");
  Serial.print(humidity);
  Serial.print("%, Pressure: ");
  Serial.print(pressure / 100);
  Serial.print(" mbar, Battery Voltage: ");
  Serial.print(batteryVoltage);
  Serial.print(" mV, Battery Level: ");
  Serial.print(batteryLevel);
  Serial.print(" %, Brightness: ");
  Serial.print(brightness);
  Serial.print(" lx, Rain: ");
  Serial.print(rain);
  Serial.print(" cts, Winddir: ");
  Serial.print(wind_direction, BIN);
  Serial.print(" dir ");

  Serial.print("\n");

#endif

#if ENABLE_RAINSENSOR == 1
  rain = 0;
#endif
}

#if ENABLE_WINDSPEEDSENSOR == 1
void windcounter() { windspeed++; }
#endif

#if ENABLE_RAINSENSOR == 1
void raincounter() {
  // Serial.println(rain);
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200) {
    rain = rain + 1;
  }
  last_interrupt_time = interrupt_time;
  rain = rain + 1;
}

#endif

void setup() {
  boardInitMcu();
#if DEBUG_UART == 1

  Serial.begin(115200);
#endif
#if ENABLE_RAINSENSOR == 1
  pinMode(RAIN_PIN, INPUT);
  attachInterrupt(RAIN_PIN, raincounter, FALLING);
#endif

  deviceState = DEVICE_STATE_INIT;
  LoRaWAN.ifskipjoin();
}

void loop() {
#if DEBUG_UART == 1
  prepareTxFrame(2);
#else
  if (true) {
    switch (deviceState) {
      case DEVICE_STATE_INIT: {
        printDevParam();
        LoRaWAN.init(loraWanClass, loraWanRegion);
        deviceState = DEVICE_STATE_JOIN;
        break;
      }
      case DEVICE_STATE_JOIN: {
        LoRaWAN.join();
        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
      case DEVICE_STATE_SEND: {
        prepareTxFrame(appPort);
        LoRaWAN.send();
        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
      case DEVICE_STATE_CYCLE: {
        // Schedule next packet transmission
        txDutyCycleTime = appTxDutyCycle + randr(0, APP_TX_DUTYCYCLE_RND);
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        break;
      }
      case DEVICE_STATE_SLEEP: {
        LoRaWAN.sleep();
        break;
      }
      default: {
        deviceState = DEVICE_STATE_INIT;
        break;
      }
    }
  }
#endif
}